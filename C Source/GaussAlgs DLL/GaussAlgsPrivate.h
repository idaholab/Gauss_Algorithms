/*
 * Copyright 2017 Battelle Energy Alliance
 */

/*
 *  GaussAlgsPrivate.h - contains typedefs and prototypes for private
 *                       procedures used in the Gauss Algorithms Library
 */

#ifndef GAUSSALGSPRIVATE_H
#define GAUSSALGSPRIVATE_H

/*
 * I got these macros from the now obsolete /usr/include/macros.h
 * Do not use anything too fancy for arguments...  such as incrementing.
 *
 * I recommend that arguments a and b be enclosed in parentheses.
 */

#define GAP_max(a,b) 	(a<b ? b : a)
#define GAP_min(a,b) 	(a>b ? b : a)


/* define strings for all of the java classes */
#define GAP_CLASS_BUFSIZE 1024
#define GAP_CLASS_GA_PKG "gov/inl/gaussAlgorithms"
#define GAP_CLASS_BACK "BackgroundEquation"
#define GAP_CLASS_CHNRNG "ChannelRange"
#define GAP_CLASS_CURVE "Curve"
#define GAP_CLASS_ECAL "EnergyCalibrating"
#define GAP_CLASS_EX "EnergyEquation"
#define GAP_CLASS_FIT "Fit"
#define GAP_CLASS_FIT_IN "FitInputs"
#define GAP_CLASS_FIT_PARM "FitParameters"
#define GAP_CLASS_PK "Peak"
#define GAP_CLASS_PK_SUMM "PeakSummary"
#define GAP_CLASS_PK_SRCH "PeakSearching"
#define GAP_CLASS_PK_SRCH_RSLTS "PeakSearchResults"
#define GAP_CLASS_RGN_FIT "RegionFitting"
#define GAP_CLASS_RGN_SRCH "RegionSearching"
#define GAP_CLASS_RGN_SRCHPARM "RegionSearchParameters"
#define GAP_CLASS_SRCH_PK "SearchPeak"
#define GAP_CLASS_SPEC "Spectrum"
#define GAP_CLASS_SUMM "Summary"
#define GAP_CLASS_VERSION "Version"
#define GAP_CLASS_WCAL "WidthCalibrating"
#define GAP_CLASS_WX "WidthEquation"



/*
 * Prototypes for private procedures in the Gauss Library
 */

#ifdef __cplusplus
extern "C" {
#endif


/*
 * GAP_curve_alloc
 *
 *    allocate memory for the curve structure.
 *    listlength is set in the returned structure.
 *
 *    If routine fails, returns NULL.
 */

   GLCurve *GAP_curve_alloc(int nchannels, int nplots_per_chan, int npeaks);


/*
 * GAP_curve_free
 *
 *    free curve structure memory that was allocated with GL_curve_alloc().
 */

   void GAP_curve_free(GLCurve *curve);


/*
 * GAP_delete_local_refs
 *
 *    convenience routine to release all previously created and stored
 *    local java references. If any reference is NULL, it is skipped.
 */

   void GAP_delete_local_refs(JNIEnv *env, jobject *localRefs, int nRefs);


/*
 * GAP_fitrec_free
 *
 *    free fitrecord structure memory that was allocated with
 *    GAP_fitrec_alloc().
 */

   void GAP_fitrec_free(GLFitRecord *fitrec);


/*
 * GAP_free_object_array
 *
 *    free memory that was allocated with GAP_get_array_from_jtreeset
 *    (or allocated with get_array_from_jvector() in RegionFitting).
 */

   void GAP_free_object_array(JNIEnv *env, jobject *objectArray,
                              int arrayLength);


/*
 * GAP_get_array_from_jtreeset
 *
 *    extract an array of Java objects from a Java Treeset.
 *
 *    If routine fails, returns NULL.
 */

   jobject *GAP_get_array_from_jtreeset(JNIEnv *env, const jobject tree_object,
                                        const char *class_name,
                                        int *array_length, char *error_message,
                                        int error_message_length);


/*
 * GAP_get_exception_message
 *
 *    extract a character array from the Java Exception.
 */

   GLRtnCode GAP_get_exception_message(JNIEnv *env, const jthrowable exception,
                                       char *message_buffer, int buffer_length,
                                       char *error_message,
                                       int error_message_length);


/*
 * GAP_get_jboolean
 *
 *    return the jboolean equivalent of the GLboolean passed in.
 */

   jboolean GAP_get_jboolean(GLboolean value);


/*
 * GAP_get_jchannelrange
 *
 *    construct and return a Java channel range object.
 *
 *    If routine fails, returns NULL.
 */

   jobject GAP_get_jchannelrange(JNIEnv *env, const GLChanRange chanrange,
                                 char *error_message,
                                 int error_message_length);


/*
 * GAP_get_jdouble_array
 *
 *    construct and return a jdoubleArray.
 *
 *    If routine fails, returns NULL.
 */

   jdoubleArray GAP_get_jdouble_array(JNIEnv *env, const double *data,
								      int numDataItems);


/*
 * GAP_get_jenergyequationmode
 *
 *    construct and return a Java energy equation mode object.
 *
 *    If routine fails, returns NULL.
 */

   jobject GAP_get_jenergyequationmode(JNIEnv *env, GLEgyEqnMode mode,
                                       char *error_message,
                                       int error_message_length);


/*
 * GAP_get_jpeaktreeset
 *
 *    construct and return a Java Treeset containing Peak objects.
 *
 *    If routine fails, returns NULL.
 */

   jobject GAP_get_jpeaktreeset(JNIEnv *env, const GLPeakList *peaks,
                                char *error_message, int error_message_length);


/*
 * GAP_get_jspectrum
 *
 *    construct and return a Java Spectrum object.
 *
 *    If routine fails, returns NULL.
 */

   jobject GAP_get_jspectrum(JNIEnv *env, const GLSpectrum *spectrum,
                             char *error_message, int error_message_length);


/*
 * GAP_get_jvm
 *
 *    return a handle to a running Java Virtual Machine, launching the
 *    machine as needed.
 *
 *    If routine fails, returns NULL.
 */

   JNIEnv *GAP_get_jvm(const char *javaClassPath, char *errMsg,
                       int errMsgLength);


/*
 * GAP_get_jwidthequation
 *
 *    construct and return a Java width equation object.
 *
 *    If routine fails, returns NULL.
 */

   jobject GAP_get_jwidthequation(JNIEnv *env, const GLWidthEqn *wx,
                                  char *error_message,
                                  int error_message_length);


/*
 * GAP_get_jwidthequationmode
 *
 *    construct and return a Java width equation mode object.
 *
 *    If routine fails, returns NULL.
 */

   jobject GAP_get_jwidthequationmode(JNIEnv *env, GLWidEqnMode mode,
                                      char *error_message,
                                      int error_message_length);


/*
 * GAP_set_boolean
 *
 *    set the GLboolean variable using the provided jboolean value.
 */

   void GAP_set_boolean(jboolean jvalue, GLboolean *value);


/*
 * GAP_set_chanrange
 *
 *    set the GLChanRange variable with the value extracted from
 *    the Java object.
 */

   GLRtnCode GAP_set_chanrange(JNIEnv *env, const jobject jchanrange,
                               GLChanRange *chanrange, char *error_message,
                               int error_message_length);


/*
 * GAP_summ_alloc
 *
 *    allocate memory for the summary structure.
 *    Enter npeaks for the listlength.
 *    Listlength is set in the returned structure.
 *
 *    If routine fails, returns NULL.
 */

   GLSummary *GAP_summ_alloc(int listlength);


/*
 * GAP_summ_free
 *
 *    free summary structure memory that was allocated with GL_summ_alloc().
 */

   void GAP_summ_free(GLSummary *summary);


#ifdef __cplusplus
}
#endif


#endif  /* GAUSSALGSPRIVATE_H */
