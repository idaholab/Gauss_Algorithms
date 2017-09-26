/*
 * Copyright 2017 Battelle Energy Alliance
 */

/*
 *  private.c - contains various routines for accessing, reading,
 *              and writing any format spectra.
 *
 *  Prototypes for the routines can be found in the specfileprv.h
 *  include file.
 *
 */

#include <sys/types.h>	/* for _sopen_s() */
#include <sys/stat.h>	/* for _sopen_s() */
#include <fcntl.h>	    /* for _sopen_s() */
#include <share.h>      /* for _sopen_s() */
#include <io.h>         /* for _sopen_s() */
#include <string.h>	    /* for strrchr(), strcpy_s() */
#include <math.h>	    /* for pow() */

#include "SpecFileLib.h"	/* for types and definitions */
#include "specfileprv.h"


/* implementation of public routines */

void PRV_byte_reverse(void *bytes, int count)
{
char	*input, temp;
int	i;

input = (char *) bytes;

for (i = 0; i < count/2; i++)
   {
   temp = input[i];
   input[i] = input[count - 1 - i];
   input[count - 1 - i] = temp;
   }
}

SFReturnCode PRV_close(int fildes)
{
if (_close(fildes) == 0)
   return(SF_SUCCESS);
else
   return(SF_CLOSE_ERR);
}

char *PRV_get_lower_suffix(const char *filename)
{
const char  *period;
static char	suffix[PRV_MAX_SUFFIX_LEN];

/* look for last period */

   if ((period = strrchr(filename, '.')) == NULL)
      suffix[0] = '\0';
   else
      strcpy_s(suffix, PRV_MAX_SUFFIX_LEN, period + 1);

/* convert to lower case */

_strlwr_s(suffix, PRV_MAX_SUFFIX_LEN);

return(suffix);
}

SFReturnCode PRV_open(const char *filename, SFOpenMode mode, int *fildes,
                      char *error_message, int error_message_length)
{
errno_t         err;
SFReturnCode	ret_code;

ret_code = SF_SUCCESS;

switch(mode)
   {
   case SF_OPEN_READONLY:
/*
      if ((*fildes = open(filename, O_RDONLY | O_NONBLOCK, NULL)) == -1)
 */
      if ((err = _sopen_s(fildes, filename, O_RDONLY, _SH_DENYNO, _S_IREAD))
          != 0)
         {
         ret_code = SF_OPENR_ERR;
         strcpy_s(error_message, error_message_length,
                  "read permission denied");
         }
      break;
   case SF_OPEN_WRITEONLY:
      if ((err = _sopen_s(fildes, filename, O_WRONLY, _SH_DENYNO, _S_IREAD))
          != 0)
         {
         ret_code = SF_OPENW_ERR;
         strcpy_s(error_message, error_message_length,
                  "write permission denied");
         }
      break;
   case SF_OPEN_READWRITE:
      if ((err = _sopen_s(fildes, filename, O_RDWR, _SH_DENYNO, _S_IREAD))
          != 0)
         {
         ret_code = SF_OPENRW_ERR;
         strcpy_s(error_message, error_message_length,
                  "read/write permission denied");
         }
      break;
   case SF_OPEN_CREATE:
      if ((err = _sopen_s(fildes, filename, O_RDWR | O_CREAT, _SH_DENYNO,
                          _S_IREAD | _S_IWRITE)) != 0)
         {
         ret_code = SF_OPENC_ERR;
         strcpy_s(error_message, error_message_length,
                  "not allowed to create file");
         }
      break;
   default:
      ret_code = SF_FAILURE;
      strcpy_s(error_message, error_message_length,
               "unknown software open request");
      break;
   }

	/* do not look for line endings during reads */
	/* do not add line endings during write */
	/* learned this 10/2003 with Matlab PINS task on MSWindows */
	if (ret_code == SF_SUCCESS)
		{
		_setmode(*fildes, _O_BINARY);
		}

return(ret_code);
}

float PRV_vax_to_ieeeflt(float vaxflt)
{
struct
   {
   unsigned int	sign : 1;
   unsigned int	exponent : 8;
   unsigned int fraction : 23;
   } fbits;

double		temp1, temp2;
float		answer;
unsigned int	temp_exponent, temp_fraction;
int		floatsize = sizeof(fbits);

/* check special case of zero */

   if (vaxflt == 0.0)
      {
      answer = (float) 0.0;
      return(answer);
      }

memcpy(&fbits, &vaxflt, floatsize);

#ifdef SF_BIG_ENDIAN
PRV_byte_reverse(&fbits, floatsize);
#endif

temp_exponent = fbits.exponent;
temp1 = pow(2.0, (int) temp_exponent - 129);
temp2 = pow(2.0, -23);

temp_fraction = fbits.fraction;
answer = (float) (temp1 * (1 + ((int) temp_fraction * temp2)));

if (fbits.sign == 1)
   answer = -answer;

return(answer);
}
