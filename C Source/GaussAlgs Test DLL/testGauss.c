/*
 * Copyright 2017 Battelle Energy Alliance
 */

/*
 * testGauss.c - contains a program for testing the C JNI wrapper
 *               of the all-Java implementation of Gauss Algorithms
 */

#include <stdio.h>	/* fprintf */
#include <stdlib.h>	/* exit */
#include <string.h> /* strcpy_s */
#include <GaussAlgsLib.h>
#include "SpecFileLib.h"
#include "SFChnFile.h"

#define TRUE_STRING "TRUE"
#define FALSE_STRING "FALSE"
#define CYCLE_DONE_STRING "DONE"
#define CYCLE_DELETE_STRING "DELETE"
#define CYCLE_ADD_STRING "ADD"
#define CYCLE_CONTINUE_STRING "CONTINUE"
#define CYCLE_UNKNOWN_STRING "UNKNOWN"
#define PKWD_VARIES_STRING "VARIES"
#define PKWD_FIXED_STRING "FIXED"
#define CC_LARGER_STRING "LARGER"
#define CC_SMALLER_STRING "SMALLER"
#define CC_LARGERINC_STRING "LARGER INCREASING"
#define UNDEFINED "UNDEFINED"

/* prototypes */
static char *get_boolean_string(GLboolean value);
static char *get_cc_type_string(GLCCType type);
static char *get_cycle_return_string(GLCycleReturn cycle_return);
static char *get_pkwd_mode_string(GLPkwdMode mode);
static SFReturnCode read_spectrum(const char* spec_path, GLSpectrum *spectrum,
                                  GLEnergyEqn *ex, GLWidthEqn *wx,
                                  char *error_message,
                                  int error_message_length);
static void set_ex_string(const GLEnergyEqn *ex, char *ex_string,
                          int ex_string_length);
static void set_wx_string(const GLWidthEqn *wx, char *wx_string,
                          int wx_string_length);
static GLRtnCode test_ecalib(const char *java_class_path, GLEgyEqnMode mode,
                             GLboolean weighted, GLEnergyEqn *ex,
                             char *error_message, int error_message_length);
static GLRtnCode test_exceeds_width(const char *java_class_path,
                                    const GLRegions *regions,
                                    int max_width_channels,
                                    char *error_message,
                                    int error_message_length);
static GLRtnCode test_fit(const char *java_class_path,
                          const GLChanRange *fit_region,
                          const GLSpectrum *spectrum,
                          const GLPeakList *peaklist, const GLFitParms *parms,
                          const GLEnergyEqn *ex, const GLWidthEqn *wx,
                          char *error_message, int error_message_length);
static GLRtnCode test_get_regn_pks(const GLChanRange *region,
                                   const GLPeakList *peaks,
                                   GLPeakList *pks_in_rgn,
                                   char *error_message,
                                   int error_message_length);
static GLRtnCode test_neg_alarms(const char *java_class_path,
                                 const GLChanRange *fit_region,
                                 const GLSpectrum *spectrum,
                                 const GLPeakList *peaklist,
                                 const GLFitParms *parms,
                                 const GLEnergyEqn *ex,
                                 const GLWidthEqn *wx,
                                 char *error_message,
                                 int error_message_length);
static GLRtnCode test_outside_alarm(const char *java_class_path,
                                    const GLChanRange *fit_region,
                                    const GLSpectrum *spectrum,
                                    const GLPeakList *peaklist,
                                    const GLFitParms *parms,
                                    const GLEnergyEqn *ex,
                                    const GLWidthEqn *wx,
                                    char *error_message,
                                    int error_message_length);
static GLRtnCode test_pksrch(const char *java_class_path, const GLWidthEqn *wx,
                             int srch_threshold, const GLSpectrum *spectrum,
                             GLPeakSearchResults *results,
                             const GLEnergyEqn *ex,
                             char *error_message, int error_message_length);
static GLRtnCode test_prune_pks(const char *java_class_path,
                                const GLWidthEqn *wx,
                                const GLPeakList *searchpks,
                                const GLEnergyEqn *ex,
                                char *error_message,
                                int error_message_length);
static GLRtnCode test_rgnsrch(const char *java_class_path,
                              const GLWidthEqn *wx, double srch_threshold,
                              int irw, int irch, const GLSpectrum *spectrum,
                              const GLPeakList *peaklist, GLRgnSrchMode mode,
                              int maxrgnwid, GLRegions *regions,
                              char *error_message, int error_message_length);
static GLRtnCode test_wcalib(const char *java_class_path, GLWidEqnMode mode,
                             GLboolean weighted, GLWidthEqn *wx,
                             char *error_message, int error_message_length);


int main(int argc, char *argv[])
{
char                 *java_class_path;
char                 *test_spectrum_name;
char                 version[256];
GLEnergyEqn          ex;
GLWidthEqn           wx;
double               maxWidthPossible;
GLSpectrum           spectrum;
SFReturnCode         read_rc;
int                  pksrch_threshold;
GLPeakSearchResults  *results;
GLChanRange          region;
GLPeakList           *pks_in_rgn;
double               rgnsrch_threshold;
int                  irw;
int                  irch;
int                  maxrgnwid;
GLRegions            *regions;
GLChanRange          fit_region;
GLPeakList           *fit_peaks;
GLFitParms           fitparms;
char                 message[2056];
int                  message_length = 2056;
GLRtnCode            ret_code;

if (2 > argc)
   {
   fprintf_s(stdout, "USAGE: %s <java class path> <spectrum name>",
             argv[0]);
   exit(-1);
   }

java_class_path = argv[1];
test_spectrum_name = argv[2];

/* test the DLL */

ret_code = GL_get_version(java_class_path, version, 256, message,
                          message_length);

if (GL_SUCCESS != ret_code)
   {
   fprintf_s(stdout, "GL_get_version error: %s\n", message);
   exit(-ret_code);
   }
else
   {
   fprintf_s(stdout, "Gauss Version is %s\n\n", version);
   }

/* test Java energy calibration */

ret_code = test_ecalib(java_class_path, GL_EGY_QUADRATIC, GL_FALSE, &ex,
                       message, message_length);

if (GL_SUCCESS != ret_code)
   {
   fprintf_s(stdout, "test_ecalib error: %s\n", message);
   exit(-ret_code);
   }
else
   {
   fprintf_s(stdout, "test_ecalib returned success\n\n");
   }

/* test Java width calibration */

ret_code = test_wcalib(java_class_path, GL_WID_SQRT, GL_FALSE, &wx,
                       message, message_length);

if (GL_SUCCESS != ret_code)
   {
   fprintf_s(stdout, "test_wcalib error: %s\n", message);
   exit(-ret_code);
   }
else
   {
   fprintf_s(stdout, "test_wcalib returned success\n\n");
   }

/* set up spectrum */

ret_code = GL_spectrum_counts_alloc(&spectrum, 8192);

read_rc = read_spectrum(test_spectrum_name, &spectrum, &ex, &wx, message,
                        message_length);
if (read_rc != SF_SUCCESS)
   {
   fprintf_s(stdout, "error reading CHN: %s\n", message);
   exit(-read_rc);
   }

/* test using width calibration */

ret_code = GL_chan_to_w(&wx, 8191, &maxWidthPossible);

if (GL_SUCCESS != ret_code)
   {
   fprintf_s(stdout, "GL_chan_to_w error: %d\n", ret_code);
   exit(-ret_code);
   }
else
   {
   fprintf_s(stdout, "peak width at channel 8191 is %f\n\n", maxWidthPossible);
   }

/* test Java peak searching */

pksrch_threshold = 10;

results = GL_peak_results_alloc(spectrum.nchannels, spectrum.nchannels);
if (NULL == results)
   {
   fprintf_s(stdout, "unable to allocate space for peak searching\n");
   exit(-GL_BADMALLOC);
   }

ret_code = test_pksrch(java_class_path, &wx, pksrch_threshold, &spectrum,
                       results, &ex, message, message_length);

if (GL_SUCCESS != ret_code)
   {
   fprintf_s(stdout, "test_pksrch error: %s\n", message);
   exit(-ret_code);
   }
else
   {
   fprintf_s(stdout, "test_pksrch returned success\n\n");
   }

/* test making sublist of peaks residing in indicated region */

region.first = 1579;
region.last = 1610;
pks_in_rgn = GL_peaks_alloc(results->peaklist->npeaks);

ret_code = test_get_regn_pks(&region, results->peaklist, pks_in_rgn,
                             message, message_length);

if (GL_SUCCESS != ret_code)
   {
   fprintf_s(stdout, "test_get_regn_pks error: %s\n", message);
   exit(-ret_code);
   }
else
   {
   fprintf_s(stdout, "test_get_regn_pks returned success\n\n");
   }

/* test GL_prune_rqdpks */

ret_code = test_prune_pks(java_class_path, &wx, results->peaklist, &ex,
                          message, message_length);
if (GL_SUCCESS != ret_code)
   {
   fprintf_s(stdout, "test_prune_pks error: %s\n", message);
   exit(-ret_code);
   }
else
   {
   fprintf_s(stdout, "test_prune_pks returned success\n\n");
   }

/* test Java region searching */

rgnsrch_threshold = 2;
irw = 3;
irch = 2;
maxrgnwid = 150;

regions = GL_regions_alloc(spectrum.nchannels);
regions->nregions = 0;

ret_code = test_rgnsrch(java_class_path, &wx, rgnsrch_threshold, irw, irch,
                        &spectrum, results->peaklist, GL_RGNSRCH_FORPKS,
                        maxrgnwid, regions, message, message_length);
if (GL_SUCCESS != ret_code)
   {
   fprintf_s(stdout, "test_rgnsrch error: %s\n", message);
   exit(-ret_code);
   }
else
   {
   fprintf_s(stdout, "test_rgnsrch returned success\n\n");
   }

/* test GL_exceeds_width */

ret_code = test_exceeds_width(java_class_path, regions, maxrgnwid, message,
                              message_length);
if (GL_SUCCESS != ret_code)
   {
   fprintf_s(stdout, "test_exceeds_width error: %s\n", message);
   exit(-ret_code);
   }
else
   {
   fprintf_s(stdout, "test_exceeds_width returned success\n\n");
   }

ret_code = test_exceeds_width(java_class_path, regions, 5, message,
                              message_length);
if (GL_SUCCESS != ret_code)
   {
   fprintf_s(stdout, "test_exceeds_width error: %s\n", message);
   exit(-ret_code);
   }
else
   {
   fprintf_s(stdout, "test_exceeds_width returned success\n\n");
   }

/* test Java region fitting */

fit_region.first = 1579;
fit_region.last = 1610;
fit_peaks = GL_peaks_alloc(2);
fit_peaks->npeaks = 0;
ret_code = GL_add_chanpeak(1592.08, fit_peaks);
ret_code = GL_add_chanpeak(1600.88, fit_peaks);
fitparms.cc_type = GL_CC_LARGER;
fitparms.max_npeaks = 10;
fitparms.max_resid = 20;
fitparms.ncycle = 10;
fitparms.nout = 1;
fitparms.pkwd_mode = GL_PKWD_VARIES;

ret_code = test_fit(java_class_path, &fit_region, &spectrum, fit_peaks,
                    &fitparms, &ex, &wx, message, message_length);
if (GL_SUCCESS != ret_code)
   {
   fprintf_s(stdout, "test_fit error: %s\n", message);
   exit(-ret_code);
   }
else
   {
   fprintf_s(stdout, "test_fit returned success\n\n");
   }

/* test outsidepeak alarm */

fit_region.first = 740;
fit_region.last = 761;
GL_peaks_free(fit_peaks);
fit_peaks = GL_peaks_alloc(2);
fit_peaks->npeaks = 0;
ret_code = GL_add_chanpeak(748.19, fit_peaks);
ret_code = GL_add_chanpeak(752.49, fit_peaks);

ret_code = test_outside_alarm(java_class_path, &fit_region, &spectrum,
                              fit_peaks, &fitparms, &ex, &wx,
                              message, message_length);
if (GL_SUCCESS != ret_code)
   {
   fprintf_s(stdout, "test_outside_alarm error: %s\n", message);
   exit(-ret_code);
   }
else
   {
   fprintf_s(stdout, "test_outside_alarm returned success\n\n");
   }

/* test for both neg peak alarm and pos-neg peak pair alarm */

fit_region.first = 1152;
fit_region.last = 1169;
GL_peaks_free(fit_peaks);
fit_peaks = GL_peaks_alloc(4);
fit_peaks->npeaks = 0;
ret_code = GL_add_chanpeak(1157.82, fit_peaks);
ret_code = GL_add_chanpeak(1161.21, fit_peaks);
ret_code = GL_add_chanpeak(1162.49, fit_peaks);
ret_code = GL_add_chanpeak(1165.17, fit_peaks);

ret_code = test_neg_alarms(java_class_path, &fit_region, &spectrum,
                             fit_peaks, &fitparms, &ex, &wx,
                             message, message_length);
if (GL_SUCCESS != ret_code)
   {
   fprintf_s(stdout, "test_neg_alarms error: %s\n", message);
   exit(-ret_code);
   }
else
   {
   fprintf_s(stdout, "test_neg_alarms returned success\n\n");
   }

/* cleanup */

fprintf_s(stdout, "all tests complete\n");

GL_spectrum_counts_free(&spectrum);
GL_peak_results_free(results);
GL_peaks_free(pks_in_rgn);
GL_regions_free(regions);
GL_peaks_free(fit_peaks);
	
exit(0);
}

static char *get_boolean_string(GLboolean value)
{
char *answer;

if (GL_TRUE == value)
   {
   answer = TRUE_STRING;
   }
else
   {
   answer = FALSE_STRING;
   }

return(answer);
}

static char *get_cc_type_string(GLCCType type)
{
char *answer;

switch(type)
   {
   case GL_CC_LARGER:
      answer = CC_LARGER_STRING;
      break;
   case GL_CC_SMALLER:
      answer = CC_SMALLER_STRING;
      break;
   case GL_CC_LARGER_INC:
      answer = CC_LARGERINC_STRING;
      break;
   default:
      answer = UNDEFINED;
      break;
   }

return(answer);
}

static char *get_cycle_return_string(GLCycleReturn cycle_return)
{
char *answer;

switch(cycle_return)
   {
   case GL_CYCLE_DONE:
      answer = CYCLE_DONE_STRING;
      break;
   case GL_CYCLE_DELETE:
      answer = CYCLE_DELETE_STRING;
      break;
   case GL_CYCLE_ADD:
      answer = CYCLE_ADD_STRING;
      break;
   case GL_CYCLE_CONTINUE:
      answer = CYCLE_CONTINUE_STRING;
      break;
   default:
      answer = CYCLE_UNKNOWN_STRING;
      break;
   }

return(answer);
}

static char *get_pkwd_mode_string(GLPkwdMode mode)
{
char *answer;

switch(mode)
   {
   case GL_PKWD_VARIES:
      answer = PKWD_VARIES_STRING;
      break;
   case GL_PKWD_FIXED:
      answer = PKWD_FIXED_STRING;
      break;
   default:
      answer = UNDEFINED;
      break;
   }

return(answer);
}

static SFReturnCode read_spectrum(const char* spec_path, GLSpectrum *spectrum,
                                  GLEnergyEqn *ex, GLWidthEqn *wx,
                                  char *error_message,
                                  int error_message_length)
{
SFboolean      is_chn;
SFChnHeader    header;
SFChnTrailer   trailer;
SFReturnCode   ret_code;

ret_code = SF_chn_file(spec_path, &is_chn, error_message,
                       error_message_length);
if (SF_SUCCESS != ret_code)
   {
   return(ret_code);
   }

ret_code = SF_chn_get_counts(spec_path, spectrum->listlength, spectrum->count,
                             error_message, error_message_length);
if (SF_SUCCESS != ret_code)
   {
   return(ret_code);
   }

ret_code = SF_chn_get_header(spec_path, &header, error_message,
                             error_message_length);
if (SF_SUCCESS != ret_code)
   {
   return(ret_code);
   }
spectrum->nchannels = header.nchannels;
spectrum->firstchannel = header.min_chan;

ret_code = SF_chn_get_trailer(spec_path, &trailer, error_message,
                              error_message_length);
if (SF_SUCCESS != ret_code)
   {
   return(ret_code);
   }
ex->a = trailer.const_ecalib;
ex->b = trailer.lin_ecalib;
ex->c = trailer.quad_ecalib;
ex->chi_sq = 0;
ex->mode = GL_EGY_QUADRATIC;
wx->alpha = trailer.const_wcalib;
wx->beta = trailer.lin_wcalib;
wx->chi_sq = 0;
wx->mode = GL_WID_LINEAR;

return(ret_code);
}

static void set_ex_string(const GLEnergyEqn *ex, char *ex_string,
                          int ex_string_length)
{
if (GL_EGY_LINEAR == ex->mode)
   {
   sprintf_s(ex_string, ex_string_length, "e(x) = %f + %fx  (X^2 = %f)",
             ex->a, ex->b, ex->chi_sq);
   }
else
   {
   sprintf_s(ex_string, ex_string_length, "e(x) = %f + %fx + %fx^2  (X^2 = %f)",
             ex->a, ex->b, ex->c, ex->chi_sq);
   }
}

static void set_wx_string(const GLWidthEqn *wx, char *wx_string,
                          int wx_string_length)
{
if (GL_WID_LINEAR == wx->mode)
   {
   sprintf_s(wx_string, wx_string_length, "w(x) = %f + %fx  (X^2 = %f)",
             wx->alpha, wx->beta, wx->chi_sq);
   }
else
   {
   sprintf_s(wx_string, wx_string_length, "w(x) = ( %f + %fx )^1/2  (X^2 = %f)",
             wx->alpha, wx->beta, wx->chi_sq);
   }
}

static GLRtnCode test_ecalib(const char *java_class_path, GLEgyEqnMode mode,
                             GLboolean weighted, GLEnergyEqn *ex,
                             char *error_message, int error_message_length)
{
double     channel[10];
double     energy[10];
double     sige[10];
int        count;
GLRtnCode  ret_code;

count = 10;
channel[0] = 20;
channel[1] = 30;
channel[2] = 40;
channel[3] = 50;
channel[4] = 60;
channel[5] = 70;
channel[6] = 80;
channel[7] = 90;
channel[8] = 100;
channel[9] = 110;
energy[0] = 15;
energy[1] = 31;
energy[2] = 47;
energy[3] = 63;
energy[4] = 79;
energy[5] = 95;
energy[6] = 111;
energy[7] = 127;
energy[8] = 143;
energy[9] = 159;
sige[0] = 0;
sige[1] = 0;
sige[2] = 0;
sige[3] = 0;
sige[4] = 0;
sige[5] = 0;
sige[6] = 0;
sige[7] = 0;
sige[8] = 0;
sige[9] = 0;

ret_code = GL_ecalib(java_class_path, count, channel, energy, sige, mode,
                     weighted, ex, error_message, error_message_length);

if (GL_SUCCESS == ret_code)
   {
   set_ex_string(ex, error_message, error_message_length);
   fprintf_s(stdout, "%s\n", error_message);
   error_message[0] = '\0';
   }

return(ret_code);
}

static GLRtnCode test_exceeds_width(const char *java_class_path,
                                    const GLRegions *regions,
                                    int max_width_channels,
                                    char *error_message,
                                    int error_message_length)
{
GLboolean  answer;
GLRtnCode  ret_code;

ret_code = GL_exceeds_width(java_class_path, regions, max_width_channels,
                            &answer, error_message, error_message_length);

if (GL_SUCCESS == ret_code)
   {
   fprintf_s(stdout, "the region search exceeds width=%d test returned: %s\n",
             max_width_channels, get_boolean_string(answer));
   }

return(ret_code);
}

static GLRtnCode test_fit(const char *java_class_path,
                          const GLChanRange *fit_region,
                          const GLSpectrum *spectrum,
                          const GLPeakList *peaklist, const GLFitParms *parms,
                          const GLEnergyEqn *ex, const GLWidthEqn *wx,
                          char *error_message, int error_message_length)
{
int           plots_per_chan;
GLFitRecList  *fitlist;
GLCurve       *curve;
int           channel;
GLSummary     *summary;
int           nchannels;
int           npoints;
int           i;
GLFitRecord   *fit_record;
GLRtnCode     ret_code;

plots_per_chan = 10;
fitlist = NULL;

ret_code = GL_fitregn(java_class_path, fit_region, spectrum, peaklist,
                      parms, ex, wx, plots_per_chan, &fitlist, error_message,
                      error_message_length);

if (GL_SUCCESS == ret_code)
   {
   fit_record = fitlist->record;

   fprintf_s(stdout, "fit returned with record from cycle %d\n",
             fit_record->cycle_number);
   fprintf_s(stdout, "fit cycle_return code is %s\n",
             get_cycle_return_string(fit_record->cycle_return));
   fprintf_s(stdout, "fit cycle_exception message is %s\n\n",
             fit_record->cycle_exception);

   fprintf_s(stdout, "here is the data from the curve\n");
   fprintf_s(stdout, "channel\tcurve\tpeak_1\tpeak2\tbackground\t\n");
   curve = fitlist->record->curve;
   npoints = curve->npoints;
   for (i = 0; i < npoints; i++)
      {
      fprintf_s(stdout, "%.1f\t%.3f\t%.3f\t%.3f\t%.3f\n",
                curve->x_offset[i], curve->fitcurve[i], curve->fitpeak[0][i],
                curve->fitpeak[1][i], curve->back[i]);
      }

   fprintf_s(stdout, "\nchannel\tresidual\n");
   channel = (int) curve->x_offset[0];
   nchannels = fit_region->last - fit_region->first + 1;
   for (i = 0; i < nchannels; i++, channel++)
      {
      fprintf_s(stdout, "%d\t%.3f\n", channel, curve->resid[i]);
      }

   fprintf_s(stdout, "\nhere is the background curve\n");
   fprintf_s(stdout, "b(x) = %.3f + %.3fx (sigi=%.3f sigs=%.3f)\n",
             fit_record->back_linear.intercept, fit_record->back_linear.slope,
             fit_record->back_linear.sigi, fit_record->back_linear.sigs);

   fprintf_s(stdout, "\nhere is the data from the summary\n");
   summary = fit_record->summary;
   fprintf_s(stdout, "ratio=%.3f\n", summary->ratio);
   fprintf_s(stdout,
"fixed\tchannel\tsigc\theight\tsigh\twidth\tsigw\tarea\tsiga\tenergy\tsige\n");
   for (i = 0; i < summary->npeaks; i++)
      {
      
      fprintf_s(stdout,
         "%s\t%.1f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\n",
         get_boolean_string(summary->fixed[i]),
         summary->channel[i], summary->sigc[i], summary->height[i],
         summary->sigh[i], summary->wid[i], summary->sigw[i], summary->area[i],
         summary->siga[i], summary->energy[i], summary->sige[i]);
      }

   fprintf_s(stdout, "\ninput fields in fit record\n");

   fprintf_s(stdout, "used region: %d-->%d\n",
             fit_record->used_chanrange.first,
             fit_record->used_chanrange.last);
   fprintf_s(stdout,
             "used fitparms: ncycle=%d nout=%d maxnpeaks=%d maxresid=%.3f\n",
             fit_record->used_parms.ncycle, fit_record->used_parms.nout,
             fit_record->used_parms.max_npeaks,
             fit_record->used_parms.max_resid);
   fprintf_s(stdout, "used fitparms: pkwd_mode=%s cc_type=%s\n",
             get_pkwd_mode_string(fit_record->used_parms.pkwd_mode),
             get_cc_type_string(fit_record->used_parms.cc_type));
   set_ex_string(ex, error_message, error_message_length);
   fprintf_s(stdout, "used ex: %s\n", error_message);
   set_wx_string(wx, error_message, error_message_length);
   fprintf_s(stdout, "used wx: %s\n", error_message);
   fprintf_s(stdout,
             "used spectrum has %d channels, and has %d counts at 1600\n",
             fit_record->used_spectrum.nchannels,
             fit_record->used_spectrum.count[1600]);
   fprintf_s(stdout,
             "there are %d input peaks with first one at channel %.3f\n",
             fit_record->input_peaks.npeaks,
             fit_record->input_peaks.peak[0].channel);

   GL_fitreclist_free(fitlist);
   error_message[0] = '\0';
   }

return(ret_code);
}

static GLRtnCode test_get_regn_pks(const GLChanRange *region,
                                   const GLPeakList *peaks,
                                   GLPeakList *pks_in_rgn,
                                   char *error_message,
                                   int error_message_length)
{
int          npeaks;
int          i;
GLRtnCode    ret_code;

ret_code = GL_get_regnpks(region, peaks, pks_in_rgn);

if (GL_SUCCESS == ret_code)
   {
   fprintf_s(stdout, "found these peaks in region %d-->%d:\n",
             region->first, region->last);
   npeaks = pks_in_rgn->npeaks;
   for (i = 0; i < npeaks; i++)
      {
      fprintf_s(stdout, "    %.2f\n", pks_in_rgn->peak[i].channel);
      }
   }
else if (GL_OVRLMT == ret_code)
   {
   sprintf_s(error_message, error_message_length,
             "not enough space to hold answer");
   }
else
   {
   sprintf_s(error_message, error_message_length,
             "unknown error");
   }

return(ret_code);
}

static GLRtnCode test_neg_alarms(const char *java_class_path,
                                 const GLChanRange *fit_region,
                                 const GLSpectrum *spectrum,
                                 const GLPeakList *peaklist,
                                 const GLFitParms *parms,
                                 const GLEnergyEqn *ex,
                                 const GLWidthEqn *wx,
                                 char *error_message,
                                 int error_message_length)
{
int           plots_per_chan;
GLFitRecList  *fitlist;
GLSummary     *summary;
GLFitRecord   *fit_record;
int           i;
GLRtnCode     ret_code;

plots_per_chan = 10;
fitlist = NULL;

ret_code = GL_fitregn(java_class_path, fit_region, spectrum, peaklist,
                      parms, ex, wx, plots_per_chan, &fitlist, error_message,
                      error_message_length);

if (GL_SUCCESS == ret_code)
   {
   fit_record = fitlist->record;
   summary = fit_record->summary;

   fprintf_s(stdout, "fit cycle#=%d X2=%.3f return=%s\n",
             fit_record->cycle_number, fit_record->chi_sq,
             get_cycle_return_string(fit_record->cycle_return));
   fprintf_s(stdout, "cycle exception=%s\n", fit_record->cycle_exception);

   fprintf_s(stdout, "these are negative peaks:\n    ");
   for (i = 0; i < summary->npeaks; i++)
      {
      if (GL_TRUE == summary->negpeak_alarm[i])
         {
         fprintf_s(stdout, " %f", summary->channel[i]);
         }
      }
   fprintf_s(stdout, "\n");

   fprintf_s(stdout,
   "these are +/- pair peaks in no particular order:\n    ");
   for (i = 0; i < summary->npeaks; i++)
      {
      if (GL_TRUE == summary->posnegpeakpair_alarm[i])
         {
         fprintf_s(stdout, " %f", summary->channel[i]);
         }
      }
   fprintf_s(stdout, "\n");

   GL_fitreclist_free(fitlist);
   }

return(ret_code);
}

static GLRtnCode test_outside_alarm(const char *java_class_path,
                                    const GLChanRange *fit_region,
                                    const GLSpectrum *spectrum,
                                    const GLPeakList *peaklist,
                                    const GLFitParms *parms,
                                    const GLEnergyEqn *ex,
                                    const GLWidthEqn *wx,
                                    char *error_message,
                                    int error_message_length)
{
int           plots_per_chan;
GLFitRecList  *fitlist;
GLSummary     *summary;
GLFitRecord   *fit_record;
int           i;
GLRtnCode     ret_code;

plots_per_chan = 10;
fitlist = NULL;

ret_code = GL_fitregn(java_class_path, fit_region, spectrum, peaklist,
                      parms, ex, wx, plots_per_chan, &fitlist, error_message,
                      error_message_length);

if (GL_SUCCESS == ret_code)
   {
   fit_record = fitlist->record;
   summary = fit_record->summary;

   fprintf_s(stdout, "fit cycle#=%d X2=%.3f return=%s\n",
             fit_record->cycle_number, fit_record->chi_sq,
             get_cycle_return_string(fit_record->cycle_return));
   fprintf_s(stdout, "cycle exception=%s\n", fit_record->cycle_exception);

   fprintf_s(stdout, "these centroids lie outside:\n    ");
   for (i = 0; i < summary->npeaks; i++)
      {
      if (GL_TRUE == summary->outsidepeak_alarm[i])
         {
         fprintf_s(stdout, " %f", summary->channel[i]);
         }
      }
   fprintf_s(stdout, "\n");

   GL_fitreclist_free(fitlist);
   }

return(ret_code);
}

static GLRtnCode test_pksrch(const char *java_class_path, const GLWidthEqn *wx,
                             int srch_threshold, const GLSpectrum *spectrum,
                             GLPeakSearchResults *results,
                             const GLEnergyEqn *ex,
                             char *error_message, int error_message_length)
{
GLChanRange  search_range;
int          i;
GLRtnCode    ret_code;

search_range.first = spectrum->firstchannel + 20;
search_range.last = spectrum->firstchannel + spectrum->nchannels - 20;

ret_code = GL_peaksearch(java_class_path, &search_range, wx, srch_threshold,
                         spectrum, results, error_message,
                         error_message_length);

if (GL_SUCCESS == ret_code)
   {
   fprintf_s(stdout, "found %d peaks\n", results->peaklist->npeaks);

   GL_update_peaklist(ex, results->peaklist);

   for (i = 0; i < results->peaklist->npeaks; i++)
      {
      fprintf_s(stdout, "peak@%f  %fkeV\n", results->peaklist->peak[i].channel,
                results->peaklist->peak[i].energy);
      }

   fprintf_s(stdout, "here are the cross-correlations:\n");
   for (i = 0; i < results->listlength; )
      {
      fprintf_s(stdout, "%d,%d  ", i, results->crosscorrs[i]);
      i++;
      fprintf_s(stdout, "%d,%d  ", i, results->crosscorrs[i]);
      i++;
      fprintf_s(stdout, "%d,%d  ", i, results->crosscorrs[i]);
      i++;
      fprintf_s(stdout, "%d,%d  ", i, results->crosscorrs[i]);
      i++;
      fprintf_s(stdout, "%d,%d  ", i, results->crosscorrs[i]);
      i++;
      fprintf_s(stdout, "%d,%d  ", i, results->crosscorrs[i]);
      i++;
      fprintf_s(stdout, "%d,%d  ", i, results->crosscorrs[i]);
      i++;
      fprintf_s(stdout, "%d,%d\n", i, results->crosscorrs[i]);
      i++;
      }

   fprintf_s(stdout, "here are the refinements\n");
   fprintf_s(stdout, "rawChan\t\trefineRegn\tarea\tbkgd\trefChan\t\tuseRef\n");
   for (i = 0; i < results->peaklist->npeaks; i++)
      {
      fprintf_s(stdout, "%f\t%d-->%d\t%.3f\t%.3f\t%f\t%s\n",
                results->refinements[i].raw_channel,
                results->refinements[i].refine_region.first,
                results->refinements[i].refine_region.last,
                results->refinements[i].net_area,
                results->refinements[i].background,
                results->refinements[i].refined_channel,
                get_boolean_string(results->refinements[i].use_refinement));
      }
   }

return(ret_code);
}

static GLRtnCode test_prune_pks(const char *java_class_path,
                                const GLWidthEqn *wx,
                                const GLPeakList *searchpks,
                                const GLEnergyEqn *ex,
                                char *error_message,
                                int error_message_length)
{
GLPeakList    *curr_rqd;
GLPeakList    *new_rqd;
double        sige;
int           i;
GLRtnCode     ret_code;

/* initialize curr_rqd */
curr_rqd = GL_peaks_alloc(10);
curr_rqd->npeaks = 0;
sige = 0;

GL_add_egypeak(400, sige, curr_rqd);
GL_add_egypeak(511, sige, curr_rqd); /* should be removed */
GL_add_egypeak(600, sige, curr_rqd);
GL_add_egypeak(800, sige, curr_rqd);
GL_add_egypeak(868, sige, curr_rqd); /* should be removed */
GL_add_egypeak(1000, sige, curr_rqd);
GL_add_egypeak(1014, sige, curr_rqd); /* should be removed */
GL_add_egypeak(1500, sige, curr_rqd);
GL_add_egypeak(2000, sige, curr_rqd);
GL_add_egypeak(6769, sige, curr_rqd); /* should be removed */

GL_update_peaklist(ex, curr_rqd);

/* allocate space for new_rqd */
new_rqd = GL_peaks_alloc(10);
new_rqd->npeaks = 0;

ret_code = GL_prune_rqdpks(java_class_path, wx, searchpks, curr_rqd, new_rqd,
                           error_message, error_message_length);

if (GL_SUCCESS == ret_code)
   {
   fprintf_s(stdout, "here is new required peak list of %d peaks:\n",
             new_rqd->npeaks);
   for (i = 0; i < new_rqd->npeaks; i++)
      {
      fprintf_s(stdout, "peak@%.3fkeV\n", new_rqd->peak[i].energy);
      }
   }

return(ret_code);
}

static GLRtnCode test_rgnsrch(const char *java_class_path,
                              const GLWidthEqn *wx, double srch_threshold,
                              int irw, int irch, const GLSpectrum *spectrum,
                              const GLPeakList *peaklist, GLRgnSrchMode mode,
                              int maxrgnwid, GLRegions *regions,
                              char *error_message, int error_message_length)
{
GLChanRange  search_range;
int          i;
GLRtnCode    ret_code;

search_range.first = spectrum->firstchannel + 20;
search_range.last = spectrum->firstchannel + spectrum->nchannels - 20;

ret_code = GL_regnsearch(java_class_path, &search_range, wx, srch_threshold,
                         irw, irch, spectrum, peaklist, mode, maxrgnwid,
                         regions, error_message, error_message_length);

if (GL_SUCCESS == ret_code)
   {
   fprintf_s(stdout, "found %d regions\n", regions->nregions);
   for (i = 0; i < regions->nregions; i++)
      {
      fprintf_s(stdout, "%d --> %d\n", regions->chanrange[i].first,
              regions->chanrange[i].last);
      }
   }

return(ret_code);
}

static GLRtnCode test_wcalib(const char *java_class_path, GLWidEqnMode mode,
                             GLboolean weighted, GLWidthEqn *wx,
                             char *error_message, int error_message_length)
{
double     channel[10];
double     wid[10];
double     sigw[10];
int        count;
GLRtnCode  ret_code;

count = 10;
channel[0] = 20;
channel[1] = 30;
channel[2] = 40;
channel[3] = 50;
channel[4] = 60;
channel[5] = 70;
channel[6] = 80;
channel[7] = 90;
channel[8] = 100;
channel[9] = 110;
wid[0] = 15;
wid[1] = 29;
wid[2] = 43;
wid[3] = 57;
wid[4] = 71;
wid[5] = 85;
wid[6] = 99;
wid[7] = 113;
wid[8] = 127;
wid[9] = 141;
sigw[0] = 0;
sigw[1] = 0;
sigw[2] = 0;
sigw[3] = 0;
sigw[4] = 0;
sigw[5] = 0;
sigw[6] = 0;
sigw[7] = 0;
sigw[8] = 0;
sigw[9] = 0;

ret_code = GL_wcalib(java_class_path, count, channel, wid, sigw, mode,
                     weighted, wx, error_message, error_message_length);

if (GL_SUCCESS == ret_code)
   {
   set_wx_string(wx, error_message, error_message_length);
   fprintf_s(stdout, "%s\n", error_message);
   error_message[0] = '\0';
   }

return(ret_code);
}

