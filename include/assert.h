#ifndef __ASSERT_H__
#define __ASSERT_H__
#ifdef __cplusplus
extern "C" {
#endif
/**************************************************************************
 *									  *
 * 		 Copyright (C) 1984, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "$Revision: 1.17 $"

/* ANSI C Notes:
 *
 * - THE IDENTIFIERS APPEARING OUTSIDE OF #ifdef __EXTENSIONS__ IN THIS
 *   standard header ARE SPECIFIED BY ANSI!  CONFORMANCE WILL BE ALTERED
 *   IF ANY NEW IDENTIFIERS ARE ADDED TO THIS AREA UNLESS THEY ARE IN ANSI's
 *   RESERVED NAMESPACE. (i.e., unless they are prefixed by __[a-z] or
 *   _[A-Z].  For external objects, identifiers with the prefix _[a-z]
 *   are also reserved.)
 */

#ifdef NDEBUG
#undef assert
#define assert(EX) ((void)0)

#else

extern void		osSyncPrintf(const char *fmt, ...);
#define assert(EX)  if(!(EX))osSyncPrintf("\n--- ASSERTION FAULT - %s - %s, line %d\n\n", # EX , __FILE__, __LINE__)

//  Test EX and print MSG if fail - possibly cannonically "Debug" from rmon
#define assertmsg(EX, MSG) if (!(EX)) osSyncPrintf(MSG)

// rmon Debug copied temporarelly below would leave object accesses in assembly without using or printing them
// eg Debug("some message %f\n", sin(x));
// would result in sin(x) being calculated but not printed or used
#if defined(RMONDEBUG)
    #define Debug rmonPrintf
#else /* not RMONDEBUG, do nothing */
     #define Debug
#endif

// Test EX and print MSG and file info if fail
#define assertmsg2(EX, MSG) \
    if (!(EX)) osSyncPrintf("%s, file %s, line %d\n", MSG, __FILE__, __LINE__)

// extern void __assert(const char *, const char *, int);
// #ifdef __ANSI_CPP__
// #define assert(EX)  ((EX)?((void)0):__assert( # EX , __FILE__, __LINE__))
// #else
// #define assert(EX)  ((EX)?((void)0):__assert("EX", __FILE__, __LINE__))
// #endif
#endif /* NDEBUG */

#ifdef __cplusplus
}
#endif

#endif /* !__ASSERT_H__ */
