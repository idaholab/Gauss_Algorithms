/*
 * Copyright 2017 Battelle Energy Alliance
 */

/*
 *  GaussAlgsLib.c - contains JNI wrapper
 */


#include <jni.h>
#include <stdlib.h>            /* calloc(), exit(), NULL */
#include <string.h>            /* strcpy_s(), strcat_s() */
#include <math.h>		       /* for log, sqrt */
#include "GaussAlgsLib.h"
#include "GaussAlgsPrivate.h"


GLRtnCode GL_add_chanpeak(double channel, GLPeakList *peaks)
{
int	i;

i = peaks->npeaks;

if ((i+1) > peaks->listlength)
   return(GL_OVRLMT);

peaks->peak[i].type = GL_PEAK_CHANNEL;
peaks->peak[i].channel_valid = GL_TRUE;
peaks->peak[i].channel = channel;
peaks->peak[i].energy_valid = GL_FALSE;
peaks->peak[i].fixed_centroid = GL_FALSE;
peaks->npeaks++;

return(GL_SUCCESS);
}

GLRtnCode GL_add_egypeak(double energy, double sige, GLPeakList *peaks)
{
int	i;

i = peaks->npeaks;

if ((i+1) > peaks->listlength)
   return(GL_OVRLMT);

peaks->peak[i].type = GL_PEAK_ENERGY;
peaks->peak[i].channel_valid = GL_FALSE;
peaks->peak[i].energy_valid = GL_TRUE;
peaks->peak[i].energy = energy;
peaks->peak[i].sige = sige;
peaks->peak[i].fixed_centroid = GL_TRUE;
peaks->npeaks++;

return(GL_SUCCESS);
}

GLRtnCode GL_add_peak(const GLPeak *peak, GLPeakList *peaks)
{
int	i;

i = peaks->npeaks;

if ((i+1) > peaks->listlength)
   return(GL_OVRLMT);

peaks->peak[i].type = peak->type;
peaks->peak[i].channel_valid = peak->channel_valid;
peaks->peak[i].channel = peak->channel;
peaks->peak[i].energy_valid = peak->energy_valid;
peaks->peak[i].energy = peak->energy;
peaks->peak[i].sige = peak->sige;
peaks->peak[i].fixed_centroid = peak->fixed_centroid;
peaks->npeaks++;

return(GL_SUCCESS);
}

void GL_chan_to_e(const GLEnergyEqn *ex, double channel, double *energy)
{
switch(ex->mode)
   {
   case GL_EGY_LINEAR:
      *energy = ex->a + (ex->b * channel);
      break;
   case GL_EGY_QUADRATIC:
   default:
      *energy = ex->a + (ex->b * channel) + (ex->c * channel * channel);
      break;
   }
}

GLRtnCode GL_chan_to_w(const GLWidthEqn *wx, double channel, double *width)
{
double	temp;

temp = wx->alpha + (wx->beta * channel);

switch(wx->mode)
   {
   case GL_WID_LINEAR:
      *width = temp;
      return(GL_SUCCESS);
      /* break; */
   case GL_WID_SQRT:
   default:
      if (temp < 0)
         return(GL_FAILURE);
      else
         {
         *width = sqrt(temp);
         return(GL_SUCCESS);
         }
      /* break; */
   }
}

GLRtnCode GL_e_to_chan(const GLEnergyEqn *ex, double energy, double *channel)
{
double	bsqr_4ac; 	/* from quadratic formula for ax**2 + bx + c */

if ((ex->mode == GL_EGY_LINEAR) || (ex->c == 0.0))
   {
   if (ex->b == 0.0)
      return(GL_FAILURE);
   else
      *channel = (energy - ex->a) / ex->b;
   }
else
   {
   bsqr_4ac = (ex->b * ex->b) - (4.0 * ex->c * (ex->a - energy));

   if (bsqr_4ac < 0.0)
      return(GL_FAILURE);
   else
      *channel = GAP_max(0.0, (-ex->b + sqrt(bsqr_4ac)) / (2.0 * ex->c));
   }

return(GL_SUCCESS);
}

void GL_fitreclist_free(GLFitRecList *fitreclist)
{
if (fitreclist == NULL)
   return;

if (fitreclist->next != NULL)
   GL_fitreclist_free(fitreclist->next);

GAP_fitrec_free(fitreclist->record);
free(fitreclist);

return;
}

GLRtnCode GL_get_regnpks(const GLChanRange *region, const GLPeakList *peaks,
                         GLPeakList *pks_in_rgn)
{
int	i;

if (pks_in_rgn->listlength <= 0)
   return(GL_OVRLMT);

pks_in_rgn->npeaks = 0;

for (i = 0; i < peaks->npeaks; i++)
   {
   if ((peaks->peak[i].channel_valid == GL_TRUE) &&
       (peaks->peak[i].channel >= region->first) &&
       (peaks->peak[i].channel <= region->last))
      {
      if (GL_add_peak(&peaks->peak[i], pks_in_rgn) == GL_OVRLMT)
         {
         return(GL_OVRLMT);
         }
      }
   }

return(GL_SUCCESS);
}

GLRtnCode GL_get_version(const char *java_class_path, char *version,
                         int version_length, char *error_message,
                         int error_message_length)
{
JNIEnv      *env;
jobject     localRefs[10];
int         nRefs;
char        class_buf[GAP_CLASS_BUFSIZE];
jclass      verClass;
jmethodID   mid;
jstring     jversion;
jboolean    isCopy;
const char  *version_chars;

nRefs = 0;

/* set up default answer */
strcpy_s(version, version_length, "unknown");

/* look for JVM */

env = GAP_get_jvm(java_class_path, error_message, error_message_length);
if (NULL == env)
   {
   return(GL_NOJVM);
   }

/* get the java method */

sprintf_s(class_buf, GAP_CLASS_BUFSIZE, "%s/%s", GAP_CLASS_GA_PKG,
          GAP_CLASS_VERSION);
verClass = (*env)->FindClass(env, class_buf);
localRefs[nRefs++] = verClass;

if (NULL == verClass)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find class %s", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

mid = (*env)->GetStaticMethodID(env, verClass, "getVersion",
                                "()Ljava/lang/String;");
if (NULL == mid)
   {
   sprintf_s(error_message, error_message_length,
             "unable to find getVersion method in class %s\n", class_buf);
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* invoke the java method */

jversion = (*env)->CallStaticObjectMethod(env, verClass, mid);
localRefs[nRefs++] = jversion;

/* translate java string to C */

if (NULL == jversion)
   {
   sprintf_s(error_message, error_message_length,
             "getVersion method returned NULL\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

/* copy message into answer */

version_chars = (*env)->GetStringUTFChars(env, jversion, &isCopy);
if (NULL == version_chars)
   {
   sprintf_s(error_message, error_message_length,
             "unable to get chars of version\n");
   GAP_delete_local_refs(env, localRefs, nRefs);
   return(GL_JNIERROR);
   }

sprintf_s(version, version_length, "%s", version_chars);

if (JNI_TRUE == isCopy)
   {
   (*env)->ReleaseStringUTFChars(env, jversion, version_chars);
   }

GAP_delete_local_refs(env, localRefs, nRefs);

return(GL_SUCCESS);
}

GLPeakSearchResults *GL_peak_results_alloc(int peak_listlength,
                                           int spectrum_nchannels)
{
GLPeakSearchResults	*results;

if ((results = (GLPeakSearchResults *)
	malloc(sizeof(GLPeakSearchResults))) == NULL)
   return(NULL);

if ((results->peaklist = GL_peaks_alloc(peak_listlength)) == NULL)
   {
   free(results);
   return(NULL);
   }

if ((results->refinements = (GLPeakRefinement *)
	calloc(peak_listlength, sizeof(GLPeakRefinement))) == NULL)
   {
   GL_peaks_free(results->peaklist);
   free(results);
   return(NULL);
   }

if ((results->crosscorrs = (int *)
	calloc(spectrum_nchannels, sizeof(int))) == NULL)
   {
   free(results->refinements);
   GL_peaks_free(results->peaklist);
   free(results);
   return(NULL);
   }
results->listlength = spectrum_nchannels;

return(results);
}

void GL_peak_results_free(GLPeakSearchResults *results)
{
free(results->crosscorrs);
free(results->refinements);
GL_peaks_free(results->peaklist);
free(results);
}

GLPeakList *GL_peaks_alloc(int listlength)
{
GLPeakList	*peaks;

if ((peaks = (GLPeakList *) malloc(sizeof(GLPeakList))) == NULL)
   return(NULL);

if ((peaks->peak = (GLPeak *) calloc(listlength, sizeof (GLPeak))) == NULL)
   {
   free(peaks);
   return(NULL);
   }
peaks->listlength = listlength;

return(peaks);
}

void GL_peaks_free(GLPeakList *peaks)
{
free(peaks->peak);
free(peaks);
}

GLRegions *GL_regions_alloc(int listlength)
{
GLRegions	*regions;

if ((regions = (GLRegions *) malloc(sizeof(GLRegions))) == NULL)
   return(NULL);

if ((regions->chanrange =
       (GLChanRange *) calloc(listlength, sizeof (GLChanRange))) == NULL)
   {
   free(regions);
   return(NULL);
   }
regions->listlength = listlength;

return(regions);
}

void GL_regions_free(GLRegions *regions)
{
free(regions->chanrange);
free(regions);
}

GLRtnCode GL_spectrum_counts_alloc(GLSpectrum *spectrum, int listlength)
{
#if (! defined(GL_LINUX)) && (! defined(GL_MACOSX))
/* for Visual C++ */
spectrum->count = (__int32 *) calloc(listlength, sizeof(__int32));
#else
spectrum->count = (int *) calloc(listlength, sizeof(int));
#endif

if (NULL == spectrum->count)
   {
   spectrum->listlength = 0;
   spectrum->nchannels = 0;
   return(GL_BADMALLOC);
   }

spectrum->listlength = listlength;
spectrum->nchannels = 0;

return(GL_SUCCESS);
}

void GL_spectrum_counts_free(GLSpectrum *spectrum)
{
free(spectrum->count);
spectrum->listlength = 0;
spectrum->nchannels = 0;
}

void GL_update_peaklist(const GLEnergyEqn *ex, GLPeakList *peaks)
{
int	i;

if (ex != NULL)
   {
   for (i = 0; i < peaks->npeaks; i++)
      switch(peaks->peak[i].type)
         {
         case GL_PEAK_CHANNEL:
            GL_chan_to_e(ex, peaks->peak[i].channel, &peaks->peak[i].energy);
            peaks->peak[i].sige = 0;
            peaks->peak[i].energy_valid = GL_TRUE;
            break;
         case GL_PEAK_ENERGY:
            GL_e_to_chan(ex, peaks->peak[i].energy, &peaks->peak[i].channel);
            peaks->peak[i].channel_valid = GL_TRUE;
            break;
         default:
            break;
         }
   }
else
   {
   for (i = 0; i < peaks->npeaks; i++)
      switch(peaks->peak[i].type)
         {
         case GL_PEAK_CHANNEL:
            peaks->peak[i].energy_valid = GL_FALSE;
            break;
         case GL_PEAK_ENERGY:
            peaks->peak[i].channel_valid = GL_FALSE;
            break;
         default:
            break;
         }
   }
}
