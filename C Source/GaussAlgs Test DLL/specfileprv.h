/*
 * Copyright 2017 Battelle Energy Alliance
 */

/*
 * specfileprv.h - contains prototypes for the routines in private.c
 */

#ifndef SPECFILEPRV_H
#define SPECFILEPRV_H

/*
 * When building a DLL in Visual C++, declspec() is required.
 * Otherwise, leave this undefined.
 */

/*
#define DLLEXPORT
 */

#define DLLEXPORT __declspec(dllexport)

#define	PRV_MAX_SUFFIX_LEN	10

/*
 * enumeration of ways to open a file
 */

   typedef enum
      {
      SF_OPEN_READONLY,
      SF_OPEN_WRITEONLY,
      SF_OPEN_READWRITE,
      SF_OPEN_CREATE
      } SFOpenMode;


/*
 * Prototypes in alphabetical order
 */

#ifdef __cplusplus
extern "C" {
#endif


/*
 * PRV_byte_reverse	reverse byte order of any array of even size.
 *			Can be used to reverse order for an integer, float,
 *			or short int.
 *
 *			If you use it to reverse array of integers, must
 *			call this to reverse each integer separately.
 */

   DLLEXPORT void PRV_byte_reverse(void *bytes, int count);


/*
 *  PRV_close	closes a file that was opened with PRV_open.
 */

   DLLEXPORT SFReturnCode PRV_close(int fildes);


/*
 *  PRV_get_suffix	gets lower-case suffix from a filename.
 */

   DLLEXPORT char *PRV_get_lower_suffix(const char *filename);


/*
 *  PRV_open	opens a file and returns a file descriptor for use
 *		in reads, writes, and closes.
 *
 *		calling software must provide storage for err_msg:
 *		err_msg[SF_MAX_NAMLEN].
 */

   DLLEXPORT SFReturnCode PRV_open(const char *filename, SFOpenMode mode,
	                               int *fildes, char *error_message,
                                   int error_message_length);


/*
 * PRV_vax_to_ieeeflt	convert vax float into ieee float
 *			pass in bitstream as received from vax over the net
 */

   DLLEXPORT float PRV_vax_to_ieeeflt(float vaxflt);

#ifdef __cplusplus
}
#endif


#endif  /* SPECFILEPRV_H */
