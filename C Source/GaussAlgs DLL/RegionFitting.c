/*
 * Copyright 2017 Battelle Energy Alliance
 */

/*
 *  RegionFitting.c implements JNI wrapper for fitting regions
 */

#include <jni.h>
#include <string.h>            /* strcpy_s(), strcat_s() */
#include <stdlib.h>            /* malloc(), calloc(), exit(), NULL */
#include <math.h>              /* floor() */
#include "GaussAlgsLib.h"
#include "GaussAlgsPrivate.h"

/* prototypes for private methods */
static GLFitRecList *alloc_fitreclist_item();
static void clean_pkfitarray_refs(JNIEnv *env, jobject **objects, int npeaks,
                                  int npoints);
static GLFitRecord *fitrec_alloc();
static jobject *get_array_from_jvector(JNIEnv *env,
                                       const jobject vector_object,
                                       const char *class_name,
                                       int *array_length, char *error_message,
                                       int error_message_length);
static jobject get_jcc_type(JNIEnv *env, GLCCType type, char *error_message,
                            int error_message_length);
static jobject get_jenergyequation(JNIEnv *env, const GLEnergyEqn *ex,
                                   char *error_message,
                                   int error_message_length);
static jobject get_jfit_inputs(JNIEnv *env, const jobject jspectrum,
                               const jobject jex, const jobject jwx,
                               const jobject jregion, const jobject jpeaks,
                               const jobject jfitparms, char *error_message,
                               int error_message_length);
static jobject get_jfitparms(JNIEnv *env, const GLFitParms *fitparms,
                             char *error_message, int error_message_length);
static jobject get_jpeakwidth_mode(JNIEnv *env, GLPkwdMode mode,
                                   char *error_message,
                                   int error_message_length);
static GLRtnCode set_background(JNIEnv *env, const jobject jbackground,
                                GLFitBackLin *back, char *error_message,
                                int error_message_length);
static GLRtnCode set_curve(JNIEnv *env, const jobject jcurve,
                           const GLChanRange *chanrange, GLCurve **curve,
                           char *error_message, int error_message_length);
static GLRtnCode set_cycle_return(JNIEnv *env, const jobject jcycleReturnCode,
                                  GLCycleReturn *cycle_return,
                                  char *error_message,
                                  int error_message_length);
static GLRtnCode set_fit_list(JNIEnv *env, const GLChanRange *chanrange,
                              const GLSpectrum *spectrum,
                              const GLPeakList *peaks,
                              const GLFitParms *fitparms,
                              const GLEnergyEqn *ex, const GLWidthEqn *wx,
                              int nplots_per_chan,
                              const jobject fitVectorObject,
                              GLFitRecList **fitlist, char *error_message,
                              int error_message_length);
static GLRtnCode set_fit_record(JNIEnv *env, const GLChanRange *chanrange,
                                const GLSpectrum *spectrum,
                                const GLPeakList *peaks,
                                const GLFitParms *fitparms,
                                const GLEnergyEqn *ex, const GLWidthEqn *wx,
                                int nplots_per_chan, const jobject fitObject,
                                jmethodID cyc_mid, jmethodID chi_mid,
                                jmethodID rc_mid, jmethodID except_mid,
                                jmethodID back_mid, jmethodID sum_mid,
                                jmethodID curv_mid, const char *peakclass_name,
                                jmethodID chan_mid, GLFitRecord *fitRecord,
                                char *error_message, int error_message_length);
static GLRtnCode set_summary(JNIEnv *env, const jobject jsummary,
                             GLSummary **summary, char *error_message,
                             int error_message_length);

/* public methods */

GLRtnCode GL_fitregn(const char *java_class_path, const GLChanRange *region,
                     const GLSpectrum *spectrum, const GLPeakList *peaks,
                     const GLFitParms *fitparms, const GLEnergyEqn *ex,
                     const GLWidthEqn *wx, int nplots_per_chan,
                     GLFitRecList **fitlist, char *error_message,
                     int error_message_length)
{
JNIEnv      *env;
jobject     localRefs[20];
int         nRefs;
jobject     jspectrum;
jobject     jex;
jobject     jwx;
jobject     jregion;
jobject     jpeakTreeSet;
jobject     jfitParms;
jobject     jfitInputs;
char        class_buf[GAP_CLASS_BUFSIZE];
jclass      fittingClass;
char        inputs_class_buf[GAP_CLASS_BUFSIZE];
char        sig_buf[GAP_CLASS_BUFSIZE];
jmethodID   mid;
jobject     fitVectorObject;
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

*fitlist = NULL;

jspectrum = GAP_get_jspectrum(env, spectrum, error_message,
                              error_message_length);
localRefs[nRefs++] = jspectrum;
if (NULL == jspectrum)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

jex = get_jenergyequation(env, ex, error_message, error_message_length);
localRefs[nRefs++] = jex;
if (NULL == jex)
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

jregion = GAP_get_jchannelrange(env, *region, error_message,
                                error_message_length);
localRefs[nRefs++] = jregion;
if (NULL == jregion)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

jpeakTreeSet = GAP_get_jpeaktreeset(env, peaks, error_message,
                                    error_message_length);
localRefs[nRefs++] = jpeakTreeSet;
if (NULL == jpeakTreeSet)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

jfitParms = get_jfitparms(env, fitparms, error_message, error_message_length);
localRefs[nRefs++] = jfitParms;
if (NULL == jfitParms)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

jfitInputs = get_jfit_inputs(env, jspectrum, jex, jwx, jregion, jpeakTreeSet,
                             jfitParms, error_message, error_message_length);
localRefs[nRefs++] = jfitInputs;
if (NULL == jfitInputs)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* get the java method */

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s",
          GAP_CLASS_GA_PKG, GAP_CLASS_RGN_FIT);
fittingClass = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = fittingClass;
if (NULL == fittingClass)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

sprintf_s(inputs_class_buf, GAP_CLASS_BUFSIZE, "%s/%s",
          GAP_CLASS_GA_PKG, GAP_CLASS_FIT_IN);
sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "(L%s;)Ljava/util/Vector;",
          inputs_class_buf);
mid = (*env)->GetStaticMethodID(env, fittingClass, "fitRegion", sig_buf);
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find fitRegion method in class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* fit the region */

fitVectorObject = (*env)->CallStaticObjectMethod(env, fittingClass, mid,
		                                         jfitInputs);
localRefs[nRefs++] = fitVectorObject;

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
                "fitRegion Exception: %s\n", ex_msg_buf);
      }

   (*env)->ExceptionClear(env);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JEXCEPTION);
   }

if (NULL == fitVectorObject)
   {
   sprintf_s(error_message, error_message_length,
             "fitRegion method in class %s returned NULL\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* decode the fit vector into the C fitlist */

ret_code = set_fit_list(env, region, spectrum, peaks, fitparms, ex, wx,
                        nplots_per_chan, fitVectorObject, fitlist,
                        error_message, error_message_length);

GAP_delete_local_refs(env, localRefs, nRefs);

return(ret_code);
}

/* private utilities */

static GLFitRecList *alloc_fitreclist_item()
{
GLFitRecList	*fitreclist;

if ((fitreclist = (GLFitRecList *) malloc(sizeof(GLFitRecList))) == NULL)
   return(NULL);

fitreclist->next = NULL;

if ((fitreclist->record = fitrec_alloc()) == NULL)
   {
   free(fitreclist);
   return(NULL);
   }

return(fitreclist);
}

static void clean_pkfitarray_refs(JNIEnv *env, jobject **objects, int npeaks,
                                  int npoints)
{
int  i;

for (i = 0; i < npeaks; i++)
   {
   GAP_free_object_array(env, objects[i], npoints);
   }

free(objects);
}

GLFitRecord *fitrec_alloc()
{
GLFitRecord	*fitrec;

if ((fitrec = (GLFitRecord *) malloc(sizeof(GLFitRecord))) == NULL)
   return(NULL);

fitrec->used_spectrum.count = NULL;
fitrec->used_spectrum.listlength = 0;
fitrec->used_spectrum.nchannels = 0;

fitrec->input_peaks.peak = NULL;
fitrec->input_peaks.listlength = 0;
fitrec->input_peaks.npeaks = 0;

fitrec->cycle_exception = NULL;
fitrec->summary = NULL;
fitrec->curve = NULL;

return(fitrec);
}

static jobject *get_array_from_jvector(JNIEnv *env,
                                       const jobject vector_object,
                                       const char *class_name,
                                       int *array_length,
                                       char *error_message,
                                       int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
jclass     vector_class;
char       sig_buf[GAP_CLASS_BUFSIZE];
jmethodID  midSize;
jobject    *arrayOfObjects;
jmethodID  midIterator;
jobject    itObject;
jclass     it_class;
jmethodID  midHasNext;
jmethodID  midNext;
int        i;
jboolean   hasNext;

nRefs = 0;

/* get size */

vector_class = (*env)->GetObjectClass(env, vector_object);
localRefs[nRefs++] = vector_class;

if (NULL == vector_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to determine class of Vector<%s> object\n", class_name);
   return(NULL);
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()I");
midSize = (*env)->GetMethodID(env, vector_class, "size", sig_buf);
if (NULL == midSize)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find size() method for class Vector<%s>\n",
             class_name);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

*array_length = (*env)->CallIntMethod(env, vector_object, midSize);

/* set up answer */

arrayOfObjects = (jobject *) calloc(*array_length, sizeof(jobject));
if (NULL == arrayOfObjects)
   {
   strcpy_s(error_message, error_message_length,
            "calloc error while constructing array from Vector\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

/* get iterator */

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()Ljava/util/Iterator;",
          class_name);
midIterator = (*env)->GetMethodID(env, vector_class, "iterator", sig_buf);
if (NULL == midIterator)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find iterator() method for class Vector<%s>\n",
             class_name);
   GAP_delete_local_refs(env, localRefs, nRefs);
   free(arrayOfObjects);
   return(NULL);
   }

itObject = (*env)->CallObjectMethod(env, vector_object, midIterator);
localRefs[nRefs++] = itObject;

if (NULL == itObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to get Vector<%s> iterator object\n", class_name);
   GAP_delete_local_refs(env, localRefs, nRefs);
   free(arrayOfObjects);
   return(NULL);
   }

/* get mids for hasNext() and next() */

it_class = (*env)->GetObjectClass(env, itObject);
localRefs[nRefs++] = it_class;

if (NULL == it_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to determine Vector<%s> iterator's class\n", class_name);
   GAP_delete_local_refs(env, localRefs, nRefs);
   free(arrayOfObjects);
   return(NULL);
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()Z");
midHasNext = (*env)->GetMethodID(env, it_class, "hasNext", sig_buf);
if (NULL == midHasNext)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find Vector<%s> iterator's hasNext() method\n",
             class_name);
   GAP_delete_local_refs(env, localRefs, nRefs);
   free(arrayOfObjects);
   return(NULL);
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()Ljava/lang/Object;");
midNext = (*env)->GetMethodID(env, it_class, "next", sig_buf);
if (NULL == midNext)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find Vector<%s> iterator's next() method\n",
             class_name);
   GAP_delete_local_refs(env, localRefs, nRefs);
   free(arrayOfObjects);
   return(NULL);
   }

/* loop over objects, saving refs in answer */

i = 0;
hasNext = (*env)->CallBooleanMethod(env, itObject, midHasNext);

while ((JNI_TRUE == hasNext) && (*array_length > i))
   {
   arrayOfObjects[i] = (*env)->CallObjectMethod(env, itObject, midNext);
   if (NULL == arrayOfObjects[i])
      {
      sprintf_s(error_message, error_message_length,
                "unable to get object #%d from Vector<%s>\n", i, class_name);
      GAP_delete_local_refs(env, localRefs, nRefs);
      GAP_free_object_array(env, arrayOfObjects, i);
      free(arrayOfObjects);
      return(NULL);
      }
   i++;

   hasNext = (*env)->CallBooleanMethod(env, itObject, midHasNext);
   }

GAP_delete_local_refs(env, localRefs, nRefs);

return(arrayOfObjects);
}

static jobject get_jcc_type(JNIEnv *env, GLCCType type, char *error_message,
                            int error_message_length)
{
jobject   localRefs[5];
int       nRefs;
char      class_buf[GAP_CLASS_BUFSIZE];
jclass    type_class;
char      sig_buf[GAP_CLASS_BUFSIZE];
jfieldID  fid;
char      field_name[20];
jobject   typeObject;

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s$CCType",
          GAP_CLASS_GA_PKG, GAP_CLASS_FIT_PARM);
type_class = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = type_class;

if (NULL == type_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s\n", class_buf);
   return(NULL);
   }

switch(type)
   {
   case GL_CC_LARGER:
      strcpy_s(field_name, 20, "LARGER");
      break;
   case GL_CC_SMALLER:
      strcpy_s(field_name, 20, "SMALLER");
      break;
   case GL_CC_LARGER_INC:
      strcpy_s(field_name, 20, "LARGER_INC");
      break;
   default:
      break;
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "L%s;", class_buf);
fid = (*env)->GetStaticFieldID(env, type_class, field_name, sig_buf);
if (NULL == fid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find %s field in class %s\n", field_name, class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

typeObject = (*env)->GetStaticObjectField(env, type_class, fid);

GAP_delete_local_refs(env, localRefs, nRefs);

if (NULL == typeObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to fetch %s from %s\n", field_name, class_buf);
   }

return(typeObject);
}

static jobject get_jenergyequation(JNIEnv *env, const GLEnergyEqn *ex,
                                   char *error_message,
                                   int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
char       class_buf[GAP_CLASS_BUFSIZE];
jclass     ex_class;
jmethodID  mid;
char       sig_buf[GAP_CLASS_BUFSIZE];
jobject    modeObject;
jobject    exObject;

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s",
          GAP_CLASS_GA_PKG, GAP_CLASS_EX);
ex_class = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = ex_class;

if (NULL == ex_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s\n", class_buf);
   return(NULL);
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "(DDDDL%s$MODE;)V", class_buf);
mid = (*env)->GetMethodID(env, ex_class, "<init>", sig_buf);
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find constructor for class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

modeObject = GAP_get_jenergyequationmode(env, ex->mode, error_message,
                                         error_message_length);
localRefs[nRefs++] = modeObject;

if (NULL == modeObject)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

exObject = (*env)->NewObject(env, ex_class, mid, ex->a, ex->b, ex->c,
                             ex->chi_sq, modeObject);

GAP_delete_local_refs(env, localRefs, nRefs);

if (NULL == exObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to construct object %s", class_buf);
   }

return(exObject);
}

static jobject get_jfit_inputs(JNIEnv *env, const jobject jspectrum,
                               const jobject jex, const jobject jwx,
                               const jobject jregion, const jobject jpeaks,
                               const jobject jfitparms, char *error_message,
                               int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
char       class_buf[GAP_CLASS_BUFSIZE];
jclass     inputClass;
char       pk_class_buf[GAP_CLASS_BUFSIZE];
char       tree_class_buf[GAP_CLASS_BUFSIZE];
char       sig_buf[GAP_CLASS_BUFSIZE];
jmethodID  mid;
jobject    answer;

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s",
          GAP_CLASS_GA_PKG, GAP_CLASS_FIT_IN);
inputClass = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = inputClass;

if (NULL == inputClass)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s", class_buf);
   return(NULL);
   }

sprintf_s(pk_class_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_PK);
sprintf_s(tree_class_buf, GAP_CLASS_BUFSIZE, "java/util/TreeSet");
sprintf_s(sig_buf, GAP_CLASS_BUFSIZE,
          "(L%s/%s;L%s/%s;L%s/%s;L%s/%s;L%s;L%s/%s;)V",
          GAP_CLASS_GA_PKG, GAP_CLASS_SPEC,
          GAP_CLASS_GA_PKG, GAP_CLASS_EX,
          GAP_CLASS_GA_PKG, GAP_CLASS_WX,
          GAP_CLASS_GA_PKG, GAP_CLASS_CHNRNG,
          tree_class_buf,
          GAP_CLASS_GA_PKG, GAP_CLASS_FIT_PARM);

mid = (*env)->GetMethodID(env, inputClass, "<init>", sig_buf);
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find constructor for class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

answer = (*env)->NewObject(env, inputClass, mid, jspectrum, jex, jwx, jregion,
                           jpeaks, jfitparms);
if (NULL == answer)
   {
   sprintf_s(error_message, error_message_length,
             "unable to construct object %s\n", class_buf);
   }

GAP_delete_local_refs(env, localRefs, nRefs);

return(answer);
}

static jobject get_jfitparms(JNIEnv *env, const GLFitParms *fitparms,
                             char *error_message, int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
char       class_buf[GAP_CLASS_BUFSIZE];
jclass     parm_class;
char       sig_buf[GAP_CLASS_BUFSIZE];
jmethodID  mid;
jobject    modeObject;
jobject    typeObject;
jobject    parmObject;

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s",
          GAP_CLASS_GA_PKG, GAP_CLASS_FIT_PARM);
parm_class = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = parm_class;

if (NULL == parm_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s\n", class_buf);
   return(NULL);
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE,
          "(IIIL%s$PeakWidthMode;L%s$CCType;F)V", class_buf, class_buf);
mid = (*env)->GetMethodID(env, parm_class, "<init>", sig_buf);
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find constructor for class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

modeObject = get_jpeakwidth_mode(env, fitparms->pkwd_mode, error_message,
                                 error_message_length);
localRefs[nRefs++] = modeObject;

if (NULL == modeObject)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

typeObject = get_jcc_type(env, fitparms->cc_type, error_message,
                          error_message_length);
localRefs[nRefs++] = typeObject;

if (NULL == typeObject)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

parmObject = (*env)->NewObject(env, parm_class, mid, fitparms->ncycle,
                               fitparms->nout, fitparms->max_npeaks,
                               modeObject, typeObject, fitparms->max_resid);
if (NULL == parmObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to construct object %s\n", class_buf);
   }

GAP_delete_local_refs(env, localRefs, nRefs);

return(parmObject);
}

static jobject get_jpeakwidth_mode(JNIEnv *env, GLPkwdMode mode,
                                   char *error_message,
                                   int error_message_length)
{
jobject   localRefs[5];
int       nRefs;
char      class_buf[GAP_CLASS_BUFSIZE];
jclass    mode_class;
jfieldID  fid;
char      field_name[20];
char      sig_buf[GAP_CLASS_BUFSIZE];
jobject   modeObject;

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s$PeakWidthMode",
          GAP_CLASS_GA_PKG, GAP_CLASS_FIT_PARM);
mode_class = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = mode_class;

if (NULL == mode_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s\n", class_buf);
   return(NULL);
   }

switch(mode)
   {
   case GL_PKWD_VARIES:
      strcpy_s(field_name, 20, "VARIES");
      break;
   case GL_PKWD_FIXED:
      strcpy_s(field_name, 20, "FIXED");
      break;
   default:
      break;
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "L%s;", class_buf);
fid = (*env)->GetStaticFieldID(env, mode_class, field_name, sig_buf);
if (NULL == fid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find %s field in class %s\n", field_name, class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

modeObject = (*env)->GetStaticObjectField(env, mode_class, fid);

GAP_delete_local_refs(env, localRefs, nRefs);

if (NULL == modeObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to fetch %s from %s\n", field_name, class_buf);
   }

return(modeObject);
}

static GLRtnCode set_background(JNIEnv *env, const jobject jbackground,
                                GLFitBackLin *back, char *error_message,
                                int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
jclass     back_class;
jmethodID  mid;

nRefs = 0;

/* get the background equation class */

back_class = (*env)->GetObjectClass(env, jbackground);
localRefs[nRefs++] = back_class;

if (NULL == back_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to determine class of BackgroundEquation object\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* set intercept */

mid = (*env)->GetMethodID(env, back_class, "getIntercept", "()D");
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getIntercept method of BackgroundEquation\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

back->intercept = (*env)->CallDoubleMethod(env, jbackground, mid);

/* set intercept uncertainty */

mid = (*env)->GetMethodID(env, back_class, "getInterceptUncert", "()D");
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getInterceptUncert method of BackgroundEquation\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

back->sigi = (*env)->CallDoubleMethod(env, jbackground, mid);

/* set slope */

mid = (*env)->GetMethodID(env, back_class, "getSlope", "()D");
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getSlope method of BackgroundEquation\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

back->slope = (*env)->CallDoubleMethod(env, jbackground, mid);

/* set slope uncertainty */

mid = (*env)->GetMethodID(env, back_class, "getSlopeUncert", "()D");
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getSlopeUncert method of Background\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

back->sigs = (*env)->CallDoubleMethod(env, jbackground, mid);

GAP_delete_local_refs(env, localRefs, nRefs);

return(GL_SUCCESS);
}

static GLRtnCode set_curve(JNIEnv *env, const jobject jcurve,
                           const GLChanRange *chanrange, GLCurve **curve,
                           char *error_message, int error_message_length)
{
jobject    localRefs[10];
int        nRefs;
jclass     curve_class;
jmethodID  mid;
int        npeaks;
int        nplots_per_chan;
int        npoints;
int        nchannels;
char       pt_class_buf[GAP_CLASS_BUFSIZE];
char       vector_class_buf[GAP_CLASS_BUFSIZE];
char       sig_buf[GAP_CLASS_BUFSIZE];
int        i, j;
int        temp_npoints;
jobject    peakFitVectorObject;
jobject    **peakFitArrayObjects;
jobject    fitCurveVectorObject;
jobject    *fitCurveArrayObject;
jobject    fitBackVectorObject;
jobject    *fitBackArrayObject;
jobject    fitResidVectorObject;
jobject    *fitResidArrayObject;
jclass     point_class;
jmethodID  getX_mid;
jmethodID  getY_mid;

nRefs = 0;

/* get the Curve class */

curve_class = (*env)->GetObjectClass(env, jcurve);
localRefs[nRefs++] = curve_class;

if (NULL == curve_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to determine class of Curve object\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* set npeaks */

mid = (*env)->GetMethodID(env, curve_class, "getNpeaks", "()I");
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getNpeaks method of Curve\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

npeaks = (*env)->CallIntMethod(env, jcurve, mid);

/* set nplots_per_chan */

mid = (*env)->GetMethodID(env, curve_class, "getNPlotsPerChannel", "()I");
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getNPlotsPerChannel method of Curve\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

nplots_per_chan = (*env)->CallIntMethod(env, jcurve, mid);

/* set npoints */

mid = (*env)->GetMethodID(env, curve_class, "getNumPlottedPoints", "()I");
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getNumPlottedPoints method of Curve\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

npoints = (*env)->CallIntMethod(env, jcurve, mid);

/* set storage for curve */

nchannels = chanrange->last - chanrange->first + 1;

*curve = GAP_curve_alloc(nchannels, nplots_per_chan, npeaks);
if (NULL == *curve)
   {
   strcpy_s(error_message, error_message_length,
            "unable to allocate space for curve\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_BADMALLOC);
   }

if ((*curve)->npoints != npoints)
   {
   sprintf_s(error_message, error_message_length,
             "java npoints does not match C storage npoints\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_JNIERROR);
   }

(*curve)->chanrange.first = chanrange->first;
(*curve)->chanrange.last = chanrange->last;

/* get vectors of points and convert to arrays of Double java objects */

sprintf_s(vector_class_buf, GAP_CLASS_BUFSIZE, "java/util/Vector");
sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "(I)L%s;", vector_class_buf);
mid = (*env)->GetMethodID(env, curve_class, "getComponentPoints", sig_buf);
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getComponentPoints method of Curve\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_JNIERROR);
   }

if ((peakFitArrayObjects =
     (jobject **) calloc((*curve)->npeaks, sizeof(jobject *))) == NULL)
   {
   strcpy_s(error_message, error_message_length,
            "unable to allocate space for array of peakfits\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_BADMALLOC);
   }
for (i = 0; i < (*curve)->npeaks; i++)
   {
   peakFitArrayObjects[i] = NULL;
   }

sprintf_s(pt_class_buf, GAP_CLASS_BUFSIZE, "java/awt/geom/Point2D$Double");

for (i = 0; i < (*curve)->npeaks; i++)
   {
   peakFitVectorObject = (*env)->CallObjectMethod(env, jcurve, mid, i);

   if (NULL == peakFitVectorObject)
      {
      strcpy_s(error_message, error_message_length,
               "Curve.getComponentPoints returned NULL\n");
      GAP_delete_local_refs(env, localRefs, nRefs);
      clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                            (*curve)->npoints);
      GAP_curve_free(*curve);
      *curve = NULL;
      return(GL_JNIERROR);
      }

   peakFitArrayObjects[i] = get_array_from_jvector(env, peakFitVectorObject,
                                                   pt_class_buf, &temp_npoints,
                                                   error_message,
                                                   error_message_length);
   (*env)->DeleteLocalRef(env, peakFitVectorObject);
   if (NULL == peakFitArrayObjects[i])
      {
      GAP_delete_local_refs(env, localRefs, nRefs);
      clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                            (*curve)->npoints);
      GAP_curve_free(*curve);
      *curve = NULL;
      return(GL_JNIERROR);
      }
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()L%s;", vector_class_buf);
mid = (*env)->GetMethodID(env, curve_class, "getCurvePoints", sig_buf);
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getCurvePoints method of Curve\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                         (*curve)->npoints);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_JNIERROR);
   }

fitCurveVectorObject = (*env)->CallObjectMethod(env, jcurve, mid);
localRefs[nRefs++] = fitCurveVectorObject;
if (NULL == fitCurveVectorObject)
   {
   strcpy_s(error_message, error_message_length,
            "Curve.getCurvePoints returned NULL\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                         (*curve)->npoints);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_JNIERROR);
   }
fitCurveArrayObject = get_array_from_jvector(env, fitCurveVectorObject,
                                             pt_class_buf, &temp_npoints,
                                             error_message,
                                             error_message_length);
if (NULL == fitCurveArrayObject)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                         (*curve)->npoints);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_JNIERROR);
   }

mid = (*env)->GetMethodID(env, curve_class, "getBackPoints", sig_buf);
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getBackPoints method of Curve\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                         (*curve)->npoints);
   GAP_free_object_array(env, fitCurveArrayObject, (*curve)->npoints);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_JNIERROR);
   }

fitBackVectorObject = (*env)->CallObjectMethod(env, jcurve, mid);
localRefs[nRefs++] = fitBackVectorObject;
if (NULL == fitBackVectorObject)
   {
   strcpy_s(error_message, error_message_length,
            "Curve.getBackPoints returned NULL\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                         (*curve)->npoints);
   GAP_free_object_array(env, fitCurveArrayObject, (*curve)->npoints);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_JNIERROR);
   }
fitBackArrayObject = get_array_from_jvector(env, fitBackVectorObject,
                                            pt_class_buf, &temp_npoints,
                                            error_message,
                                            error_message_length);
if (NULL == fitBackArrayObject)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                         (*curve)->npoints);
   GAP_free_object_array(env, fitCurveArrayObject, (*curve)->npoints);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_JNIERROR);
   }

mid = (*env)->GetMethodID(env, curve_class, "getResiduals", sig_buf);
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getResiduals method of Curve\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                         (*curve)->npoints);
   GAP_free_object_array(env, fitCurveArrayObject, (*curve)->npoints);
   GAP_free_object_array(env, fitBackArrayObject, (*curve)->npoints);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_JNIERROR);
   }

fitResidVectorObject = (*env)->CallObjectMethod(env, jcurve, mid);
localRefs[nRefs++] = fitResidVectorObject;
if (NULL == fitResidVectorObject)
   {
   strcpy_s(error_message, error_message_length,
            "Curve.getResiduals returned NULL\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                         (*curve)->npoints);
   GAP_free_object_array(env, fitCurveArrayObject, (*curve)->npoints);
   GAP_free_object_array(env, fitBackArrayObject, (*curve)->npoints);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_JNIERROR);
   }
fitResidArrayObject = get_array_from_jvector(env, fitResidVectorObject,
                                             pt_class_buf, &temp_npoints,
                                             error_message,
                                             error_message_length);
if (NULL == fitResidArrayObject)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                         (*curve)->npoints);
   GAP_free_object_array(env, fitCurveArrayObject, (*curve)->npoints);
   GAP_free_object_array(env, fitBackArrayObject, (*curve)->npoints);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_JNIERROR);
   }

/* get Point2D.Double class and mids of getX and getY */

point_class = (*env)->FindClass(env, pt_class_buf);
localRefs[nRefs++] = point_class;

if (NULL == point_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s", pt_class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                         (*curve)->npoints);
   GAP_free_object_array(env, fitCurveArrayObject, (*curve)->npoints);
   GAP_free_object_array(env, fitBackArrayObject, (*curve)->npoints);
   GAP_free_object_array(env, fitResidArrayObject, (*curve)->npoints);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_JNIERROR);
   }

getX_mid = (*env)->GetMethodID(env, point_class, "getX", "()D");
if (NULL == getX_mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getX method of Point2D.Double\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                         (*curve)->npoints);
   GAP_free_object_array(env, fitCurveArrayObject, (*curve)->npoints);
   GAP_free_object_array(env, fitBackArrayObject, (*curve)->npoints);
   GAP_free_object_array(env, fitResidArrayObject, (*curve)->npoints);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_JNIERROR);
   }
getY_mid = (*env)->GetMethodID(env, point_class, "getY", "()D");
if (NULL == getY_mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getY method of Point2D.Double\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                         (*curve)->npoints);
   GAP_free_object_array(env, fitCurveArrayObject, (*curve)->npoints);
   GAP_free_object_array(env, fitBackArrayObject, (*curve)->npoints);
   GAP_free_object_array(env, fitResidArrayObject, (*curve)->npoints);
   GAP_curve_free(*curve);
   *curve = NULL;
   return(GL_JNIERROR);
   }

/* loop over arrays, setting curve fields */

for (j = 0; j < (*curve)->npoints; j++)
   {
   for (i = 0; i < (*curve)->npeaks; i++)
      {
      (*curve)->fitpeak[i][j] =
         (*env)->CallDoubleMethod(env, peakFitArrayObjects[i][j], getY_mid);
      }

   (*curve)->x_offset[j] =
         (*env)->CallDoubleMethod(env, fitCurveArrayObject[j], getX_mid);
   (*curve)->fitcurve[j] =
         (*env)->CallDoubleMethod(env, fitCurveArrayObject[j], getY_mid);
   (*curve)->back[j] =
         (*env)->CallDoubleMethod(env, fitBackArrayObject[j], getY_mid);
   }

for (j = 0; j < nchannels; j++)
   {
   (*curve)->resid[j] =
         (*env)->CallDoubleMethod(env, fitResidArrayObject[j], getY_mid);
   }

GAP_delete_local_refs(env, localRefs, nRefs);
clean_pkfitarray_refs(env, peakFitArrayObjects, (*curve)->npeaks,
                      (*curve)->npoints);
GAP_free_object_array(env, fitCurveArrayObject, (*curve)->npoints);
GAP_free_object_array(env, fitBackArrayObject, (*curve)->npoints);
GAP_free_object_array(env, fitResidArrayObject, nchannels);

return(GL_SUCCESS);
}

static GLRtnCode set_cycle_return(JNIEnv *env, const jobject jcycleReturnCode,
                                  GLCycleReturn *cycle_return,
                                  char *error_message,
                                  int error_message_length)
{
jobject     localRefs[10];
int         nRefs;
jclass      return_class;
jmethodID   mid;
jstring     codeLabel;
const char  *codeChars;
jboolean    isCopy;

nRefs = 0;

return_class = (*env)->GetObjectClass(env, jcycleReturnCode);
localRefs[nRefs++] = return_class;

if (NULL == return_class)
   {
   strcpy_s(error_message, error_message_length,
            "unable to determine class of cycle return code object\n");
   return(GL_JNIERROR);
   }

mid = (*env)->GetMethodID(env, return_class, "name",
						  "()Ljava/lang/String;");
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get name method of Fit.CycleReturnCode\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

codeLabel = (*env)->CallObjectMethod(env, jcycleReturnCode, mid);
localRefs[nRefs++] = codeLabel;

if (NULL == codeLabel)
   {
   strcpy_s(error_message, error_message_length,
            "Fit.CycleReturnCode.name() returned null\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

codeChars = (*env)->GetStringUTFChars(env, codeLabel, &isCopy);
if (NULL == codeChars)
   {
   strcpy_s(error_message, error_message_length,
            "failed to decode EnergyEquation.MODE's label\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

if (0 == strncmp(codeChars, "DONE", 4))
   {
   *cycle_return = GL_CYCLE_DONE;
   }
else if (0 == strncmp(codeChars, "DEL", 3))
   {
   *cycle_return = GL_CYCLE_DELETE;
   }
else if (0 == strncmp(codeChars, "ADD", 3))
   {
   *cycle_return = GL_CYCLE_ADD;
   }
else
   {
   *cycle_return = GL_CYCLE_CONTINUE;
   }

(*env)->ReleaseStringUTFChars(env, codeLabel, codeChars);
GAP_delete_local_refs(env, localRefs, nRefs);

return(GL_SUCCESS);
}

static GLRtnCode set_fit_list(JNIEnv *env, const GLChanRange *chanrange,
                              const GLSpectrum *spectrum,
                              const GLPeakList *peaks,
                              const GLFitParms *fitparms,
                              const GLEnergyEqn *ex, const GLWidthEqn *wx,
                              int nplots_per_chan,
                              const jobject fitVectorObject,
                              GLFitRecList **fitlist, char *error_message,
                              int error_message_length)
{
jobject       localRefs[5];
int           nRefs;
char          fitclass_name[GAP_CLASS_BUFSIZE];
int           array_length;
jobject       *jfits;
jclass        fit_class;
char          sig_buf[GAP_CLASS_BUFSIZE];
jmethodID     cyc_mid;
jmethodID     chi_mid;
jmethodID     rc_mid;
jmethodID     except_mid;
jmethodID     back_mid;
jmethodID     sum_mid;
jmethodID     curv_mid;
char          peakclass_name[GAP_CLASS_BUFSIZE];
jclass        peak_class;
jmethodID     chan_mid;
int           i;
GLFitRecord	  *curr_record;
GLFitRecList  *curr_list, *last_list;
GLRtnCode     ret_code;

*fitlist = NULL;

nRefs = 0;

/* get Vector class, then iterator object, then hasNext & next methods */

sprintf_s(fitclass_name, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_FIT);

jfits = get_array_from_jvector(env, fitVectorObject, fitclass_name,
                               &array_length, error_message,
                               error_message_length);
if (NULL == jfits)
   {
   return(GL_JNIERROR);
   }

if (0 >= array_length)
   {
   return(GL_SUCCESS);
   }

/* get mids */

fit_class = (*env)->GetObjectClass(env, jfits[0]);
localRefs[nRefs++] = fit_class;

if (NULL == fit_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to determine class of Fit object\n");
   GAP_free_object_array(env, jfits, array_length);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

cyc_mid = (*env)->GetMethodID(env, fit_class, "getCycleNumber", "()I");

chi_mid = (*env)->GetMethodID(env, fit_class, "getChiSquared", "()D");

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()L%s/%s$CycleReturnCode;",
          GAP_CLASS_GA_PKG, GAP_CLASS_FIT);
rc_mid = (*env)->GetMethodID(env, fit_class, "getCycleReturnCode", sig_buf);

except_mid = (*env)->GetMethodID(env, fit_class, "getCycleException",
                          "()Ljava/lang/Exception;");

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()L%s/%s;", GAP_CLASS_GA_PKG,
          GAP_CLASS_BACK);
back_mid = (*env)->GetMethodID(env, fit_class, "getBackground", sig_buf);

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()L%s/%s;", GAP_CLASS_GA_PKG,
          GAP_CLASS_SUMM);
sum_mid = (*env)->GetMethodID(env, fit_class, "getSummary", sig_buf);

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "(I)L%s/%s;", GAP_CLASS_GA_PKG,
          GAP_CLASS_CURVE);
curv_mid = (*env)->GetMethodID(env, fit_class, "getCurve", sig_buf);

if ((NULL == cyc_mid) || (NULL == chi_mid) || (NULL == rc_mid) ||
    (NULL == except_mid) || (NULL == back_mid) || (NULL == sum_mid) ||
    (NULL == curv_mid))
   {
   strcpy_s(error_message, error_message_length,
            "unable to get method of Fit\n");
   GAP_free_object_array(env, jfits, array_length);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

sprintf_s(peakclass_name, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_PK);
peak_class = (*env)->FindClass(env, peakclass_name);
localRefs[nRefs++] = peak_class;

if (NULL == peak_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s\n", peakclass_name);
   GAP_free_object_array(env, jfits, array_length);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

chan_mid = (*env)->GetMethodID(env, peak_class, "getChannel", "()D");
if (NULL == chan_mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getChannel method of Peak\n");
   GAP_free_object_array(env, jfits, array_length);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* loop over fit records and set them */

*fitlist = alloc_fitreclist_item();
if (NULL == *fitlist)
   {
   strcpy_s(error_message, error_message_length,
            "unable to allocate space for fit list\n");
   GAP_free_object_array(env, jfits, array_length);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_BADMALLOC);
   }
last_list = NULL;
curr_list = *fitlist;
curr_record = curr_list->record;

for (i = 0; i < array_length; i++)
   {
   ret_code = set_fit_record(env, chanrange, spectrum, peaks, fitparms, ex, wx,
                             nplots_per_chan, jfits[i], cyc_mid, chi_mid,
                             rc_mid, except_mid, back_mid, sum_mid, curv_mid,
                             peakclass_name, chan_mid, curr_record,
                             error_message, error_message_length);
   if (GL_SUCCESS != ret_code)
      {
      GAP_free_object_array(env, jfits, array_length);
      GAP_delete_local_refs(env, localRefs, nRefs);
      GL_fitreclist_free(*fitlist);
      *fitlist = NULL;
      return(ret_code);
      }

   curr_list->next = alloc_fitreclist_item();
   if (NULL == curr_list->next)
      {
      strcpy_s(error_message, error_message_length,
               "unable to allocate space for the next fit record\n");
      GAP_free_object_array(env, jfits, array_length);
      GAP_delete_local_refs(env, localRefs, nRefs);
      GL_fitreclist_free(*fitlist);
      *fitlist = NULL;
      return(GL_BADMALLOC);
      }
   last_list = curr_list;
   curr_list = curr_list->next;
   curr_record = curr_list->record;
   }
/* clean up (free) last allocation which should be unused */
GL_fitreclist_free(curr_list);
last_list->next = NULL;

GAP_free_object_array(env, jfits, array_length);
GAP_delete_local_refs(env, localRefs, nRefs);

return(GL_SUCCESS);
}

static GLRtnCode set_fit_record(JNIEnv *env, const GLChanRange *chanrange,
                                const GLSpectrum *spectrum,
                                const GLPeakList *peaks,
                                const GLFitParms *fitparms,
                                const GLEnergyEqn *ex, const GLWidthEqn *wx,
                                int nplots_per_chan, const jobject fitObject,
                                jmethodID cyc_mid, jmethodID chi_mid,
                                jmethodID rc_mid, jmethodID except_mid,
                                jmethodID back_mid, jmethodID sum_mid,
                                jmethodID curv_mid, const char *peakclass_name,
                                jmethodID chan_mid, GLFitRecord *fitRecord,
                                char *error_message, int error_message_length)
{
jobject     localRefs[10];
int         nRefs;
int         copy_size;
int         i;
GLRtnCode   ret_code;
jobject     jcycleReturnCode;
jthrowable  jcycleException;
jobject     jbackground;
jobject     jsummary;
jobject     jcurve;

nRefs = 0;

/* set the used fields */

fitRecord->used_chanrange.first = chanrange->first;
fitRecord->used_chanrange.last = chanrange->last;

fitRecord->used_parms.cc_type = fitparms->cc_type;
fitRecord->used_parms.max_npeaks = fitparms->max_npeaks;
fitRecord->used_parms.max_resid = fitparms->max_resid;
fitRecord->used_parms.ncycle = fitparms->ncycle;
fitRecord->used_parms.nout = fitparms->nout;
fitRecord->used_parms.pkwd_mode = fitparms->pkwd_mode;

fitRecord->used_ex.a = ex->a;
fitRecord->used_ex.b = ex->b;
fitRecord->used_ex.c = ex->c;
fitRecord->used_ex.chi_sq = ex->chi_sq;
fitRecord->used_ex.mode = ex->mode;

fitRecord->used_wx.alpha = wx->alpha;
fitRecord->used_wx.beta = wx->beta;
fitRecord->used_wx.chi_sq = wx->chi_sq;
fitRecord->used_wx.mode = wx->mode;

ret_code = GL_spectrum_counts_alloc(&(fitRecord->used_spectrum),
                                    spectrum->nchannels);
if (GL_SUCCESS != ret_code)
   {
   strcpy_s(error_message, error_message_length,
            "unable to allocate space for used spectrum in fit record\n");
   return(ret_code);
   }

fitRecord->used_spectrum.firstchannel = spectrum->firstchannel;
fitRecord->used_spectrum.nchannels = spectrum->nchannels;
#if (! defined(GL_LINUX)) && (! defined(GL_MACOSX))
copy_size = spectrum->nchannels * sizeof(__int32);
#else
copy_size = spectrum->nchannels * sizeof(int);
#endif
memcpy_s(fitRecord->used_spectrum.count, copy_size,
         spectrum->count, copy_size);

fitRecord->input_peaks.peak = (GLPeak *) calloc(peaks->npeaks, sizeof(GLPeak));
if (NULL == fitRecord->input_peaks.peak)
   {
   strcpy_s(error_message, error_message_length,
            "unable to allocate space for input peaks in fit record\n");
   return(GL_BADMALLOC);
   }
fitRecord->input_peaks.listlength = peaks->npeaks;
fitRecord->input_peaks.npeaks = 0;
for (i = 0; i < peaks->npeaks; i++)
   {
   GL_add_peak(&(peaks->peak[i]), &(fitRecord->input_peaks));
   }

/* set results */

fitRecord->cycle_number = (*env)->CallIntMethod(env, fitObject, cyc_mid);
fitRecord->chi_sq = (*env)->CallDoubleMethod(env, fitObject, chi_mid);

jcycleReturnCode = (*env)->CallObjectMethod(env, fitObject, rc_mid);
localRefs[nRefs++] = jcycleReturnCode;

if (NULL == jcycleReturnCode)
   {
   strcpy_s(error_message, error_message_length,
            "Fit.getCycleReturnCode returned NULL\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

ret_code = set_cycle_return(env, jcycleReturnCode, &(fitRecord->cycle_return),
                            error_message, error_message_length);
if (GL_SUCCESS != ret_code)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(ret_code);
   }

jcycleException = (jthrowable) (*env)->CallObjectMethod(env, fitObject,
                                                        except_mid);
localRefs[nRefs++] = jcycleException;

if (NULL == jcycleException)
   {
   fitRecord->cycle_exception = NULL;
   }
else
   {
   if ((fitRecord->cycle_exception =
        (char *) malloc(5000 * sizeof(char))) == NULL)
      {
      strcpy_s(error_message, error_message_length,
               "unable to allocate space for fit exception message\n");
      GAP_delete_local_refs(env, localRefs, nRefs);
      return(GL_BADMALLOC);
      }

   ret_code = GAP_get_exception_message(env, jcycleException,
                                        fitRecord->cycle_exception, 5000,
                                        error_message, error_message_length);
   if (GL_SUCCESS != ret_code)
      {
      sprintf_s(fitRecord->cycle_exception, 5000,
                "failed to retrieve exception message from Java\n");
      }
   }

/* set the background */

jbackground = (*env)->CallObjectMethod(env, fitObject, back_mid);
localRefs[nRefs++] = jbackground;

if (NULL == jbackground)
   {
   strcpy_s(error_message, error_message_length,
            "Fit.getBackground returned NULL\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

ret_code = set_background(env, jbackground, &(fitRecord->back_linear),
                          error_message, error_message_length);
if (GL_SUCCESS != ret_code)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(ret_code);
   }

/* set the summary */

jsummary = (*env)->CallObjectMethod(env, fitObject, sum_mid);
localRefs[nRefs++] = jsummary;

if (NULL == jsummary)
   {
   strcpy_s(error_message, error_message_length,
            "Fit.getSummary returned NULL\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

ret_code = set_summary(env, jsummary, &(fitRecord->summary), error_message,
                       error_message_length);
if (GL_SUCCESS != ret_code)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(ret_code);
   }

/* set the curve */

jcurve = (*env)->CallObjectMethod(env, fitObject, curv_mid, nplots_per_chan);
localRefs[nRefs++] = jcurve;

if (NULL == jcurve)
   {
   strcpy_s(error_message, error_message_length,
            "Fit.getCurve returned NULL\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

ret_code = set_curve(env, jcurve, chanrange, &(fitRecord->curve),
                     error_message, error_message_length);

GAP_delete_local_refs(env, localRefs, nRefs);

return(ret_code);
}

static GLRtnCode set_summary(JNIEnv *env, const jobject jsummary,
                             GLSummary **summary, char *error_message,
                             int error_message_length)
{
jobject    localRefs[10];
int        nRefs;
jclass     summary_class;
jmethodID  mid;
char       sig_buf[GAP_CLASS_BUFSIZE];
jobject    tree_peak_summaries;
char       pksum_class_name[GAP_CLASS_BUFSIZE];
jobject    *peaksum_objects;
int        npeaks;
jclass     peaksum_class;
jmethodID  chan_mid;
jmethodID  sigc_mid;
jmethodID  ht_mid;
jmethodID  sigh_mid;
jmethodID  wid_mid;
jmethodID  sigw_mid;
jmethodID  area_mid;
jmethodID  siga_mid;
jmethodID  egy_mid;
jmethodID  sige_mid;
jmethodID  fix_mid;
jboolean   jfixed;
jmethodID  negpk_alarm_mid;
jboolean   jnegpk_alarm;
jmethodID  outside_alarm_mid;
jboolean   joutside_alarm;
jmethodID  posneg_alarm_mid;
jboolean   jposneg_alarm;
int        i;

nRefs = 0;

/* get the Summary class */

summary_class = (*env)->GetObjectClass(env, jsummary);
localRefs[nRefs++] = summary_class;

if (NULL == summary_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to determine class of Summary object\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* get TreeSet<PeakSummary> */

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()Ljava/util/TreeSet;",
          GAP_CLASS_GA_PKG, GAP_CLASS_SUMM, GAP_CLASS_PK_SUMM);
mid = (*env)->GetMethodID(env, summary_class, "getPeakSummaries", sig_buf);
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getPeakSummaries method of Summary\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

tree_peak_summaries = (*env)->CallObjectMethod(env, jsummary, mid);
localRefs[nRefs++] = tree_peak_summaries;

if (NULL == tree_peak_summaries)
   {
   strcpy_s(error_message, error_message_length,
            "Summary.getPeakSummaries returned NULL\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

peaksum_objects = GAP_get_array_from_jtreeset(env, tree_peak_summaries,
                                              pksum_class_name, &npeaks,
                                              error_message,
                                              error_message_length);
if (NULL == peaksum_objects)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* allocate space for summary */

*summary = GAP_summ_alloc(npeaks);
if (NULL == summary)
   {
   strcpy_s(error_message, error_message_length,
            "unable to allocate space for summary\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   GAP_free_object_array(env, peaksum_objects, (*summary)->npeaks);
   return(GL_BADMALLOC);
   }
(*summary)->npeaks = npeaks;

/* get ratio */

mid = (*env)->GetMethodID(env, summary_class, "getRatio", "()D");
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get getRatio method of Summary\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   GAP_free_object_array(env, peaksum_objects, (*summary)->npeaks);
   free(*summary);
   *summary = NULL;
   return(GL_JNIERROR);
   }

(*summary)->ratio = (*env)->CallDoubleMethod(env, jsummary, mid);

/* set up PeakSummary class, and method ids */

peaksum_class = (*env)->GetObjectClass(env, peaksum_objects[0]);
localRefs[nRefs++] = peaksum_class;

if (NULL == peaksum_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to determine class of Summary$PeakSummary object\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   GAP_free_object_array(env, peaksum_objects, (*summary)->npeaks);
   free(*summary);
   *summary = NULL;
   return(GL_JNIERROR);
   }

chan_mid = (*env)->GetMethodID(env, peaksum_class, "getChannel", "()D");
sigc_mid = (*env)->GetMethodID(env, peaksum_class, "getChannelUncertainty",
                               "()D");
ht_mid = (*env)->GetMethodID(env, peaksum_class, "getHeight", "()D");
sigh_mid = (*env)->GetMethodID(env, peaksum_class, "getHeightUncertainty",
                               "()D");
wid_mid = (*env)->GetMethodID(env, peaksum_class, "getWidth", "()D");
sigw_mid = (*env)->GetMethodID(env, peaksum_class, "getWidthUncertainty",
                               "()D");
area_mid = (*env)->GetMethodID(env, peaksum_class, "getArea", "()D");
siga_mid = (*env)->GetMethodID(env, peaksum_class, "getAreaUncertainty",
                               "()D");
egy_mid = (*env)->GetMethodID(env, peaksum_class, "getEnergy", "()D");
sige_mid = (*env)->GetMethodID(env, peaksum_class, "getEnergyUncertainty",
                               "()D");
fix_mid = (*env)->GetMethodID(env, peaksum_class, "isChannelFixed", "()Z");
negpk_alarm_mid = (*env)->GetMethodID(env, peaksum_class, "isNegPeak", "()Z");
outside_alarm_mid = (*env)->GetMethodID(env, peaksum_class, "isOutsidePeak",
                                        "()Z");
posneg_alarm_mid = (*env)->GetMethodID(env, peaksum_class, "ofPosNegPair",
                                       "()Z");

if ((NULL == chan_mid) || (NULL == sigc_mid) || (NULL == ht_mid) ||
    (NULL == sigh_mid) || (NULL == wid_mid) || (NULL == sigw_mid) ||
    (NULL == area_mid) || (NULL == siga_mid) || (NULL == egy_mid) ||
    (NULL == sige_mid) || (NULL == fix_mid) || (NULL == negpk_alarm_mid) ||
    (NULL == outside_alarm_mid) || (NULL == posneg_alarm_mid))
   {
   strcpy_s(error_message, error_message_length,
            "unable to get method of PeakSummary\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   GAP_free_object_array(env, peaksum_objects, (*summary)->npeaks);
   free(*summary);
   *summary = NULL;
   return(GL_JNIERROR);
   }

/* set each field of summary */

for (i = 0; i < (*summary)->npeaks; i++)
   {
   (*summary)->channel[i] = (*env)->CallDoubleMethod(env, peaksum_objects[i],
                                                     chan_mid);
   (*summary)->sigc[i] = (*env)->CallDoubleMethod(env, peaksum_objects[i],
                                                  sigc_mid);
   (*summary)->height[i] = (*env)->CallDoubleMethod(env, peaksum_objects[i],
                                                    ht_mid);
   (*summary)->sigh[i] = (*env)->CallDoubleMethod(env, peaksum_objects[i],
                                                  sigh_mid);
   (*summary)->wid[i] = (*env)->CallDoubleMethod(env, peaksum_objects[i],
                                                 wid_mid);
   (*summary)->sigw[i] = (*env)->CallDoubleMethod(env, peaksum_objects[i],
                                                  sigw_mid);
   (*summary)->area[i] = (*env)->CallDoubleMethod(env, peaksum_objects[i],
                                                  area_mid);
   (*summary)->siga[i] = (*env)->CallDoubleMethod(env, peaksum_objects[i],
                                                  siga_mid);
   (*summary)->energy[i] = (*env)->CallDoubleMethod(env, peaksum_objects[i],
                                                    egy_mid);
   (*summary)->sige[i] = (*env)->CallDoubleMethod(env, peaksum_objects[i],
                                                  sige_mid);

   jfixed = (*env)->CallBooleanMethod(env, peaksum_objects[i], fix_mid);
   GAP_set_boolean(jfixed, &((*summary)->fixed[i]));

   jnegpk_alarm = (*env)->CallBooleanMethod(env, peaksum_objects[i],
                                            negpk_alarm_mid);
   GAP_set_boolean(jnegpk_alarm, &((*summary)->negpeak_alarm[i]));

   joutside_alarm = (*env)->CallBooleanMethod(env, peaksum_objects[i],
                                              outside_alarm_mid);
   GAP_set_boolean(joutside_alarm, &((*summary)->outsidepeak_alarm[i]));

   jposneg_alarm = (*env)->CallBooleanMethod(env, peaksum_objects[i],
                                             posneg_alarm_mid);
   GAP_set_boolean(jposneg_alarm, &((*summary)->posnegpeakpair_alarm[i]));
   }

GAP_delete_local_refs(env, localRefs, nRefs);
GAP_free_object_array(env, peaksum_objects, (*summary)->npeaks);

return(GL_SUCCESS);
}
