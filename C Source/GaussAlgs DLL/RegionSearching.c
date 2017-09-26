/*
 * Copyright 2017 Battelle Energy Alliance
 */

/*
 *  RegionSearching.c implements JNI wrapper for searching for regions
 */

#include <jni.h>
#include <string.h>            /* strcpy_s(), strcat_s() */
#include "GaussAlgsLib.h"
#include "GaussAlgsPrivate.h"

/* prototypes for private methods */
static jobject get_jrgn_srch_parms(JNIEnv *env, GLRgnSrchMode mode,
                                   double threshold, int irw, int irch,
                                   int maxrgnwid, int maxNumReturned,
                                   char *error_message,
                                   int error_message_length);
static jobject get_jrgn_srch_parms_mode(JNIEnv *env, GLRgnSrchMode mode,
                                        char *error_message,
                                        int error_message_length);
static jobject get_jrgn_treeset(JNIEnv *env, const GLRegions *regions,
                                char *error_message, int error_message_length);
static GLRtnCode set_regions(JNIEnv *env, jobject regionsTreeObject,
                             GLRegions *regions, char *error_message,
                             int error_message_length);

/* public methods */

GLRtnCode GL_exceeds_width(const char *java_class_path, const GLRegions *regions,
                           int max_width_channels, GLboolean *answer,
                           char *error_message, int error_message_length)
{
JNIEnv      *env = NULL;
jobject     localRefs[10];
int         nRefs;
jobject     rgnTreeSetObject;
jint        jmaxWidth;
char        class_buf[GAP_CLASS_BUFSIZE];
jclass      rgn_srch_class;
jmethodID   mid;
char        sig_buf[GAP_CLASS_BUFSIZE];
jboolean    janswer;

/* set up Java inputs */

env = GAP_get_jvm(java_class_path, error_message, error_message_length);
if (NULL == env)
   {
   return(GL_NOJVM);
   }

nRefs = 0;

rgnTreeSetObject = get_jrgn_treeset(env, regions, error_message,
                                    error_message_length);
localRefs[nRefs++] = rgnTreeSetObject;

if (NULL == rgnTreeSetObject)
   {
   return(GL_JNIERROR);
   }

jmaxWidth = max_width_channels;

/* invoke the Java method */

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_RGN_SRCH);

rgn_srch_class = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = rgn_srch_class;

if (NULL == rgn_srch_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "(Ljava/util/TreeSet;I)Z");
mid = (*env)->GetStaticMethodID(env, rgn_srch_class, "exceedsWidth", sig_buf);
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to get exceedsWidth method in class %s\n",
             class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

janswer = (*env)->CallStaticBooleanMethod(env, rgn_srch_class, mid,
                                          rgnTreeSetObject, jmaxWidth);

/* decode the answer */

GAP_set_boolean(janswer, answer);

GAP_delete_local_refs(env, localRefs, nRefs);

return(GL_SUCCESS);
}

GLRtnCode GL_regnsearch(const char *java_class_path,
                        const GLChanRange *chanrange, const GLWidthEqn *wx,
                        double threshold, int irw, int irch,
                        const GLSpectrum *spectrum, const GLPeakList *peaks,
                        GLRgnSrchMode mode, int maxrgnwid, GLRegions *regions,
                        char *error_message, int error_message_length)
{
JNIEnv      *env = NULL;
jobject     localRefs[10];
int         nRefs;
jobject     jspectrum;
jobject     jchanrange;
jobject     jwx;
jobject     jpeaks;
jobject     jparms;
char        class_buf[GAP_CLASS_BUFSIZE];
jclass      rsClass;
char        spec_buf[GAP_CLASS_BUFSIZE];
char        range_buf[GAP_CLASS_BUFSIZE];
char        wx_buf[GAP_CLASS_BUFSIZE];
char        tree_buf[GAP_CLASS_BUFSIZE];
char        parm_buf[GAP_CLASS_BUFSIZE];
char        sig_buf[GAP_CLASS_BUFSIZE];
jmethodID   mid;
jobject     jregions;
jthrowable  exception;
char        ex_msg_buf[GAP_CLASS_BUFSIZE];
GLRtnCode   ret_code;

/* construct java format inputs */

env = GAP_get_jvm(java_class_path, error_message, error_message_length);
if (NULL == env)
   {
   return(GL_NOJVM);
   }

nRefs = 0;

jspectrum = GAP_get_jspectrum(env, spectrum, error_message,
                              error_message_length);
localRefs[nRefs++] = jspectrum;
if (NULL == jspectrum)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

jchanrange = GAP_get_jchannelrange(env, *chanrange, error_message,
                                   error_message_length);
localRefs[nRefs++] = jchanrange;
if (NULL == jchanrange)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

jwx = GAP_get_jwidthequation(env, wx, error_message, error_message_length);
localRefs[nRefs++] = jwx;
if (NULL == jwx)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

jpeaks = GAP_get_jpeaktreeset(env, peaks, error_message, error_message_length);
localRefs[nRefs++] = jpeaks;
if (NULL == jpeaks)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

jparms = get_jrgn_srch_parms(env, mode, threshold, irw, irch, maxrgnwid,
                             regions->listlength, error_message,
                             error_message_length);
localRefs[nRefs++] = jparms;
if (NULL == jparms)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* find search method */

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_RGN_SRCH);
rsClass = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = rsClass;

if (NULL == rsClass)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

sprintf_s(spec_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_SPEC);
sprintf_s(range_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_CHNRNG);
sprintf_s(wx_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_WX);
sprintf_s(tree_buf, GAP_CLASS_BUFSIZE, "java/util/TreeSet");
sprintf_s(parm_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_RGN_SRCHPARM);
sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "(L%s;L%s;L%s;L%s;L%s;)L%s;", spec_buf,
          range_buf, wx_buf, tree_buf, parm_buf, tree_buf);

mid = (*env)->GetStaticMethodID(env, rsClass, "search", sig_buf);
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find search method in class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* search for regions */

jregions = (*env)->CallStaticObjectMethod(env, rsClass, mid,
		jspectrum, jchanrange, jwx, jpeaks, jparms);
localRefs[nRefs++] = jregions;

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
                "region search Exception: %s\n", ex_msg_buf);
      }

   (*env)->ExceptionClear(env);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JEXCEPTION);
   }

if (NULL == jregions)
   {
   sprintf_s(error_message, error_message_length,
             "search method in class %s returned NULL\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* decode java region treeset into a region list */

ret_code = set_regions(env, jregions, regions, error_message,
                       error_message_length);

GAP_delete_local_refs(env, localRefs, nRefs);

return(ret_code);
}

static jobject get_jrgn_srch_parms(JNIEnv *env, GLRgnSrchMode mode,
                                   double threshold, int irw, int irch,
                                   int maxrgnwid, int maxNumReturned,
                                   char *error_message,
                                   int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
char       class_buf[GAP_CLASS_BUFSIZE];
jclass     parmsClass;
char       sig_buf[GAP_CLASS_BUFSIZE];
jmethodID  mid;
jobject    modeObject;
jobject    parmsObject;

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s",
          GAP_CLASS_GA_PKG, GAP_CLASS_RGN_SRCHPARM);
parmsClass = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = parmsClass;

if (NULL == parmsClass)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s", class_buf);
   return(NULL);
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "(L%s$SEARCHMODE;DIIII)V", class_buf);
mid = (*env)->GetMethodID(env, parmsClass, "<init>", sig_buf);
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find constructor for class %s", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

modeObject = get_jrgn_srch_parms_mode(env, mode, error_message,
                                      error_message_length);
localRefs[nRefs++] = modeObject;

if (NULL == modeObject)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

parmsObject = (*env)->NewObject(env, parmsClass, mid, modeObject, threshold,
                                irw, irch, maxrgnwid, maxNumReturned);

GAP_delete_local_refs(env, localRefs, nRefs);

if (NULL == parmsObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to construct object %s", class_buf);
   }

return(parmsObject);
}

static jobject get_jrgn_srch_parms_mode(JNIEnv *env, GLRgnSrchMode mode,
                                        char *error_message,
                                        int error_message_length)
{
jobject   localRefs[5];
int       nRefs;
char      class_buf[GAP_CLASS_BUFSIZE];
jclass    modeClass;
char      sig_buf[GAP_CLASS_BUFSIZE];
jfieldID  fid;
char      field_name[20];
jobject   modeObject;

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s$SEARCHMODE",
          GAP_CLASS_GA_PKG, GAP_CLASS_RGN_SRCHPARM);
modeClass = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = modeClass;

if (NULL == modeClass)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s", class_buf);
   return(NULL);
   }

switch(mode)
   {
   case GL_RGNSRCH_ALL:
      strcpy_s(field_name, 20, "ALL");
      break;
   case GL_RGNSRCH_FORPKS:
      strcpy_s(field_name, 20, "FORPEAKS");
      break;
   default:
      break;
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "L%s;", class_buf);
fid = (*env)->GetStaticFieldID(env, modeClass, field_name, sig_buf);
if (NULL == fid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find %s field in class %s", field_name, class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

modeObject = (*env)->GetStaticObjectField(env, modeClass, fid);

GAP_delete_local_refs(env, localRefs, nRefs);

if (NULL == modeObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to fetch %s from %s", field_name, class_buf);
   }

return(modeObject);
}

static jobject get_jrgn_treeset(JNIEnv *env, const GLRegions *regions,
                                char *error_message, int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
char       rgn_class_buf[GAP_CLASS_BUFSIZE];
char       tree_class_buf[GAP_CLASS_BUFSIZE];
jclass     tree_class;
jmethodID  mid;
jobject    treeObject;
int        i;
int        num_rgns;
jobject    rgnObject;
jboolean   addResult;

nRefs = 0;

sprintf_s(rgn_class_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_CHNRNG);
sprintf_s(tree_class_buf, GAP_CLASS_BUFSIZE, "java/util/TreeSet");
tree_class = (*env)->FindClass(env, tree_class_buf);
localRefs[nRefs++] = tree_class;

if (NULL == tree_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s", tree_class_buf);
   return(NULL);
   }

mid = (*env)->GetMethodID(env, tree_class, "<init>", "()V");
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find constructor for class %s", tree_class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

treeObject = (*env)->NewObject(env, tree_class, mid);
if (NULL == treeObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to construct object %s", tree_class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

mid = (*env)->GetMethodID(env, tree_class, "add", "(Ljava/lang/Object;)Z");
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find add method in class %s\n", tree_class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   (*env)->DeleteLocalRef(env, treeObject);
   return(NULL);
   }

num_rgns = regions->nregions;
for (i = 0; i < num_rgns; i++)
   {
   rgnObject = GAP_get_jchannelrange(env, regions->chanrange[i],
                                     error_message, error_message_length);
   if (NULL == rgnObject)
      {
      GAP_delete_local_refs(env, localRefs, nRefs);
      (*env)->DeleteLocalRef(env, treeObject);
      return(NULL);
      }

   addResult = (*env)->CallBooleanMethod(env, treeObject, mid, rgnObject);
   (*env)->DeleteLocalRef(env, rgnObject);

   if (JNI_FALSE == addResult)
      {
      sprintf_s(error_message, error_message_length,
                "unable to add region to TreeSet");
      GAP_delete_local_refs(env, localRefs, nRefs);
      (*env)->DeleteLocalRef(env, treeObject);
      return(NULL);
      }
   }

GAP_delete_local_refs(env, localRefs, nRefs);

return(treeObject);
}

static GLRtnCode set_regions(JNIEnv *env, jobject regionsTreeObject,
                             GLRegions *regions, char *error_message,
                             int error_message_length)
{
char       rgn_buf[GAP_CLASS_BUFSIZE];
jobject    *jregions;
int        array_length;
int        i;
GLRtnCode  ret_code;

sprintf_s(rgn_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_CHNRNG);

/* loop over java regions, copying them to C regions */

regions->nregions = 0;

jregions = GAP_get_array_from_jtreeset(env, regionsTreeObject, rgn_buf,
                                       &array_length, error_message,
                                       error_message_length);
if (NULL == jregions)
   {
   return(GL_JNIERROR);
   }

for (i = 0; i < array_length; i++)
   {
   ret_code = GAP_set_chanrange(env, jregions[i], &(regions->chanrange[i]),
                                error_message, error_message_length);
   if (GL_SUCCESS == ret_code)
      {
      regions->nregions++;
      }
   else
      {
      regions->nregions = 0;
      break;
      }
   }

GAP_free_object_array(env, jregions, array_length);

return(ret_code);
}
