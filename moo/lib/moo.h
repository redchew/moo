/*
 * $Id$
 *
    Copyright (c) 2014-2017 Chung, Hyung-Hwan. All rights reserved.

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

#ifndef _MOO_H_
#define _MOO_H_

#include "moo-cmn.h"
#include "moo-rbt.h"

/* TODO: move this macro out to the build files.... */
#define MOO_INCLUDE_COMPILER

/* define this to allow an pointer(OOP) object to have trailing bytes 
 * this is used to embed bytes codes into the back of a compile method
 * object instead of putting in in a separate byte array. */
#define MOO_USE_METHOD_TRAILER

/* ========================================================================== */

/**
 * The moo_errnum_t type defines the error codes.
 */
enum moo_errnum_t
{
	MOO_ENOERR,   /**< no error */
	MOO_EGENERIC, /**< generic error */

	MOO_ENOIMPL,  /**< not implemented */
	MOO_ESYSERR,  /**< system error */
	MOO_EINTERN,  /**< internal error */
	MOO_ESYSMEM,  /**< insufficient system memory */
	MOO_EOOMEM,   /**< insufficient object memory */
	MOO_ETYPE,    /**< invalid class/type */

	MOO_EINVAL,   /**< invalid parameter or data */
	MOO_ENOENT,   /**< data not found */
	MOO_EEXIST,   /**< existing/duplicate data */
	MOO_EBUSY, 
	MOO_EACCES,
	MOO_EPERM,
	MOO_ENOTDIR,
	MOO_EINTR,
	MOO_EPIPE,
	MOO_EAGAIN,
	MOO_EBADHND,

	MOO_ENOAVAIL, /**< data not available*/
	MOO_EMSGRCV,  /**< mesasge receiver error */
	MOO_EMSGSND,  /**< message sending error. even doesNotUnderstand: is not found */
	MOO_ENUMARGS, /**< wrong number of arguments */
	MOO_ERANGE,   /**< range error. overflow and underflow */
	MOO_EBCFULL,  /**< byte-code full */
	MOO_EDFULL,   /**< dictionary full */
	MOO_EPFULL,   /**< processor full */
	MOO_ESHFULL,  /**< semaphore heap full */
	MOO_ESLFULL,  /**< semaphore list full */
	MOO_EDIVBY0,  /**< divide by zero */
	MOO_EIOERR,   /**< I/O error */
	MOO_EECERR,   /**< encoding conversion error */
	MOO_EBUFFULL, /**< buffer full */

#if defined(MOO_INCLUDE_COMPILER)
	MOO_ESYNERR  /** < syntax error */
#endif
};
typedef enum moo_errnum_t moo_errnum_t;

enum moo_option_t
{
	MOO_TRAIT,
	MOO_LOG_MASK,
	MOO_LOG_MAXCAPA,
	MOO_SYMTAB_SIZE,  /* default system table size */
	MOO_SYSDIC_SIZE,  /* default system dictionary size */
	MOO_PROCSTK_SIZE  /* default process stack size */
};
typedef enum moo_option_t moo_option_t;

/* [NOTE] ensure that it is a power of 2 */
#define MOO_LOG_CAPA_ALIGN 512

enum moo_option_dflval_t
{
	MOO_DFL_LOG_MAXCAPA = MOO_LOG_CAPA_ALIGN * 16,
	MOO_DFL_SYMTAB_SIZE = 5000,
	MOO_DFL_SYSDIC_SIZE = 5000,
	MOO_DFL_PROCSTK_SIZE = 5000
};
typedef enum moo_option_dflval_t moo_option_dflval_t;

enum moo_trait_t
{
#if !defined(NDEBUG)
	MOO_DEBUG_GC     = (1 << 0),
	MOO_DEBUG_BIGINT = (1 << 1),
#endif

	/* perform no garbage collection when the heap is full. 
	 * you still can use moo_gc() explicitly. */
	MOO_NOGC = (1 << 8),

	/* wait for running process when exiting from the main method */
	MOO_AWAIT_PROCS = (1 << 9)
};
typedef enum moo_trait_t moo_trait_t;

typedef struct moo_obj_t           moo_obj_t;
typedef struct moo_obj_t*          moo_oop_t;

/* these are more specialized types for moo_obj_t */
typedef struct moo_obj_oop_t       moo_obj_oop_t;
typedef struct moo_obj_char_t      moo_obj_char_t;
typedef struct moo_obj_byte_t      moo_obj_byte_t;
typedef struct moo_obj_halfword_t  moo_obj_halfword_t;
typedef struct moo_obj_word_t      moo_obj_word_t;

/* these are more specialized types for moo_oop_t */
typedef struct moo_obj_oop_t*      moo_oop_oop_t;
typedef struct moo_obj_char_t*     moo_oop_char_t;
typedef struct moo_obj_byte_t*     moo_oop_byte_t;
typedef struct moo_obj_halfword_t* moo_oop_halfword_t;
typedef struct moo_obj_word_t*     moo_oop_word_t;

#define MOO_OOW_BITS  (MOO_SIZEOF_OOW_T * 8)
#define MOO_OOI_BITS  (MOO_SIZEOF_OOI_T * 8)
#define MOO_OOP_BITS  (MOO_SIZEOF_OOP_T * 8)
#define MOO_OOHW_BITS (MOO_SIZEOF_OOHW_T * 8)


/* ========================================================================= */
/* BIGINT TYPES AND MACROS                                                   */
/* ========================================================================= */
#if (MOO_SIZEOF_UINTMAX_T > MOO_SIZEOF_OOW_T)
#	define MOO_USE_FULL_WORD
#endif

#if defined(MOO_USE_FULL_WORD)
	typedef moo_oow_t          moo_liw_t; /* large integer word */
	typedef moo_ooi_t          moo_lii_t;
	typedef moo_uintmax_t      moo_lidw_t; /* large integer double word */
	typedef moo_intmax_t       moo_lidi_t;
#	define MOO_SIZEOF_LIW_T    MOO_SIZEOF_OOW_T
#	define MOO_SIZEOF_LIDW_T   MOO_SIZEOF_UINTMAX_T
#	define MOO_LIW_BITS        MOO_OOW_BITS
#	define MOO_LIDW_BITS       (MOO_SIZEOF_UINTMAX_T * 8) 

	typedef moo_oop_word_t     moo_oop_liword_t;
#	define MOO_OBJ_TYPE_LIWORD MOO_OBJ_TYPE_WORD

#else
	typedef moo_oohw_t         moo_liw_t;
	typedef moo_oohi_t         moo_lii_t;
	typedef moo_oow_t          moo_lidw_t;
	typedef moo_ooi_t          moo_lidi_t;
#	define MOO_SIZEOF_LIW_T    MOO_SIZEOF_OOHW_T
#	define MOO_SIZEOF_LIDW_T   MOO_SIZEOF_OOW_T
#	define MOO_LIW_BITS        MOO_OOHW_BITS
#	define MOO_LIDW_BITS       MOO_OOW_BITS

	typedef moo_oop_halfword_t moo_oop_liword_t;
#	define MOO_OBJ_TYPE_LIWORD MOO_OBJ_TYPE_HALFWORD

#endif

enum moo_method_type_t
{
	MOO_METHOD_INSTANCE = 0,
	MOO_METHOD_CLASS    = 1,
	MOO_METHOD_DUAL     = 2
};
typedef enum moo_method_type_t moo_method_type_t;

/* 
 * OOP encoding
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
#define MOO_OOP_TAG_HANDLE      11   /* 1011 */
#define MOO_OOP_TAG_RESERVED1   15   /* 1111 */

#define MOO_OOP_GET_TAG_LO(oop) (((moo_oow_t)oop) & MOO_LBMASK(moo_oow_t, MOO_OOP_TAG_BITS_LO))
#define MOO_OOP_GET_TAG_LOHI(oop) (((moo_oow_t)oop) & MOO_LBMASK(moo_oow_t, MOO_OOP_TAG_BITS_LO + MOO_OOP_TAG_BITS_HI))
#define MOO_OOP_GET_TAG(oop) (MOO_OOP_GET_TAG_LO(oop) == MOO_OOP_TAG_EXTENDED? MOO_OOP_GET_TAG_LOHI(oop): MOO_OOP_GET_TAG_LO(oop))

#define MOO_OOP_IS_NUMERIC(oop) (MOO_OOP_GET_TAG_LO(oop) != 0)
#define MOO_OOP_IS_POINTER(oop) (MOO_OOP_GET_TAG_LO(oop) == 0)

#define MOO_OOP_IS_SMOOI(oop) (MOO_OOP_GET_TAG_LO(oop) == MOO_OOP_TAG_SMOOI)
#define MOO_OOP_IS_SMPTR(oop) (MOO_OOP_GET_TAG_LO(oop) == MOO_OOP_TAG_SMPTR)

#define MOO_SMOOI_TO_OOP(num) ((moo_oop_t)((((moo_ooi_t)(num)) << MOO_OOP_TAG_BITS_LO) | MOO_OOP_TAG_SMOOI))
#define MOO_OOP_TO_SMOOI(oop) (((moo_ooi_t)oop) >> MOO_OOP_TAG_BITS_LO)
/*
#define MOO_SMPTR_TO_OOP(num) ((moo_oop_t)((((moo_ooi_t)(num)) << MOO_OOP_TAG_BITS_LO) | MOO_OOP_TAG_SMPTR))
#define MOO_OOP_TO_SMPTR(oop) (((moo_ooi_t)oop) >> MOO_OOP_TAG_BITS_LO)
*/
#define MOO_SMPTR_TO_OOP(ptr) ((moo_oop_t)(((moo_oow_t)ptr) | MOO_OOP_TAG_SMPTR))
#define MOO_OOP_TO_SMPTR(oop) ((void*)(((moo_oow_t)oop) & ~MOO_LBMASK(moo_oow_t, MOO_OOP_TAG_BITS_LO)))


#define MOO_OOP_IS_CHAR(oop) (MOO_OOP_GET_TAG(oop) == MOO_OOP_TAG_CHAR)
#define MOO_OOP_IS_ERROR(oop) (MOO_OOP_GET_TAG(oop) == MOO_OOP_TAG_ERROR)
#define MOO_OOP_IS_HANDLE(oop) (MOO_OOP_GET_TAG(oop) == MOO_OOP_TAG_HANDLE)

#define MOO_OOP_TO_CHAR(oop) (((moo_oow_t)oop) >> (MOO_OOP_TAG_BITS_LO + MOO_OOP_TAG_BITS_LO))
#define MOO_CHAR_TO_OOP(num) ((moo_oop_t)((((moo_oow_t)(num)) << (MOO_OOP_TAG_BITS_LO + MOO_OOP_TAG_BITS_LO)) | MOO_OOP_TAG_CHAR))
#define MOO_OOP_TO_ERROR(oop) (((moo_oow_t)oop) >> (MOO_OOP_TAG_BITS_LO + MOO_OOP_TAG_BITS_LO))
#define MOO_ERROR_TO_OOP(num) ((moo_oop_t)((((moo_oow_t)(num)) << (MOO_OOP_TAG_BITS_LO + MOO_OOP_TAG_BITS_LO)) | MOO_OOP_TAG_ERROR))
#define MOO_OOP_TO_HANDLE(oop) (((moo_oow_t)oop) >> (MOO_OOP_TAG_BITS_LO + MOO_OOP_TAG_BITS_LO))
#define MOO_HANDLE_TO_OOP(num) ((moo_oop_t)((((moo_oow_t)(num)) << (MOO_OOP_TAG_BITS_LO + MOO_OOP_TAG_BITS_LO)) | MOO_OOP_TAG_ERROR))

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

/*
 * Object structure
 */
enum moo_obj_type_t
{
	MOO_OBJ_TYPE_OOP,
	MOO_OBJ_TYPE_CHAR,
	MOO_OBJ_TYPE_BYTE,
	MOO_OBJ_TYPE_HALFWORD,
	MOO_OBJ_TYPE_WORD

/*
	MOO_OBJ_TYPE_UINT8,
	MOO_OBJ_TYPE_UINT16,
	MOO_OBJ_TYPE_UINT32,
*/

/* NOTE: you can have MOO_OBJ_SHORT, MOO_OBJ_INT
 * MOO_OBJ_LONG, MOO_OBJ_FLOAT, MOO_OBJ_DOUBLE, etc 
 * type type field being 6 bits long, you can have up to 64 different types.

	MOO_OBJ_TYPE_SHORT,
	MOO_OBJ_TYPE_INT,
	MOO_OBJ_TYPE_LONG,
	MOO_OBJ_TYPE_FLOAT,
	MOO_OBJ_TYPE_DOUBLE */
};
typedef enum moo_obj_type_t moo_obj_type_t;

enum moo_gcfin_t
{
	MOO_GCFIN_FINALIZABLE = (1 << 0),
	MOO_GCFIN_FINALIZED   = (1 << 1),
	MOO_GCFIN_RESERVED_0  = (1 << 2),
	MOO_GCFIN_RESERVED_1  = (1 << 3)
};
typedef enum moo_gcfin_t moo_gcfin_t;

/* =========================================================================
 * Object header structure 
 * 
 * _flags:
 *   type: the type of a payload item. 
 *         one of MOO_OBJ_TYPE_OOP, MOO_OBJ_TYPE_CHAR, 
 *                MOO_OBJ_TYPE_BYTE, MOO_OBJ_TYPE_HALFWORD, MOO_OBJ_TYPE_WORD
 *   unit: the size of a payload item in bytes. 
 *   extra: 0 or 1. 1 indicates that the payload contains 1 more
 *          item than the value of the size field. used for a 
 *          terminating null in a variable-character object. internel
 *          use only.
 *   kernel: 0, 1, or 2. indicates that the object is a kernel object.
 *           VM disallows layout changes of a kernel object.
 *           internal use only. during ignition, it's set to 1.
 *           when full definition is available, it's set to 2.
 *   moved: 0 or 1. used by GC. internal use only.
 *   ngc: 0 or 1, used by GC. internal use only.
 *   rdonly: 0 or 1. indicates that an object is immutable.
 *   gcfin: represents finalzation state.
 *   trailer: 0 or 1. indicates that there are trailing bytes
 *            after the object payload. internal use only.
 *
 * _size: the number of payload items in an object.
 *        it doesn't include the header size.
 * 
 * The total number of bytes occupied by an object can be calculated
 * with this fomula:
 *    sizeof(moo_obj_t) + ALIGN((size + extra) * unit), sizeof(moo_oop_t))
 * 
 * If the type is known to be not MOO_OBJ_TYPE_CHAR, you can assume that 
 * 'extra' is 0. So you can simplify the fomula in such a context.
 *    sizeof(moo_obj_t) + ALIGN(size * unit), sizeof(moo_oop_t))
 *
 * The ALIGN() macro is used above since allocation adjusts the payload
 * size to a multiple of sizeof(moo_oop_t). it assumes that sizeof(moo_obj_t)
 * is a multiple of sizeof(moo_oop_t). See the MOO_BYTESOF() macro.
 * 
 * Due to the header structure, an object can only contain items of
 * homogeneous data types in the payload. 
 *
 * It's actually possible to split the size field into 2. For example,
 * the upper half contains the number of oops and the lower half contains
 * the number of bytes/chars. This way, a variable-byte class or a variable-char
 * class can have normal instance variables. On the contrary, the actual byte
 * size calculation and the access to the payload fields become more complex. 
 * Therefore, i've dropped the idea.
 * ========================================================================= */
#define MOO_OBJ_FLAGS_TYPE_BITS     6
#define MOO_OBJ_FLAGS_UNIT_BITS     5
#define MOO_OBJ_FLAGS_EXTRA_BITS    1
#define MOO_OBJ_FLAGS_KERNEL_BITS   2
#define MOO_OBJ_FLAGS_MOVED_BITS    1
#define MOO_OBJ_FLAGS_NGC_BITS      1
#define MOO_OBJ_FLAGS_RDONLY_BITS   1
#define MOO_OBJ_FLAGS_GCFIN_BITS    4
#define MOO_OBJ_FLAGS_TRAILER_BITS  1

#define MOO_OBJ_FLAGS_TYPE_SHIFT    (MOO_OBJ_FLAGS_UNIT_BITS    + MOO_OBJ_FLAGS_UNIT_SHIFT)
#define MOO_OBJ_FLAGS_UNIT_SHIFT    (MOO_OBJ_FLAGS_EXTRA_BITS   + MOO_OBJ_FLAGS_EXTRA_SHIFT)
#define MOO_OBJ_FLAGS_EXTRA_SHIFT   (MOO_OBJ_FLAGS_KERNEL_BITS  + MOO_OBJ_FLAGS_KERNEL_SHIFT)
#define MOO_OBJ_FLAGS_KERNEL_SHIFT  (MOO_OBJ_FLAGS_MOVED_BITS   + MOO_OBJ_FLAGS_MOVED_SHIFT)
#define MOO_OBJ_FLAGS_MOVED_SHIFT   (MOO_OBJ_FLAGS_NGC_BITS     + MOO_OBJ_FLAGS_NGC_SHIFT)
#define MOO_OBJ_FLAGS_NGC_SHIFT     (MOO_OBJ_FLAGS_RDONLY_BITS  + MOO_OBJ_FLAGS_RDONLY_SHIFT)
#define MOO_OBJ_FLAGS_RDONLY_SHIFT  (MOO_OBJ_FLAGS_GCFIN_BITS   + MOO_OBJ_FLAGS_GCFIN_SHIFT)
#define MOO_OBJ_FLAGS_GCFIN_SHIFT   (MOO_OBJ_FLAGS_TRAILER_BITS + MOO_OBJ_FLAGS_TRAILER_SHIFT)
#define MOO_OBJ_FLAGS_TRAILER_SHIFT (0)

#define MOO_OBJ_GET_FLAGS_TYPE(oop)       MOO_GETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_TYPE_SHIFT,    MOO_OBJ_FLAGS_TYPE_BITS)
#define MOO_OBJ_GET_FLAGS_UNIT(oop)       MOO_GETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_UNIT_SHIFT,    MOO_OBJ_FLAGS_UNIT_BITS)
#define MOO_OBJ_GET_FLAGS_EXTRA(oop)      MOO_GETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_EXTRA_SHIFT,   MOO_OBJ_FLAGS_EXTRA_BITS)
#define MOO_OBJ_GET_FLAGS_KERNEL(oop)     MOO_GETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_KERNEL_SHIFT,  MOO_OBJ_FLAGS_KERNEL_BITS)
#define MOO_OBJ_GET_FLAGS_MOVED(oop)      MOO_GETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_MOVED_SHIFT,   MOO_OBJ_FLAGS_MOVED_BITS)
#define MOO_OBJ_GET_FLAGS_NGC(oop)        MOO_GETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_NGC_SHIFT,     MOO_OBJ_FLAGS_NGC_BITS)
#define MOO_OBJ_GET_FLAGS_RDONLY(oop)     MOO_GETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_RDONLY_SHIFT,  MOO_OBJ_FLAGS_RDONLY_BITS)
#define MOO_OBJ_GET_FLAGS_GCFIN(oop)      MOO_GETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_GCFIN_SHIFT,   MOO_OBJ_FLAGS_GCFIN_BITS)
#define MOO_OBJ_GET_FLAGS_TRAILER(oop)    MOO_GETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_TRAILER_SHIFT, MOO_OBJ_FLAGS_TRAILER_BITS)

#define MOO_OBJ_SET_FLAGS_TYPE(oop,v)     MOO_SETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_TYPE_SHIFT,    MOO_OBJ_FLAGS_TYPE_BITS,     v)
#define MOO_OBJ_SET_FLAGS_UNIT(oop,v)     MOO_SETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_UNIT_SHIFT,    MOO_OBJ_FLAGS_UNIT_BITS,     v)
#define MOO_OBJ_SET_FLAGS_EXTRA(oop,v)    MOO_SETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_EXTRA_SHIFT,   MOO_OBJ_FLAGS_EXTRA_BITS,    v)
#define MOO_OBJ_SET_FLAGS_KERNEL(oop,v)   MOO_SETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_KERNEL_SHIFT,  MOO_OBJ_FLAGS_KERNEL_BITS,   v)
#define MOO_OBJ_SET_FLAGS_MOVED(oop,v)    MOO_SETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_MOVED_SHIFT,   MOO_OBJ_FLAGS_MOVED_BITS,    v)
#define MOO_OBJ_SET_FLAGS_NGC(oop,v)      MOO_SETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_NGC_SHIFT,     MOO_OBJ_FLAGS_NGC_BITS,      v)
#define MOO_OBJ_SET_FLAGS_RDONLY(oop,v)   MOO_SETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_RDONLY_SHIFT,  MOO_OBJ_FLAGS_RDONLY_BITS,   v)
#define MOO_OBJ_SET_FLAGS_GCFIN(oop,v)    MOO_SETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_GCFIN_SHIFT,   MOO_OBJ_FLAGS_GCFIN_BITS,    v)
#define MOO_OBJ_SET_FLAGS_TRAILER(oop,v)  MOO_SETBITS(moo_oow_t, (oop)->_flags, MOO_OBJ_FLAGS_TRAILER_SHIFT, MOO_OBJ_FLAGS_TRAILER_BITS,  v)

#define MOO_OBJ_GET_SIZE(oop) ((oop)->_size)
#define MOO_OBJ_GET_CLASS(oop) ((moo_oop_class_t)((oop)->_class))

#define MOO_OBJ_SET_SIZE(oop,v) ((oop)->_size = (v))
#define MOO_OBJ_SET_CLASS(oop,c) ((oop)->_class = (moo_oop_t)(c))

/* [NOTE] this macro doesn't include the size of the trailer */
#define MOO_OBJ_BYTESOF(oop) ((MOO_OBJ_GET_SIZE(oop) + MOO_OBJ_GET_FLAGS_EXTRA(oop)) * MOO_OBJ_GET_FLAGS_UNIT(oop))

#define MOO_OBJ_IS_OOP_POINTER(oop)     (MOO_OOP_IS_POINTER(oop) && (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_OOP))
#define MOO_OBJ_IS_CHAR_POINTER(oop)     (MOO_OOP_IS_POINTER(oop) && (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_CHAR))
#define MOO_OBJ_IS_BYTE_POINTER(oop)     (MOO_OOP_IS_POINTER(oop) && (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_BYTE))
#define MOO_OBJ_IS_HALFWORD_POINTER(oop) (MOO_OOP_IS_POINTER(oop) && (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_HALFWORD))
#define MOO_OBJ_IS_WORD_POINTER(oop)     (MOO_OOP_IS_POINTER(oop) && (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_WORD))

/* [NOTE] this macro doesn't check the range of the actual value.
 *        make sure that the value of each bit fields given falls within the 
 *        possible range of the defined bits */
#define MOO_OBJ_MAKE_FLAGS(t,u,e,k,m,g,r) ( \
	(((moo_oow_t)(t)) << MOO_OBJ_FLAGS_TYPE_SHIFT) | \
	(((moo_oow_t)(u)) << MOO_OBJ_FLAGS_UNIT_SHIFT) | \
	(((moo_oow_t)(e)) << MOO_OBJ_FLAGS_EXTRA_SHIFT) | \
	(((moo_oow_t)(k)) << MOO_OBJ_FLAGS_KERNEL_SHIFT) | \
	(((moo_oow_t)(m)) << MOO_OBJ_FLAGS_MOVED_SHIFT) | \
	(((moo_oow_t)(g)) << MOO_OBJ_FLAGS_NGC_SHIFT) | \
	(((moo_oow_t)(r)) << MOO_OBJ_FLAGS_TRAILER_SHIFT) \
)

#define MOO_OBJ_HEADER \
	moo_oow_t _flags; \
	moo_oow_t _size; \
	moo_oop_t _class

struct moo_obj_t
{
	MOO_OBJ_HEADER;
};

struct moo_obj_oop_t
{
	MOO_OBJ_HEADER;
	moo_oop_t slot[1];
};

struct moo_obj_char_t
{
	MOO_OBJ_HEADER;
	moo_ooch_t slot[1];
};

struct moo_obj_byte_t
{
	MOO_OBJ_HEADER;
	moo_oob_t slot[1];
};

struct moo_obj_halfword_t
{
	MOO_OBJ_HEADER;
	moo_oohw_t slot[1];
};

struct moo_obj_word_t
{
	MOO_OBJ_HEADER;
	moo_oow_t slot[1];
};

#define MOO_OBJ_GET_OOP_SLOT(oop)      (((moo_oop_oop_t)(oop))->slot)
#define MOO_OBJ_GET_CHAR_SLOT(oop)     (((moo_oop_char_t)(oop))->slot)
#define MOO_OBJ_GET_BYTE_SLOT(oop)     (((moo_oop_byte_t)(oop))->slot)
#define MOO_OBJ_GET_HALFWORD_SLOT(oop) (((moo_oop_halfword_t)(oop))->slot)
#define MOO_OBJ_GET_WORD_SLOT(oop)     (((moo_oop_word_t)(oop))->slot)

typedef struct moo_trailer_t moo_trailer_t;
struct moo_trailer_t
{
	moo_oow_t size;
	moo_oob_t slot[1];
};

#define MOO_OBJ_GET_TRAILER_BYTE(oop) ((moo_oob_t*)&((moo_oop_oop_t)oop)->slot[MOO_OBJ_GET_SIZE(oop) + 1])
#define MOO_OBJ_GET_TRAILER_SIZE(oop) ((moo_oow_t)((moo_oop_oop_t)oop)->slot[MOO_OBJ_GET_SIZE(oop)])

#define MOO_DIC_NAMED_INSTVARS 2
typedef struct moo_dic_t moo_dic_t;
typedef struct moo_dic_t* moo_oop_dic_t;
struct moo_dic_t
{
	MOO_OBJ_HEADER;
	moo_oop_t     tally;  /* SmallInteger */
	moo_oop_oop_t bucket; /* Array */
};

/* [NOTE] the beginning of the moo_nsdic_t structure
 *        must be identical to the moo_dic_t streucture */
#define MOO_NSDIC_NAMED_INSTVARS 4
typedef struct moo_nsdic_t moo_nsdic_t;
typedef struct moo_nsdic_t* moo_oop_nsdic_t;

#define MOO_CLASS_NAMED_INSTVARS 18
typedef struct moo_class_t moo_class_t;
typedef struct moo_class_t* moo_oop_class_t;

struct moo_nsdic_t
{
	MOO_OBJ_HEADER;
	moo_oop_t tally;
	moo_oop_t bucket;
	/* same as moo_dic_t so far */

	moo_oop_char_t name; /* a symbol. if it belongs to a class, it will be the same as the name of the class */
	moo_oop_t nsup; /* a class if it belongs to the class. another nsdic if it doesn't */
};

struct moo_class_t
{
	MOO_OBJ_HEADER;

	moo_oop_t      spec;          /* SmallInteger. instance specification */
	moo_oop_t      selfspec;      /* SmallInteger. specification of the class object itself */

	moo_oop_t      superclass;    /* Another class */
	moo_oop_t      subclasses;    /* Array of subclasses */

	moo_oop_char_t name;          /* Symbol */
	moo_oop_t      modname;       /* Symbol if importing a module. nil if not. */

	/* == NEVER CHANGE THIS ORDER OF 3 ITEMS BELOW == */
	moo_oop_char_t instvars;      /* String */
	moo_oop_char_t classinstvars; /* String */
	moo_oop_char_t classvars;     /* String */
	/* == NEVER CHANGE THE ORDER OF 3 ITEMS ABOVE == */

	moo_oop_char_t pooldics;      /* String - pool dictionaries imported */

	/* [0] - instance methods, MethodDictionary
	 * [1] - class methods, MethodDictionary */
	moo_oop_dic_t  mthdic[2];      

	moo_oop_nsdic_t nsup; /* pointer to the upper namespace */
	moo_oop_nsdic_t nsdic; /* dictionary used for namespacing - may be nil when there are no subitems underneath */

	moo_oop_t      trsize; /* trailer size for new instances */
	moo_oop_t      trgc; /* trailer gc callback */

	/* [0] - initial values for instance variables of new instances 
	 * [1] - initial values for class instance variables */
	moo_oop_t      initv[2]; 

	/* indexed part afterwards */
	moo_oop_t      slot[1];   /* class instance variables and class variables. */
};

#define MOO_ASSOCIATION_NAMED_INSTVARS 2
typedef struct moo_association_t moo_association_t;
typedef struct moo_association_t* moo_oop_association_t;
struct moo_association_t
{
	MOO_OBJ_HEADER;
	moo_oop_t key;
	moo_oop_t value;
};

#if defined(MOO_USE_METHOD_TRAILER)
#	define MOO_METHOD_NAMED_INSTVARS 8
#else
#	define MOO_METHOD_NAMED_INSTVARS 9
#endif
typedef struct moo_method_t moo_method_t;
typedef struct moo_method_t* moo_oop_method_t;
struct moo_method_t
{
	MOO_OBJ_HEADER;

	moo_oop_class_t owner; /* Class */

	moo_oop_char_t  name; /* Symbol, method name */

	/* primitive number */
	moo_oop_t       preamble; /* SmallInteger */
	moo_oop_t       preamble_data[2]; /* SmallInteger */

	/* number of temporaries including arguments */
	moo_oop_t       tmpr_count; /* SmallInteger */

	/* number of arguments in temporaries */
	moo_oop_t       tmpr_nargs; /* SmallInteger */

#if defined(MOO_USE_METHOD_TRAILER)
	/* no code field is used */
#else
	moo_oop_byte_t  code; /* ByteArray */
#endif

	moo_oop_t       source; /* TODO: what should I put? */

	/* == variable indexed part == */
	moo_oop_t       slot[1]; /* it stores literals */
};

#if defined(MOO_USE_METHOD_TRAILER)

	/* if m is to be type-cast to moo_oop_method_t, the macro must be
	 * redefined to this:
	 *   (&((moo_oop_method_t)m)>slot[MOO_OBJ_GET_SIZE(m) + 1 - MOO_METHOD_NAMED_INSTVARS])
	 */

	/*((moo_oob_t*)&((moo_oop_oop_t)m)->slot[MOO_OBJ_GET_SIZE(m) + 1])*/
#	define MOO_METHOD_GET_CODE_BYTE(m) MOO_OBJ_GET_TRAILER_BYTE(m)
	/*((moo_oow_t)((moo_oop_oop_t)m)->slot[MOO_OBJ_GET_SIZE(m)])*/
#	define MOO_METHOD_GET_CODE_SIZE(m) MOO_OBJ_GET_TRAILER_SIZE(m) 
#else
#	define MOO_METHOD_GET_CODE_BYTE(m) ((m)->code->slot)
#	define MOO_METHOD_GET_CODE_SIZE(m) MOO_OBJ_GET_SIZE((m)->code)
#endif

/* The preamble field is composed of:
 *    4-bit flag
 *    5-bit code  (0 to 31)
 *    16-bit index
 *
 * The code can be one of the following values:
 *  0 - no special action
 *  1 - return self (receiver)
 *  2 - return thisContext (not used)
 *  3 - return thisProcess
 *  4 - return nil
 *  5 - return true
 *  6 - return false 
 *  7 - return index.
 *  8 - return -index.
 *  9 - return selfns
 * 10 - return instvar[index] 
 * 11 - do primitive[index]
 * 12 - do named primitive[index]
 * 13 - exception handler
 * 14 - ensure block
 */

/* [NOTE] changing preamble code bit structure requires changes to CompiledMethod>>preambleCode */
#define MOO_METHOD_MAKE_PREAMBLE(code,index,flags)  ((((moo_ooi_t)index) << 9) | ((moo_ooi_t)code << 4) | flags)
#define MOO_METHOD_GET_PREAMBLE_CODE(preamble) ((((moo_ooi_t)preamble) >> 4) & 0x1F)
#define MOO_METHOD_GET_PREAMBLE_INDEX(preamble) (((moo_ooi_t)preamble) >> 9)
#define MOO_METHOD_GET_PREAMBLE_FLAGS(preamble) (((moo_ooi_t)preamble) & 0xF)

/* preamble codes */
#define MOO_METHOD_PREAMBLE_NONE                0
#define MOO_METHOD_PREAMBLE_RETURN_RECEIVER     1
#define MOO_METHOD_PREAMBLE_RETURN_CONTEXT      2
#define MOO_METHOD_PREAMBLE_RETURN_PROCESS      3
#define MOO_METHOD_PREAMBLE_RETURN_RECEIVER_NS  4
#define MOO_METHOD_PREAMBLE_RETURN_NIL          5
#define MOO_METHOD_PREAMBLE_RETURN_TRUE         6
#define MOO_METHOD_PREAMBLE_RETURN_FALSE        7
#define MOO_METHOD_PREAMBLE_RETURN_INDEX        8
#define MOO_METHOD_PREAMBLE_RETURN_NEGINDEX     9
#define MOO_METHOD_PREAMBLE_RETURN_INSTVAR      10
#define MOO_METHOD_PREAMBLE_PRIMITIVE           11
#define MOO_METHOD_PREAMBLE_NAMED_PRIMITIVE     12 /* index is an index to the symbol table */
#define MOO_METHOD_PREAMBLE_EXCEPTION           13 /* NOTE changing this requires changes in Except.st */
#define MOO_METHOD_PREAMBLE_ENSURE              14 /* NOTE changing this requires changes in Except.st */

/* the index is an 16-bit unsigned integer. */
#define MOO_METHOD_PREAMBLE_INDEX_MIN 0x0000
#define MOO_METHOD_PREAMBLE_INDEX_MAX 0xFFFF
#define MOO_OOI_IN_METHOD_PREAMBLE_INDEX_RANGE(num) ((num) >= MOO_METHOD_PREAMBLE_INDEX_MIN && (num) <= MOO_METHOD_PREAMBLE_INDEX_MAX)

/* preamble flags */
#define MOO_METHOD_PREAMBLE_FLAG_VARIADIC (1 << 0)  /* allows variable arguments. but all named paramenters must be passed in */
#define MOO_METHOD_PREAMBLE_FLAG_LIBERAL  (1 << 1)  /* not all named parameters need to get passed in */
#define MOO_METHOD_PREAMBLE_FLAG_DUAL     (1 << 2)
#define MOO_METHOD_PREAMBLE_FLAG_LENIENT  (1 << 3)  /* lenient primitive method - no exception upon failure. return an error instead */

/* NOTE: if you change the number of instance variables for moo_context_t,
 *       you need to change the defintion of BlockContext and MethodContext.
 *       plus, you need to update various exception handling code in MethodContext */
#define MOO_CONTEXT_NAMED_INSTVARS 8
typedef struct moo_context_t moo_context_t;
typedef struct moo_context_t* moo_oop_context_t;
struct moo_context_t
{
	MOO_OBJ_HEADER;

	/* it points to the active context at the moment when
	 * this context object has been activated. a new method context
	 * is activated as a result of normal message sending and a block
	 * context is activated when it is sent 'value'. it's set to
	 * nil if a block context created hasn't received 'value'. */
	moo_oop_context_t  sender;

	/* SmallInteger, instruction pointer */
	moo_oop_t          ip;

	/* SmallInteger, stack pointer. the actual stack pointer is in the active
	 * process. For a method context, it stores the stack pointer of the active
	 * process before it gets activated. the stack pointer of the active 
	 * process is restored using this value upon returning. This field is
	 * almost useless for a block context. */
	moo_oop_t          sp;

	/* SmallInteger. Number of temporaries.
	 * For a block context, it's inclusive of the temporaries
	 * defined its 'home'. */
	moo_oop_t          ntmprs;

	/* CompiledMethod for a method context, 
	 * SmallInteger for a block context */
	moo_oop_t          method_or_nargs;

	/* it points to the receiver of the message for a method context.
	 * a base block context(created but not yet activated) has nil in this 
	 * field. if a block context is activated by 'value', it points 
	 * to the block context object used as a base for shallow-copy. */
	moo_oop_t          receiver_or_source;

	/* it is set to nil for a method context.
	 * for a block context, it points to the active context at the 
	 * moment the block context was created. that is, it points to 
	 * a method context where the base block has been defined. 
	 * an activated block context copies this field from the source. */
	moo_oop_t          home;

	/* when a method context is created, it is set to itself. no change is
	 * made when the method context is activated. when a block context is 
	 * created (when MAKE_BLOCK or BLOCK_COPY is executed), it is set to the
	 * origin of the active context. when the block context is shallow-copied
	 * for activation (when it is sent 'value'), it is set to the origin of
	 * the source block context. */
	moo_oop_context_t  origin; 

	/* variable indexed part - actual arguments and temporaries are placed here */
	moo_oop_t          slot[1]; /* stack */
};


#define MOO_PROCESS_NAMED_INSTVARS 12
typedef struct moo_process_t moo_process_t;
typedef struct moo_process_t* moo_oop_process_t;

#define MOO_SEMAPHORE_NAMED_INSTVARS 13
typedef struct moo_semaphore_t moo_semaphore_t;
typedef struct moo_semaphore_t* moo_oop_semaphore_t;

#define MOO_SEMAPHORE_GROUP_NAMED_INSTVARS 7
typedef struct moo_semaphore_group_t moo_semaphore_group_t;
typedef struct moo_semaphore_group_t* moo_oop_semaphore_group_t;

struct moo_process_t
{
	MOO_OBJ_HEADER;
	moo_oop_context_t   initial_context;
	moo_oop_context_t   current_context;

	moo_oop_t           id;    /* SmallInteger */
	moo_oop_t           state; /* SmallInteger */
	moo_oop_t           sp;    /* stack pointer. SmallInteger */

	struct
	{
		moo_oop_process_t   prev;
		moo_oop_process_t   next;
	} ps; /* links to use with the process scheduler */

	struct
	{
		moo_oop_process_t   prev;
		moo_oop_process_t   next;
	} sem_wait; /* links to use with a semaphore */

	moo_oop_t sem; /* nil, semaphore, or semaphore group */
	moo_oop_t perr; /* last error set by a primitive function */
	moo_oop_t perrmsg;

	/* == variable indexed part == */
	moo_oop_t slot[1]; /* process stack */
};

#define MOO_SEMAPHORE_IO_MASK_INPUT   1
#define MOO_SEMAPHORE_IO_MASK_OUTPUT  2
#define MOO_SEMAPHORE_IO_MASK_HANGUP  4
#define MOO_SEMAPHORE_IO_MASK_ERROR   8

struct moo_semaphore_t
{
	MOO_OBJ_HEADER;

	/* [IMPORTANT] make sure that the position of 'waiting' in moo_semaphore_t
	 *             must be exactly the same as its position in moo_semaphore_group_t */
	struct
	{
		moo_oop_process_t first;
		moo_oop_process_t last;
	} waiting; /* list of processes waiting on this semaphore */
	/* [END IMPORTANT] */

	moo_oop_t count; /* SmallInteger */

	moo_oop_t heap_index; /* index to the heap */
	moo_oop_t heap_ftime_sec; /* firing time */
	moo_oop_t heap_ftime_nsec; /* firing time */

	moo_oop_t io_index; 
	moo_oop_t io_handle;
	moo_oop_t io_mask; /* SmallInteger */

	moo_oop_t signal_action;
	moo_oop_semaphore_group_t group; /* nil or belonging semaphore group */
	struct
	{
		moo_oop_semaphore_t prev;
		moo_oop_semaphore_t next;
	} grm; /* group membership chain */
};

#define MOO_SEMAPHORE_GROUP_SEMS_UNSIG 0
#define MOO_SEMAPHORE_GROUP_SEMS_SIG   1

struct moo_semaphore_group_t
{
	MOO_OBJ_HEADER;

	/* [IMPORTANT] make sure that the position of 'waiting' in moo_semaphore_group_t
	 *             must be exactly the same as its position in moo_semaphore_t */
	struct
	{
		moo_oop_process_t first;
		moo_oop_process_t last; /* list of processes waiting on this semaphore group */
	} waiting;
	/* [END IMPORTANT] */

	struct
	{
		moo_oop_semaphore_t first;
		moo_oop_semaphore_t last;
	} sems[2]; /* sems[0] - unsignaled semaphores, sems[1] - signaled semaphores */

	moo_oop_t sem_io_count;
};

#define MOO_PROCESS_SCHEDULER_NAMED_INSTVARS 9
typedef struct moo_process_scheduler_t moo_process_scheduler_t;
typedef struct moo_process_scheduler_t* moo_oop_process_scheduler_t;
struct moo_process_scheduler_t
{
	MOO_OBJ_HEADER;
	moo_oop_process_t active; /*  pointer to an active process in the runnable process list */
	moo_oop_t should_exit; /* Boolean */
	moo_oop_t total_count; /* SmallIntger, total number of processes - runnable/running/suspended */

	struct
	{
		moo_oop_t count; /* SmallInteger, the number of runnable/running processes */
		moo_oop_process_t first;
		moo_oop_process_t last;
	} runnable; /* runnable process list */

	struct
	{
		moo_oop_t count; /* SmallInteger - suspended*/
		moo_oop_process_t first;
		moo_oop_process_t last;
	} suspended;
};

/**
 * The MOO_CLASSOF() macro return the class of an object including a numeric
 * object encoded into a pointer.
 */
#define MOO_CLASSOF(moo,oop) \
	(MOO_OOP_GET_TAG(oop)? (*moo->tagged_classes[MOO_OOP_GET_TAG(oop)]): MOO_OBJ_GET_CLASS(oop))

/**
 * The MOO_BYTESOF() macro returns the size of the payload of
 * an object in bytes. If the pointer given encodes a numeric value, 
 * it returns the size of #moo_oow_t in bytes.
 */
#define MOO_BYTESOF(moo,oop) \
	(MOO_OOP_IS_NUMERIC(oop)? MOO_SIZEOF(moo_oow_t): MOO_OBJ_BYTESOF(oop))


typedef struct moo_heap_t moo_heap_t;

struct moo_heap_t
{
	moo_uint8_t* base;  /* start of a heap */
	moo_uint8_t* limit; /* end of a heap */
	moo_uint8_t* ptr;   /* next allocation pointer */
};

/* =========================================================================
 * VIRTUAL MACHINE PRIMITIVES
 * ========================================================================= */
enum moo_vmprim_opendl_flag_t
{
	MOO_VMPRIM_OPENDL_PFMOD = (1 << 0)
};
typedef enum moo_vmprim_opendl_flag_t moo_vmprim_opendl_flag_t;

typedef void* (*moo_vmprim_dlopen_t) (moo_t* moo, const moo_ooch_t* name, int flags);
typedef void (*moo_vmprim_dlclose_t) (moo_t* moo, void* handle);
typedef void* (*moo_vmprim_dlsym_t) (moo_t* moo, void* handle, const moo_ooch_t* name);

typedef void (*moo_log_write_t) (moo_t* moo, moo_oow_t mask, const moo_ooch_t* msg, moo_oow_t len);

typedef int (*moo_vmprim_startup_t) (moo_t* moo);
typedef void (*moo_vmprim_cleanup_t) (moo_t* moo);
typedef void (*moo_vmprim_gettime_t) (moo_t* moo, moo_ntime_t* now);

typedef int (*moo_vmprim_muxadd_t) (moo_t* moo, moo_oop_semaphore_t sem);
typedef int (*moo_vmprim_muxmod_t) (moo_t* moo, moo_oop_semaphore_t sem);
typedef int (*moo_vmprim_muxdel_t) (moo_t* moo, moo_oop_semaphore_t sem);

typedef void (*moo_vmprim_muxwait_cb_t) (moo_t* moo, moo_ooi_t mask, void* ctx);
typedef void (*moo_vmprim_muxwait_t) (moo_t* moo, const moo_ntime_t* duration, moo_vmprim_muxwait_cb_t muxwcb);

typedef void (*moo_vmprim_sleep_t) (moo_t* moo, const moo_ntime_t* duration);
struct moo_vmprim_t
{
	moo_vmprim_dlopen_t   dl_open;
	moo_vmprim_dlclose_t  dl_close;
	moo_vmprim_dlsym_t    dl_getsym;
	moo_log_write_t       log_write;

	moo_vmprim_startup_t  vm_startup;
	moo_vmprim_cleanup_t  vm_cleanup;
	moo_vmprim_gettime_t  vm_gettime;
	moo_vmprim_muxadd_t   vm_muxadd;
	moo_vmprim_muxdel_t   vm_muxdel;
	moo_vmprim_muxmod_t   vm_muxmod;
	moo_vmprim_muxwait_t  vm_muxwait;
	moo_vmprim_sleep_t    vm_sleep;
};

typedef struct moo_vmprim_t moo_vmprim_t;

/* =========================================================================
 * IO MANIPULATION
 * ========================================================================= */

/* TODO: MOVE moo_ioimpl_t HERE */

/* =========================================================================
 * CALLBACK MANIPULATION
 * ========================================================================= */
typedef void (*moo_cbimpl_t) (moo_t* moo);

typedef struct moo_cb_t moo_cb_t;
struct moo_cb_t
{
	moo_cbimpl_t gc;
	moo_cbimpl_t fini;

	/* private below */
	moo_cb_t*     prev;
	moo_cb_t*     next;
};


/* =========================================================================
 * PRIMITIVE FUNCTIONS
 * ========================================================================= */
enum moo_pfrc_t
{
	MOO_PF_HARD_FAILURE = -1,
	MOO_PF_FAILURE      = 0,
	MOO_PF_SUCCESS      = 1
};
typedef enum moo_pfrc_t moo_pfrc_t;

/* primitive function implementation type */
typedef moo_pfrc_t (*moo_pfimpl_t) (
	moo_t*    moo,
	moo_ooi_t nargs
);

typedef struct moo_pfbase_t moo_pfbase_t;
struct moo_pfbase_t
{
	moo_pfimpl_t handler;
	moo_oow_t    minargs;
	moo_oow_t    maxargs;
};

typedef struct moo_pfinfo_t moo_pfinfo_t;
struct moo_pfinfo_t
{
	moo_method_type_t type;
	moo_ooch_t        mthname[32];
	int               variadic;
	moo_pfbase_t      base;
};


/* receiver check failure leads to hard failure.
 * RATIONAL: the primitive handler should be used by relevant classes and
 *           objects only. if the receiver check fails, you must review
 *           your class library */
#define MOO_PF_CHECK_RCV(moo,cond) do { \
	if (!(cond)) { moo_seterrnum((moo), MOO_EMSGRCV); return MOO_PF_HARD_FAILURE; } \
} while(0)

/* argument check failure causes the wrapping method to return an error.
 * RATIONAL: Being a typeless language, it's hard to control the kinds of
 *           arguments.
 */
#define MOO_PF_CHECK_ARGS(moo,nargs,cond) do { \
	if (!(cond)) { moo_seterrnum (moo, MOO_EINVAL); return MOO_PF_FAILURE; } \
} while(0)

/* =========================================================================
 * MODULE MANIPULATION
 * ========================================================================= */
#define MOO_MOD_NAME_LEN_MAX 120

typedef struct moo_mod_t moo_mod_t;

enum moo_mod_hint_t
{
	MOO_MOD_LOAD_FOR_IMPORT = (1 << 0)
};
typedef enum moo_mod_hint_t moo_mod_hint_t;

typedef int (*moo_mod_load_t) (
	moo_t*     moo,
	moo_mod_t* mod
);

typedef int (*moo_mod_import_t) (
	moo_t*           moo,
	moo_mod_t*       mod,
	moo_oop_class_t  _class
);

typedef moo_pfbase_t* (*moo_mod_query_t) (
	moo_t*            moo,
	moo_mod_t*        mod,
	const moo_ooch_t* name,
	moo_oow_t         namelen
);

typedef void (*moo_mod_unload_t) (
	moo_t*     moo,
	moo_mod_t* mod
);

typedef void (*moo_mod_gc_t) (
	moo_t*     moo,
	moo_mod_t* mod
);

struct moo_mod_t
{
	/* input */
	const moo_ooch_t name[MOO_MOD_NAME_LEN_MAX + 1];
	/*const*/ int hints; /* bitwised-ORed of moo_mod_hint_t enumerators */

	/* user-defined data */
	moo_mod_import_t import;
	moo_mod_query_t  query;
	moo_mod_unload_t unload;
	moo_mod_gc_t     gc;
	void*            ctx;
};

struct moo_mod_data_t 
{
	void*           handle;
	moo_rbt_pair_t* pair; /* internal backreference to moo->modtab */
	moo_mod_t       mod;
};
typedef struct moo_mod_data_t moo_mod_data_t;

struct moo_sbuf_t
{
	moo_ooch_t* ptr;
	moo_oow_t   len;
	moo_oow_t   capa;
};
typedef struct moo_sbuf_t moo_sbuf_t;

typedef struct moo_finalizable_t moo_finalizable_t;
struct moo_finalizable_t
{
	moo_oop_t oop;
	moo_finalizable_t* prev;
	moo_finalizable_t* next;
};

/* special callback to be called for trailer */
typedef void (*moo_trgc_t) (moo_t* moo, moo_oop_t obj);

/* =========================================================================
 * MOO VM
 * ========================================================================= */
#if defined(MOO_INCLUDE_COMPILER)
typedef struct moo_compiler_t moo_compiler_t;
#endif

struct moo_t
{
	moo_mmgr_t*  mmgr;
	moo_cmgr_t*  cmgr;
	moo_errnum_t errnum;

	struct
	{
		moo_ooch_t buf[2048];
		moo_oow_t len;
	} errmsg;

	struct
	{
		unsigned int trait;
		unsigned int log_mask;
		moo_oow_t log_maxcapa;
		moo_oow_t dfl_symtab_size;
		moo_oow_t dfl_sysdic_size;
		moo_oow_t dfl_procstk_size; 

#if !defined(NDEBUG)
		/* set automatically when trait is set */
		int karatsuba_cutoff;
#endif
	} option;

	moo_vmprim_t vmprim;

	moo_cb_t* cblist;
	moo_rbt_t modtab; /* primitive module table */

	struct
	{
		moo_ooch_t* ptr;
		moo_oow_t len;
		moo_oow_t capa;
		int last_mask;
	} log;


	/* ========================= */

	moo_heap_t* permheap; /* TODO: put kernel objects to here */
	moo_heap_t* curheap;
	moo_heap_t* newheap;

	/* =============================================================
	 * nil, true, false
	 * ============================================================= */
	moo_oop_t _nil;  /* pointer to the nil object */
	moo_oop_t _true;
	moo_oop_t _false;

	/* =============================================================
	 * KERNEL CLASSES 
	 *  Be sure to Keep these kernel class pointers registered in the 
	 *  kernel_classes table in gc.c
	 * ============================================================= */
	moo_oop_class_t _apex; /* Apex */
	moo_oop_class_t _undefined_object; /* UndefinedObject */
	moo_oop_class_t _class; /* Class */
	moo_oop_class_t _object; /* Object */
	moo_oop_class_t _string; /* String */

	moo_oop_class_t _symbol; /* Symbol */
	moo_oop_class_t _array; /* Array */
	moo_oop_class_t _byte_array; /* ByteArray */
	moo_oop_class_t _symbol_set; /* SymbolSet */
	moo_oop_class_t _dictionary;

	moo_oop_class_t _namespace; /* Namespace */
	moo_oop_class_t _pool_dictionary; /* PoolDictionary */
	moo_oop_class_t _method_dictionary; /* MethodDictionary */
	moo_oop_class_t _method; /* CompiledMethod */
	moo_oop_class_t _association; /* Association */

	moo_oop_class_t _method_context; /* MethodContext */
	moo_oop_class_t _block_context; /* BlockContext */
	moo_oop_class_t _process; /* Process */
	moo_oop_class_t _semaphore; /* Semaphore */
	moo_oop_class_t _semaphore_group; /* SemaphoreGroup */
	moo_oop_class_t _process_scheduler; /* ProcessScheduler */

	moo_oop_class_t _error_class; /* Error */
	moo_oop_class_t _true_class; /* True */
	moo_oop_class_t _false_class; /* False */
	moo_oop_class_t _character; /* Character */
	moo_oop_class_t _small_integer; /* SmallInteger */

	moo_oop_class_t _large_positive_integer; /* LargePositiveInteger */
	moo_oop_class_t _large_negative_integer; /* LargeNegativeInteger */

	moo_oop_class_t _small_pointer;
	moo_oop_class_t _system;
	/* =============================================================
	 * END KERNEL CLASSES 
	 * ============================================================= */

	/* =============================================================
	 * KEY SYSTEM DICTIONARIES
	 * ============================================================= */
	/* 2 tag bits(lo) + 2 extended tag bits(hi). not all slots are used
	 * because the 2 high extended bits are used only if the low tag bits
	 * are 3 */
	moo_oop_class_t* tagged_classes[16]; 

	moo_oop_dic_t symtab; /* system-wide symbol table. instance of SymbolSet */
	moo_oop_nsdic_t sysdic; /* system dictionary. instance of Namespace */
	moo_oop_process_scheduler_t processor; /* instance of ProcessScheduler */
	moo_oop_process_t nil_process; /* instance of Process */
	moo_oop_char_t dicnewsym; /* symbol new: for dictionary */
	moo_oop_char_t dicputassocsym; /* symbol put_assoc: for dictionary */

	/* pending asynchronous semaphores */
	moo_oop_semaphore_t* sem_list;
	moo_oow_t sem_list_count;
	moo_oow_t sem_list_capa;

	/* semaphores sorted according to time-out. 
	 * organize entries using heap as the earliest entry
	 * needs to be checked first */
	moo_oop_semaphore_t* sem_heap;
	moo_oow_t sem_heap_count;
	moo_oow_t sem_heap_capa;

	/* semaphores for I/O handling. plain array */
	moo_oop_semaphore_t* sem_io;
	moo_oow_t sem_io_count;
	moo_oow_t sem_io_capa;
	moo_oow_t sem_io_wait_count;

	/* semaphore to notify finalizable objects */
	moo_oop_semaphore_t sem_gcfin;
	int sem_gcfin_sigreq;

	moo_oop_t* tmp_stack[256]; /* stack for temporaries */
	moo_oow_t tmp_count;


	moo_oop_t* proc_map;
	moo_oow_t proc_map_capa;
	moo_ooi_t proc_map_free_first;
	moo_ooi_t proc_map_free_last;

	/* =============================================================
	 * EXECUTION REGISTERS
	 * ============================================================= */
	moo_oop_context_t initial_context; /* fake initial context */
	moo_oop_context_t active_context;
	moo_oop_method_t active_method;
	moo_oob_t* active_code;
	moo_ooi_t sp;
	moo_ooi_t ip;
	int proc_switched; /* TODO: this is temporary. implement something else to skip immediate context switching */
	int switch_proc;
	int abort_req;
	moo_ntime_t exec_start_time;
	moo_ntime_t exec_end_time;
	/* =============================================================
	 * END EXECUTION REGISTERS
	 * ============================================================= */

	/* == BIGINT CONVERSION == */
	struct
	{
		int safe_ndigits;
		moo_oow_t multiplier;
	} bigint[37];
	/* == END BIGINT CONVERSION == */

	moo_sbuf_t sbuf[64];

	struct
	{
		moo_finalizable_t* first;
		moo_finalizable_t* last;
	} collectable;

	struct
	{
		moo_finalizable_t* first;
		moo_finalizable_t* last;
	} finalizable;

	moo_uintmax_t inst_counter;
	moo_ooi_t last_inst_pointer;

#if defined(MOO_INCLUDE_COMPILER)
	moo_compiler_t* c;
#endif
};

/* TODO: proper stack bound check when pushing */
#define MOO_STACK_PUSH(moo,v) \
	do { \
		(moo)->sp = (moo)->sp + 1; \
		MOO_ASSERT (moo, (moo)->sp < (moo_ooi_t)(MOO_OBJ_GET_SIZE((moo)->processor->active) - MOO_PROCESS_NAMED_INSTVARS)); \
		(moo)->processor->active->slot[(moo)->sp] = v; \
	} while(0)

#define MOO_STACK_GET(moo,v_sp) ((moo)->processor->active->slot[v_sp])
#define MOO_STACK_SET(moo,v_sp,v_obj) \
	do { \
		MOO_ASSERT (moo, (v_sp) < (moo_ooi_t)(MOO_OBJ_GET_SIZE((moo)->processor->active) - MOO_PROCESS_NAMED_INSTVARS)); \
		(moo)->processor->active->slot[v_sp] = v_obj; \
	} while(0)

#define MOO_STACK_GETTOP(moo) MOO_STACK_GET(moo, (moo)->sp)
#define MOO_STACK_SETTOP(moo,v_obj) MOO_STACK_SET(moo, (moo)->sp, v_obj)

#define MOO_STACK_POP(moo) ((moo)->sp = (moo)->sp - 1)
#define MOO_STACK_POPS(moo,count) ((moo)->sp = (moo)->sp - (count))
#define MOO_STACK_ISEMPTY(moo) ((moo)->sp <= -1)

/* get the stack pointer of the argument at the given index */
#define MOO_STACK_GETARGSP(moo,nargs,idx) ((moo)->sp - ((nargs) - (idx) - 1))
/* get the argument at the given index */
#define MOO_STACK_GETARG(moo,nargs,idx) MOO_STACK_GET(moo, (moo)->sp - ((nargs) - (idx) - 1))
/* get the receiver of a message */
#define MOO_STACK_GETRCV(moo,nargs) MOO_STACK_GET(moo, (moo)->sp - nargs)

/* you can't access arguments and receiver after these macros. 
 * also you must not call this macro more than once */
#define MOO_STACK_SETRET(moo,nargs,retv) \
	do { \
		MOO_STACK_POPS(moo, nargs); \
		MOO_STACK_SETTOP(moo, (retv)); \
	} while(0)
#define MOO_STACK_SETRETTORCV(moo,nargs) (MOO_STACK_POPS(moo, nargs))
#define MOO_STACK_SETRETTOERRNUM(moo,nargs) MOO_STACK_SETRET(moo, nargs, MOO_ERROR_TO_OOP(moo->errnum))
#define MOO_STACK_SETRETTOERROR(moo,nargs,ec) MOO_STACK_SETRET(moo, nargs, MOO_ERROR_TO_OOP(ec))

/* =========================================================================
 * MOO VM LOGGING
 * ========================================================================= */

enum moo_log_mask_t
{
	MOO_LOG_DEBUG     = (1 << 0),
	MOO_LOG_INFO      = (1 << 1),
	MOO_LOG_WARN      = (1 << 2),
	MOO_LOG_ERROR     = (1 << 3),
	MOO_LOG_FATAL     = (1 << 4),

	MOO_LOG_MNEMONIC  = (1 << 7), /* bytecode mnemonic */
	MOO_LOG_GC        = (1 << 8),
	MOO_LOG_IC        = (1 << 9), /* instruction cycle, fetch-decode-execute */
	MOO_LOG_PRIMITIVE = (1 << 10),
	MOO_LOG_APP       = (1 << 11), /* moo applications, set by moo logging primitive */


	MOO_LOG_ALL_LEVELS = (MOO_LOG_DEBUG  | MOO_LOG_INFO | MOO_LOG_WARN | MOO_LOG_ERROR | MOO_LOG_FATAL),
	MOO_LOG_ALL_TYPES = (MOO_LOG_MNEMONIC | MOO_LOG_GC | MOO_LOG_IC | MOO_LOG_PRIMITIVE | MOO_LOG_APP),


	MOO_LOG_STDOUT    = (1 << 14), /* write log messages to stdout without timestamp. MOO_LOG_STDOUT wins over MOO_LOG_STDERR. */
	MOO_LOG_STDERR    = (1 << 15), /* write log messages to stderr without timestamp. */

};
typedef enum moo_log_mask_t moo_log_mask_t;

/* all bits must be set to get enabled */
#define MOO_LOG_ENABLED(moo,mask) (((moo)->option.log_mask & (mask)) == (mask))

#define MOO_LOG0(moo,mask,fmt) do { if (MOO_LOG_ENABLED(moo,mask)) moo_logbfmt(moo, mask, fmt); } while(0)
#define MOO_LOG1(moo,mask,fmt,a1) do { if (MOO_LOG_ENABLED(moo,mask)) moo_logbfmt(moo, mask, fmt, a1); } while(0)
#define MOO_LOG2(moo,mask,fmt,a1,a2) do { if (MOO_LOG_ENABLED(moo,mask)) moo_logbfmt(moo, mask, fmt, a1, a2); } while(0)
#define MOO_LOG3(moo,mask,fmt,a1,a2,a3) do { if (MOO_LOG_ENABLED(moo,mask)) moo_logbfmt(moo, mask, fmt, a1, a2, a3); } while(0)
#define MOO_LOG4(moo,mask,fmt,a1,a2,a3,a4) do { if (MOO_LOG_ENABLED(moo,mask)) moo_logbfmt(moo, mask, fmt, a1, a2, a3, a4); } while(0)
#define MOO_LOG5(moo,mask,fmt,a1,a2,a3,a4,a5) do { if (MOO_LOG_ENABLED(moo,mask)) moo_logbfmt(moo, mask, fmt, a1, a2, a3, a4, a5); } while(0)
#define MOO_LOG6(moo,mask,fmt,a1,a2,a3,a4,a5,a6) do { if (MOO_LOG_ENABLED(moo,mask)) moo_logbfmt(moo, mask, fmt, a1, a2, a3, a4, a5, a6); } while(0)

#if defined(NDEBUG)
	/* [NOTE]
	 *  get rid of debugging message totally regardless of
	 *  the log mask in the release build.
	 */
#	define MOO_DEBUG0(moo,fmt)
#	define MOO_DEBUG1(moo,fmt,a1)
#	define MOO_DEBUG2(moo,fmt,a1,a2)
#	define MOO_DEBUG3(moo,fmt,a1,a2,a3)
#	define MOO_DEBUG4(moo,fmt,a1,a2,a3,a4)
#	define MOO_DEBUG5(moo,fmt,a1,a2,a3,a4,a5)
#	define MOO_DEBUG6(moo,fmt,a1,a2,a3,a4,a5,a6)
#else
#	define MOO_DEBUG0(moo,fmt) MOO_LOG0(moo, MOO_LOG_DEBUG, fmt)
#	define MOO_DEBUG1(moo,fmt,a1) MOO_LOG1(moo, MOO_LOG_DEBUG, fmt, a1)
#	define MOO_DEBUG2(moo,fmt,a1,a2) MOO_LOG2(moo, MOO_LOG_DEBUG, fmt, a1, a2)
#	define MOO_DEBUG3(moo,fmt,a1,a2,a3) MOO_LOG3(moo, MOO_LOG_DEBUG, fmt, a1, a2, a3)
#	define MOO_DEBUG4(moo,fmt,a1,a2,a3,a4) MOO_LOG4(moo, MOO_LOG_DEBUG, fmt, a1, a2, a3, a4)
#	define MOO_DEBUG5(moo,fmt,a1,a2,a3,a4,a5) MOO_LOG5(moo, MOO_LOG_DEBUG, fmt, a1, a2, a3, a4, a5)
#	define MOO_DEBUG6(moo,fmt,a1,a2,a3,a4,a5,a6) MOO_LOG6(moo, MOO_LOG_DEBUG, fmt, a1, a2, a3, a4, a5, a6)
#endif

#define MOO_INFO0(moo,fmt) MOO_LOG0(moo, MOO_LOG_INFO, fmt)
#define MOO_INFO1(moo,fmt,a1) MOO_LOG1(moo, MOO_LOG_INFO, fmt, a1)
#define MOO_INFO2(moo,fmt,a1,a2) MOO_LOG2(moo, MOO_LOG_INFO, fmt, a1, a2)
#define MOO_INFO3(moo,fmt,a1,a2,a3) MOO_LOG3(moo, MOO_LOG_INFO, fmt, a1, a2, a3)
#define MOO_INFO4(moo,fmt,a1,a2,a3,a4) MOO_LOG4(moo, MOO_LOG_INFO, fmt, a1, a2, a3, a4)
#define MOO_INFO5(moo,fmt,a1,a2,a3,a4,a5) MOO_LOG5(moo, MOO_LOG_INFO, fmt, a1, a2, a3, a4, a5)
#define MOO_INFO6(moo,fmt,a1,a2,a3,a4,a5,a6) MOO_LOG6(moo, MOO_LOG_INFO, fmt, a1, a2, a3, a4, a5, a6)


/* =========================================================================
 * MOO ASSERTION
 * ========================================================================= */
#if defined(NDEBUG)
#	define MOO_ASSERT(moo,expr) ((void)0)
#else
#	define MOO_ASSERT(moo,expr) ((void)((expr) || (moo_assertfailed (moo, #expr, __FILE__, __LINE__), 0)))
#endif


#if defined(MOO_INCLUDE_COMPILER)
enum moo_iocmd_t
{
	MOO_IO_OPEN,
	MOO_IO_CLOSE,
	MOO_IO_READ
};
typedef enum moo_iocmd_t moo_iocmd_t;

struct moo_ioloc_t
{
	moo_oow_t line; /**< line */
	moo_oow_t colm; /**< column */
	const moo_ooch_t* file; /**< file specified in #include */
};
typedef struct moo_ioloc_t moo_ioloc_t;

typedef struct moo_ioarg_t moo_ioarg_t;

typedef moo_ooi_t (*moo_ioimpl_t) (
	moo_t*       moo,
	moo_iocmd_t  cmd,
	moo_ioarg_t* arg
);

enum moo_synerrnum_t
{
	MOO_SYNERR_NOERR,
	MOO_SYNERR_ILCHR,           /* illegal character */
	MOO_SYNERR_CMTNC,           /* comment not closed */
	MOO_SYNERR_STRNC,           /* string not closed */
	MOO_SYNERR_CLTNT,           /* character literal not terminated */
	MOO_SYNERR_HLTNT,           /* hased literal not terminated */
	MOO_SYNERR_CHARLITINVAL,    /* wrong character literal */
	MOO_SYNERR_COLON,           /* : expected */
	MOO_SYNERR_STRING,          /* string expected */
	MOO_SYNERR_RADIXINVAL,      /* invalid radix */
	MOO_SYNERR_RADNUMLITINVAL,  /* invalid numeric literal with radix */
	MOO_SYNERR_BYTERANGE,       /* byte too small or too large */
	MOO_SYNERR_ERRLITINVAL,     /* wrong error literal */
	MOO_SYNERR_LBRACE,          /* { expected */
	MOO_SYNERR_RBRACE,          /* } expected */
	MOO_SYNERR_LPAREN,          /* ( expected */
	MOO_SYNERR_RPAREN,          /* ) expected */
	MOO_SYNERR_RBRACK,          /* ] expected */
	MOO_SYNERR_PERIOD,          /* . expected */
	MOO_SYNERR_COMMA,           /* , expected */
	MOO_SYNERR_VBAR,            /* | expected */
	MOO_SYNERR_GT,              /* > expected */
	MOO_SYNERR_ASSIGN,          /* := expected */
	MOO_SYNERR_IDENT,           /* identifier expected */
	MOO_SYNERR_INTEGER,         /* integer expected */
	MOO_SYNERR_PRIMITIVE,       /* primitive: expected */
	MOO_SYNERR_DIRECTIVEINVAL,  /* wrong directive */
	MOO_SYNERR_CLASSUNDEF,      /* undefined class */
	MOO_SYNERR_CLASSDUPL,       /* duplicate class */
	MOO_SYNERR_CLASSCONTRA,     /* contradictory class */
	MOO_SYNERR_CLASSNAMEINVAL,  /* wrong class name */
	MOO_SYNERR_CLASSTRSIZE,     /* non-pointer class now allowed to inherit a superclass with trailer size set */
	MOO_SYNERR_CLASSFINAL,      /* now allowed to inherit a #final class */
	MOO_SYNERR_VARDCLBANNED,    /* variable declaration not allowed */
	MOO_SYNERR_MODIFIER,        /* modifier expected */
	MOO_SYNERR_MODIFIERINVAL,   /* wrong modifier */
	MOO_SYNERR_MODIFIERBANNED,  /* modifier not allowed */
	MOO_SYNERR_MODIFIERDUPL,    /* duplicate modifier */
	MOO_SYNERR_MTHNAME,         /* method name expected */
	MOO_SYNERR_MTHNAMEDUPL,     /* duplicate method name */
	MOO_SYNERR_VARIADMTHINVAL,  /* invalid variadic method definition */
	MOO_SYNERR_VARNAME,         /* variable name expected */
	MOO_SYNERR_ARGNAMEDUPL,     /* duplicate argument name */
	MOO_SYNERR_TMPRNAMEDUPL,    /* duplicate temporary variable name */
	MOO_SYNERR_VARNAMEDUPL,     /* duplicate variable name */
	MOO_SYNERR_BLKARGNAMEDUPL,  /* duplicate block argument name */
	MOO_SYNERR_VARUNDCL,        /* undeclared variable */
	MOO_SYNERR_VARUNUSE,        /* unsuable variable in compiled code */
	MOO_SYNERR_VARINACC,        /* inaccessible variable - e.g. accessing an instance variable from a class method is not allowed. */
	MOO_SYNERR_VARAMBIG,        /* ambiguious variable - e.g. the variable is found in multiple pool dictionaries imported */
	MOO_SYNERR_SELFINACC,       /* inaccessible self */
	MOO_SYNERR_PRIMARYINVAL,    /* wrong expression primary */
	MOO_SYNERR_TMPRFLOOD,       /* too many temporaries */
	MOO_SYNERR_ARGFLOOD,        /* too many arguments */
	MOO_SYNERR_BLKTMPRFLOOD,    /* too many block temporaries */
	MOO_SYNERR_BLKARGFLOOD,     /* too many block arguments */
	MOO_SYNERR_BLKFLOOD,        /* too large block */
	MOO_SYNERR_ARREXPFLOOD,     /* too large array expression */
	MOO_SYNERR_PFNUMINVAL,      /* wrong primitive function number */
	MOO_SYNERR_PFIDINVAL,       /* wrong primitive function identifier */
	MOO_SYNERR_PFARGDEFINVAL,   /* wrong primitive function argument definition */
	MOO_SYNERR_MODNAMEINVAL,    /* wrong module name */
	MOO_SYNERR_MODIMPFAIL,      /* failed to import module */
	MOO_SYNERR_INCLUDE,         /* #include error */
	MOO_SYNERR_PRAGMAINVAL,     /* wrong pragma name */
	MOO_SYNERR_NAMESPACEINVAL,  /* wrong namespace name */
	MOO_SYNERR_POOLDICINVAL,    /* wrong pool dictionary */
	MOO_SYNERR_POOLDICDUPL,     /* duplicate pool dictionary */
	MOO_SYNERR_LITERAL,         /* literal expected */
	MOO_SYNERR_NOTINLOOP,       /* break or continue not within a loop */
	MOO_SYNERR_INBLOCK,         /* break or continue within a block */
	MOO_SYNERR_WHILE            /* while expected */
};
typedef enum moo_synerrnum_t moo_synerrnum_t;

struct moo_synerr_t
{
	moo_synerrnum_t num;
	moo_ioloc_t     loc;
	moo_oocs_t      tgt;
};
typedef struct moo_synerr_t moo_synerr_t;
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(MOO_HAVE_INLINE)
	static MOO_INLINE void moo_switchprocess(moo_t* moo) { moo->switch_proc = 1; }
#else
#	define moo_switchprocess(moo) ((moo)->switch_proc = 1)
#endif

MOO_EXPORT moo_t* moo_open (
	moo_mmgr_t*         mmgr,
	moo_oow_t           xtnsize,
	moo_oow_t           heapsize,
	const moo_vmprim_t* vmprim,
	moo_errnum_t*       errnum
);

MOO_EXPORT void moo_close (
	moo_t* moo
);

MOO_EXPORT int moo_init (
	moo_t*              moo,
	moo_mmgr_t*         mmgr,
	moo_oow_t           heapsize,
	const moo_vmprim_t* vmprim
);

MOO_EXPORT void moo_fini (
	moo_t* moo
);

#if defined(MOO_HAVE_INLINE)
	static MOO_INLINE moo_mmgr_t* moo_getmmgr (moo_t* moo) { return moo->mmgr; }
	static MOO_INLINE void* moo_getxtn (moo_t* moo) { return (void*)(moo + 1); }

	static MOO_INLINE moo_cmgr_t* moo_getcmgr (moo_t* moo) { return moo->cmgr; }
	static MOO_INLINE void moo_setcmgr (moo_t* moo, moo_cmgr_t* cmgr) { moo->cmgr = cmgr; }

	static MOO_INLINE moo_errnum_t moo_geterrnum (moo_t* moo) { return moo->errnum; }
	static MOO_INLINE void moo_seterrnum (moo_t* moo, moo_errnum_t errnum) { moo->errnum = errnum; moo->errmsg.len = 0; }
#else
#	define moo_getmmgr(moo) ((moo)->mmgr)
#	define moo_getxtn(moo) ((void*)((moo) + 1))

#	define moo_getcmgr(moo) ((moo)->cmgr)
#	define moo_setcmgr(moo,mgr) ((moo)->cmgr = (mgr))

#	define moo_geterrnum(moo) ((moo)->errnum)
#	define moo_seterrnum(moo,num) ((moo)->errmsg.len = 0, (moo)->errnum = (num))
#endif


MOO_EXPORT void moo_seterrbfmt (
	moo_t*           moo,
	moo_errnum_t     errnum,
	const moo_bch_t* fmt,
	...
);

MOO_EXPORT void moo_seterrufmt (
	moo_t*           moo,
	moo_errnum_t     errnum,
	const moo_uch_t* fmt,
	...
);

MOO_EXPORT void moo_seterrwithsyserr (
	moo_t* moo,
	int    syserr
);

MOO_EXPORT const moo_ooch_t* moo_geterrmsg (
	moo_t* moo
);

MOO_EXPORT const moo_ooch_t* moo_geterrstr (
	moo_t* moo
);

/**
 * The moo_getoption() function gets the value of an option
 * specified by \a id into the buffer pointed to by \a value.
 *
 * \return 0 on success, -1 on failure
 */
MOO_EXPORT int moo_getoption (
	moo_t*        moo,
	moo_option_t  id,
	void*         value
);

/**
 * The moo_setoption() function sets the value of an option 
 * specified by \a id to the value pointed to by \a value.
 *
 * \return 0 on success, -1 on failure
 */
MOO_EXPORT int moo_setoption (
	moo_t*       moo,
	moo_option_t id,
	const void*   value
);


MOO_EXPORT moo_cb_t* moo_regcb (
	moo_t*    moo,
	moo_cb_t* tmpl
);

MOO_EXPORT void moo_deregcb (
	moo_t*    moo,
	moo_cb_t* cb
);

/**
 * The moo_gc() function performs garbage collection.
 * It is not affected by #MOO_NOGC.
 */
MOO_EXPORT void moo_gc (
	moo_t* moo
);


/**
 * The moo_moveoop() function moves an object and returns an updated pointer
 * after having moved. it must be called in the GC callback context only.
 */
MOO_EXPORT moo_oop_t moo_moveoop (
	moo_t*     moo,
	moo_oop_t  oop
);

/**
 * The moo_instantiate() function creates a new object instance of the class 
 * \a _class. The size of the fixed part is taken from the information
 * contained in the class defintion. The \a vlen parameter specifies the length
 * of the variable part. The \a vptr parameter points to the memory area to
 * copy into the variable part of the new object. If \a vptr is #MOO_NULL,
 * the variable part is initialized to 0 or an equivalent value depending
 * on the type. \a vptr is not used when the new instance is of the 
 * #MOO_OBJ_TYPE_OOP type.
 * 
 */
MOO_EXPORT moo_oop_t moo_instantiate (
	moo_t*           moo,
	moo_oop_class_t  _class,
	const void*      vptr,
	moo_oow_t        vlen
);

MOO_EXPORT moo_oop_t moo_instantiatewithtrailer (
	moo_t*           moo, 
	moo_oop_class_t  _class,
	moo_oow_t        vlen,
	const moo_oob_t* trptr,
	moo_oow_t        trlen
);

MOO_EXPORT moo_oop_t moo_shallowcopy (
	moo_t*          moo,
	moo_oop_t       oop
);

/**
 * The moo_ignite() function creates key initial objects.
 */
MOO_EXPORT int moo_ignite (
	moo_t* moo
);

/**
 * The moo_execute() function executes an activated context.
 */
MOO_EXPORT int moo_execute (
	moo_t* moo
);

/**
 * The moo_invoke() function sends a message named \a mthname to an object
 * named \a objname.
 */
MOO_EXPORT int moo_invoke (
	moo_t*            moo,
	const moo_oocs_t* objname,
	const moo_oocs_t* mthname
);

MOO_EXPORT void moo_abort (
	moo_t* moo
);

/* =========================================================================
 * COMMON OBJECT MANAGEMENT FUNCTIONS
 * ========================================================================= */
MOO_EXPORT moo_oop_t moo_makesymbol (
	moo_t*             moo,
	const moo_ooch_t*  ptr,
	moo_oow_t          len
);

MOO_EXPORT moo_oop_t moo_makestringwithuchars (
	moo_t*            moo, 
	const moo_uch_t*  ptr, 
	moo_oow_t         len
);

MOO_EXPORT moo_oop_t moo_makestringwithbchars (
	moo_t*            moo, 
	const moo_bch_t*  ptr, 
	moo_oow_t         len
);

#if defined(MOO_OOCH_IS_UCH)
#	define moo_makestring(moo,ptr,len) moo_makestringwithuchars(moo,ptr,len)
#else
#	define moo_makestring(moo,ptr,len) moo_makestringwithbchars(moo,ptr,len)
#endif

MOO_EXPORT moo_oop_t moo_oowtoint (
	moo_t*    moo,
	moo_oow_t w
);

MOO_EXPORT moo_oop_t moo_ooitoint (
	moo_t*    moo,
	moo_ooi_t i
);


MOO_EXPORT int moo_inttooow (
	moo_t*     moo,
	moo_oop_t  x,
	moo_oow_t* w
);

MOO_EXPORT int moo_inttoooi (
	moo_t*     moo,
	moo_oop_t  x,
	moo_ooi_t* i
);

MOO_EXPORT moo_oop_t moo_findclass (
	moo_t*            moo,
	moo_oop_nsdic_t   nsdic,
	const moo_ooch_t* name
);

MOO_EXPORT int moo_iskindof (
	moo_t*          moo,
	moo_oop_t       obj,
	moo_oop_class_t _class
);

/* =========================================================================
 * TRAILER MANAGEMENT
 * ========================================================================= */
MOO_EXPORT int moo_setclasstrsize (
	moo_t*          moo,
	moo_oop_class_t _class,
	moo_oow_t       size,
	moo_trgc_t      trgc
);

MOO_EXPORT void* moo_getobjtrailer (
	moo_t*      moo,
	moo_oop_t   obj,
	moo_oow_t*  size
);

/* =========================================================================
 * TEMPORARY OOP MANAGEMENT FUNCTIONS
 * ========================================================================= */
MOO_EXPORT void moo_pushtmp (
	moo_t*     moo,
	moo_oop_t* oop_ptr
);

MOO_EXPORT void moo_poptmp (
	moo_t*     moo
);

MOO_EXPORT void moo_poptmps (
	moo_t*     moo,
	moo_oow_t  count
);

/* =========================================================================
 * SYSTEM MEMORY MANAGEMENT FUCNTIONS VIA MMGR
 * ========================================================================= */
MOO_EXPORT void* moo_allocmem (
	moo_t*     moo,
	moo_oow_t  size
);

MOO_EXPORT void* moo_callocmem (
	moo_t*     moo,
	moo_oow_t  size
);

MOO_EXPORT void* moo_reallocmem (
	moo_t*      moo,
	void*       ptr,
	moo_oow_t   size
);

MOO_EXPORT void moo_freemem (
	moo_t*  moo,
	void*   ptr
);

/* =========================================================================
 * PRIMITIVE METHOD MANIPULATION
 * ========================================================================= */
MOO_EXPORT int moo_genpfmethod (
	moo_t*            moo,
	moo_mod_t*        mod,
	moo_oop_class_t   _class,
	moo_method_type_t type,
	const moo_ooch_t* mthname,
	int               variadic,
	const moo_ooch_t* name
);

/*
MOO_EXPORT int moo_genpfmethods (
	moo_t*              moo,
	moo_mod_t*          mod,
	moo_oop_class_t     _class,
	const moo_pfinfo_t* pfinfo,
	moo_oow_t           pfcount
);
*/

MOO_EXPORT moo_pfbase_t* moo_findpfbase (
	moo_t*              moo,
	moo_pfinfo_t*       pfinfo,
	moo_oow_t           pfcount,
	const moo_ooch_t*   name,
	moo_oow_t           namelen
);

/* =========================================================================
 * STRING ENCODING CONVERSION
 * ========================================================================= */

#if defined(MOO_OOCH_IS_UCH)
#	define moo_convootobchars(moo,oocs,oocslen,bcs,bcslen) moo_convutobchars(moo,oocs,oocslen,bcs,bcslen)
#	define moo_convbtooochars(moo,bcs,bcslen,oocs,oocslen) moo_convbtouchars(moo,bcs,bcslen,oocs,oocslen)
#	define moo_convootobcstr(moo,oocs,oocslen,bcs,bcslen) moo_convutobcstr(moo,oocs,oocslen,bcs,bcslen)
#	define moo_convbtooocstr(moo,bcs,bcslen,oocs,oocslen) moo_convbtoucstr(moo,bcs,bcslen,oocs,oocslen)
#else
#	define moo_convootouchars(moo,oocs,oocslen,bcs,bcslen) moo_convbtouchars(moo,oocs,oocslen,bcs,bcslen)
#	define moo_convutooochars(moo,bcs,bcslen,oocs,oocslen) moo_convutobchars(moo,bcs,bcslen,oocs,oocslen)
#	define moo_convootoucstr(moo,oocs,oocslen,bcs,bcslen) moo_convbtoucstr(moo,oocs,oocslen,bcs,bcslen)
#	define moo_convutooocstr(moo,bcs,bcslen,oocs,oocslen) moo_convutobcstr(moo,bcs,bcslen,oocs,oocslen)
#endif

MOO_EXPORT int moo_convbtouchars (
	moo_t*           moo,
	const moo_bch_t* bcs,
	moo_oow_t*       bcslen,
	moo_uch_t*       ucs,
	moo_oow_t*       ucslen
);

MOO_EXPORT int moo_convutobchars (
	moo_t*           moo,
	const moo_uch_t* ucs,
	moo_oow_t*       ucslen,
	moo_bch_t*       bcs,
	moo_oow_t*       bcslen
);


/**
 * The moo_convbtoucstr() function converts a null-terminated byte string 
 * to a wide string.
 */
MOO_EXPORT int moo_convbtoucstr (
	moo_t*           moo,
	const moo_bch_t* bcs,
	moo_oow_t*       bcslen,
	moo_uch_t*       ucs,
	moo_oow_t*       ucslen
);


/**
 * The moo_convutobcstr() function converts a null-terminated wide string
 * to a byte string.
 */
int moo_convutobcstr (
	moo_t*           moo,
	const moo_uch_t* ucs,
	moo_oow_t*       ucslen,
	moo_bch_t*       bcs,
	moo_oow_t*       bcslen
);


#if defined(MOO_OOCH_IS_UCH)
#	define moo_dupootobcharswithheadroom(moo,hrb,oocs,oocslen,bcslen) moo_duputobcharswithheadroom(moo,hrb,oocs,oocslen,bcslen)
#	define moo_dupbtooocharswithheadroom(moo,hrb,bcs,bcslen,oocslen) moo_dupbtoucharswithheadroom(moo,hrb,bcs,bcslen,oocslen)
#	define moo_dupootobchars(moo,oocs,oocslen,bcslen) moo_duputobchars(moo,oocs,oocslen,bcslen)
#	define moo_dupbtooochars(moo,bcs,bcslen,oocslen) moo_dupbtouchars(moo,bcs,bcslen,oocslen)

#	define moo_dupootobcstrwithheadroom(moo,hrb,oocs,bcslen) moo_duputobcstrwithheadroom(moo,hrb,oocs,bcslen)
#	define moo_dupbtooocstrwithheadroom(moo,hrb,bcs,oocslen) moo_dupbtoucstrwithheadroom(moo,hrb,bcs,oocslen)
#	define moo_dupootobcstr(moo,oocs,bcslen) moo_duputobcstr(moo,oocs,bcslen)
#	define moo_dupbtooocstr(moo,bcs,oocslen) moo_dupbtoucstr(moo,bcs,oocslen)
#else
#	define moo_dupootoucharswithheadroom(moo,hrb,oocs,oocslen,ucslen) moo_dupbtoucharswithheadroom(moo,hrb,oocs,oocslen,ucslen)
#	define moo_duputooocharswithheadroom(moo,hrb,ucs,ucslen,oocslen) moo_duputobcharswithheadroom(moo,hrb,ucs,ucslen,oocslen)
#	define moo_dupootouchars(moo,oocs,oocslen,ucslen) moo_dupbtouchars(moo,oocs,oocslen,ucslen)
#	define moo_duputooochars(moo,ucs,ucslen,oocslen) moo_duputobchars(moo,ucs,ucslen,oocslen)

#	define moo_dupootoucstrwithheadroom(moo,hrb,oocs,ucslen) moo_dupbtoucstrwithheadroom(moo,hrb,oocs,ucslen)
#	define moo_duputooocstrwithheadroom(moo,hrb,ucs,oocslen) moo_duputobcstrwithheadroom(moo,hrb,ucs,oocslen)
#	define moo_dupootoucstr(moo,oocs,ucslen) moo_dupbtoucstr(moo,oocs,ucslen)
#	define moo_duputooocstr(moo,ucs,oocslen) moo_duputobcstr(moo,ucs,oocslen)
#endif


MOO_EXPORT moo_uch_t* moo_dupbtoucharswithheadroom (
	moo_t*           moo,
	moo_oow_t        headroom_bytes,
	const moo_bch_t* bcs,
	moo_oow_t        bcslen,
	moo_oow_t*       ucslen
);

MOO_EXPORT moo_bch_t* moo_duputobcharswithheadroom (
	moo_t*           moo,
	moo_oow_t        headroom_bytes,
	const moo_uch_t* ucs,
	moo_oow_t        ucslen,
	moo_oow_t*       bcslen
);

MOO_EXPORT moo_uch_t* moo_dupbtouchars (
	moo_t*           moo,
	const moo_bch_t* bcs,
	moo_oow_t        bcslen,
	moo_oow_t*       ucslen
);

MOO_EXPORT moo_bch_t* moo_duputobchars (
	moo_t*           moo,
	const moo_uch_t* ucs,
	moo_oow_t        ucslen,
	moo_oow_t*       bcslen
);


MOO_EXPORT moo_uch_t* moo_dupbtoucstrwithheadroom (
	moo_t*           moo,
	moo_oow_t        headroom_bytes,
	const moo_bch_t* bcs,
	moo_oow_t*       ucslen
);

MOO_EXPORT moo_bch_t* moo_duputobcstrwithheadroom (
	moo_t*           moo,
	moo_oow_t        headroom_bytes,
	const moo_uch_t* ucs,
	moo_oow_t* bcslen
);

MOO_EXPORT moo_uch_t* moo_dupbtoucstr (
	moo_t*           moo,
	const moo_bch_t* bcs,
	moo_oow_t*       ucslen /* optional: length of returned string */
);

MOO_EXPORT moo_bch_t* moo_duputobcstr (
	moo_t*           moo,
	const moo_uch_t* ucs,
	moo_oow_t*       bcslen /* optional: length of returned string */
);


#if defined(MOO_OOCH_IS_UCH)
#	define moo_dupoochars(moo,oocs,oocslen) moo_dupuchars(moo,oocs,oocslen)
#else
#	define moo_dupoochars(moo,oocs,oocslen) moo_dupbchars(moo,oocs,oocslen)
#endif

MOO_EXPORT moo_uch_t* moo_dupuchars (
	moo_t*           moo,
	const moo_uch_t* ucs,
	moo_oow_t        ucslen
);

MOO_EXPORT moo_bch_t* moo_dupbchars (
	moo_t*           moo,
	const moo_bch_t* bcs,
	moo_oow_t        bcslen
);


/* =========================================================================
 * MOO VM LOGGING
 * ========================================================================= */

MOO_EXPORT moo_ooi_t moo_logbfmt (
	moo_t*           moo,
	moo_oow_t        mask,
	const moo_bch_t* fmt,
	...
);

MOO_EXPORT moo_ooi_t moo_logufmt (
	moo_t*            moo,
	moo_oow_t         mask,
	const moo_uch_t*  fmt,
	...
);
 
#if defined(MOO_OOCH_IS_UCH)
#	define moo_logoofmt moo_logufmt
#else
#	define moo_logoofmt moo_logbfmt
#endif

/* =========================================================================
 * MISCELLANEOUS HELPER FUNCTIONS
 * ========================================================================= */

MOO_EXPORT int moo_decode (
	moo_t*            moo,
	moo_oop_method_t  mth,
	const moo_oocs_t* classfqn
);

MOO_EXPORT void moo_assertfailed (
	moo_t*           moo,
	const moo_bch_t* expr,
	const moo_bch_t* file,
	moo_oow_t        line
);

MOO_EXPORT moo_errnum_t moo_syserr_to_errnum (
	int syserr
);

MOO_EXPORT const moo_ooch_t* moo_errnum_to_errstr (
	moo_errnum_t errnum
);


#if defined(MOO_INCLUDE_COMPILER)

/* =========================================================================
 * COMPILER FUNCTIONS
 * ========================================================================= */

MOO_EXPORT int moo_compile (
	moo_t*       moo,
	moo_ioimpl_t io
);

MOO_EXPORT void moo_getsynerr (
	moo_t*        moo,
	moo_synerr_t* synerr
);

MOO_EXPORT const moo_ooch_t* moo_synerrnum_to_errstr (
	moo_synerrnum_t errnum
);
#endif

#if defined(__cplusplus)
}
#endif


#endif
