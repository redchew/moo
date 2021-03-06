/*
 * $Id$
 *
    Copyright (c) 2014-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MOO_CMN_H_
#define _MOO_CMN_H_

/* WARNING: NEVER CHANGE/DELETE THE FOLLOWING MOO_HAVE_CFG_H DEFINITION. 
 *          IT IS USED FOR DEPLOYMENT BY MAKEFILE.AM */
/*#define MOO_HAVE_CFG_H*/

#if defined(MOO_HAVE_CFG_H)
#	include <moo-cfg.h>
#elif defined(_WIN32)
#	include <moo-msw.h>
#elif defined(__OS2__)
#	include <moo-os2.h>
#elif defined(__DOS__)
#	include <moo-dos.h>
#elif defined(macintosh)
#	include <moo-mac.h> /* class mac os */
#else
#	error UNSUPPORTED SYSTEM
#endif


/* =========================================================================
 * ARCHITECTURE/COMPILER TWEAKS
 * ========================================================================= */

#if defined(EMSCRIPTEN)
#	if defined(MOO_SIZEOF___INT128)
#		undef MOO_SIZEOF___INT128 
#		define MOO_SIZEOF___INT128 0
#	endif
#	if defined(MOO_SIZEOF_LONG) && defined(MOO_SIZEOF_INT) && (MOO_SIZEOF_LONG > MOO_SIZEOF_INT)
		/* autoconf doesn't seem to match actual emscripten */
#		undef MOO_SIZEOF_LONG
#		define MOO_SIZEOF_LONG MOO_SIZEOF_INT
#	endif
#	include <emscripten/emscripten.h> /* EMSCRIPTEN_KEEPALIVE */
#endif

#if defined(__GNUC__) && defined(__arm__)  && !defined(__ARM_ARCH)
#	if defined(__ARM_ARCH_8__)
#		define __ARM_ARCH 8
#	elif defined(__ARM_ARCH_7__)
#		define __ARM_ARCH 7
#	elif defined(__ARM_ARCH_6__)
#		define __ARM_ARCH 6
#	elif defined(__ARM_ARCH_5__)
#		define __ARM_ARCH 5
#	elif defined(__ARM_ARCH_4__)
#		define __ARM_ARCH 4
#	endif
#endif

/* =========================================================================
 * PRIMITIVE TYPE DEFINTIONS
 * ========================================================================= */

/* moo_int8_t */
#if defined(MOO_SIZEOF_CHAR) && (MOO_SIZEOF_CHAR == 1)
#	define MOO_HAVE_UINT8_T
#	define MOO_HAVE_INT8_T
#	define MOO_SIZEOF_UINT8_T (MOO_SIZEOF_CHAR)
#	define MOO_SIZEOF_INT8_T (MOO_SIZEOF_CHAR)
	typedef unsigned char      moo_uint8_t;
	typedef signed char        moo_int8_t;
#elif defined(MOO_SIZEOF___INT8) && (MOO_SIZEOF___INT8 == 1)
#	define MOO_HAVE_UINT8_T
#	define MOO_HAVE_INT8_T
#	define MOO_SIZEOF_UINT8_T (MOO_SIZEOF___INT8)
#	define MOO_SIZEOF_INT8_T (MOO_SIZEOF___INT8)
	typedef unsigned __int8    moo_uint8_t;
	typedef signed __int8      moo_int8_t;
#elif defined(MOO_SIZEOF___INT8_T) && (MOO_SIZEOF___INT8_T == 1)
#	define MOO_HAVE_UINT8_T
#	define MOO_HAVE_INT8_T
#	define MOO_SIZEOF_UINT8_T (MOO_SIZEOF___INT8_T)
#	define MOO_SIZEOF_INT8_T (MOO_SIZEOF___INT8_T)
	typedef unsigned __int8_t  moo_uint8_t;
	typedef signed __int8_t    moo_int8_t;
#else
#	define MOO_HAVE_UINT8_T
#	define MOO_HAVE_INT8_T
#	define MOO_SIZEOF_UINT8_T (1)
#	define MOO_SIZEOF_INT8_T (1)
	typedef unsigned char      moo_uint8_t;
	typedef signed char        moo_int8_t;
#endif

/* moo_int16_t */
#if defined(MOO_SIZEOF_SHORT) && (MOO_SIZEOF_SHORT == 2)
#	define MOO_HAVE_UINT16_T
#	define MOO_HAVE_INT16_T
#	define MOO_SIZEOF_UINT16_T (MOO_SIZEOF_SHORT)
#	define MOO_SIZEOF_INT16_T (MOO_SIZEOF_SHORT)
	typedef unsigned short int  moo_uint16_t;
	typedef signed short int    moo_int16_t;
#elif defined(MOO_SIZEOF___INT16) && (MOO_SIZEOF___INT16 == 2)
#	define MOO_HAVE_UINT16_T
#	define MOO_HAVE_INT16_T
#	define MOO_SIZEOF_UINT16_T (MOO_SIZEOF___INT16)
#	define MOO_SIZEOF_INT16_T (MOO_SIZEOF___INT16)
	typedef unsigned __int16    moo_uint16_t;
	typedef signed __int16      moo_int16_t;
#elif defined(MOO_SIZEOF___INT16_T) && (MOO_SIZEOF___INT16_T == 2)
#	define MOO_HAVE_UINT16_T
#	define MOO_HAVE_INT16_T
#	define MOO_SIZEOF_UINT16_T (MOO_SIZEOF___INT16_T)
#	define MOO_SIZEOF_INT16_T (MOO_SIZEOF___INT16_T)
	typedef unsigned __int16_t  moo_uint16_t;
	typedef signed __int16_t    moo_int16_t;
#else
#	define MOO_HAVE_UINT16_T
#	define MOO_HAVE_INT16_T
#	define MOO_SIZEOF_UINT16_T (2)
#	define MOO_SIZEOF_INT16_T (2)
	typedef unsigned short int  moo_uint16_t;
	typedef signed short int    moo_int16_t;
#endif


/* moo_int32_t */
#if defined(MOO_SIZEOF_INT) && (MOO_SIZEOF_INT == 4)
#	define MOO_HAVE_UINT32_T
#	define MOO_HAVE_INT32_T
#	define MOO_SIZEOF_UINT32_T (MOO_SIZEOF_INT)
#	define MOO_SIZEOF_INT32_T (MOO_SIZEOF_INT)
	typedef unsigned int        moo_uint32_t;
	typedef signed int          moo_int32_t;
#elif defined(MOO_SIZEOF_LONG) && (MOO_SIZEOF_LONG == 4)
#	define MOO_HAVE_UINT32_T
#	define MOO_HAVE_INT32_T
#	define MOO_SIZEOF_UINT32_T (MOO_SIZEOF_LONG)
#	define MOO_SIZEOF_INT32_T (MOO_SIZEOF_LONG)
	typedef unsigned long int   moo_uint32_t;
	typedef signed long int     moo_int32_t;
#elif defined(MOO_SIZEOF___INT32) && (MOO_SIZEOF___INT32 == 4)
#	define MOO_HAVE_UINT32_T
#	define MOO_HAVE_INT32_T
#	define MOO_SIZEOF_UINT32_T (MOO_SIZEOF___INT32)
#	define MOO_SIZEOF_INT32_T (MOO_SIZEOF___INT32)
	typedef unsigned __int32    moo_uint32_t;
	typedef signed __int32      moo_int32_t;
#elif defined(MOO_SIZEOF___INT32_T) && (MOO_SIZEOF___INT32_T == 4)
#	define MOO_HAVE_UINT32_T
#	define MOO_HAVE_INT32_T
#	define MOO_SIZEOF_UINT32_T (MOO_SIZEOF___INT32_T)
#	define MOO_SIZEOF_INT32_T (MOO_SIZEOF___INT32_T)
	typedef unsigned __int32_t  moo_uint32_t;
	typedef signed __int32_t    moo_int32_t;
#elif defined(__DOS__)
#	define MOO_HAVE_UINT32_T
#	define MOO_HAVE_INT32_T
#	define MOO_SIZEOF_UINT32_T (4)
#	define MOO_SIZEOF_INT32_T (4)
	typedef unsigned long int   moo_uint32_t;
	typedef signed long int     moo_int32_t;
#else
#	define MOO_HAVE_UINT32_T
#	define MOO_HAVE_INT32_T
#	define MOO_SIZEOF_UINT32_T (4)
#	define MOO_SIZEOF_INT32_T (4)
	typedef unsigned int        moo_uint32_t;
	typedef signed int          moo_int32_t;
#endif

/* moo_int64_t */
#if defined(MOO_SIZEOF_INT) && (MOO_SIZEOF_INT == 8)
#	define MOO_HAVE_UINT64_T
#	define MOO_HAVE_INT64_T
#	define MOO_SIZEOF_UINT64_T (MOO_SIZEOF_INT)
#	define MOO_SIZEOF_INT64_T (MOO_SIZEOF_INT)
	typedef unsigned int        moo_uint64_t;
	typedef signed int          moo_int64_t;
#elif defined(MOO_SIZEOF_LONG) && (MOO_SIZEOF_LONG == 8)
#	define MOO_HAVE_UINT64_T
#	define MOO_HAVE_INT64_T
#	define MOO_SIZEOF_UINT64_T (MOO_SIZEOF_LONG)
#	define MOO_SIZEOF_INT64_T (MOO_SIZEOF_LONG)
	typedef unsigned long int  moo_uint64_t;
	typedef signed long int    moo_int64_t;
#elif defined(MOO_SIZEOF_LONG_LONG) && (MOO_SIZEOF_LONG_LONG == 8)
#	define MOO_HAVE_UINT64_T
#	define MOO_HAVE_INT64_T
#	define MOO_SIZEOF_UINT64_T (MOO_SIZEOF_LONG_LONG)
#	define MOO_SIZEOF_INT64_T (MOO_SIZEOF_LONG_LONG)
	typedef unsigned long long int  moo_uint64_t;
	typedef signed long long int    moo_int64_t;
#elif defined(MOO_SIZEOF___INT64) && (MOO_SIZEOF___INT64 == 8)
#	define MOO_HAVE_UINT64_T
#	define MOO_HAVE_INT64_T
#	define MOO_SIZEOF_UINT64_T (MOO_SIZEOF_LONG___INT64)
#	define MOO_SIZEOF_INT64_T (MOO_SIZEOF_LONG___INT64)
	typedef unsigned __int64    moo_uint64_t;
	typedef signed __int64      moo_int64_t;
#elif defined(MOO_SIZEOF___INT64_T) && (MOO_SIZEOF___INT64_T == 8)
#	define MOO_HAVE_UINT64_T
#	define MOO_HAVE_INT64_T
#	define MOO_SIZEOF_UINT64_T (MOO_SIZEOF_LONG___INT64_T)
#	define MOO_SIZEOF_INT64_T (MOO_SIZEOF_LONG___INT64_T)
	typedef unsigned __int64_t  moo_uint64_t;
	typedef signed __int64_t    moo_int64_t;
#else
	/* no 64-bit integer */
#endif

/* moo_int128_t */
#if defined(MOO_SIZEOF_INT) && (MOO_SIZEOF_INT == 16)
#	define MOO_HAVE_UINT128_T
#	define MOO_HAVE_INT128_T
#	define MOO_SIZEOF_UINT128_T (MOO_SIZEOF_INT)
#	define MOO_SIZEOF_INT128_T (MOO_SIZEOF_INT)
	typedef unsigned int        moo_uint128_t;
	typedef signed int          moo_int128_t;
#elif defined(MOO_SIZEOF_LONG) && (MOO_SIZEOF_LONG == 16)
#	define MOO_HAVE_UINT128_T
#	define MOO_HAVE_INT128_T
#	define MOO_SIZEOF_UINT128_T (MOO_SIZEOF_LONG)
#	define MOO_SIZEOF_INT128_T (MOO_SIZEOF_LONG)
	typedef unsigned long int   moo_uint128_t;
	typedef signed long int     moo_int128_t;
#elif defined(MOO_SIZEOF_LONG_LONG) && (MOO_SIZEOF_LONG_LONG == 16)
#	define MOO_HAVE_UINT128_T
#	define MOO_HAVE_INT128_T
#	define MOO_SIZEOF_UINT128_T (MOO_SIZEOF_LONG_LONG)
#	define MOO_SIZEOF_INT128_T (MOO_SIZEOF_LONG_LONG)
	typedef unsigned long long int moo_uint128_t;
	typedef signed long long int   moo_int128_t;
#elif defined(MOO_SIZEOF___INT128) && (MOO_SIZEOF___INT128 == 16)
#	define MOO_HAVE_UINT128_T
#	define MOO_HAVE_INT128_T
#	define MOO_SIZEOF_UINT128_T (MOO_SIZEOF___INT128)
#	define MOO_SIZEOF_INT128_T (MOO_SIZEOF___INT128)
	typedef unsigned __int128    moo_uint128_t;
	typedef signed __int128      moo_int128_t;
#elif defined(MOO_SIZEOF___INT128_T) && (MOO_SIZEOF___INT128_T == 16)
#	define MOO_HAVE_UINT128_T
#	define MOO_HAVE_INT128_T
#	define MOO_SIZEOF_UINT128_T (MOO_SIZEOF___INT128_T)
#	define MOO_SIZEOF_INT128_T (MOO_SIZEOF___INT128_T)
	#if defined(MOO_SIZEOF___UINT128_T) && (MOO_SIZEOF___UINT128_T == MOO_SIZEOF___INT128_T)
	typedef __uint128_t  moo_uint128_t;
	typedef __int128_t   moo_int128_t;
	#elif defined(__clang__)
	typedef __uint128_t  moo_uint128_t;
	typedef __int128_t   moo_int128_t;
	#else
	typedef unsigned __int128_t  moo_uint128_t;
	typedef signed __int128_t    moo_int128_t;
	#endif
#else
	/* no 128-bit integer */
#endif

#if defined(MOO_HAVE_UINT8_T) && (MOO_SIZEOF_VOID_P == 1)
#	error UNSUPPORTED POINTER SIZE
#elif defined(MOO_HAVE_UINT16_T) && (MOO_SIZEOF_VOID_P == 2)
	typedef moo_uint16_t moo_uintptr_t;
	typedef moo_int16_t moo_intptr_t;
	typedef moo_uint8_t moo_ushortptr_t;
	typedef moo_int8_t moo_shortptr_t;
#elif defined(MOO_HAVE_UINT32_T) && (MOO_SIZEOF_VOID_P == 4)
	typedef moo_uint32_t moo_uintptr_t;
	typedef moo_int32_t moo_intptr_t;
	typedef moo_uint16_t moo_ushortptr_t;
	typedef moo_int16_t moo_shortptr_t;
#elif defined(MOO_HAVE_UINT64_T) && (MOO_SIZEOF_VOID_P == 8)
	typedef moo_uint64_t moo_uintptr_t;
	typedef moo_int64_t moo_intptr_t;
	typedef moo_uint32_t moo_ushortptr_t;
	typedef moo_int32_t moo_shortptr_t;
#elif defined(MOO_HAVE_UINT128_T) && (MOO_SIZEOF_VOID_P == 16) 
	typedef moo_uint128_t moo_uintptr_t;
	typedef moo_int128_t moo_intptr_t;
	typedef moo_uint64_t moo_ushortptr_t;
	typedef moo_int64_t moo_shortptr_t;
#else
#	error UNKNOWN POINTER SIZE
#endif

#define MOO_SIZEOF_INTPTR_T MOO_SIZEOF_VOID_P
#define MOO_SIZEOF_UINTPTR_T MOO_SIZEOF_VOID_P
#define MOO_SIZEOF_SHORTPTR_T (MOO_SIZEOF_VOID_P / 2)
#define MOO_SIZEOF_USHORTPTR_T (MOO_SIZEOF_VOID_P / 2)

#if defined(MOO_HAVE_INT128_T)
#	define MOO_SIZEOF_INTMAX_T 16
#	define MOO_SIZEOF_UINTMAX_T 16
	typedef moo_int128_t moo_intmax_t;
	typedef moo_uint128_t moo_uintmax_t;
#elif defined(MOO_HAVE_INT64_T)
#	define MOO_SIZEOF_INTMAX_T 8
#	define MOO_SIZEOF_UINTMAX_T 8
	typedef moo_int64_t moo_intmax_t;
	typedef moo_uint64_t moo_uintmax_t;
#elif defined(MOO_HAVE_INT32_T)
#	define MOO_SIZEOF_INTMAX_T 4
#	define MOO_SIZEOF_UINTMAX_T 4
	typedef moo_int32_t moo_intmax_t;
	typedef moo_uint32_t moo_uintmax_t;
#elif defined(MOO_HAVE_INT16_T)
#	define MOO_SIZEOF_INTMAX_T 2
#	define MOO_SIZEOF_UINTMAX_T 2
	typedef moo_int16_t moo_intmax_t;
	typedef moo_uint16_t moo_uintmax_t;
#elif defined(MOO_HAVE_INT8_T)
#	define MOO_SIZEOF_INTMAX_T 1
#	define MOO_SIZEOF_UINTMAX_T 1
	typedef moo_int8_t moo_intmax_t;
	typedef moo_uint8_t moo_uintmax_t;
#else
#	error UNKNOWN INTMAX SIZE
#endif

/* =========================================================================
 * FLOATING-POINT TYPE
 * ========================================================================= */
/** \typedef moo_fltbas_t
 * The moo_fltbas_t type defines the largest floating-pointer number type
 * naturally supported.
 */
#if defined(__FreeBSD__) || defined(__MINGW32__)
	/* TODO: check if the support for long double is complete.
	 *       if so, use long double for moo_flt_t */
	typedef double moo_fltbas_t;
#	define MOO_SIZEOF_FLTBAS_T MOO_SIZEOF_DOUBLE
#elif MOO_SIZEOF_LONG_DOUBLE > MOO_SIZEOF_DOUBLE
	typedef long double moo_fltbas_t;
#	define MOO_SIZEOF_FLTBAS_T MOO_SIZEOF_LONG_DOUBLE
#else
	typedef double moo_fltbas_t;
#	define MOO_SIZEOF_FLTBAS_T MOO_SIZEOF_DOUBLE
#endif

/** \typedef moo_fltmax_t
 * The moo_fltmax_t type defines the largest floating-pointer number type
 * ever supported.
 */
#if MOO_SIZEOF___FLOAT128 >= MOO_SIZEOF_FLTBAS_T
	/* the size of long double may be equal to the size of __float128
	 * for alignment on some platforms */
	typedef __float128 moo_fltmax_t;
#	define MOO_SIZEOF_FLTMAX_T MOO_SIZEOF___FLOAT128
#	define MOO_FLTMAX_REQUIRE_QUADMATH 1
#else
	typedef moo_fltbas_t moo_fltmax_t;
#	define MOO_SIZEOF_FLTMAX_T MOO_SIZEOF_FLTBAS_T
#	undef MOO_FLTMAX_REQUIRE_QUADMATH
#endif

#if defined(MOO_USE_FLTMAX)
typedef moo_fltmax_t moo_flt_t;
#define MOO_SIZEOF_FLT_T MOO_SIZEOF_FLTMAX_T
#else
typedef moo_fltbas_t moo_flt_t;
#define MOO_SIZEOF_FLT_T MOO_SIZEOF_FLTBAS_T
#endif

/* =========================================================================
 * BASIC HARD-CODED DEFINES
 * ========================================================================= */
#define MOO_BITS_PER_BYTE (8)
/* the maximum number of bch charaters to represent a single uch character */
#define MOO_BCSIZE_MAX 6

/* =========================================================================
 * BASIC MOO TYPES
 * ========================================================================= */

typedef char                    moo_bch_t;
typedef int                     moo_bci_t;
typedef unsigned int            moo_bcu_t;
typedef unsigned char           moo_bchu_t; /* unsigned version of moo_bch_t for inner working */
#define MOO_SIZEOF_BCH_T MOO_SIZEOF_CHAR
#define MOO_SIZEOF_BCI_T MOO_SIZEOF_INT

#if defined(MOO_UNICODE_SIZE) && (MOO_UNICODE_SIZE >= 4)
#	if defined(__GNUC__) && defined(__CHAR32_TYPE__)
	typedef __CHAR32_TYPE__    moo_uch_t;
#	else
	typedef moo_uint32_t       moo_uch_t;
#	endif
	typedef moo_uint32_t       moo_uchu_t; /* same as moo_uch_t as it is already unsigned */
#	define MOO_SIZEOF_UCH_T 4

#elif defined(__GNUC__) && defined(__CHAR16_TYPE__)
	typedef __CHAR16_TYPE__    moo_uch_t; 
	typedef moo_uint16_t       moo_uchu_t; /* same as moo_uch_t as it is already unsigned */
#	define MOO_SIZEOF_UCH_T 2
#else
	typedef moo_uint16_t       moo_uch_t;
	typedef moo_uint16_t       moo_uchu_t; /* same as moo_uch_t as it is already unsigned */
#	define MOO_SIZEOF_UCH_T 2
#endif

typedef moo_int32_t             moo_uci_t;
typedef moo_uint32_t            moo_ucu_t;
#define MOO_SIZEOF_UCI_T 4

typedef moo_uint8_t             moo_oob_t;

/* [NOTE] sizeof(moo_oop_t) must be equal to sizeof(moo_oow_t) */
typedef moo_uintptr_t           moo_oow_t;
typedef moo_intptr_t            moo_ooi_t;
#define MOO_SIZEOF_OOW_T MOO_SIZEOF_UINTPTR_T
#define MOO_SIZEOF_OOI_T MOO_SIZEOF_INTPTR_T
#define MOO_OOW_BITS  (MOO_SIZEOF_OOW_T * MOO_BITS_PER_BYTE)
#define MOO_OOI_BITS  (MOO_SIZEOF_OOI_T * MOO_BITS_PER_BYTE)

typedef moo_ushortptr_t         moo_oohw_t; /* half word - half word */
typedef moo_shortptr_t          moo_oohi_t; /* signed half word */
#define MOO_SIZEOF_OOHW_T MOO_SIZEOF_USHORTPTR_T
#define MOO_SIZEOF_OOHI_T MOO_SIZEOF_SHORTPTR_T
#define MOO_OOHW_BITS  (MOO_SIZEOF_OOHW_T * MOO_BITS_PER_BYTE)
#define MOO_OOHI_BITS  (MOO_SIZEOF_OOHI_T * MOO_BITS_PER_BYTE)

struct moo_ucs_t
{
	moo_uch_t* ptr;
	moo_oow_t  len;
};
typedef struct moo_ucs_t moo_ucs_t;

struct moo_bcs_t
{
	moo_bch_t* ptr;
	moo_oow_t  len;
};
typedef struct moo_bcs_t moo_bcs_t;

#if defined(MOO_ENABLE_UNICODE)
	typedef moo_uch_t               moo_ooch_t;
	typedef moo_uchu_t              moo_oochu_t;
	typedef moo_uci_t               moo_ooci_t;
	typedef moo_ucu_t               moo_oocu_t;
	typedef moo_ucs_t               moo_oocs_t;
#	define MOO_OOCH_IS_UCH
#	define MOO_SIZEOF_OOCH_T MOO_SIZEOF_UCH_T
#else
	typedef moo_bch_t               moo_ooch_t;
	typedef moo_bchu_t              moo_oochu_t;
	typedef moo_bci_t               moo_ooci_t;
	typedef moo_bcu_t               moo_oocu_t;
	typedef moo_bcs_t               moo_oocs_t;
#	define MOO_OOCH_IS_BCH
#	define MOO_SIZEOF_OOCH_T MOO_SIZEOF_BCH_T
#endif

typedef unsigned int moo_bitmask_t;

/* =========================================================================
 * BASIC OOP ENCODING
 * ========================================================================= */
/* actual structure defined in moo.h */
typedef struct moo_obj_t           moo_obj_t;
typedef struct moo_obj_t*          moo_oop_t;

/* 
 * An object pointer(OOP) is an ordinary pointer value to an object.
 * but some simple numeric values are also encoded into OOP using a simple
 * bit-shifting and masking.
 *
 * A real OOP is stored without any bit-shifting while a non-pointer value encoded
 * in an OOP is bit-shifted to the left by 2 and the 2 least-significant bits
 * are set to 1 or 2.
 * 
 * This scheme works because the object allocators aligns the object size to
 * a multiple of sizeof(moo_oop_t). This way, the 2 least-significant bits
 * of a real OOP are always 0s.
 *
 * With 2 bits, i can encode only 3 special types except object pointers. 
 * Since I need more than 3 special types, I extend the tag bits up to 4 bits
 * to represent a special data type that doesn't require a range as wide
 * as a small integer. A unicode character, for instance, only requires 21 
 * bits at most. An error doesn't need to be as diverse as a small integer.
 */

#define MOO_OOP_TAG_BITS_LO     2
#define MOO_OOP_TAG_BITS_HI     2

#define MOO_OOP_TAG_SMOOI       1    /* 01 */
#define MOO_OOP_TAG_SMPTR       2    /* 10 */
#define MOO_OOP_TAG_EXTENDED    3    /* 11 - internal use only */
#define MOO_OOP_TAG_CHAR        3    /* 0011 */
#define MOO_OOP_TAG_ERROR       7    /* 0111 */
#define MOO_OOP_TAG_RESERVED0   11   /* 1011 */
#define MOO_OOP_TAG_RESERVED1   15   /* 1111 */

#define MOO_OOP_GET_TAG_LO(oop) (((moo_oow_t)oop) & MOO_LBMASK(moo_oow_t, MOO_OOP_TAG_BITS_LO))
#define MOO_OOP_GET_TAG_LOHI(oop) (((moo_oow_t)oop) & MOO_LBMASK(moo_oow_t, MOO_OOP_TAG_BITS_LO + MOO_OOP_TAG_BITS_HI))
#define MOO_OOP_GET_TAG(oop) (MOO_OOP_GET_TAG_LO(oop) == MOO_OOP_TAG_EXTENDED? MOO_OOP_GET_TAG_LOHI(oop): MOO_OOP_GET_TAG_LO(oop))

#define MOO_OOP_IS_NUMERIC(oop) (MOO_OOP_GET_TAG_LO(oop) != 0)
#define MOO_OOP_IS_POINTER(oop) (MOO_OOP_GET_TAG_LO(oop) == 0)

#define MOO_OOP_IS_SMOOI(oop) (MOO_OOP_GET_TAG_LO(oop) == MOO_OOP_TAG_SMOOI)
#define MOO_OOP_IS_SMPTR(oop) (MOO_OOP_GET_TAG_LO(oop) == MOO_OOP_TAG_SMPTR)

#define MOO_SMOOI_TO_OOP(num) ((moo_oop_t)((((moo_oow_t)(moo_ooi_t)(num)) << MOO_OOP_TAG_BITS_LO) | MOO_OOP_TAG_SMOOI))
#define MOO_OOP_TO_SMOOI(oop) (((moo_ooi_t)oop) >> MOO_OOP_TAG_BITS_LO)
/*
#define MOO_SMPTR_TO_OOP(ptr) ((moo_oop_t)((((moo_oow_t)(ptr)) << MOO_OOP_TAG_BITS_LO) | MOO_OOP_TAG_SMPTR))
#define MOO_OOP_TO_SMPTR(oop) (((moo_ooi_t)oop) >> MOO_OOP_TAG_BITS_LO)
*/
#define MOO_SMPTR_TO_OOP(ptr) ((moo_oop_t)(((moo_oow_t)ptr) | MOO_OOP_TAG_SMPTR))
#define MOO_OOP_TO_SMPTR(oop) ((void*)(((moo_oow_t)oop) & ~MOO_LBMASK(moo_oow_t, MOO_OOP_TAG_BITS_LO)))

#define MOO_OOP_IS_CHAR(oop) (MOO_OOP_GET_TAG(oop) == MOO_OOP_TAG_CHAR)
#define MOO_OOP_IS_ERROR(oop) (MOO_OOP_GET_TAG(oop) == MOO_OOP_TAG_ERROR)

#define MOO_OOP_TO_CHAR(oop) (((moo_oow_t)oop) >> (MOO_OOP_TAG_BITS_LO + MOO_OOP_TAG_BITS_LO))
#define MOO_CHAR_TO_OOP(num) ((moo_oop_t)((((moo_oow_t)(num)) << (MOO_OOP_TAG_BITS_LO + MOO_OOP_TAG_BITS_LO)) | MOO_OOP_TAG_CHAR))
#define MOO_OOP_TO_ERROR(oop) (((moo_oow_t)oop) >> (MOO_OOP_TAG_BITS_LO + MOO_OOP_TAG_BITS_LO))
#define MOO_ERROR_TO_OOP(num) ((moo_oop_t)((((moo_oow_t)(num)) << (MOO_OOP_TAG_BITS_LO + MOO_OOP_TAG_BITS_LO)) | MOO_OOP_TAG_ERROR))

/* -------------------------------- */

/* SMOOI takes up 62 bits on a 64-bit architecture and 30 bits 
 * on a 32-bit architecture. The absolute value takes up 61 bits and 29 bits
 * respectively for the sign bit. */
#define MOO_SMOOI_BITS (MOO_OOI_BITS - MOO_OOP_TAG_BITS_LO)
#define MOO_SMOOI_ABS_BITS (MOO_SMOOI_BITS - 1)
#define MOO_SMOOI_MAX ((moo_ooi_t)(~((moo_oow_t)0) >> (MOO_OOP_TAG_BITS_LO + 1)))
/* Sacrificing 1 bit pattern for a negative SMOOI makes 
 * implementation a lot eaisier in many aspects. */
/*#define MOO_SMOOI_MIN (-MOO_SMOOI_MAX - 1)*/
#define MOO_SMOOI_MIN (-MOO_SMOOI_MAX)
#define MOO_IN_SMOOI_RANGE(ooi) ((ooi) >= MOO_SMOOI_MIN && (ooi) <= MOO_SMOOI_MAX)

/* SMPTR is a special value which has been devised to encode an address value
 * whose low MOO_OOP_TAG_BITS_LO bits are 0. its class is SmallPointer. A pointer
 * returned by most of system functions would be aligned to the pointer size. 
 * you can use the followings macros when converting such a pointer without loss. */
#define MOO_IN_SMPTR_RANGE(ptr) ((((moo_oow_t)ptr) & MOO_LBMASK(moo_oow_t, MOO_OOP_TAG_BITS_LO)) == 0)

#define MOO_CHAR_BITS (MOO_OOI_BITS - MOO_OOP_TAG_BITS_LO - MOO_OOP_TAG_BITS_HI)
#define MOO_CHAR_MIN 0
#define MOO_CHAR_MAX (~((moo_oow_t)0) >> (MOO_OOP_TAG_BITS_LO + MOO_OOP_TAG_BITS_HI))

#define MOO_ERROR_BITS (MOO_OOI_BITS - MOO_OOP_TAG_BITS_LO - MOO_OOP_TAG_BITS_HI)
#define MOO_ERROR_MIN 0
#define MOO_ERROR_MAX (~((moo_oow_t)0) >> (MOO_OOP_TAG_BITS_LO + MOO_OOP_TAG_BITS_HI))

/* TODO: There are untested code where a small integer is converted to moo_oow_t.
 *       for example, the spec making macro treats the number as moo_oow_t instead of moo_ooi_t.
 *       as of this writing, i skip testing that part with the spec value exceeding MOO_SMOOI_MAX.
 *       later, please verify it works, probably by limiting the value ranges in such macros
 */


/* =========================================================================
 * TIME-RELATED TYPES
 * =========================================================================*/
#define MOO_MSECS_PER_SEC  (1000)
#define MOO_MSECS_PER_MIN  (MOO_MSECS_PER_SEC * MOO_SECS_PER_MIN)
#define MOO_MSECS_PER_HOUR (MOO_MSECS_PER_SEC * MOO_SECS_PER_HOUR)
#define MOO_MSECS_PER_DAY  (MOO_MSECS_PER_SEC * MOO_SECS_PER_DAY)

#define MOO_USECS_PER_MSEC (1000)
#define MOO_NSECS_PER_USEC (1000)
#define MOO_NSECS_PER_MSEC (MOO_NSECS_PER_USEC * MOO_USECS_PER_MSEC)
#define MOO_USECS_PER_SEC  (MOO_USECS_PER_MSEC * MOO_MSECS_PER_SEC)
#define MOO_NSECS_PER_SEC  (MOO_NSECS_PER_USEC * MOO_USECS_PER_MSEC * MOO_MSECS_PER_SEC)

#define MOO_SECNSEC_TO_MSEC(sec,nsec) \
        (((moo_intptr_t)(sec) * MOO_MSECS_PER_SEC) + ((moo_intptr_t)(nsec) / MOO_NSECS_PER_MSEC))

#define MOO_SECNSEC_TO_USEC(sec,nsec) \
        (((moo_intptr_t)(sec) * MOO_USECS_PER_SEC) + ((moo_intptr_t)(nsec) / MOO_NSECS_PER_USEC))

#define MOO_SECNSEC_TO_NSEC(sec,nsec) \
        (((moo_intptr_t)(sec) * MOO_NSECS_PER_SEC) + (moo_intptr_t)(nsec))

#define MOO_SEC_TO_MSEC(sec) ((sec) * MOO_MSECS_PER_SEC)
#define MOO_MSEC_TO_SEC(sec) ((sec) / MOO_MSECS_PER_SEC)

#define MOO_USEC_TO_NSEC(usec) ((usec) * MOO_NSECS_PER_USEC)
#define MOO_NSEC_TO_USEC(nsec) ((nsec) / MOO_NSECS_PER_USEC)

#define MOO_MSEC_TO_NSEC(msec) ((msec) * MOO_NSECS_PER_MSEC)
#define MOO_NSEC_TO_MSEC(nsec) ((nsec) / MOO_NSECS_PER_MSEC)

#define MOO_MSEC_TO_USEC(msec) ((msec) * MOO_USECS_PER_MSEC)
#define MOO_USEC_TO_MSEC(usec) ((usec) / MOO_USECS_PER_MSEC)

#define MOO_SEC_TO_NSEC(sec) ((sec) * MOO_NSECS_PER_SEC)
#define MOO_NSEC_TO_SEC(nsec) ((nsec) / MOO_NSECS_PER_SEC)

#define MOO_SEC_TO_USEC(sec) ((sec) * MOO_USECS_PER_SEC)
#define MOO_USEC_TO_SEC(usec) ((usec) / MOO_USECS_PER_SEC)

typedef struct moo_ntime_t moo_ntime_t;
struct moo_ntime_t
{
	moo_intptr_t  sec;
	moo_int32_t   nsec; /* nanoseconds */
};

#define MOO_INIT_NTIME(c,s,ns) (((c)->sec = (s)), ((c)->nsec = (ns)))
#define MOO_CLEAR_NTIME(c) MOO_INIT_NTIME(c, 0, 0)

#define MOO_ADD_NTIME(c,a,b) \
	do { \
		(c)->sec = (a)->sec + (b)->sec; \
		(c)->nsec = (a)->nsec + (b)->nsec; \
		while ((c)->nsec >= MOO_NSECS_PER_SEC) { (c)->sec++; (c)->nsec -= MOO_NSECS_PER_SEC; } \
	} while(0)

#define MOO_ADD_NTIME_SNS(c,a,s,ns) \
	do { \
		(c)->sec = (a)->sec + (s); \
		(c)->nsec = (a)->nsec + (ns); \
		while ((c)->nsec >= MOO_NSECS_PER_SEC) { (c)->sec++; (c)->nsec -= MOO_NSECS_PER_SEC; } \
	} while(0)

#define MOO_SUB_NTIME(c,a,b) \
	do { \
		(c)->sec = (a)->sec - (b)->sec; \
		(c)->nsec = (a)->nsec - (b)->nsec; \
		while ((c)->nsec < 0) { (c)->sec--; (c)->nsec += MOO_NSECS_PER_SEC; } \
	} while(0)

#define MOO_SUB_NTIME_SNS(c,a,s,ns) \
	do { \
		(c)->sec = (a)->sec - s; \
		(c)->nsec = (a)->nsec - ns; \
		while ((c)->nsec < 0) { (c)->sec--; (c)->nsec += MOO_NSECS_PER_SEC; } \
	} while(0)


#define MOO_CMP_NTIME(a,b) (((a)->sec == (b)->sec)? ((a)->nsec - (b)->nsec): ((a)->sec - (b)->sec))

/* if time has been normalized properly, nsec must be equal to or
 * greater than 0. */
#define MOO_IS_NEG_NTIME(x) ((x)->sec < 0)
#define MOO_IS_POS_NTIME(x) ((x)->sec > 0 || ((x)->sec == 0 && (x)->nsec > 0))
#define MOO_IS_ZERO_NTIME(x) ((x)->sec == 0 && (x)->nsec == 0)

/* =========================================================================
 * PRIMITIVE MACROS
 * ========================================================================= */
#define MOO_UCI_EOF ((moo_uci_t)-1)
#define MOO_BCI_EOF ((moo_bci_t)-1)
#define MOO_OOCI_EOF ((moo_ooci_t)-1)

#define MOO_SIZEOF(x) (sizeof(x))
#define MOO_COUNTOF(x) (sizeof(x) / sizeof((x)[0]))
#define MOO_BITSOF(x) (sizeof(x) * MOO_BITS_PER_BYTE)

/**
 * The MOO_OFFSETOF() macro returns the offset of a field from the beginning
 * of a structure.
 */
#define MOO_OFFSETOF(type,member) ((moo_uintptr_t)&((type*)0)->member)

/**
 * The MOO_ALIGNOF() macro returns the alignment size of a structure.
 * Note that this macro may not work reliably depending on the type given.
 */
#define MOO_ALIGNOF(type) MOO_OFFSETOF(struct { moo_uint8_t d1; type d2; }, d2)
        /*(sizeof(struct { moo_uint8_t d1; type d2; }) - sizeof(type))*/

#if defined(__cplusplus)
#	if (__cplusplus >= 201103L) /* C++11 */
#		define MOO_NULL nullptr
#	else
#		define MOO_NULL (0)
#	endif
#else
#	define MOO_NULL ((void*)0)
#endif

/* make a bit mask that can mask off low n bits */
#define MOO_LBMASK(type,n) (~(~((type)0) << (n))) 
#define MOO_LBMASK_SAFE(type,n) (((n) < MOO_BITSOF(type))? MOO_LBMASK(type,n): ~(type)0)

/* make a bit mask that can mask off hig n bits */
#define MOO_HBMASK(type,n) (~(~((type)0) >> (n)))
#define MOO_HBMASK_SAFE(type,n) (((n) < MOO_BITSOF(type))? MOO_HBMASK(type,n): ~(type)0)

/* get 'length' bits starting from the bit at the 'offset' */
#define MOO_GETBITS(type,value,offset,length) \
	((((type)(value)) >> (offset)) & MOO_LBMASK(type,length))

#define MOO_CLEARBITS(type,value,offset,length) \
	(((type)(value)) & ~(MOO_LBMASK(type,length) << (offset)))

#define MOO_SETBITS(type,value,offset,length,bits) \
	(value = (MOO_CLEARBITS(type,value,offset,length) | (((bits) & MOO_LBMASK(type,length)) << (offset))))

#define MOO_FLIPBITS(type,value,offset,length) \
	(((type)(value)) ^ (MOO_LBMASK(type,length) << (offset)))

#define MOO_ORBITS(type,value,offset,length,bits) \
	(value = (((type)(value)) | (((bits) & MOO_LBMASK(type,length)) << (offset))))


/** 
 * The MOO_BITS_MAX() macros calculates the maximum value that the 'nbits'
 * bits of an unsigned integer of the given 'type' can hold.
 * \code
 * printf ("%u", MOO_BITS_MAX(unsigned int, 5));
 * \endcode
 */
/*#define MOO_BITS_MAX(type,nbits) ((((type)1) << (nbits)) - 1)*/
#define MOO_BITS_MAX(type,nbits) ((~(type)0) >> (MOO_BITSOF(type) - (nbits)))

/* =========================================================================
 * MMGR
 * ========================================================================= */
typedef struct moo_mmgr_t moo_mmgr_t;

/** 
 * allocate a memory chunk of the size \a n.
 * \return pointer to a memory chunk on success, #MOO_NULL on failure.
 */
typedef void* (*moo_mmgr_alloc_t)   (moo_mmgr_t* mmgr, moo_oow_t n);
/** 
 * resize a memory chunk pointed to by \a ptr to the size \a n.
 * \return pointer to a memory chunk on success, #MOO_NULL on failure.
 */
typedef void* (*moo_mmgr_realloc_t) (moo_mmgr_t* mmgr, void* ptr, moo_oow_t n);
/**
 * free a memory chunk pointed to by \a ptr.
 */
typedef void  (*moo_mmgr_free_t)    (moo_mmgr_t* mmgr, void* ptr);

/**
 * The moo_mmgr_t type defines the memory management interface.
 * As the type is merely a structure, it is just used as a single container
 * for memory management functions with a pointer to user-defined data. 
 * The user-defined data pointer \a ctx is passed to each memory management 
 * function whenever it is called. You can allocate, reallocate, and free 
 * a memory chunk.
 *
 * For example, a moo_xxx_open() function accepts a pointer of the moo_mmgr_t
 * type and the xxx object uses it to manage dynamic data within the object. 
 */
struct moo_mmgr_t
{
	moo_mmgr_alloc_t   alloc;   /**< allocation function */
	moo_mmgr_realloc_t realloc; /**< resizing function */
	moo_mmgr_free_t    free;    /**< disposal function */
	void*               ctx;     /**< user-defined data pointer */
};

/**
 * The MOO_MMGR_ALLOC() macro allocates a memory block of the \a size bytes
 * using the \a mmgr memory manager.
 */
#define MOO_MMGR_ALLOC(mmgr,size) ((mmgr)->alloc(mmgr,size))

/**
 * The MOO_MMGR_REALLOC() macro resizes a memory block pointed to by \a ptr 
 * to the \a size bytes using the \a mmgr memory manager.
 */
#define MOO_MMGR_REALLOC(mmgr,ptr,size) ((mmgr)->realloc(mmgr,ptr,size))

/** 
 * The MOO_MMGR_FREE() macro deallocates the memory block pointed to by \a ptr.
 */
#define MOO_MMGR_FREE(mmgr,ptr) ((mmgr)->free(mmgr,ptr))


/* =========================================================================
 * CMGR
 * =========================================================================*/

typedef struct moo_cmgr_t moo_cmgr_t;

typedef moo_oow_t (*moo_cmgr_bctouc_t) (
	const moo_bch_t*   mb, 
	moo_oow_t         size,
	moo_uch_t*         wc
);

typedef moo_oow_t (*moo_cmgr_uctobc_t) (
	moo_uch_t    wc,
	moo_bch_t*   mb,
	moo_oow_t   size
);

/**
 * The moo_cmgr_t type defines the character-level interface to 
 * multibyte/wide-character conversion. This interface doesn't 
 * provide any facility to store conversion state in a context
 * independent manner. This leads to the limitation that it can
 * handle a stateless multibyte encoding only.
 */
struct moo_cmgr_t
{
	moo_cmgr_bctouc_t bctouc;
	moo_cmgr_uctobc_t uctobc;
};

/* =========================================================================
 * FORWARD DECLARATION FOR MAIN MOO STRUCTURE
 * =========================================================================*/
typedef struct moo_t moo_t;

/* =========================================================================
 * MACROS THAT CHANGES THE BEHAVIORS OF THE C COMPILER/LINKER
 * =========================================================================*/

#if defined(EMSCRIPTEN)
#	define MOO_IMPORT
#	define MOO_EXPORT EMSCRIPTEN_KEEPALIVE
#	define MOO_PRIVATE
#elif defined(__BORLANDC__) && (__BORLANDC__ < 0x500)
#	define MOO_IMPORT
#	define MOO_EXPORT
#	define MOO_PRIVATE
#elif defined(_WIN32) || (defined(__WATCOMC__) && (__WATCOMC__ >= 1000) && !defined(__WINDOWS_386__))
#	define MOO_IMPORT __declspec(dllimport)
#	define MOO_EXPORT __declspec(dllexport)
#	define MOO_PRIVATE 
#elif defined(__GNUC__) && ((__GNUC__>= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3))
#	define MOO_IMPORT __attribute__((visibility("default")))
#	define MOO_EXPORT __attribute__((visibility("default")))
#	define MOO_PRIVATE __attribute__((visibility("hidden")))
/*#	define MOO_PRIVATE __attribute__((visibility("internal")))*/
#else
#	define MOO_IMPORT
#	define MOO_EXPORT
#	define MOO_PRIVATE
#endif

#if defined(__cplusplus) || (defined(__STDC_VERSION__) && (__STDC_VERSION__>=199901L))
	/* C++/C99 has inline */
#	define MOO_INLINE inline
#	define MOO_HAVE_INLINE
#elif defined(__GNUC__) && defined(__GNUC_GNU_INLINE__)
	/* gcc disables inline when -std=c89 or -ansi is used. 
	 * so use __inline__ supported by gcc regardless of the options */
#	define MOO_INLINE /*extern*/ __inline__
#	define MOO_HAVE_INLINE
#else
#	define MOO_INLINE 
#	undef MOO_HAVE_INLINE
#endif

#if defined(__GNUC__) && (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4))
#	define MOO_UNUSED __attribute__((__unused__))
#else
#	define MOO_UNUSED
#endif

/**
 * The MOO_TYPE_IS_SIGNED() macro determines if a type is signed.
 * \code
 * printf ("%d\n", (int)MOO_TYPE_IS_SIGNED(int));
 * printf ("%d\n", (int)MOO_TYPE_IS_SIGNED(unsigned int));
 * \endcode
 */
#define MOO_TYPE_IS_SIGNED(type) (((type)0) > ((type)-1))

/**
 * The MOO_TYPE_IS_SIGNED() macro determines if a type is unsigned.
 * \code
 * printf ("%d\n", MOO_TYPE_IS_UNSIGNED(int));
 * printf ("%d\n", MOO_TYPE_IS_UNSIGNED(unsigned int));
 * \endcode
 */
#define MOO_TYPE_IS_UNSIGNED(type) (((type)0) < ((type)-1))

#define MOO_TYPE_SIGNED_MAX(type) \
	((type)~((type)1 << ((type)MOO_BITSOF(type) - 1)))
#define MOO_TYPE_UNSIGNED_MAX(type) ((type)(~(type)0))

#define MOO_TYPE_SIGNED_MIN(type) \
	((type)((type)1 << ((type)MOO_BITSOF(type) - 1)))
#define MOO_TYPE_UNSIGNED_MIN(type) ((type)0)

#define MOO_TYPE_MAX(type) \
	((MOO_TYPE_IS_SIGNED(type)? MOO_TYPE_SIGNED_MAX(type): MOO_TYPE_UNSIGNED_MAX(type)))
#define MOO_TYPE_MIN(type) \
	((MOO_TYPE_IS_SIGNED(type)? MOO_TYPE_SIGNED_MIN(type): MOO_TYPE_UNSIGNED_MIN(type)))

/* round up a positive integer x to the nearst multiple of y */
#define MOO_ALIGN(x,y) ((((x) + (y) - 1) / (y)) * (y))

/* round up a positive integer x to the nearst multiple of y where
 * y must be a multiple of a power of 2*/
#define MOO_ALIGN_POW2(x,y) ((((x) + (y) - 1)) & ~((y) - 1))

#define MOO_IS_UNALIGNED_POW2(x,y) ((x) & ((y) - 1))
#define MOO_IS_ALIGNED_POW2(x,y) (!MOO_IS_UNALIGNED_POW2(x,y))

/* =========================================================================
 * COMPILER FEATURE TEST MACROS
 * =========================================================================*/
#if !defined(__has_builtin) && defined(_INTELC32_)
	/* intel c code builder 1.0 ended up with an error without this override */
	#define __has_builtin(x) 0
#endif

/*
#if !defined(__is_identifier)
	#define __is_identifier(x) 0
#endif

#if !defined(__has_attribute)
	#define __has_attribute(x) 0
#endif
*/

#if defined(__has_builtin) 
	#if __has_builtin(__builtin_ctz)
		#define MOO_HAVE_BUILTIN_CTZ
	#endif
	#if __has_builtin(__builtin_ctzl)
		#define MOO_HAVE_BUILTIN_CTZL
	#endif
	#if __has_builtin(__builtin_ctzll)
		#define MOO_HAVE_BUILTIN_CTZLL
	#endif

	#if __has_builtin(__builtin_clz)
		#define MOO_HAVE_BUILTIN_CLZ
	#endif
	#if __has_builtin(__builtin_clzl)
		#define MOO_HAVE_BUILTIN_CLZL
	#endif
	#if __has_builtin(__builtin_clzll)
		#define MOO_HAVE_BUILTIN_CLZLL
	#endif

	#if __has_builtin(__builtin_uadd_overflow)
		#define MOO_HAVE_BUILTIN_UADD_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_uaddl_overflow)
		#define MOO_HAVE_BUILTIN_UADDL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_uaddll_overflow)
		#define MOO_HAVE_BUILTIN_UADDLL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_umul_overflow)
		#define MOO_HAVE_BUILTIN_UMUL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_umull_overflow)
		#define MOO_HAVE_BUILTIN_UMULL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_umulll_overflow)
		#define MOO_HAVE_BUILTIN_UMULLL_OVERFLOW 
	#endif

	#if __has_builtin(__builtin_sadd_overflow)
		#define MOO_HAVE_BUILTIN_SADD_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_saddl_overflow)
		#define MOO_HAVE_BUILTIN_SADDL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_saddll_overflow)
		#define MOO_HAVE_BUILTIN_SADDLL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_smul_overflow)
		#define MOO_HAVE_BUILTIN_SMUL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_smull_overflow)
		#define MOO_HAVE_BUILTIN_SMULL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_smulll_overflow)
		#define MOO_HAVE_BUILTIN_SMULLL_OVERFLOW 
	#endif

	#if __has_builtin(__builtin_expect)
		#define MOO_HAVE_BUILTIN_EXPECT
	#endif


	#if __has_builtin(__sync_lock_test_and_set)
		#define MOO_HAVE_SYNC_LOCK_TEST_AND_SET
	#endif
	#if __has_builtin(__sync_lock_release)
		#define MOO_HAVE_SYNC_LOCK_RELEASE
	#endif

	#if __has_builtin(__sync_synchronize)
		#define MOO_HAVE_SYNC_SYNCHRONIZE
	#endif
	#if __has_builtin(__sync_bool_compare_and_swap)
		#define MOO_HAVE_SYNC_BOOL_COMPARE_AND_SWAP
	#endif
	#if __has_builtin(__sync_val_compare_and_swap)
		#define MOO_HAVE_SYNC_VAL_COMPARE_AND_SWAP
	#endif


	#if __has_builtin(__builtin_bswap16)
		#define MOO_HAVE_BUILTIN_BSWAP16
	#endif
	#if __has_builtin(__builtin_bswap32)
		#define MOO_HAVE_BUILTIN_BSWAP32
	#endif
	#if __has_builtin(__builtin_bswap64)
		#define MOO_HAVE_BUILTIN_BSWAP64
	#endif
	#if __has_builtin(__builtin_bswap128)
		#define MOO_HAVE_BUILTIN_BSWAP128
	#endif
#elif defined(__GNUC__) && defined(__GNUC_MINOR__)

	#if (__GNUC__ >= 4) 
		#define MOO_HAVE_SYNC_LOCK_TEST_AND_SET
		#define MOO_HAVE_SYNC_LOCK_RELEASE

		#define MOO_HAVE_SYNC_SYNCHRONIZE
		#define MOO_HAVE_SYNC_BOOL_COMPARE_AND_SWAP
		#define MOO_HAVE_SYNC_VAL_COMPARE_AND_SWAP
	#endif

	#if (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
		#define MOO_HAVE_BUILTIN_CTZ
		#define MOO_HAVE_BUILTIN_CTZL
		#define MOO_HAVE_BUILTIN_CTZLL
		#define MOO_HAVE_BUILTIN_CLZ
		#define MOO_HAVE_BUILTIN_CLZL
		#define MOO_HAVE_BUILTIN_CLZLL
		#define MOO_HAVE_BUILTIN_EXPECT
	#endif

	#if (__GNUC__ >= 5)
		#define MOO_HAVE_BUILTIN_UADD_OVERFLOW
		#define MOO_HAVE_BUILTIN_UADDL_OVERFLOW
		#define MOO_HAVE_BUILTIN_UADDLL_OVERFLOW
		#define MOO_HAVE_BUILTIN_UMUL_OVERFLOW
		#define MOO_HAVE_BUILTIN_UMULL_OVERFLOW
		#define MOO_HAVE_BUILTIN_UMULLL_OVERFLOW

		#define MOO_HAVE_BUILTIN_SADD_OVERFLOW
		#define MOO_HAVE_BUILTIN_SADDL_OVERFLOW
		#define MOO_HAVE_BUILTIN_SADDLL_OVERFLOW
		#define MOO_HAVE_BUILTIN_SMUL_OVERFLOW
		#define MOO_HAVE_BUILTIN_SMULL_OVERFLOW
		#define MOO_HAVE_BUILTIN_SMULLL_OVERFLOW
	#endif

	#if (__GNUC__ >= 5) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
		/* 4.8.0 or later */
		#define MOO_HAVE_BUILTIN_BSWAP16
	#endif
	#if (__GNUC__ >= 5) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
		/* 4.3.0 or later */
		#define MOO_HAVE_BUILTIN_BSWAP32
		#define MOO_HAVE_BUILTIN_BSWAP64
		/*#define MOO_HAVE_BUILTIN_BSWAP128*/
	#endif
#endif

#if defined(MOO_HAVE_BUILTIN_EXPECT)
#	define MOO_LIKELY(x) (__builtin_expect(!!(x),1))
#	define MOO_UNLIKELY(x) (__builtin_expect(!!(x),0))
#else
#	define MOO_LIKELY(x) (x)
#	define MOO_UNLIKELY(x) (x)
#endif


/* =========================================================================
 * STATIC ASSERTION
 * =========================================================================*/
#define MOO_STATIC_JOIN_INNER(x, y) x ## y
#define MOO_STATIC_JOIN(x, y) MOO_STATIC_JOIN_INNER(x, y)

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#	define MOO_STATIC_ASSERT(expr)  _Static_assert (expr, "invalid assertion")
#elif defined(__cplusplus) && (__cplusplus >= 201103L)
#	define MOO_STATIC_ASSERT(expr) static_assert (expr, "invalid assertion")
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#	define MOO_STATIC_ASSERT(expr) typedef char MOO_STATIC_JOIN(MOO_STATIC_ASSERT_T_, __LINE__)[(expr)? 1: -1] MOO_UNUSED
#else
#	define MOO_STATIC_ASSERT(expr) do { typedef char MOO_STATIC_JOIN(MOO_STATIC_ASSERT_T_, __LINE__)[(expr)? 1: -1] MOO_UNUSED; } while(0)
#endif

#define MOO_STATIC_ASSERT_EXPR(expr) ((void)MOO_SIZEOF(char[(expr)? 1: -1]))

#endif
