/*
 * Copyright 2017 Battelle Energy Alliance
 */

/*
 *  WidthCalibrating.c implements JNI wrapper for calibrating peak width
 */


#include <jni.h>
#include <string.h>            /* strcpy_s(), strcat_s() */
#include "GaussAlgsLib.h"
#include "GaussAlgsPrivate.h"

/* prototypes for private methods */
static GLRtnCode get_mode_c(JNIEnv *env, jobject modej, GLWidEqnMode *modec,
                            char *error_message, int error_message_length);
static GLRtnCode set_equation(JNIEnv *env, jobject widEqnObject,
                              GLWidthEqn *ex, char *error_message,
                              int error_message_length);

/* public methods */

GLRtnCode GL_wcalib(const char *java_class_path, int count,
                    const double *channel, const double *wid,
                    const double *sigw, GLWidEqnMode mode, GLboolean weighted,
                    GLWidthEqn *wx, char *error_message,
                    int error_message_length)
{
JNIEnv        *env;
jobject       localRefs[10];
int           nRefs;
jdoubleArray  javaChannels;
jdoubleArray  javaWidths;
jdoubleArray  javaSiges;
jobject       javaMode;
jboolean      jweighted;
char          wcal_class_buf[GAP_CLASS_BUFSIZE];
jclass        wcalClass;
char          wx_class_buf[GAP_CLASS_BUFSIZE];
char          sig_buf[GAP_CLASS_BUFSIZE];
jmethodID     mid;
jobject       widEqnObject;
jthrowable    exception;
char          ex_msg_buf[GAP_CLASS_BUFSIZE];
GLRtnCode     ret_code;

/* construct java format inputs */

env = GAP_get_jvm(java_class_path, error_message, error_message_length);
if (NULL == env)
   {
   return (GL_NOJVM);
   }

nRefs = 0;

javaChannels = GAP_get_jdouble_array(env, channel, count);
localRefs[nRefs++] = javaChannels;
javaWidths = GAP_get_jdouble_array(env, wid, count);
localRefs[nRefs++] = javaWidths;
javaSiges = GAP_get_jdouble_array(env, sigw, count);
localRefs[nRefs++] = javaSiges;
if ((NULL == javaChannels) || (NULL == javaWidths) || (NULL == javaSiges))
   {
   sprintf_s(error_message, error_message_length,
             "unable to create java array for width calibration\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

javaMode = GAP_get_jwidthequationmode(env, mode, error_message,
                                      error_message_length);
localRefs[nRefs++] = javaMode;

if (NULL == javaMode)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

jweighted = GAP_get_jboolean(weighted);

/* get the java method */

sprintf_s(wcal_class_buf, GAP_CLASS_BUFSIZE, "%s/%s",
          GAP_CLASS_GA_PKG, GAP_CLASS_WCAL);
wcalClass = (*env)->FindClass(env, wcal_class_buf);
localRefs[nRefs++] = wcalClass;

if (NULL == wcalClass)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s", wcal_class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

sprintf_s(wx_class_buf, GAP_CLASS_BUFSIZE, "%s/%s",
          GAP_CLASS_GA_PKG, GAP_CLASS_WX);
sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "([D[D[DL%s$MODE;Z)L%s;",
          wx_class_buf, wx_class_buf);
mid = (*env)->GetStaticMethodID(env, wcalClass, "calibrate", sig_buf);
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find calibrate method in class %s\n", wcal_class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* calibrate */

widEqnObject = (*env)->CallStaticObjectMethod(env, wcalClass, mid,
		javaChannels, javaWidths, javaSiges, javaMode, jweighted);
localRefs[nRefs++] = widEqnObject;

exception = (*env)->ExceptionOccurred(env);
localRefs[nRefs++] = exception;

if (NULL != exception)
   {
   ret_code = GAP_get_exception_message(env, exception, ex_msg_buf,
                                        GAP_CLASS_BUFSIZE, error_message,
                                        error_message_length);
   if (GL_SUCCESS == ret_code)
      {
      sprintf_s(error_message, error_message_length,
                "width calibration Exception: %s\n", ex_msg_buf);
      }

   (*env)->ExceptionClear(env);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JEXCEPTION);
   }

if (NULL == widEqnObject)
   {
   sprintf_s(error_message, error_message_length,
             "calibrate method in class %s returned NULL\n", wcal_class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* decode the widEqnObject into the C energy equation fields */

ret_code = set_equation(env, widEqnObject, wx, error_message,
                        error_message_length);

GAP_delete_local_refs(env, localRefs, nRefs);

return(ret_code);
}

static GLRtnCode get_mode_c(JNIEnv *env, jobject modej, GLWidEqnMode *modec,
                            char *error_message, int error_message_length)
{
jobject     localRefs[5];
int         nRefs;
jclass      modeClass;
jmethodID   mid;
jstring     modeLabel;
const char  *modeChars;
jboolean    isCopy;

nRefs = 0;

modeClass = (*env)->GetObjectClass(env, modej);
localRefs[nRefs++] = modeClass;

if (NULL == modeClass)
   {
   strcpy_s(error_message, error_message_length,
            "unable to determine class of width calibration mode object");
   return(GL_JNIERROR);
   }

mid = (*env)->GetMethodID(env, modeClass, "label",
						  "()Ljava/lang/String;");
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get label method of WidthEquation.MODE");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

modeLabel = (*env)->CallObjectMethod(env, modej, mid);
localRefs[nRefs++] = modeLabel;

if (NULL == modeLabel)
   {
   strcpy_s(error_message, error_message_length,
            "WidthEquation.MODE.label() returned null");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

modeChars = (*env)->GetStringUTFChars(env, modeLabel, &isCopy);
if (NULL == modeChars)
   {
   strcpy_s(error_message, error_message_length,
            "failed to decode WidthEquation.MODE's label");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

if (0 == strncmp(modeChars, "l", 1))
   {
   *modec = GL_WID_LINEAR;
   }
else
   {
   *modec = GL_WID_SQRT;
   }

(*env)->ReleaseStringUTFChars(env, modeLabel, modeChars);
GAP_delete_local_refs(env, localRefs, nRefs);

return(GL_SUCCESS);
}


static GLRtnCode set_equation(JNIEnv *env, jobject widEqnObject,
                              GLWidthEqn *wx, char *error_message,
                              int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
char       class_buf[GAP_CLASS_BUFSIZE];
jclass     wxClass;
char       method_buf[GAP_CLASS_BUFSIZE];
jmethodID  mid;
char       sig_buf[GAP_CLASS_BUFSIZE];
jobject    answerMode;
GLRtnCode  ret_code;

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_WX);
wxClass = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = wxClass;

if (NULL == wxClass)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s", class_buf);
   return(GL_JNIERROR);
   }

sprintf_s(method_buf, GAP_CLASS_BUFSIZE, "getConstantCoefficient");
mid = (*env)->GetMethodID(env, wxClass, method_buf, "()D");
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find %s() in %s", method_buf, class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }
wx->alpha = (*env)->CallDoubleMethod(env, widEqnObject, mid);

sprintf_s(method_buf, GAP_CLASS_BUFSIZE, "getLinearCoefficient");
mid = (*env)->GetMethodID(env, wxClass, method_buf, "()D");
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find %s() in %s", method_buf, class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }
wx->beta = (*env)->CallDoubleMethod(env, widEqnObject, mid);

sprintf_s(method_buf, GAP_CLASS_BUFSIZE, "getChiSq");
mid = (*env)->GetMethodID(env, wxClass, method_buf, "()D");
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find %s() in %s", method_buf, class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }
wx->chi_sq = (*env)->CallDoubleMethod(env, widEqnObject, mid);

sprintf_s(method_buf, GAP_CLASS_BUFSIZE, "getMode");
sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()L%s$MODE;", class_buf);
mid = (*env)->GetMethodID(env, wxClass, method_buf, sig_buf);
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find %s() in %s", method_buf, class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

answerMode = (*env)->CallObjectMethod(env, widEqnObject, mid);
localRefs[nRefs++] = answerMode;

if (NULL == answerMode)
   {
   strcpy_s(error_message, error_message_length,
            "unable to fetch WidthEquation.MODE");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

ret_code = get_mode_c(env, answerMode, &(wx->mode), error_message,
                      error_message_length);

GAP_delete_local_refs(env, localRefs, nRefs);

return(ret_code);
}
