/*
 * Copyright 2017 Battelle Energy Alliance
 */

/*
 *  ChnFile.c - contains various routines for accessing, reading,
 *              and writing CHN format spectra.
 *
 *  Prototypes for the routines can be found in the SFChnFile.h, and
 *  global defines are in the SpecFileLib.h include file.
 *
 */

#include <string.h>	/* for strcpy(), etc. */
#include <stdio.h>	/* for sprintf() */
#include <ctype.h>	/* for tolower() */
#include <sys/types.h>	/* for lseek() */
#include <io.h>     /* for access(), lseek(), read() */
#include <stdlib.h>	/* for calloc() */

#include "SpecFileLib.h"
#include "specfileprv.h"
#include "SFChnFile.h"

/* prototypes for utilities private to this file */

static SFboolean CHN_checkhdlabel(const SFChnHeader *header,
                                  char *error_message,
                                  int error_message_length);
static SFboolean CHN_checktrlabel(const SFChnTrailer *trailer,
                                  char *error_message,
                                  int error_message_length);
static SFReturnCode CHN_read_counts(int fildes, int ncounts,
                                    int *counts, char *error_message,
                                    int error_message_length);
static SFReturnCode CHN_read_header(int fildes, SFChnHeader *header,
                                    char *error_message,
                                    int error_message_length);
static SFReturnCode CHN_read_trailer(int fildes, const SFChnHeader *header,
                                     SFChnTrailer *trailer,
                                     char *error_message,
                                     int error_message_length);
static SFReturnCode CHN_write_counts(int fildes, const SFChnHeader *header,
                                     const int *counts, char *error_message,
                                     int error_message_length);
static SFReturnCode CHN_write_header(int fildes, const SFChnHeader *header,
                                     char *error_message,
                                     int error_message_length);
static SFReturnCode CHN_write_trailer(int fildes, const SFChnHeader *header,
                                      const SFChnTrailer *trailer,
                                      char *error_message,
                                      int error_message_length);
/* implementation of public routines */

SFReturnCode SF_chn_file(const char *filename, SFboolean *answer,
                         char *error_message, int error_message_length)
{
int		      fildes;
SFChnHeader	  header;
char	      *lower_suffix;
SFReturnCode  ret_code;

ret_code = SF_SUCCESS;

lower_suffix = PRV_get_lower_suffix(filename);

if (strcmp(lower_suffix, SF_CHN_SUFFIX) != 0)
   {
   strcpy_s(error_message, error_message_length,
            "file must have chn, Chn, or CHN suffix");
   *answer = SF_FALSE;
   return(SF_NAME_ERR);
   }

if ((ret_code = PRV_open(filename, SF_OPEN_READONLY, &fildes, error_message,
                         error_message_length))
    != SF_SUCCESS)
   {
   *answer = SF_FALSE;
   return(ret_code);
   }

if ((ret_code = CHN_read_header(fildes, &header, error_message,
                                error_message_length)) == SF_SUCCESS)
   {
   if ((*answer = CHN_checkhdlabel(&header, error_message,
                                   error_message_length)) != SF_TRUE)
      ret_code = SF_CORRUPT;
   }
else
   *answer = SF_FALSE;

/* ret_code = */ PRV_close(fildes);

return(ret_code);
}

   SFReturnCode SF_chn_get_counts(const char *filename, int listlength,
                                  int *counts, char *error_message,
                                  int error_message_length)
{
int		fildes;
SFboolean       is_chn;
SFReturnCode	ret_code;
SFChnHeader	header;

if ((ret_code = SF_chn_file(filename, &is_chn, error_message,
                            error_message_length)) != SF_SUCCESS)
   return(ret_code);

if ((ret_code = PRV_open(filename, SF_OPEN_READONLY, &fildes, error_message,
                         error_message_length))
    != SF_SUCCESS)
   return(ret_code);

if ((ret_code = CHN_read_header(fildes, &header, error_message,
                                error_message_length)) == SF_SUCCESS)
   {
   if (header.nchannels > listlength)
      {
      /* TODO set message and return error */
      ret_code = SF_SPACE_ERR;
      }
   else if (CHN_checkhdlabel(&header, error_message, error_message_length)
       == SF_TRUE)
      {
      ret_code = CHN_read_counts(fildes, header.nchannels, counts,
                                 error_message, error_message_length);
      }
   else
      ret_code = SF_CORRUPT;
   }

/* ret_code = */ PRV_close(fildes);

return(ret_code);
}

SFReturnCode SF_chn_get_header(const char *filename, SFChnHeader *header,
                               char *error_message, int error_message_length)
{
int		fildes;
SFboolean       is_chn;
SFReturnCode	ret_code;

if ((ret_code = SF_chn_file(filename, &is_chn, error_message,
                            error_message_length)) != SF_SUCCESS)
   return(ret_code);

if ((ret_code = PRV_open(filename, SF_OPEN_READONLY, &fildes, error_message,
                         error_message_length))
    != SF_SUCCESS)
   return(ret_code);

if ((ret_code = CHN_read_header(fildes, header, error_message,
                                error_message_length)) == SF_SUCCESS)
   if (CHN_checkhdlabel(header, error_message, error_message_length)
       != SF_TRUE)
      ret_code = SF_CORRUPT;

/* ret_code = */ PRV_close(fildes);

return(ret_code);
}

SFReturnCode SF_chn_get_trailer(const char *filename, SFChnTrailer *trailer,
                                char *error_message, int error_message_length)
{
int		fildes;
SFboolean       is_chn;
SFReturnCode	ret_code;
SFChnHeader	header;

if ((ret_code = SF_chn_file(filename, &is_chn, error_message,
                            error_message_length)) != SF_SUCCESS)
   return(ret_code);

if ((ret_code = PRV_open(filename, SF_OPEN_READONLY, &fildes, error_message,
                         error_message_length))
    != SF_SUCCESS)
   return(ret_code);

if ((ret_code = CHN_read_header(fildes, &header, error_message,
                                error_message_length)) == SF_SUCCESS)
   {
   if (CHN_checkhdlabel(&header, error_message, error_message_length)
       == SF_TRUE)
      {
      if ((ret_code = CHN_read_trailer(fildes, &header, trailer, error_message,
                                       error_message_length))
          == SF_SUCCESS)
         if (CHN_checktrlabel(trailer, error_message, error_message_length)
             != SF_TRUE)
            ret_code = SF_CORRUPT;
      }
   else
      ret_code = SF_CORRUPT;
   }

/* ret_code = */ PRV_close(fildes);

return(ret_code);
}

SFReturnCode SF_chn_put_spectrum(const char *filename,
                                 const SFChnHeader *header, const int *counts,
                                 const SFChnTrailer *trailer,
                                 char *error_message, int error_message_length)
{
int		fildes;
SFboolean       is_chn;
SFReturnCode	ret_code;

if ((ret_code = SF_chn_file(filename, &is_chn, error_message,
                            error_message_length)) != SF_SUCCESS)
   {
   if (ret_code == SF_NAME_ERR)
      {
      return(ret_code);
      }

   if (_access(filename, _A_NORMAL) == -1)
      {
      if ((ret_code = PRV_open(filename, SF_OPEN_CREATE, &fildes,
                               error_message, error_message_length))
          != SF_SUCCESS)
         {
         return(ret_code);
         }
      }
   }

else if ((ret_code = PRV_open(filename, SF_OPEN_READWRITE, &fildes,
                              error_message, error_message_length))
         != SF_SUCCESS)
   return(ret_code);

if (((ret_code = CHN_write_header(fildes, header, error_message,
                                  error_message_length))
     != SF_SUCCESS) ||
    ((ret_code = CHN_write_counts(fildes, header, counts, error_message,
                                  error_message_length))
     != SF_SUCCESS) ||
    ((ret_code = CHN_write_trailer(fildes, header, trailer, error_message,
                                   error_message_length))
     != SF_SUCCESS))
   {
   /* then immediately close the file, and return the error code */
   }

/* ret_code = */ PRV_close(fildes);

return(ret_code);
}

SFReturnCode SF_chn_put_trailer(const char *filename,
                                const SFChnTrailer *trailer,
                                char *error_message, int error_message_length)
{
int		fildes;
SFboolean       is_chn;
SFReturnCode	ret_code;
SFChnHeader	header;

if ((ret_code = SF_chn_file(filename, &is_chn, error_message,
                            error_message_length)) != SF_SUCCESS)
   return(ret_code);

if ((ret_code = PRV_open(filename, SF_OPEN_READWRITE, &fildes, error_message,
                         error_message_length))
    != SF_SUCCESS)
   return(ret_code);

if ((ret_code = CHN_read_header(fildes, &header, error_message,
                                error_message_length)) == SF_SUCCESS)
   {
   if (CHN_checkhdlabel(&header, error_message, error_message_length)
       == SF_TRUE)
      ret_code = CHN_write_trailer(fildes, &header, trailer, error_message,
                                   error_message_length);
   else
      ret_code = SF_CORRUPT;
   }

/* ret_code = */ PRV_close(fildes);

return(ret_code);
}

/* implementation of private utilities */

static SFboolean CHN_checkhdlabel(const SFChnHeader *header,
                                  char *error_message,
                                  int error_message_length)
{
if ((int) header->header_tag != (int) SF_CHN_HEADER_TAG)
   {
   strcpy_s(error_message, error_message_length,
            "header tag wrong in CHN file");
   return(SF_FALSE);
   }
else
   return(SF_TRUE);
}

static SFboolean CHN_checktrlabel(const SFChnTrailer *trailer,
                                  char *error_message,
                                  int error_message_length)
{
if (((int) trailer->trailer_tag != (int) SF_CHN_TRAILER_TAG) &&
    ((int) trailer->trailer_tag != (int) SF_CHN_QUAD_TRAIL_TAG))
   {
   strcpy_s(error_message, error_message_length,
            "trailer tag wrong in CHN file");
   return(SF_FALSE);
   }
else
   return(SF_TRUE);
}

static SFReturnCode CHN_read_counts(int fildes, int ncounts,
                                    int *counts, char *error_message,
                                    int error_message_length)
{
int	counts_offset, counts_size;
int	intsize = sizeof(int);

#ifdef SF_BIG_ENDIAN
int	i;
#endif

counts_size = ncounts * intsize;
counts_offset = sizeof(SFChnHeader);

/*
 * The elements of a C structure might not occur in memory contiguously,
 *
 * But the counts are most likely adjacent to the header because
 * the header size is a multiple of 4 bytes.
 *
 * So the counts can be read in one request.
 */

if ((_lseek(fildes, counts_offset, SEEK_SET) == -1) ||
	(_read(fildes, counts, counts_size) != counts_size))
   {
   strcpy_s(error_message, error_message_length,
            "file ended before end of header.");
   return(SF_EARLY_END);
   }

#ifdef SF_BIG_ENDIAN
for (i = 0; i < ncounts; i++)
   PRV_byte_reverse(&counts[i], intsize);
#endif

return(SF_SUCCESS);
}

static SFReturnCode CHN_read_header(int fildes, SFChnHeader *header,
                                    char *error_message,
                                    int error_message_length)
{
int	header_offset, header_size;

header_size = sizeof(SFChnHeader);
header_offset = 0;

/*
 * The elements of a C structure might not occur in memory contiguously,
 *
 * But current layout of the SFChnHeader structure shows that
 * the items are most likely contiguous in memory.
 * 1. Integers start at a multiple of 4 bytes from the beginning
 * 2. short integers start at a multiple of 2 bytes from the beginning
 * 3. characters are on byte boundaries
 *
 * So they can be read in one request.
 */

if ((_lseek(fildes, header_offset, SEEK_SET) == -1) ||
    (_read(fildes, header, header_size) != header_size))
   {
   strcpy_s(error_message, error_message_length,
            "file ended before end of header.");
   return(SF_EARLY_END);
   }

#ifdef SF_BIG_ENDIAN
PRV_byte_reverse(&header->header_tag, sizeof(header->header_tag));
PRV_byte_reverse(&header->mca_no, sizeof(header->mca_no));
PRV_byte_reverse(&header->segment_no, sizeof(header->segment_no));
PRV_byte_reverse(&header->real_time, sizeof(header->real_time));
PRV_byte_reverse(&header->live_time, sizeof(header->live_time));
PRV_byte_reverse(&header->min_chan, sizeof(header->min_chan));
PRV_byte_reverse(&header->nchannels, sizeof(header->nchannels));
#endif

return(SF_SUCCESS);
}

static SFReturnCode CHN_read_trailer(int fildes, const SFChnHeader *header,
                                     SFChnTrailer *trailer,
                                     char *error_message,
                                     int error_message_length)
{
int	trailer_offset, trailer_size;

trailer_size = sizeof(SFChnTrailer);
trailer_offset = sizeof(SFChnHeader) + (header->nchannels * sizeof(int));

/*
 * The elements of a C structure might not occur in memory contiguously,
 *
 * But current layout of the SFChnTrailer structure shows that
 * the items are most likely contiguous in memory.
 * 1. floats start at a multiple of 4 bytes from the beginning
 * 2. short integers start at a multiple of 2 bytes from the beginning
 * 3. characters are on byte boundaries
 *
 * Also, the trailer is most likely adjacent to the counts because
 * the header size is a multiple of 4 bytes as is the count size.
 *
 * So the trailer can be read in one request.
 */

if ((_lseek(fildes, trailer_offset, SEEK_SET) == -1) ||
    (_read(fildes, trailer, trailer_size) != trailer_size))
   {
   strcpy_s(error_message, error_message_length,
            "file ended before end of trailer.");
   return(SF_EARLY_END);
   }

#ifdef SF_BIG_ENDIAN
PRV_byte_reverse(&trailer->trailer_tag, sizeof(trailer->trailer_tag));
PRV_byte_reverse(&trailer->const_ecalib, sizeof(trailer->const_ecalib));
PRV_byte_reverse(&trailer->lin_ecalib, sizeof(trailer->lin_ecalib));
PRV_byte_reverse(&trailer->quad_ecalib, sizeof(trailer->quad_ecalib));
PRV_byte_reverse(&trailer->const_wcalib, sizeof(trailer->const_wcalib));
PRV_byte_reverse(&trailer->lin_wcalib, sizeof(trailer->lin_wcalib));
PRV_byte_reverse(&trailer->quad_wcalib, sizeof(trailer->quad_wcalib));
#endif

return(SF_SUCCESS);
}

static SFReturnCode CHN_write_counts(int fildes, const SFChnHeader *header,
                                     const int *counts, char *error_message,
                                     int error_message_length)
{
int		counts_offset, counts_size;
int		intsize = sizeof(int);
SFReturnCode	ret_code;

#ifdef SF_BIG_ENDIAN
int	i, *countcopy;
#endif

counts_offset = sizeof(SFChnHeader);
counts_size = header->nchannels * intsize;

if (_lseek(fildes, counts_offset, SEEK_SET) == -1)
   {
   strcpy_s(error_message, error_message_length,
            "file ended before end of header.");
   return(SF_EARLY_END);
   }

/*
 * this write assumes that an integer array is stored
 * contiguously in memory
 */

#ifdef SF_BIG_ENDIAN
if ((countcopy = (int *) calloc(header->nchannels, intsize)) == NULL)
   return(SF_MALLOC_ERR);

for (i = 0; i < header->nchannels; i++)
   {
   countcopy[i] = counts[i];
   PRV_byte_reverse(&countcopy[i], intsize);
   }

if (write(fildes, countcopy, counts_size) != counts_size)
#else
if (_write(fildes, counts, counts_size) != counts_size)
#endif
   {
   strcpy_s(error_message, error_message_length,
            "write ended before end of counts");
   ret_code = SF_EARLY_END;
   }
else
   ret_code = SF_SUCCESS;

#ifdef SF_BIG_ENDIAN
free(countcopy);
#endif

return(ret_code);
}

static SFReturnCode CHN_write_header(int fildes, const SFChnHeader *header,
                                     char *error_message,
                                     int error_message_length)
{
int		header_offset, header_size;

#ifdef SF_BIG_ENDIAN
SFChnHeader	header_copy;
#endif

header_offset = 0;
header_size = sizeof(SFChnHeader);

if (_lseek(fildes, header_offset, SEEK_SET) == -1)
   {
   strcpy_s(error_message, error_message_length,
            "file ended before header.");
   return(SF_EARLY_END);
   }

/*
 * The elements of a C structure might not occur in memory contiguously,
 *
 * But current layout of the SFChnHeader structure shows that
 * the items are most likely contiguous in memory.
 * 1. Integers start at a multiple of 4 bytes from the beginning
 * 2. short integers start at a multiple of 2 bytes from the beginning
 * 3. characters are on byte boundaries
 *
 * So they can be written in one request.
 */

#ifdef SF_BIG_ENDIAN
memcpy(&header_copy, header, header_size);

PRV_byte_reverse(&header_copy.header_tag, sizeof(header->header_tag));
PRV_byte_reverse(&header_copy.mca_no, sizeof(header->mca_no));
PRV_byte_reverse(&header_copy.segment_no, sizeof(header->segment_no));
PRV_byte_reverse(&header_copy.real_time, sizeof(header->real_time));
PRV_byte_reverse(&header_copy.live_time, sizeof(header->live_time));
PRV_byte_reverse(&header_copy.min_chan, sizeof(header->min_chan));
PRV_byte_reverse(&header_copy.nchannels, sizeof(header->nchannels));

if (write(fildes, &header_copy, header_size) != header_size)
#else
if (_write(fildes, header, header_size) != header_size)
#endif
   {
   strcpy_s(error_message, error_message_length,
            "write ended before end of header");
   return(SF_EARLY_END);
   }

return(SF_SUCCESS);
}

static SFReturnCode CHN_write_trailer(int fildes, const SFChnHeader *header,
                                      const SFChnTrailer *trailer,
                                      char *error_message,
                                      int error_message_length)
{
int		trailer_offset, trailer_size;

#ifdef SF_BIG_ENDIAN
SFChnTrailer	trailer_copy;
#endif

trailer_offset = sizeof(SFChnHeader) + (header->nchannels * sizeof(int));
trailer_size = sizeof(SFChnTrailer);

if (_lseek(fildes, trailer_offset, SEEK_SET) == -1)
   {
   strcpy_s(error_message, error_message_length,
            "file ended before trailer.");
   return(SF_EARLY_END);
   }

/*
 * The elements of a C structure might not occur in memory contiguously,
 *
 * But current layout of the SFChnTrailer structure shows that
 * the items are most likely contiguous in memory.
 * 1. floats start at a multiple of 4 bytes from the beginning
 * 2. short integers start at a multiple of 2 bytes from the beginning
 * 3. characters are on byte boundaries
 *
 * Also, the trailer is most likely adjacent to the counts because
 * the header size is a multiple of 4 bytes as is the count size.
 *
 * So the trailer can be written in one request.
 */

#ifdef SF_BIG_ENDIAN
memcpy(&trailer_copy, trailer, trailer_size);

PRV_byte_reverse(&trailer_copy.trailer_tag, sizeof(trailer->trailer_tag));
PRV_byte_reverse(&trailer_copy.const_ecalib, sizeof(trailer->const_ecalib));
PRV_byte_reverse(&trailer_copy.lin_ecalib, sizeof(trailer->lin_ecalib));
PRV_byte_reverse(&trailer_copy.quad_ecalib, sizeof(trailer->quad_ecalib));
PRV_byte_reverse(&trailer_copy.const_wcalib, sizeof(trailer->const_wcalib));
PRV_byte_reverse(&trailer_copy.lin_wcalib, sizeof(trailer->lin_wcalib));
PRV_byte_reverse(&trailer_copy.quad_wcalib, sizeof(trailer->quad_wcalib));

if (write(fildes, &trailer_copy, trailer_size) != trailer_size)
#else
if (_write(fildes, trailer, trailer_size) != trailer_size)
#endif
   {
   strcpy_s(error_message, error_message_length,
            "write ended before end of trailer");
   return(SF_EARLY_END);
   }

return(SF_SUCCESS);
}
