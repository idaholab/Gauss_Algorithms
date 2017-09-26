/*
 * Copyright 2017 Battelle Energy Alliance
 */
 
/*
 *  GaussAlgsPrivate.c - contains supporting routines
 */

#include <jni.h>
#include <stdlib.h>            /* calloc(), exit(), NULL */
#include <string.h>            /* strcpy_s(), strcat_s() */
#include <math.h>		       /* for log, sqrt */
#include "GaussAlgsLib.h"
#include "GaussAlgsPrivate.h"

#define GL_OPTION_JARPATH	"-Djava.class.path="

/* prototypes for static routines */
static jobject get_jpeak(JNIEnv *env, const GLPeak peak, char *error_message,
                         int error_message_length);
static jobject get_jpeak_type(JNIEnv *env, GLPeakType type,
                              char *error_message, int error_message_length);

/* implementation of private routines */

GLCurve *GAP_curve_alloc(int nchannels, int nplots_per_chan, int npeaks)
{
GLCurve	 *curve;
int      npoints;
double   *storage;
int      i, j;

if ((curve = (GLCurve *) malloc(sizeof(GLCurve))) == NULL)
   {
   return(NULL);
   }

npoints = ((nchannels - 1) * nplots_per_chan) + 1;

if ((curve->x_offset = (double *) calloc(npoints, sizeof(double))) == NULL)
   {
   free(curve);
   return(NULL);
   }

if ((curve->fitpeak = (double **) calloc(npeaks, sizeof(double *))) == NULL)
   {
   free(curve->x_offset);
   free(curve);
   return(NULL);
   }

if ((storage = (double *) calloc((npoints * npeaks), sizeof(double))) == NULL)
   {
   free(curve->x_offset);
   free(curve->fitpeak);
   free(curve);
   return(NULL);
   }

j = 0;
for (i = 0; i < npeaks; i++)
   {
   curve->fitpeak[i] = &(storage[j]);
   j += npoints;
   }

if ((curve->fitcurve = (double *) calloc(npoints, sizeof(double))) == NULL)
   {
   free(curve->x_offset);
   free(storage);
   free(curve->fitpeak);
   free(curve);
   return(NULL);
   }

if ((curve->back = (double *) calloc(npoints, sizeof(double))) == NULL)
   {
   free(curve->x_offset);
   free(storage);
   free(curve->fitpeak);
   free(curve->fitcurve);
   free(curve);
   return(NULL);
   }

if ((curve->resid = (double *) calloc(nchannels, sizeof(double))) == NULL)
   {
   free(curve->x_offset);
   free(storage);
   free(curve->fitpeak);
   free(curve->fitcurve);
   free(curve->back);
   free(curve);
   return(NULL);
   }

curve->listlength = npoints;
curve->chanrange.first = 0;
curve->chanrange.last = 0;
curve->nplots_per_chan = nplots_per_chan;
curve->npoints = npoints;
curve->npeaks = npeaks;

return(curve);
}

void GAP_curve_free(GLCurve *curve)
{
if (curve->npeaks > 0)
   free(curve->fitpeak[0]);

free(curve->x_offset);
free(curve->fitpeak);
free(curve->fitcurve);
free(curve->back);
free(curve->resid);
free(curve);
}

void GAP_delete_local_refs(JNIEnv *env, jobject *localRefs, int nRefs)
{
int    i;

for (i = 0; i < nRefs; i++)
   {
   (*env)->DeleteLocalRef(env, localRefs[i]);
   }
}

void GAP_fitrec_free(GLFitRecord *fitrec)
{
if (NULL != fitrec->used_spectrum.count)
   free(fitrec->used_spectrum.count);

if (NULL != fitrec->input_peaks.peak)
   free(fitrec->input_peaks.peak);

if (NULL != fitrec->cycle_exception)
   free(fitrec->cycle_exception);

if (NULL != fitrec->summary)
   GAP_summ_free(fitrec->summary);

if (NULL != fitrec->curve)
   GAP_curve_free(fitrec->curve);

free(fitrec);
}

void GAP_free_object_array(JNIEnv *env, jobject *objectArray, int arrayLength)
{
int  i;

if (NULL == objectArray)
   {
   return;
   }

for (i = 0; i < arrayLength; i++)
   {
   (*env)->DeleteLocalRef(env, objectArray[i]);
   }

free(objectArray);
}

jobject *GAP_get_array_from_jtreeset(JNIEnv *env, const jobject tree_object,
                                     const char *class_name, int *array_length,
                                     char *error_message,
                                     int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
jclass     tree_class;
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

tree_class = (*env)->GetObjectClass(env, tree_object);
localRefs[nRefs++] = tree_class;

if (NULL == tree_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to determine class of TreeSet<%s> object\n", class_name);
   return(NULL);
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()I");
midSize = (*env)->GetMethodID(env, tree_class, "size", sig_buf);
if (NULL == midSize)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find size() method for class TreeSet<%s>\n",
             class_name);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

*array_length = (*env)->CallIntMethod(env, tree_object, midSize);

/* set up answer */

arrayOfObjects = (jobject *) calloc(*array_length, sizeof(jobject));
if (NULL == arrayOfObjects)
   {
   strcpy_s(error_message, error_message_length,
            "calloc error while constructing array from TreeSet\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

/* get iterator */

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()Ljava/util/Iterator;",
          class_name);
midIterator = (*env)->GetMethodID(env, tree_class, "iterator", sig_buf);
if (NULL == midIterator)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find iterator() method for class TreeSet<%s>\n",
             class_name);
   GAP_delete_local_refs(env, localRefs, nRefs);
   free(arrayOfObjects);
   return(NULL);
   }

itObject = (*env)->CallObjectMethod(env, tree_object, midIterator);
localRefs[nRefs++] = itObject;

if (NULL == itObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to get TreeSet<%s> iterator object\n", class_name);
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
             "unable to determine TreeSet<%s> iterator's class\n", class_name);
   GAP_delete_local_refs(env, localRefs, nRefs);
   free(arrayOfObjects);
   return(NULL);
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()Z");
midHasNext = (*env)->GetMethodID(env, it_class, "hasNext", sig_buf);
if (NULL == midHasNext)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find TreeSet<%s> iterator's hasNext() method\n",
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
             "unable to find TreeSet<%s> iterator's next() method\n",
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
                "unable to get object #%d from TreeSet<%s>\n", i, class_name);
      GAP_delete_local_refs(env, localRefs, nRefs);
      GAP_free_object_array(env, arrayOfObjects, i);
      free(arrayOfObjects);
      return(NULL);
      }
   i++;

   hasNext = (*env)->CallBooleanMethod(env, itObject, midHasNext);
   }

if (*array_length != i)
   {
   strcpy_s(error_message, error_message_length,
            "size of TreeSet did not match what iterator returned\n");
   GAP_free_object_array(env, arrayOfObjects, i);
   free(arrayOfObjects);
   return(NULL);
   }

GAP_delete_local_refs(env, localRefs, nRefs);

return(arrayOfObjects);
}

GLRtnCode GAP_get_exception_message(JNIEnv *env, const jthrowable exception,
                                    char *message_buffer, int buffer_length,
                                    char *error_message,
                                    int error_message_length)
{
jobject     localRefs[5];
int         nRefs;
jclass      exception_class;
jmethodID   mid;
jstring     jmessage;
const char  *message_chars;
jboolean    isCopy;

nRefs = 0;

exception_class = (*env)->GetObjectClass(env, exception);
localRefs[nRefs++] = exception_class;

if (NULL == exception_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to determine class of Throwable object\n");
   return(GL_JNIERROR);
   }

mid = (*env)->GetMethodID(env, exception_class, "getMessage",
                          "()Ljava/lang/String;");
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find getMessage method for class Throwable\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

jmessage = (*env)->CallObjectMethod(env, exception, mid);
localRefs[nRefs++] = jmessage;

if (NULL == jmessage)
   {
   sprintf_s(error_message, error_message_length,
             "Throwable's getMessage method returned NULL\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* copy message into answer */

message_chars = (*env)->GetStringUTFChars(env, jmessage, &isCopy);
if (NULL == message_chars)
   {
   sprintf_s(error_message, error_message_length,
             "unable to get chars of exception message\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

sprintf_s(message_buffer, buffer_length, "%s\n", message_chars);

if (JNI_TRUE == isCopy)
   {
   (*env)->ReleaseStringUTFChars(env, jmessage, message_chars);
   }
GAP_delete_local_refs(env, localRefs, nRefs);

return(GL_SUCCESS);
}

jboolean GAP_get_jboolean(GLboolean value)
{
jboolean answer;

if (GL_TRUE == value)
   {
   answer = JNI_TRUE;
   }
else
   {
   answer = JNI_FALSE;
   }

return(answer);
}

jobject GAP_get_jchannelrange(JNIEnv *env, const GLChanRange chanrange,
                              char *error_message, int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
char       class_buf[GAP_CLASS_BUFSIZE];
jclass     range_class;
jmethodID  mid;
jobject    rangeObject;

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_CHNRNG);
range_class = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = range_class;

if (NULL == range_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s\n", class_buf);
   return(NULL);
   }

mid = (*env)->GetMethodID(env, range_class, "<init>", "(II)V");
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find constructor for class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

rangeObject = (*env)->NewObject(env, range_class, mid,
                                chanrange.first, chanrange.last);
if (NULL == rangeObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to construct object %s\n", class_buf);
   }

GAP_delete_local_refs(env, localRefs, nRefs);

return(rangeObject);
}

jdoubleArray GAP_get_jdouble_array(JNIEnv *env, const double *data,
								   int numDataItems)
{
jdoubleArray javaDoubleArray;

/* this assumes that jdouble is defined to be double */

javaDoubleArray = (*env)->NewDoubleArray(env, numDataItems);
if (NULL == javaDoubleArray)
   {
   return(NULL);
   }
(*env)->SetDoubleArrayRegion(env, javaDoubleArray, 0, numDataItems, data);

return(javaDoubleArray);
}

jobject GAP_get_jenergyequationmode(JNIEnv *env, GLEgyEqnMode mode,
                                    char *error_message,
                                    int error_message_length)
{
jobject   localRefs[5];
int       nRefs;
char      class_buf[GAP_CLASS_BUFSIZE];
jclass    modeClass;
jfieldID  fid;
char      field_name[20];
char      sig_buf[GAP_CLASS_BUFSIZE];
jobject   modeObject;

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s$MODE",
          GAP_CLASS_GA_PKG, GAP_CLASS_EX);
modeClass = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = modeClass;

if (NULL == modeClass)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s\n", class_buf);
   return(NULL);
   }

switch(mode)
   {
   case GL_EGY_LINEAR:
      strcpy_s(field_name, 20, "LINEAR");
      break;
   case GL_EGY_QUADRATIC:
      strcpy_s(field_name, 20, "QUADRATIC");
      break;
   default:
      break;
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "L%s;", class_buf);
fid = (*env)->GetStaticFieldID(env, modeClass, field_name, sig_buf);
if (NULL == fid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find %s field in class %s\n", field_name, class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

modeObject = (*env)->GetStaticObjectField(env, modeClass, fid);

GAP_delete_local_refs(env, localRefs, nRefs);

if (NULL == modeObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to fetch %s from %s\n", field_name, class_buf);
   }

return(modeObject);
}

jobject GAP_get_jpeaktreeset(JNIEnv *env, const GLPeakList *peaks,
                             char *error_message, int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
char       pk_class_buf[GAP_CLASS_BUFSIZE];
char       tree_class_buf[GAP_CLASS_BUFSIZE];
jclass     tree_class;
jmethodID  mid;
jobject    treeObject;
int        i;
int        num_peaks;
jobject    peakObject;
jboolean   addResult;

nRefs = 0;

sprintf_s(pk_class_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_PK);
sprintf_s(tree_class_buf, GAP_CLASS_BUFSIZE, "java/util/TreeSet");
tree_class = (*env)->FindClass(env, tree_class_buf);
localRefs[nRefs++] = tree_class;

if (NULL == tree_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s\n", tree_class_buf);
   return(NULL);
   }

mid = (*env)->GetMethodID(env, tree_class, "<init>", "()V");
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find constructor for class %s\n", tree_class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

treeObject = (*env)->NewObject(env, tree_class, mid);
if (NULL == treeObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to construct object %s\n", tree_class_buf);
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

num_peaks = peaks->npeaks;
for (i = 0; i < num_peaks; i++)
   {
   peakObject = get_jpeak(env, peaks->peak[i], error_message,
                          error_message_length);
   if (NULL == peakObject)
      {
      GAP_delete_local_refs(env, localRefs, nRefs);
      (*env)->DeleteLocalRef(env, treeObject);
      return(NULL);
      }

   addResult = (*env)->CallBooleanMethod(env, treeObject, mid, peakObject);
   (*env)->DeleteLocalRef(env, peakObject);

   if (JNI_FALSE == addResult)
      {
      sprintf_s(error_message, error_message_length,
                "unable to add peak to TreeSet\n");
      GAP_delete_local_refs(env, localRefs, nRefs);
      (*env)->DeleteLocalRef(env, treeObject);
      return(NULL);
      }
   }

GAP_delete_local_refs(env, localRefs, nRefs);

return(treeObject);
}

jobject GAP_get_jspectrum(JNIEnv *env, const GLSpectrum *spectrum,
                          char *error_message, int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
char       class_buf[GAP_CLASS_BUFSIZE];
jclass     spec_class;
jmethodID  mid;
jobject    specObject;
jint       firstChan;
int        i;
int        nchannels;
jintArray  specCountsArray;
#if (! defined(GL_LINUX)) && (! defined(GL_MACOSX))
jint       *specJintCounts;
#endif

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s",
          GAP_CLASS_GA_PKG, GAP_CLASS_SPEC);
spec_class = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = spec_class;

if (NULL == spec_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s\n", class_buf);
   return(NULL);
   }

mid = (*env)->GetMethodID(env, spec_class, "<init>", "(I[I)V");
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find constructor for class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

nchannels = spectrum->nchannels;
firstChan = spectrum->firstchannel;
specCountsArray = (*env)->NewIntArray(env, nchannels);
localRefs[nRefs++] = specCountsArray;

if ((specJintCounts = (jint *) calloc(nchannels, sizeof(jint))) == NULL)
   {
   strcpy_s(error_message, error_message_length,
            "unable to allocate space for spectrum counts\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return NULL;
   }

for (i = 0; i < nchannels; i++)
   {
   specJintCounts[i] = (int) spectrum->count[i];
   }

(*env)->SetIntArrayRegion(env, specCountsArray, 0, nchannels, specJintCounts);

specObject = (*env)->NewObject(env, spec_class, mid, firstChan,
                               specCountsArray);
if (NULL == specObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to construct object %s\n", class_buf);
   }

#if (! defined(GL_LINUX)) && (! defined(GL_MACOSX))
   free(specJintCounts);
#endif

GAP_delete_local_refs(env, localRefs, nRefs);

return(specObject);
}

JNIEnv *GAP_get_jvm(const char *javaClassPath, char *errMsg, int errMsgLength)
{
JNIEnv *env;
JavaVM *jvm;
JavaVM (*vmBuf)[3];
jsize nVMs;
jint res;
JavaVMOption options[1];
char optionString[1026];
JavaVMInitArgs vm_init_args;
JavaVMAttachArgs vm_attach_args;
jint version;

errMsg[0] = '\0';
env = NULL;
jvm = NULL;

version = 0x00010002;

res = JNI_GetCreatedJavaVMs((JavaVM **) &vmBuf, 3, &nVMs);
if ((JNI_OK == res) && (0 < nVMs))
   {
   jvm = vmBuf[0];

   vm_attach_args.group = NULL;
   vm_attach_args.name = NULL;
   vm_attach_args.version = version;
   if (JNI_OK != (*jvm)->AttachCurrentThread(jvm, (void**) &env,
                                             &vm_attach_args))
      {
      if (JNI_OK != (*jvm)->GetEnv(jvm, (void**) &env, version))
         {
         strcpy_s(errMsg, errMsgLength, "Can't get Java VM environment\n");
         return(env);
         }
      }
   }
else
   {
   vm_init_args.version = version;
   strcpy_s(optionString, sizeof(optionString), GL_OPTION_JARPATH);
   strcat_s(optionString, sizeof(optionString), javaClassPath);
   options[0].optionString = optionString;
   vm_init_args.options = options;
   vm_init_args.nOptions = 1;
   vm_init_args.ignoreUnrecognized = JNI_TRUE;

   if (JNI_OK != JNI_CreateJavaVM(&jvm, (void**) &env, &vm_init_args))
      {
      strcpy_s(errMsg, errMsgLength, "Can't create Java VM\n");
      return(env);
      }
   }

return(env);
}

jobject GAP_get_jwidthequation(JNIEnv *env, const GLWidthEqn *wx,
                               char *error_message, int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
char       class_buf[GAP_CLASS_BUFSIZE];
jclass     wx_class;
jmethodID  mid;
char       sig_buf[GAP_CLASS_BUFSIZE];
jobject    modeObject;
jobject    weqnObject;

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_WX);
wx_class = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = wx_class;

if (NULL == wx_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s\n", class_buf);
   return(NULL);
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "(DDDL%s$MODE;)V", class_buf);
mid = (*env)->GetMethodID(env, wx_class, "<init>", sig_buf);
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find constructor for class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

modeObject = GAP_get_jwidthequationmode(env, wx->mode, error_message,
                                        error_message_length);
localRefs[nRefs++] = modeObject;

if (NULL == modeObject)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

weqnObject = (*env)->NewObject(env, wx_class, mid,
              wx->alpha, wx->beta, wx->chi_sq, modeObject);

GAP_delete_local_refs(env, localRefs, nRefs);

if (NULL == weqnObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to construct object %s\n", class_buf);
   }

return(weqnObject);
}

jobject GAP_get_jwidthequationmode(JNIEnv *env, GLWidEqnMode mode,
                                   char *error_message,
                                   int error_message_length)
{
jobject   localRefs[5];
int       nRefs;
char      class_buf[GAP_CLASS_BUFSIZE];
jclass    mode_class;
char      sig_buf[GAP_CLASS_BUFSIZE];
jfieldID  fid;
char      field_name[20];
jobject   modeObject;

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s$MODE",
          GAP_CLASS_GA_PKG, GAP_CLASS_WX);
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
   case GL_WID_LINEAR:
      strcpy_s(field_name, 20, "LINEAR");
      break;
   case GL_WID_SQRT:
      strcpy_s(field_name, 20, "SQUARE_ROOT");
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

void GAP_set_boolean(jboolean jvalue, GLboolean *value)
{
if (JNI_TRUE == jvalue)
   {
   *value = GL_TRUE;
   }
else
   {
   *value = GL_FALSE;
   }
}

GLRtnCode GAP_set_chanrange(JNIEnv *env, const jobject jchanrange,
                            GLChanRange *chanrange, char *error_message,
                            int error_message_length)
{
jobject   localRefs[5];
int       nRefs;
jclass    range_class;
jfieldID  fidFirst;
jfieldID  fidLast;
jint      jfirst;
jint      jlast;

nRefs = 0;

range_class = (*env)->GetObjectClass(env, jchanrange);
localRefs[nRefs++] = range_class;

if (NULL == range_class)
   {
   strcpy_s(error_message, error_message_length,
            "unable to determine class of ChannelRange object\n");
   return(GL_JNIERROR);
   }

fidFirst = (*env)->GetFieldID(env, range_class, "m_firstChannel", "I");
if (NULL == fidFirst)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get field ID for ChannelRange.m_firstChannel\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

fidLast = (*env)->GetFieldID(env, range_class, "m_lastChannel", "I");
if (NULL == fidLast)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get field ID for ChannelRange.m_lastChannel\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

jfirst = (*env)->GetIntField(env, jchanrange, fidFirst);
jlast = (*env)->GetIntField(env, jchanrange, fidLast);

chanrange->first = jfirst;
chanrange->last = jlast;

return(GL_SUCCESS);
}

GLSummary *GAP_summ_alloc(int listlength)
{
GLSummary	*summ;
GLboolean   *storage_bool;
double      *storage_dbl;
int         i;

if ((summ = (GLSummary *) malloc(sizeof(GLSummary))) == NULL)
   return(NULL);

/* allocate storage in minimum of calls */

   if ((storage_bool =
        (GLboolean *) calloc(4 * listlength, sizeof(GLboolean))) == NULL)
      {
      free(summ);
      return(NULL);
      }

   if ((storage_dbl =
        (double *) calloc(10 * listlength, sizeof(double))) == NULL)
      {
      free(storage_bool);
      free(summ);
      return(NULL);
      }

/* set pointers for each GLboolean array in summary structure */

   i = 0;
   summ->fixed = &storage_bool[i];
   i += listlength;
   summ->negpeak_alarm = &storage_bool[i];
   i += listlength;
   summ->outsidepeak_alarm = &storage_bool[i];
   i += listlength;
   summ->posnegpeakpair_alarm = &storage_bool[i];

/* set pointers for each double array in summary structure */

   i = 0;
   summ->channel = &storage_dbl[i];
   i += listlength;
   summ->sigc = &storage_dbl[i];
   i += listlength;
   summ->height = &storage_dbl[i];
   i += listlength;
   summ->sigh = &storage_dbl[i];
   i += listlength;
   summ->wid = &storage_dbl[i];
   i += listlength;
   summ->sigw = &storage_dbl[i];
   i += listlength;
   summ->area = &storage_dbl[i];
   i += listlength;
   summ->siga = &storage_dbl[i];
   i += listlength;
   summ->energy = &storage_dbl[i];
   i += listlength;
   summ->sige = &storage_dbl[i];

summ->listlength = listlength;

return(summ);
}

void GAP_summ_free(GLSummary *summary)
{
free(summary->channel);
free(summary->fixed);
free(summary);
}

/* private utilities */

static jobject get_jpeak(JNIEnv *env, const GLPeak peak, char *error_message,
                         int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
char       class_buf[GAP_CLASS_BUFSIZE];
jclass     peak_class;
char       sig_buf[GAP_CLASS_BUFSIZE];
jmethodID  mid;
jobject    typeObject;
jboolean   jchanValid;
jboolean   jegyValid;
jboolean   jfixedCentroid;
jobject    peakObject;

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s",
          GAP_CLASS_GA_PKG, GAP_CLASS_PK);
peak_class = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = peak_class;

if (NULL == peak_class)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s\n", class_buf);
   return(NULL);
   }

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "(L%s$TYPE;DZDZDZ)V", class_buf);

mid = (*env)->GetMethodID(env, peak_class, "<init>", sig_buf);
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find constructor for class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

typeObject = get_jpeak_type(env, peak.type, error_message,
                            error_message_length);
localRefs[nRefs++] = typeObject;

if (NULL == typeObject)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(NULL);
   }

jchanValid = GAP_get_jboolean(peak.channel_valid);
jegyValid = GAP_get_jboolean(peak.energy_valid);
jfixedCentroid = GAP_get_jboolean(peak.fixed_centroid);

peakObject = (*env)->NewObject(env, peak_class, mid, typeObject,
                               peak.channel, jchanValid, peak.energy,
                               jegyValid, peak.sige, jfixedCentroid);
if (NULL == peakObject)
   {
   sprintf_s(error_message, error_message_length,
             "unable to construct object %s\n", class_buf);
   }

GAP_delete_local_refs(env, localRefs, nRefs);

return(peakObject);
}

static jobject get_jpeak_type(JNIEnv *env, GLPeakType type,
                              char *error_message, int error_message_length)
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

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s$TYPE",
          GAP_CLASS_GA_PKG, GAP_CLASS_PK);
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
   case GL_PEAK_CHANNEL:
      strcpy_s(field_name, 20, "CHANNEL");
      break;
   case GL_PEAK_ENERGY:
      strcpy_s(field_name, 20, "ENERGY");
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
