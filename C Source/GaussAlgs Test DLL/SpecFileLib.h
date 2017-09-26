/*
 * Copyright 2017 Battelle Energy Alliance
 */

/*
 * SpecFileLib.h - contains typedefs and prototypes for the Spectrum File
 *                 Utilities library
 */

#ifndef SPECFILELIB_H
#define SPECFILELIB_H

/*
 * Automatically overload calls like strcpy with strcpy_s.
 */
/* #define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1 */

/*
 * macro for use in Spectrum File Utilities.
 * I recommend that arguments a and b be enclosed in parentheses.
 */

#define SFmax(a,b)	(a<b ? b : a)
#define SFmin(a,b)	(a>b ? b : a)


/* 
 * boolean definitions
 */

#ifndef False
#define False	0
#define True	1
#endif

   typedef enum
      {
      SF_FALSE = False,
      SF_TRUE  = True
      } SFboolean;


#define SF_MAX_NAMLEN		256  /* assumed length for err_msg */

/*
 * COMPILING ON SGI OR MAC OS X (endian).
 *
 * Some of the formats are endian specific (Chn, Dge, His, Spk, ...).
 *
 * According to web article by David K. Every (copyright 1999)
 * (see http://www.iGeek.com/articles/Programming/WhatsEndian.txt),
 * the SGI and Mac OS are "big" endian (motorola 68000's?), and
 * the MS Windows and Linux are "little" endian (Intel pentiums).
 *
 * Define SF_BIG_ENDIAN in your OS.cnf to compile on SGI or Mac OS X.
 */




/*
 * Return codes from the procedures in the Spectrum File Utilities library:
 *
 * SF_SUCCESS		no problems in execution of procedure
 * SF_FAILURE		unspecified problem in execution of procedure
 * SF_NAME_ERR		file name probably doesn't have correct suffix
 * SF_MALLOC_ERR	failure to allocate temporary workspace in memory;
 *                    procedure returned without completing.
 * SF_SPACE_ERR     insufficient space provided for storing read
 * SF_OPENR_ERR		cannot open a file to read
 * SF_OPENW_ERR		cannot open a file to write
 * SF_OPENRW_ERR	cannot open a file to read & write
 * SF_OPENC_ERR		cannot create a file
 * SF_CLOSE_ERR		cannot close a file
 * SF_EARLY_END		end of file reached before all expected data was read
 * SF_SPECDUP_ERR	spectral id already in use
 * SF_NOSPEC_ERR	spectral id not found in file
 * SF_DIMN_ERR		dimension is not one of allowed values
 * SF_CORRUPT		file format is wrong
 */

   typedef enum
      {
      SF_SUCCESS,
      SF_FAILURE,
      SF_NAME_ERR,
      SF_MALLOC_ERR,
      SF_SPACE_ERR,
      SF_OPENR_ERR,
      SF_OPENW_ERR,
      SF_OPENRW_ERR,
      SF_OPENC_ERR,
      SF_CLOSE_ERR,
      SF_EARLY_END,
      SF_SPECDUP_ERR,
      SF_NOSPEC_ERR,
      SF_DIMN_ERR,
      SF_CORRUPT
      } SFReturnCode;


#endif  /* SPECFILELIB_H */
