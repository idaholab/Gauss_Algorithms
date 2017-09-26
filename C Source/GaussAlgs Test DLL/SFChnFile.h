/*
 * Copyright 2017 Battelle Energy Alliance
 */

/*
 * SFChnFile.h - contains typedefs and prototypes for the CHN format
 *               spectral file utilities
 */

#ifndef SFCHNFILE_H
#define SFCHNFILE_H

#define SF_CHN_SUFFIX		"chn"

#define SF_CHN_DATE_LEN		8
#define SF_CHN_TIME_LEN		4
#define SF_CHN_MAX_DESC_LEN	63
#define SF_CHN_TITLELEN		130


#define	SF_CHN_HEADER_TAG	-1
#define	SF_CHN_TRAILER_TAG	-101
#define	SF_CHN_QUAD_TRAIL_TAG	-102	/* Verbally provided by Ortec */
					/* to Leo Van Ausdeln on 2/22/96. */
					/* This tag applies when energy */
					/* calibration is quadratic. */

					/* confirmed in fax of file format */
					/* received from Ken Krebs on */
					/* 7/12/2000. */

/*
 * The CHN format comes from EG&G Ortec, and is used in the MS Windows
 * environment. Therefore, the expected byte order is "little endian".
 */

/*
 * layout of *.chn file
 *
 * (header)
 * (counts)
 * (trailer)
 *
 */

/*
 *
 * byte size	data type	description
 *
 * layout of header
 *
 *	2	short int	SF_CHN_HEADER_TAG
 *	2	short int	MCA number
 *	2	short int	segment number
 *	2	char		seconds of start time
 *	4	int		real time (increments of 20ms)
 *	4	int		live time (increments of 20ms)
 *	8	char		start date (DDMMMYY'\0')
 *	4	char		start time (HHMM)
 *	2	short int	channel offset of counts
 *	2	short int	# of channels
 *
 * layout of counts
 *
 *   nchan*4	int		array of integers, one per channel
 *
 * layout of trailer
 *
 *	2	int		SF_CHN_TRAILER_TAG or SF_CHN_QUAD_TRAIL_TAG
 *	2	-		reserved
 *	4	float		constant coefficient of energy calib
 *	4	float		1st order coefficient of energy calib
 *	4	-		2nd order coeff of energy calib if QUAD TAG used
 *	4	float		constant coefficient of width calib
 *	4	float		1st order coefficient of width calib
 *	4	float		2nd order coeff of width calib if QUAD TAG used
 *	228	-		reserved
 *	1	byte		length of detector description
 *	63	char		detector description
 *	1	byte		length of sample description
 *	63	char		sample description
 *	128	-		reserved
 *
 */

typedef struct
   {
   short int	header_tag;
   short int	mca_no;
   short int	segment_no;
   char		start_time[2];	/* null byte to end string is missing */
   int		real_time;
   int		live_time;
   char		date[SF_CHN_DATE_LEN];	/* null byte to end string is missing */
   char		time[SF_CHN_TIME_LEN];	/* null byte to end string is missing */
   short int	min_chan;
   short int	nchannels;
   } SFChnHeader;

typedef struct
   {
   short int	trailer_tag;
   char		unused_1[2];
   float	const_ecalib;
   float	lin_ecalib;
   float	quad_ecalib;
   float	const_wcalib;
   float	lin_wcalib;
   float	quad_wcalib;
   char		unused_3[228];
   char		len_dtr_desc_byte;
   char		dtr_desc[SF_CHN_MAX_DESC_LEN]; /* null byte end is missing */
   char		len_smp_desc_byte;
   char		smp_desc[SF_CHN_MAX_DESC_LEN]; /* null byte end is missing */
   char		unused_4[128];
   } SFChnTrailer;


/*
 * Prototypes for CHN spectral file procedures
 * (in alphabetical order)
 */

#ifdef __cplusplus
extern "C" {
#endif


/*
 *  SF_chn_file		checks to see if file is a CHN format spectral file.
 *
 *   calling software must provide storage for error message.
 */

   SFReturnCode SF_chn_file(const char *filename, SFboolean *answer,
                            char *error_message, int error_message_length);



/*
 *  SF_chn_get_counts	reads a CHN file and returns spectral counts.
 *
 *			calling software must provide enough storage
 *			in counts (atleast header.nchannels)
 *
 *   calling software must provide storage for error message.
 */

   SFReturnCode SF_chn_get_counts(const char *filename, int listlength,
                                  int *counts, char *error_message,
                                  int error_message_length);


/*
 *  SF_chn_get_header	reads a CHN file and returns header information.
 *
 *   calling software must provide storage for error message.
 */

   SFReturnCode SF_chn_get_header(const char *filename, SFChnHeader *header,
                                  char *error_message,
                                  int error_message_length);


/*
 *  SF_chn_get_trailer	reads a CHN file and returns trailer information.
 *
 *   calling software must provide storage for error message.
 */

   SFReturnCode SF_chn_get_trailer(const char *filename, SFChnTrailer *trailer,
                                   char *error_message,
                                   int error_message_length);


/*
 *  SF_chn_put_spectrum	writes a spectrum to a CHN file.
 *
 *   calling software must provide storage for error message.
 */

   SFReturnCode SF_chn_put_spectrum(const char *filename,
                                    const SFChnHeader *header,
									const int *counts,
                                    const SFChnTrailer *trailer,
                                    char *error_message,
                                    int error_message_length);


/*
 *  SF_chn_put_trailer	writes a trailer to a CHN file.
 *
 *   calling software must provide storage for error message.
 */

   SFReturnCode SF_chn_put_trailer(const char *filename,
                                   const SFChnTrailer *trailer,
                                   char *error_message,
                                   int error_message_length);

#ifdef __cplusplus
}
#endif


#endif  /* SFCHNFILE_H */
