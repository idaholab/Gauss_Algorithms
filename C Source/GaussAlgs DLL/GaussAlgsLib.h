/*
 * Copyright 2017 Battelle Energy Alliance
 */

/*
 *  GaussAlgsLib.h - contains typedefs and prototypes for the Gauss Algorithms
 *		library
 */

#ifndef GAUSSALGSLIB_H
#define GAUSSALGSLIB_H

/*
 * When building a DLL in Visual C++, declspec() is required.
 * Otherwise, leave this undefined.
 */

#ifdef _WINDOWS
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

/*
 * Typedefs for prototype arguments:
 */


/*
 * stuff for boolean logic
 */

#ifndef False
#define	False	0
#define True	1
#endif

   typedef enum
      {
      GL_FALSE = False,
      GL_TRUE  = True
      } GLboolean;


/*
 * definition of fitting range
 */

   typedef struct
      {
      int		first;
      int		last;
      } GLChanRange;


/*
 * Energy equation modes
 */

   typedef enum
      {
      GL_EGY_LINEAR,
      GL_EGY_QUADRATIC
      } GLEgyEqnMode;


/*
 * GLEnergyEqn holds coefficients for the energy equation
 * e(x) = a + b*x + c*x**2
 * and chi squared from corresponding calibration.
 */

   typedef struct
      {
      double		a;
      double		b;
      double		c;
      double		chi_sq;
      GLEgyEqnMode	mode;
      } GLEnergyEqn;


/*
 * Width equation modes
 */

   typedef enum
      {
      GL_WID_LINEAR,
      GL_WID_SQRT
      } GLWidEqnMode;


/*
 * GLWidthEqn holds coefficients for the width equation
 *
 * linear: w(x) = alpha + beta*x
 *
 * sqrt:   w(x) = (alpha + beta*x)**1/2
 *
 * and chi squared from corresponding calibration.
 */

   typedef struct
      {
      double		alpha;
      double		beta;
      double		chi_sq;
      GLWidEqnMode	mode;
      } GLWidthEqn;


/*
 * GLSpectrum holds the counts per channel of a spectrum
 */

   typedef struct
      {
      int		listlength;	/* array size of count & sigcount */
      int		nchannels;
      int		firstchannel;
#if (! defined(GL_LINUX)) && (! defined(GL_MACOSX))
      __int32	*count;		/* for Visual C++ */
#else
      int		*count;		/* list of counts per channel */
#endif
      } GLSpectrum;


/*
 * GLPeakType indicates which data form was used to add/define the peak.
 */

   typedef enum
      {
      GL_PEAK_CHANNEL = 0,
      GL_PEAK_ENERGY = 1
      } GLPeakType;


/*
 * GLPeak is a structure to hold all information about a single peak.
 */

   typedef struct
      {
      GLPeakType	type;
      GLboolean		channel_valid;
      double		channel;
      GLboolean		energy_valid;
      double		energy;
      double		sige;
      GLboolean		fixed_centroid;
      } GLPeak;


/*
 * GLPeakList is a structure to hold a list of peaks.
 */

   typedef struct
      {
      int		listlength;	/* array size of peak */
      int		npeaks;
      GLPeak	*peak;		/* array of peaks */
      } GLPeakList;

/*
 * GLPeakRefinement is a structure to hold the results of a peak refinement.
 *                  If the peak search decides that the refinement is best,
 *                  then use_refinement=GL_TRUE.
 */

   typedef struct
      {
      double		raw_channel;
      GLChanRange	refine_region;
      double		net_area;
      double		background;
      double		refined_channel;
      GLboolean     use_refinement;
      } GLPeakRefinement;


/*
 * GLPeakSearchResults is a structure to hold the results
 *                     of a peak search.
 */

   typedef struct
      {
	  GLPeakList        *peaklist;         /* found peaks */
	  GLPeakRefinement	*refinements;      /* array of peak refinements */
	  int               listlength;        /* array size of crossproducts */
	  int               *crosscorrs;       /* cross-correlations */
      } GLPeakSearchResults;


/*
 * GLRegions is a structure to hold list of region coordinates.
 */

   typedef struct
      {
      int            listlength;	/* array size of chanrange */
      int            nregions;
      GLChanRange    *chanrange;	/* list of channel ranges */
      } GLRegions;


/*
 * Region search modes
 *
 * RGNSRCH_FORPKS is used when want only regions for existing peaks
 * and no regions without peaks.
 */

   typedef enum
      {
      GL_RGNSRCH_ALL,
      GL_RGNSRCH_FORPKS
      } GLRgnSrchMode;


/*
 * enumerations of legal fit parameter values
 */

   /*
    * legal values for the pkwd_mode parameter which is used to
    * set convergence criteria and determine when to vary peakwidth.
    *
    * See the description of GLCCType for a discussion of setting
    * the convergence criteria.
    *
    * For determining at beginning of each fit cycle whether
    * to vary peakwidth:
    *
    * if pkwd_mode == GL_PKWD_VARIES
    *      if atleast one peak centroid is not fixed, let average
    *      peakwidth vary.
    *      if jth peak is ~511keV and it is not only peak,
    *         let fitinfo.peakinfo[j].addwidth_511 vary.
    *
    * if pkwd_mode == GL_PKWD_FIXED
    *      if jth peak is ~511keV,
    *         let fitinfo.peakinfo[j].addwidth_511 vary.
    */

      typedef enum 
         {
         GL_PKWD_VARIES = 0,
         GL_PKWD_FIXED = 1
         } GLPkwdMode;


   /*
    * enumeration of types for the cc_type parameter
    *
    * cc_type is used in conjunction with pkwd_mode to choose values
    * for the convergence criteria ftol & xtol, which are inputs to
    * the Levenberg-Marquardt algorithm lmder().
    * ftol (costRelativeTolerance) measures the relative error desired
    *                              in the sum of squares.
    * xtol (parRelativeTolerance) measures the relative error desired
    *                             in the approximate solution.
    *
    * setting ftol & xtol:
    *
    * if   cc_type == GL_CC_LARGER
    * and  pkwd_mode == GL_PKWD_VARIES
    * then ftol = .00001
    * and  xtol = .00003
    *
    * if   cc_type == GL_CC_LARGER
    * and  pkwd_mode == GL_PKWD_FIXED
    * then ftol = xtol = .0001
    *
    * if   cc_type == GL_CC_SMALLER
    * then ftol = xtol = .00001
    *
    * if   cc_type == GL_CC_LARGER_INC
    * then ftol = .00001
    * and  xtol = .00003
    */

      typedef enum
         {
         GL_CC_LARGER     = 0,
         GL_CC_SMALLER    = 1,
         GL_CC_LARGER_INC = 2
         } GLCCType;


/*
 * Fit Parameters
 *
 * ncycle		- maximum number of fit cycles allowed
 * nout			- maximum number of the best fits to be used
 * max_npeaks	- maximum number of peaks allowed in a fit
 * pkwd_mode	- control peakwidth
 * cc_type		- used to set convergence criteria
 * max_resid	- fitregn() recycles until the fit residual at
 *				  each channel in the region is less than the
 *				  value of max_resid. (if not stopped sooner)
 */

   typedef struct
      {
      int		  ncycle;	  /* suggested value 10 */
      int		  nout;		  /* suggested value 1 */
      int		  max_npeaks; /* suggested value 10 */
      GLPkwdMode  pkwd_mode;
      GLCCType	  cc_type;
      float		  max_resid;  /* suggested values 2 or 20 */
      } GLFitParms;


/*
 * enumerations of fit results
 */

   /* fit cycle return code */

      typedef enum 
         {
         GL_CYCLE_DONE = 0,
         GL_CYCLE_DELETE = 1,
         GL_CYCLE_ADD = 2,
         GL_CYCLE_CONTINUE = 3
         } GLCycleReturn;


/*
 * GLFitBackLin is a structure to hold a linear fit background
 */

   typedef struct
      {
      double     intercept;
      double     sigi;
      double     slope;
      double     sigs;
      } GLFitBackLin;


/*
 * GLSummary is a structure to hold fit information of each peak
 */

   typedef struct
      {
      int		 listlength;  /* array size of channel, etc. */
      int		 npeaks;
      double	 ratio;       /* of summation area to integral area */
      GLboolean  *fixed;      /* indicates whether centroid is fixed */
      double	 *channel;
      double	 *sigc;
      double	 *height;
      double	 *sigh;
      double	 *wid;
      double	 *sigw;
      double	 *area;
      double	 *siga;
      double	 *energy;
      double	 *sige;
      GLboolean  *negpeak_alarm;         /* negative height peaks */
      GLboolean  *outsidepeak_alarm;     /* centroids outside region */
      GLboolean  *posnegpeakpair_alarm;  /* one of a +/- pair */
      } GLSummary;


/*
 * GLCurve contains coordinates for the fit, background, and residual at
 *         each channel in the region.
 *
 * fitpeak is an array of component curves, containing one curve for each peak.
 *
 * If nplots_per_chan is greater than one, then there are fit, component,
 * and background points to plot between the channels.
 *
 * In the comments below, nchannels = chanrange.last - chanrange.first + 1
 */

   typedef struct
      {
      int			listlength;       /* should be >= npoints */
      GLChanRange	chanrange;
      int			nplots_per_chan;  /* # plotpoints per channel */
      int			npoints;          /* ((nchannels-1)*nplots_per_chan)+1 */
      int			npeaks;
      double		*x_offset;        /* array of npoints */
                                      /*   'offsets relative to start */
                                      /*   of chanrange' */
      double		**fitpeak;        /* array of npeak 'curve arrays' */
      double		*fitcurve;        /* array of npoints 'curve y coords' */
      double		*back;            /* array of npoints 'back y coords' */
      double		*resid;           /* array of nchannels 'residuals' */
      } GLCurve;


/*
 * GLFitRecord contains fit information corresponding to one fit cycle.
 *
 * The 'used' information (along with the input_peaks) is passed into
 * fitregn() for the fit.
 *
 * The rest of the information in this structure is passed out by fitregn().
 */

   typedef struct
      {
      int               cycle_number;
      GLChanRange       used_chanrange;
      GLFitParms        used_parms;
      GLEnergyEqn       used_ex;
      GLWidthEqn        used_wx;
      GLSpectrum        used_spectrum;
      GLPeakList        input_peaks;
      double            chi_sq;            /* reduced chi squared of the fit */
      GLCycleReturn     cycle_return;
      char              *cycle_exception;  /* NULL when no exception */
      GLFitBackLin      back_linear;
      GLSummary         *summary;
      GLCurve           *curve;
      } GLFitRecord;


/*
 * GLFitRecList contains a linked list of fit records returned by fitregn().
 *
 * Each record in the list corresponds to one fit cycle.
 */

   typedef struct fitreclist
      {
      GLFitRecord		*record;
      struct fitreclist	*next;
      } GLFitRecList;


/*
 * Return codes from the C subroutines in Gauss Algorithms:
 *
 * GL_SUCCESS		no problems in execution of procedure
 * GL_FAILURE		unspecified problem in execution of procedure
 *
 * GL_BADMALLOC		failure to allocate temporary workspace in memory;
 *					procedure returned without completing.
 *
 * GL_OVRLMT		more peaks or regions were found than the structure
 *					could hold; the extra peaks or regions are ignored;
 *					the returned list contains the items found up to
 *					the limit.
 *
 * GL_NOJVM			cannot launch or find a Java Virtual Machine
 * GL_JNIERROR		error returned from JNI code (C wrapper).
 * GL_JEXCEPTION    exception thrown by Java code. Check exception message.
 */

   typedef enum
      {
      GL_SUCCESS,
      GL_FAILURE,
      GL_BADMALLOC,
      GL_OVRLMT,
	  GL_NOJVM,
	  GL_JNIERROR,
      GL_JEXCEPTION
      } GLRtnCode;


/*
 * Prototypes for procedures in the Gauss Library
 */

#ifdef __cplusplus
extern "C" {
#endif


/*
 * GL_add_chanpeak
 *
 *   adds a peak to the list in terms of channel. Type will be set to
 *   GL_PEAK_CHANNEL, centroid will be initialized to NOT fixed, and energy
 *   will be left undefined.
 *
 *   Possible return codes: GL_OVRLMT, GL_SUCCESS        
 */

   DLLEXPORT GLRtnCode GL_add_chanpeak(double channel, GLPeakList *peaks);


/*
 * GL_add_egypeak
 *
 *   adds a peak to the list in terms of energy. Type will be set to
 *   GL_PEAK_ENERGY, centroid will be initialized to fixed, and channel
 *   will be left undefined.
 *
 *   Possible return codes: GL_OVRLMT, GL_SUCCESS        
 */

   DLLEXPORT GLRtnCode GL_add_egypeak(double energy, double sige,
                                      GLPeakList *peaks);


/*
 * GL_add_peak
 *
 *   adds a peak to the list.
 *
 *   Possible return codes: GL_OVRLMT, GL_SUCCESS        
 */

   DLLEXPORT GLRtnCode GL_add_peak(const GLPeak *peak, GLPeakList *peaks);


/*
 * GL_chan_to_e
 *
 *   converts channel number to energy using the given energy equation.
 *   The calling routine must provide space for the answer in 'energy'.
 */

   DLLEXPORT void GL_chan_to_e(const GLEnergyEqn *ex, double channel,
                               double *energy);


/*
 * GL_chan_to_w
 *
 *   calculates the peak width at the indicated channel. The calling routine
 *   must provide space for the answer in 'width'.
 *
 *   Possible return codes: GL_FAILURE, GL_SUCCESS        
 */

   DLLEXPORT GLRtnCode GL_chan_to_w(const GLWidthEqn *wx, double channel,
                                    double *width);


/*
 * GL_e_to_chan
 *
 *   converts energy to channel using the given energy equation. The calling
 *   routine must provide space for the answer in 'channel'.
 *
 *   Possible return codes: GL_FAILURE, GL_SUCCESS        
 */

   DLLEXPORT GLRtnCode GL_e_to_chan(const GLEnergyEqn *ex, double energy,
                                    double *channel);


/*
 * GL_ecalib
 *
 *   calibrates the energy equation. channel, energy, and sige are assumed
 *   to be arrays of size count. If 'weighted' is true, then 'sige' is used;
 *   otherwise, error for each energy is fixed to be '1'.
 *
 *   java_class_path is the path to each jar needed, including
 *                   GaussAlgorithms.jar
 *
 *   The calling routine must provide space for the answer in 'ex'
 *   and space for error messages.
 *
 *   Possible return codes: GL_NOJVM, GL_JNIERROR, GL_JEXCEPTION, GL_SUCCESS
 */

   DLLEXPORT GLRtnCode GL_ecalib(const char *java_class_path, int count,
                                 const double *channel, const double *energy,
								 const double *sige, GLEgyEqnMode mode,
								 GLboolean weighted, GLEnergyEqn *ex,
								 char *error_message,
                                 int error_message_length);


/*
 * GL_exceeds_width
 *
 *   checks each region in the list to determine whether it exceeds the
 *   maximum allowed width.
 *
 *   java_class_path is the path to each jar needed, including
 *                   GaussAlgorithms.jar
 *
 *   Space for 'answer' and error messages must be provided.
 *
 *   Possible return codes: GL_NOJVM, GL_JNIERROR, GL_SUCCESS
 */

   DLLEXPORT GLRtnCode GL_exceeds_width(const char *java_class_path,
                                        const GLRegions *regions,
                                        int max_width_channels,
                                        GLboolean *answer,
                                        char *error_message,
                                        int error_message_length);


/*
 * GL_fitreclist_free
 *
 *   frees structure memory that was allocated and returned by GL_fitregn()
 *   in 'fitlist'.
 */

   DLLEXPORT void GL_fitreclist_free(GLFitRecList *fitreclist);


/*
 * GL_fitregn
 *
 *   fits the indicated region and stores the answer in fitlist.
 *
 *   If the number of peaks in the list is greater than that allowed by
 *   the fitparms, the Java code throws an exception and no fitting is done.
 *
 *   Only those peaks with defined channels are used.
 *
 *   Because the GL_curve is contained in the fitlist, the nplots_per_chan
 *   indicates how to build the curve. 'nplots_per_chan - 1' is how many
 *   coordinates will be plotted BETWEEN channels. 
 *
 *   java_class_path is the path to each jar needed, including
 *                   GaussAlgorithms.jar
 *
 *   Space for the 'fitlist' pointer and error messages must be provided.
 *
 *   Possible return codes: GL_NOJVM, GL_JNIERROR, GL_JEXCEPTION,
 *                          GL_BADMALLOC, GL_SUCCESS
 */

   DLLEXPORT GLRtnCode GL_fitregn(const char *java_class_path,
                                  const GLChanRange *region,
                                  const GLSpectrum *spectrum,
                                  const GLPeakList *peaks,
                                  const GLFitParms *fitparms,
                                  const GLEnergyEqn *ex, const GLWidthEqn *wx,
                                  int nplots_per_chan, GLFitRecList **fitlist,
                                  char *error_message,
                                  int error_message_length);


/*
 * GL_get_regnpks
 *
 *   returns a list of peaks that are within the indicated region. The peak
 *   list must provide the channel for each peak. The calling routine
 *   must provide space for the answer in 'pks_in_rgn'.
 *
 *   Possible return codes: GL_OVRLMT, GL_SUCCESS
 */

   DLLEXPORT GLRtnCode GL_get_regnpks(const GLChanRange *region,
                                      const GLPeakList *peaks,
                                      GLPeakList *pks_in_rgn);


/*
 * GL_get_version
 *
 *   returns the version string for Gauss Algorithms.
 *
 *   java_class_path is the path to each jar needed, including
 *                   GaussAlgorithms.jar
 *
 *   Space for 'version' and error messages must be provided.
 *
 *   Possible return codes: GL_NOJVM, GL_JNIERROR, GL_SUCCESS
 */

   DLLEXPORT GLRtnCode GL_get_version(const char *java_class_path,
                                      char *version, int version_length,
                                      char *error_message,
                                      int error_message_length);


/*
 * GL_peak_results_alloc 
 *
 *   allocates memory for a GLPeakSearchResults structure. Listlength is
 *   set in the returned structure.
 *
 *   Returns NULL on failure.
 */

   DLLEXPORT GLPeakSearchResults *GL_peak_results_alloc(int peak_listlength,
	                                                   int spectrum_nchannels);


/*
 * GL_peak_results_free
 *
 *   frees memory for a GLPeakSearchResults structure.
 */
   
   DLLEXPORT void GL_peak_results_free(GLPeakSearchResults *results);


/*
 * GL_peaks_alloc
 *
 *   allocates memory for a GLPeakList structure. Listlength is set in the
 *   returned structure.
 *
 *   Returns NULL on failure.
 */

   DLLEXPORT GLPeakList *GL_peaks_alloc(int listlength);


/*
 * GL_peaks_free
 *
 *   frees GLPeakList structure memory that was allocated with
 *   GL_peaks_alloc().
 */

   DLLEXPORT void GL_peaks_free(GLPeakList *peaks);


/*
 * GL_peaksearch
 *
 *   searches for peaks in the spectrum. Threshold controls the pruning out
 *   of insignificant peaks. Large values increase the pruning.
 *
 *			for low search sensitivity, use  threshold = 20
 *			for med search sensitivity, use  threshold = 10
 *			for high search sensitivity, use threshold =  5
 *
 *   The channel of each peak is returned in 'results'. For each returned
 *   peak, the type is GL_PEAK_CHANNEL, the centroid is not fixed, and
 *   the energy is undefined.
 *
 *   java_class_path is the path to each jar needed, including
 *                   GaussAlgorithms.jar
 *
 *   The calling routine must provide space for the answer in 'results'.
 *
 *   Possible return codes: GL_NOJVM, GL_JNIERROR, GL_JEXCEPTION, GL_SUCCESS
 */

   DLLEXPORT GLRtnCode GL_peaksearch(const char *java_class_path,
                                     const GLChanRange *chanrange,
                                     const GLWidthEqn *wx, int threshold,
                                     const GLSpectrum *spectrum,
                                     GLPeakSearchResults *results,
                                     char *error_message,
                                     int error_message_length);


/*
 * GL_prune_rqdpks
 *
 *   removes any required peaks that are too close to any found peaks. If
 *   the channel is not defined for a peak in either input list, then it is
 *   ignored. Furthermore, if the channel of a required peak is not defined,
 *   then it will not appear in the answer.
 *
 *   java_class_path is the path to each jar needed, including
 *                   GaussAlgorithms.jar
 *
 *   The calling routine must provide space for the answer in 'new_rqd'.
 *
 *   Possible return codes: GL_NOJVM, GL_JNIERROR, GL_SUCCESS
 */

   DLLEXPORT GLRtnCode GL_prune_rqdpks(const char *java_class_path,
                                       const GLWidthEqn *wx,
                                       const GLPeakList *searchpks,
                                       const GLPeakList *curr_rqd,
                                       GLPeakList *new_rqd,
                                       char *error_message,
                                       int error_message_length);


/*
 * GL_regions_alloc
 *
 *   allocates memory for a GLRegions structure. Listlength is set in the
 *   returned structure.
 *
 *   Returns NULL on failure.
 */

   DLLEXPORT GLRegions *GL_regions_alloc(int listlength);


/*
 * GL_regions_free
 *
 *   frees GLRegions structure memory that was allocated with
 *   GL_regions_alloc().
 */

   DLLEXPORT void GL_regions_free(GLRegions *regions);


/*
 * GL_regnsearch
 *
 *   searches for regions in the spectrum. Threshold controls the pruning
 *   out of insignificant regions. Large values increase the pruning.
 *
 *			for low search sensitivity, use  threshold = 3
 *			for med search sensitivity, use  threshold = 2
 *			for high search sensitivity, use threshold = 1
 *
 *   'irw' and 'irch' control padding the ends of the found regions. The pad
 *   starts with the gap between regions, is decremented by irch, and is
 *   increased to 'irw * peakwidth'.
 *
 *			recommended starting value for  irw = 3
 *			recommended starting value for irch = 2
 *
 *   The peak list must include the channel for each peak. maxrgnwid limits
 *   the size of each region (number of channels in a region). A good value
 *   to use is maxrgnwid = 150.
 *
 *   java_class_path is the path to each jar needed, including
 *                   GaussAlgorithms.jar
 *
 *   The calling routine must provide space for the answer in 'regions'.
 *
 *   Possible return codes: GL_NOJVM, GL_JNIERROR, GL_JEXCEPTION, GL_SUCCESS
 */

   DLLEXPORT GLRtnCode GL_regnsearch(const char *java_class_path,
                                     const GLChanRange *chanrange,
                                     const GLWidthEqn *wx, double threshold,
                                     int irw, int irch,
                                     const GLSpectrum *spectrum,
                                     const GLPeakList *peaks,
                                     GLRgnSrchMode mode, int maxrgnwid,
                                     GLRegions *regions, char *error_message,
                                     int error_message_length);


/*
 * GL_spectrum_counts_alloc
 *
 *   allocates space in the GLSpectrum structure for the count array.
 *
 *   Possible return codes: GL_BADMALLOC, GL_SUCCESS
 */

   DLLEXPORT GLRtnCode GL_spectrum_counts_alloc(GLSpectrum *spectrum,
                                                int listlength);


/*
 * GL_spectrum_counts_free
 *
 *   frees space allocated with GL_spectrum_counts_alloc.
 */

   DLLEXPORT void GL_spectrum_counts_free(GLSpectrum *spectrum);


/*
 * GL_update_peaklist
 *
 *   updates a peaklist with the specified energy calibration. For each
 *   peak of type GL_PEAK_CHANNEL, the energy value is updated.  For each
 *   peak of type GL_PEAK_ENERGY, the channel value is updated. If ex == NULL,
 *   then the updated values are flagged as invalid instead.
 */

   DLLEXPORT void GL_update_peaklist(const GLEnergyEqn *ex, GLPeakList *peaks);


/*
 * GL_wcalib
 *
 *   calibrates the width equation.  channel, wid, and sigw are assumed to be
 *   arrays of length 'count'.  If 'weighted' is true, then error for each
 *   width is calculated as sigw * wid * 2; otherwise, error for each width
 *   is fixed to be '1'.
 *
 *   java_class_path is the path to each jar needed, including
 *                   GaussAlgorithms.jar
 *
 *   The calling routine must provide space for the answer in 'wx'.
 *
 *   Possible return codes: GL_NOJVM, GL_JNIERROR, GL_JEXCEPTION, GL_SUCCESS
 */

   DLLEXPORT GLRtnCode GL_wcalib(const char *java_class_path, int count,
                                 const double *channel, const double *wid,
                                 const double *sigw, GLWidEqnMode mode,
                                 GLboolean weighted, GLWidthEqn *wx,
                                 char *error_message,
                                 int error_message_length);


#ifdef __cplusplus
}
#endif


#endif  /* GAUSSALGSLIB_H */
