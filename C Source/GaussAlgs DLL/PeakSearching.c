/*
 * Copyright 2017 Battelle Energy Alliance
 */

/*
 *  PeakSearching.c implements JNI wrapper for searching for peaks
 */

#include <jni.h>
#include <string.h>            /* strcpy_s(), strcat_s() */
#include "GaussAlgsLib.h"
#include "GaussAlgsPrivate.h"

/* prototypes for private methods */
static GLRtnCode get_type_c(JNIEnv *env, jobject typej, GLPeakType *typec,
                            char *error_message, int error_message_length);
static GLRtnCode set_cross_correlations(JNIEnv *env, jclass resultsClass,
                                        jobject peakResultsObject,
                                        int *cross_products, int listlength,
                                        char *error_message,
                                        int error_message_length);
static GLRtnCode set_peak_list(JNIEnv *env, const jobject peakTreeSet,
                               GLPeakList *peaklist, char *error_message,
                               int error_message_length);
static GLRtnCode set_peak_results(JNIEnv *env, jobject peakResultsObject,
                                  GLPeakSearchResults *results,
                                  char *error_message,
                                  int error_message_length);
static GLRtnCode set_search_peak(JNIEnv *env, jmethodID rawc_mid,
                                 jmethodID rfrgn_mid, jmethodID a_mid,
                                 jmethodID b_mid, jmethodID refc_mid,
                                 jmethodID use_mid, jobject searchPeakObject,
                                 int index, GLPeakSearchResults *results,
                                 char *error_message,
                                 int error_message_length);

/* public methods */

GLRtnCode GL_peaksearch(const char *java_class_path,
                        const GLChanRange *chanrange, const GLWidthEqn *wx,
                        int threshold, const GLSpectrum *spectrum,
                        GLPeakSearchResults *results, char *error_message,
                        int error_message_length)
{
JNIEnv      *env = NULL;
jobject     localRefs[10];
int         nRefs;
jobject     jspectrum;
jobject     jchanrange;
jobject     jwx;
jint        jthreshold;
char        class_buf[GAP_CLASS_BUFSIZE];
jclass      psClass;
char        spec_buf[GAP_CLASS_BUFSIZE];
char        range_buf[GAP_CLASS_BUFSIZE];
char        wx_buf[GAP_CLASS_BUFSIZE];
char        rslts_buf[GAP_CLASS_BUFSIZE];
char        sig_buf[GAP_CLASS_BUFSIZE];
jmethodID   mid;
jobject     peakResultsObject;
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

jthreshold = threshold;

/* find search method */

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_PK_SRCH);
psClass = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = psClass;

if (NULL == psClass)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

sprintf_s(spec_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_SPEC);
sprintf_s(range_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_CHNRNG);
sprintf_s(wx_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_WX);
sprintf_s(rslts_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_PK_SRCH_RSLTS);
sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "(L%s;L%s;L%s;I)L%s;", spec_buf,
          range_buf, wx_buf, rslts_buf);

mid = (*env)->GetStaticMethodID(env, psClass, "search", sig_buf);
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find search method in class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* search for peaks */

peakResultsObject = (*env)->CallStaticObjectMethod(env, psClass, mid,
		jspectrum, jchanrange, jwx, jthreshold);
localRefs[nRefs++] = peakResultsObject;

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
                "PeakSearching.search Exception: %s\n", ex_msg_buf);
      }

   (*env)->ExceptionClear(env);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JEXCEPTION);
   }

if (NULL == peakResultsObject)
   {
   sprintf_s(error_message, error_message_length,
             "search method in class %s returned NULL\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* copy java results into C */

ret_code = set_peak_results(env, peakResultsObject, results, error_message,
                            error_message_length);

GAP_delete_local_refs(env, localRefs, nRefs);

return(ret_code);
}

GLRtnCode GL_prune_rqdpks(const char *java_class_path, const GLWidthEqn *wx,
                          const GLPeakList *searchpks,
                          const GLPeakList *curr_rqd, GLPeakList *new_rqd,
                          char *error_message, int error_message_length)
{
JNIEnv     *env = NULL;
jobject    localRefs[10];
int        nRefs;
jobject    jwx;
jobject    jsrchPksTree;
jobject    jrqdPksTree;
char       class_buf[GAP_CLASS_BUFSIZE];
jclass     psClass;
char       pktree_buf[GAP_CLASS_BUFSIZE];
char       sig_buf[GAP_CLASS_BUFSIZE];
jmethodID  mid;
jobject    jnewPksTree;
GLRtnCode  ret_code;

/* construct java format inputs */

env = GAP_get_jvm(java_class_path, error_message, error_message_length);
if (NULL == env)
   {
   return(GL_NOJVM);
   }

nRefs = 0;

jwx = GAP_get_jwidthequation(env, wx, error_message, error_message_length);
localRefs[nRefs++] = jwx;

jsrchPksTree = GAP_get_jpeaktreeset(env, searchpks, error_message,
                                    error_message_length);
localRefs[nRefs++] = jsrchPksTree;

jrqdPksTree = GAP_get_jpeaktreeset(env, curr_rqd, error_message,
                                   error_message_length);
localRefs[nRefs++] = jrqdPksTree;

if ((NULL == jwx) || (NULL == jsrchPksTree) || (NULL == jrqdPksTree))
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* find pruneRqdPks method */

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_PK_SRCH);
psClass = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = psClass;

if (NULL == psClass)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

sprintf_s(pktree_buf, GAP_CLASS_BUFSIZE, "java/util/TreeSet");
sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "(L%s/%s;L%s;L%s;)L%s;",
          GAP_CLASS_GA_PKG, GAP_CLASS_WX, pktree_buf, pktree_buf, pktree_buf);
mid = (*env)->GetStaticMethodID(env, psClass, "pruneRqdPks", sig_buf);
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find pruneRqdPks method in class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* call the method */

jnewPksTree = (*env)->CallStaticObjectMethod(env, psClass, mid, jwx,
                                             jsrchPksTree, jrqdPksTree);
localRefs[nRefs++] = jnewPksTree;

if (NULL == jnewPksTree)
   {
   sprintf_s(error_message, error_message_length,
             "pruneRqdPks returned NULL\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* convert to C */

ret_code = set_peak_list(env, jnewPksTree, new_rqd, error_message,
                         error_message_length);

GAP_delete_local_refs(env, localRefs, nRefs);

return(ret_code);
}

/* private utilities */

static GLRtnCode get_type_c(JNIEnv *env, jobject typej, GLPeakType *typec,
                            char *error_message, int error_message_length)
{
jobject     localRefs[5];
int         nRefs;
char        class_buf[GAP_CLASS_BUFSIZE];
jclass      typeClass;
jmethodID   mid;
jstring     typeLabel;
const char  *typeChars;
jboolean    isCopy;

nRefs = 0;

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s$TYPE", GAP_CLASS_GA_PKG,
          GAP_CLASS_PK);
typeClass = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = typeClass;

if (NULL == typeClass)
   {
   sprintf_s(error_message, error_message_length, "unable to find class %s\n",
             class_buf);
   return(GL_JNIERROR);
   }

mid = (*env)->GetMethodID(env, typeClass, "label",
						  "()Ljava/lang/String;");
if (NULL == mid)
   {
   strcpy_s(error_message, error_message_length,
            "unable to get label method of Peak.TYPE\n");
  GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

typeLabel = (*env)->CallObjectMethod(env, typej, mid);
localRefs[nRefs++] = typeLabel;

if (NULL == typeLabel)
   {
   strcpy_s(error_message, error_message_length,
            "Peak.TYPE.label() returned null\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

typeChars = (*env)->GetStringUTFChars(env, typeLabel, &isCopy);
if (NULL == typeChars)
   {
   strcpy_s(error_message, error_message_length,
            "failed to decode Peak.TYPE's label\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

if (0 == strncmp(typeChars, "C", 1))
   {
   *typec = GL_PEAK_CHANNEL;
   }
else
   {
   *typec = GL_PEAK_ENERGY;
   }

(*env)->ReleaseStringUTFChars(env, typeLabel, typeChars);
GAP_delete_local_refs(env, localRefs, nRefs);

return(GL_SUCCESS);
}

static GLRtnCode set_cross_correlations(JNIEnv *env, jclass resultsClass,
                                        jobject peakResultsObject,
                                        int *cross_products, int listlength,
                                        char *error_message,
                                        int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
char       sig_buf[GAP_CLASS_BUFSIZE];
jmethodID  midGetPrds;
jintArray  prdsArray;
int        top;
jsize      prds_count;

nRefs = 0;

/* get Java cross products */

sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()[I");
midGetPrds = (*env)->GetMethodID(env, resultsClass, "getCrossProducts",
                                 sig_buf);
if (NULL == midGetPrds)
   {
   sprintf_s(error_message, error_message_length,
   "unable to find getCrossProducts method for class PeakSearchResults\n");
   return(GL_JNIERROR);
   }

prdsArray = (*env)->CallObjectMethod(env, peakResultsObject, midGetPrds);
localRefs[nRefs++] = prdsArray;

if (NULL == prdsArray)
   {
   sprintf_s(error_message, error_message_length,
             "PeakSearchResults.getCrossProducts() returned NULL\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* copy the cross products to the C array */

prds_count = (*env)->GetArrayLength(env, prdsArray);
top = GAP_min(listlength, prds_count);

(*env)->GetIntArrayRegion(env, prdsArray, 0, top, cross_products);

GAP_delete_local_refs(env, localRefs, nRefs);

return(GL_SUCCESS);
}

static GLRtnCode set_peak_list(JNIEnv *env, const jobject peakTreeSet,
                               GLPeakList *peaklist, char *error_message,
                               int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
jobject    *jpeakArray;
int        npeaks;
char       peak_class_name[GAP_CLASS_BUFSIZE];
jclass     peak_class;
char       type_buf[GAP_CLASS_BUFSIZE];
jfieldID   type_fid;
jmethodID  chan_mid;
jmethodID  chanvalid_mid;
jmethodID  egy_mid;
jmethodID  egyvalid_mid;
jmethodID  sige_mid;
jmethodID  fix_mid;
int        i;
jobject    jtype;
GLRtnCode  ret_code;

nRefs = 0;

sprintf_s(peak_class_name, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_PK);

jpeakArray = GAP_get_array_from_jtreeset(env, peakTreeSet, peak_class_name,
                                         &npeaks, error_message,
                                         error_message_length);
if (NULL == jpeakArray)
   {
   return(GL_JNIERROR);
   }

if (peaklist->listlength < npeaks)
   {
   strcpy_s(error_message, error_message_length,
            "GLPeakList destination too small to hold answer\n");
   GAP_free_object_array(env, jpeakArray, npeaks);
   return(GL_FAILURE);
   }

if (0 == npeaks)
   {
   peaklist->npeaks = 0;
   return(GL_SUCCESS);
   }

peak_class = (*env)->GetObjectClass(env, jpeakArray[0]);
localRefs[nRefs++] = peak_class;

if (NULL == peak_class)
   {
   sprintf_s(error_message, error_message_length,
            "unable to get class of %s\n", peak_class_name);
   GAP_free_object_array(env, jpeakArray, npeaks);
   return(GL_JNIERROR);
   }

/* get the java field and getter method IDs */

sprintf_s(type_buf, GAP_CLASS_BUFSIZE, "L%s$TYPE;", peak_class_name);
type_fid = (*env)->GetFieldID(env, peak_class, "m_type", type_buf);

chan_mid = (*env)->GetMethodID(env, peak_class, "getChannel", "()D");

chanvalid_mid = (*env)->GetMethodID(env, peak_class, "isChannelValid", "()Z");

egy_mid = (*env)->GetMethodID(env, peak_class, "getEnergy", "()D");

egyvalid_mid = (*env)->GetMethodID(env, peak_class, "isEnergyValid", "()Z");

sige_mid = (*env)->GetMethodID(env, peak_class, "getSige", "()D");

fix_mid = (*env)->GetMethodID(env, peak_class, "isCentroidFixed", "()Z");

if ((NULL == type_fid) || (NULL == chan_mid) || (NULL == chanvalid_mid) ||
    (NULL == egy_mid) || (NULL == egyvalid_mid) || (NULL == sige_mid) ||
    (NULL == fix_mid))
   {
   sprintf_s(error_message, error_message_length,
             "unable to find field or method of %s\n", peak_class_name);
   GAP_delete_local_refs(env, localRefs, nRefs);
   GAP_free_object_array(env, jpeakArray, npeaks);
   return(GL_JNIERROR);
   }

/* loop over the peaks, setting the C structure */

peaklist->npeaks = 0;
for (i = 0; i < npeaks; i++)
   {
   jtype = (*env)->GetObjectField(env, jpeakArray[i], type_fid);
   if (NULL == jtype)
      {
      ret_code = GL_JNIERROR;
      sprintf_s(error_message, error_message_length,
                "unable to get %s.m_type value\n", peak_class_name);
      break;
      }

   ret_code = get_type_c(env, jtype, &(peaklist->peak[i].type),
                         error_message, error_message_length);
   (*env)->DeleteLocalRef(env, jtype);
   if (GL_SUCCESS != ret_code)
      {
      break;
      }

   peaklist->peak[i].channel =
      (*env)->CallDoubleMethod(env, jpeakArray[i], chan_mid);
   peaklist->peak[i].channel_valid =
      (*env)->CallBooleanMethod(env, jpeakArray[i], chanvalid_mid);
   peaklist->peak[i].energy =
      (*env)->CallDoubleMethod(env, jpeakArray[i], egy_mid);
   peaklist->peak[i].energy_valid =
      (*env)->CallBooleanMethod(env, jpeakArray[i], egyvalid_mid);
   peaklist->peak[i].sige =
      (*env)->CallDoubleMethod(env, jpeakArray[i], sige_mid);
   peaklist->peak[i].fixed_centroid =
      (*env)->CallBooleanMethod(env, jpeakArray[i], fix_mid);

   peaklist->npeaks++;
   }

GAP_delete_local_refs(env, localRefs, nRefs);
GAP_free_object_array(env, jpeakArray, npeaks);

return(ret_code);
}

static GLRtnCode set_peak_results(JNIEnv *env, jobject peakResultsObject,
                                  GLPeakSearchResults *results,
                                  char *error_message,
                                  int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
jclass     resultsClass;
char       peak_buf[GAP_CLASS_BUFSIZE];
char       sig_buf[GAP_CLASS_BUFSIZE];
jmethodID  midGetPks;
jobject    srchPkTreeObject;
char       class_buf[GAP_CLASS_BUFSIZE];
jobject    *jsrchpkArray;
int        jsrchpk_count;
jclass     searchPeakClass;
jmethodID  rawc_mid;
jmethodID  rfrgn_mid;
jmethodID  a_mid;
jmethodID  b_mid;
jmethodID  refc_mid;
jmethodID  use_mid;
int        i, top;
GLRtnCode  ret_code;

nRefs = 0;

/* get class of results object */

resultsClass = (*env)->GetObjectClass(env, peakResultsObject);
localRefs[nRefs++] = resultsClass;

if (NULL == resultsClass)
   {
   strcpy_s(error_message, error_message_length,
            "unable to determine class of PeakSearchResults object\n");
   return(GL_JNIERROR);
   }

/* get Java searchpeak list */

sprintf_s(peak_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_SRCH_PK);
sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()Ljava/util/TreeSet;");
midGetPks = (*env)->GetMethodID(env, resultsClass, "getSearchPeakList",
                                sig_buf);
if (NULL == midGetPks)
   {
   sprintf_s(error_message, error_message_length,
   "unable to find getSearchPeakList method for class PeakSearchResults\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

srchPkTreeObject = (*env)->CallObjectMethod(env, peakResultsObject, midGetPks);
localRefs[nRefs++] = srchPkTreeObject;

if (NULL == srchPkTreeObject)
   {
   sprintf_s(error_message, error_message_length,
             "PeakSearchResults.getSearchPeakList() returned NULL\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_SRCH_PK);
jsrchpkArray = GAP_get_array_from_jtreeset(env, srchPkTreeObject, class_buf,
                                           &jsrchpk_count, error_message,
                                           error_message_length);
if (NULL == jsrchpkArray)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

results->peaklist->npeaks = 0;
if (0 == jsrchpk_count)
   {
   return(GL_SUCCESS);
   }

/* get class of search peak object */

searchPeakClass = (*env)->GetObjectClass(env, jsrchpkArray[0]);
localRefs[nRefs++] = searchPeakClass;

if (NULL == searchPeakClass)
   {
   strcpy_s(error_message, error_message_length,
            "unable to determine class of SearchPeak object\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   GAP_free_object_array(env, jsrchpkArray, jsrchpk_count);
   return(GL_JNIERROR);
   }

/* get the getter method IDs */

rawc_mid = (*env)->GetMethodID(env, searchPeakClass, "getRawCentroid", "()I");

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_CHNRNG);
sprintf_s(sig_buf, GAP_CLASS_BUFSIZE, "()L%s;", class_buf);
rfrgn_mid = (*env)->GetMethodID(env, searchPeakClass, "getRefineRegion",
                                sig_buf);

a_mid = (*env)->GetMethodID(env, searchPeakClass, "getArea", "()D");

b_mid = (*env)->GetMethodID(env, searchPeakClass, "getBackground", "()D");

refc_mid = (*env)->GetMethodID(env, searchPeakClass, "getRefinedCentroid",
                               "()D");

use_mid = (*env)->GetMethodID(env, searchPeakClass, "useRefinement", "()Z");

if ((NULL == rawc_mid) || (NULL == rfrgn_mid) || (NULL == a_mid) ||
    (NULL == b_mid) || (NULL == refc_mid) || (NULL == use_mid))
   {
   sprintf_s(error_message, error_message_length,
   "unable to find method of class SearchPeak\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   GAP_free_object_array(env, jsrchpkArray, jsrchpk_count);
   return(GL_JNIERROR);
   }

top = GAP_min(results->peaklist->listlength, jsrchpk_count);

for (i = 0; i < top; i++)
   {
   ret_code = set_search_peak(env, rawc_mid, rfrgn_mid, a_mid, b_mid, refc_mid,
                              use_mid, jsrchpkArray[i], i, results,
                              error_message, error_message_length);
   if (GL_SUCCESS != ret_code)
      {
      GAP_delete_local_refs(env, localRefs, nRefs);
      GAP_free_object_array(env, jsrchpkArray, jsrchpk_count);
      return(ret_code);
      }

   results->peaklist->npeaks++;
   }

/* copy the cross products to the C array */

ret_code = set_cross_correlations(env, resultsClass, peakResultsObject,
                                  results->crosscorrs,
                                  results->peaklist->listlength, error_message,
                                  error_message_length);

GAP_delete_local_refs(env, localRefs, nRefs);
GAP_free_object_array(env, jsrchpkArray, jsrchpk_count);

return(ret_code);
}

static GLRtnCode set_search_peak(JNIEnv *env, jmethodID rawc_mid,
                                 jmethodID rfrgn_mid, jmethodID a_mid,
                                 jmethodID b_mid, jmethodID refc_mid,
                                 jmethodID use_mid, jobject searchPeakObject,
                                 int index, GLPeakSearchResults *results,
                                 char *error_message,
                                 int error_message_length)
{
jobject    localRefs[5];
int        nRefs;
jobject    jregion;
GLRtnCode  ret_code;

nRefs = 0;

/* set raw channel */

results->refinements[index].raw_channel =
   (*env)->CallIntMethod(env, searchPeakObject, rawc_mid);

/* set refine region */

jregion = (*env)->CallObjectMethod(env, searchPeakObject, rfrgn_mid);
localRefs[nRefs++] = jregion;

if (NULL == jregion)
   {
   sprintf_s(error_message, error_message_length,
             "failed to get refine region from search peak\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

ret_code = GAP_set_chanrange(env, jregion,
                             &(results->refinements[index].refine_region),
                             error_message, error_message_length);

if (GL_SUCCESS != ret_code)
   {
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(ret_code);
   }

/* set area */

results->refinements[index].net_area =
   (*env)->CallDoubleMethod(env, searchPeakObject, a_mid);

/* set background */

results->refinements[index].background =
   (*env)->CallDoubleMethod(env, searchPeakObject, b_mid);

/* set refined channel */

results->refinements[index].refined_channel =
   (*env)->CallDoubleMethod(env, searchPeakObject, refc_mid);

/* set use refinement */

results->refinements[index].use_refinement =
   (*env)->CallBooleanMethod(env, searchPeakObject, use_mid);

/* set the GLPeak */

results->peaklist->peak[index].type = GL_PEAK_CHANNEL;

if (JNI_TRUE == results->refinements[index].use_refinement)
   {
   results->peaklist->peak[index].channel =
      results->refinements[index].refined_channel;
   }
else
   {
   results->peaklist->peak[index].channel =
      results->refinements[index].raw_channel;
   }

results->peaklist->peak[index].channel_valid = GL_TRUE;
results->peaklist->peak[index].energy = 0;
results->peaklist->peak[index].sige = 0;
results->peaklist->peak[index].energy_valid = GL_FALSE;
results->peaklist->peak[index].fixed_centroid = GL_FALSE;

GAP_delete_local_refs(env, localRefs, nRefs);

return(GL_SUCCESS);
}
