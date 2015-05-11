/*
 * $Id$
 *
    Copyright (c) 2014-2015 Chung, Hyung-Hwan. All rights reserved.

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

#ifndef _STIX_H_
#define _STIX_H_

#if defined(__MSDOS__)
#	define STIX_INCPTR(type,base,inc)  (((type __huge*)base) + (inc))
#	define STIX_DECPTR(type,base,inc)  (((type __huge*)base) - (inc))
#	define STIX_GTPTR(type,ptr1,ptr2)  (((type __huge*)ptr1) > ((type __huge*)ptr2))
#	define STIX_GEPTR(type,ptr1,ptr2)  (((type __huge*)ptr1) >= ((type __huge*)ptr2))
#	define STIX_LTPTR(type,ptr1,ptr2)  (((type __huge*)ptr1) < ((type __huge*)ptr2))
#	define STIX_LEPTR(type,ptr1,ptr2)  (((type __huge*)ptr1) <= ((type __huge*)ptr2))
#	define STIX_EQPTR(type,ptr1,ptr2)  (((type __huge*)ptr1) == ((type __huge*)ptr2))
#	define STIX_SUBPTR(type,ptr1,ptr2) (((type __huge*)ptr1) - ((type __huge*)ptr2))
#else
#	define STIX_INCPTR(type,base,inc)  (((type*)base) + (inc))
#	define STIX_DECPTR(type,base,inc)  (((type*)base) - (inc))
#	define STIX_GTPTR(type,ptr1,ptr2)  (((type*)ptr1) > ((type*)ptr2))
#	define STIX_GEPTR(type,ptr1,ptr2)  (((type*)ptr1) >= ((type*)ptr2))
#	define STIX_LTPTR(type,ptr1,ptr2)  (((type*)ptr1) < ((type*)ptr2))
#	define STIX_LEPTR(type,ptr1,ptr2)  (((type*)ptr1) <= ((type*)ptr2))
#	define STIX_EQPTR(type,ptr1,ptr2)  (((type*)ptr1) == ((type*)ptr2))
#	define STIX_SUBPTR(type,ptr1,ptr2) (((type*)ptr1) - ((type*)ptr2))
#endif

/* ========================================================================== */
/* TODO: define these types and macros using autoconf */
typedef unsigned char      stix_uint8_t;
typedef unsigned short int stix_uint16_t;
/*typedef unsigned int       stix_uint32_t;*/
typedef unsigned long int stix_uintptr_t;
typedef unsigned long int stix_size_t;

typedef unsigned short int stix_char_t; /* TODO ... wchar_t??? */

#define STIX_SIZEOF(x) (sizeof(x))
#define STIX_COUNTOF(x) (sizeof(x) / sizeof(x[0]))

/**
 * The STIX_OFFSETOF() macro returns the offset of a field from the beginning
 * of a structure.
 */
#define STIX_OFFSETOF(type,member) ((stix_uintptr_t)&((type*)0)->member)

/**
 * The STIX_ALIGNOF() macro returns the alignment size of a structure.
 * Note that this macro may not work reliably depending on the type given.
 */
#define STIX_ALIGNOF(type) STIX_OFFSETOF(struct { stix_uint8_t d1; type d2; }, d2)
        /*(sizeof(struct { stix_uint8_t d1; type d2; }) - sizeof(type))*/

#if defined(__cplusplus)
#	if (__cplusplus >= 201103L) /* C++11 */
#		define STIX_NULL nullptr
#	else
#		define STIX_NULL (0)
#	endif
#else
#	define STIX_NULL ((void*)0)
#endif


/* make a low bit mask that can mask off low n bits*/
#define STIX_LBMASK(type,n) (~(~((type)0) << (n))) 

/* get 'length' bits starting from the bit at the 'offset' */
#define STIX_GETBITS(type,value,offset,length) \
	((((type)(value)) >> (offset)) & STIX_LBMASK(type,length))

#define STIX_SETBITS(type,value,offset,length,bits) \
	(value = (((type)(value)) | (((bits) & STIX_LBMASK(type,length)) << (offset))))


/** 
 * The STIX_BITS_MAX() macros calculates the maximum value that the 'nbits'
 * bits of an unsigned integer of the given 'type' can hold.
 * \code
 * printf ("%u", STIX_BITS_MAX(unsigned int, 5));
 * \endcode
 */
/*#define STIX_BITS_MAX(type,nbits) ((((type)1) << (nbits)) - 1)*/
#define STIX_BITS_MAX(type,nbits) ((~(type)0) >> (STIX_SIZEOF(type) * 8 - (nbits)))

typedef struct stix_mmgr_t stix_mmgr_t;

/** 
 * allocate a memory chunk of the size \a n.
 * @return pointer to a memory chunk on success, #STIX_NULL on failure.
 */
typedef void* (*stix_mmgr_alloc_t)   (stix_mmgr_t* mmgr, stix_size_t n);
/** 
 * resize a memory chunk pointed to by \a ptr to the size \a n.
 * @return pointer to a memory chunk on success, #STIX_NULL on failure.
 */
typedef void* (*stix_mmgr_realloc_t) (stix_mmgr_t* mmgr, void* ptr, stix_size_t n);
/**
 * free a memory chunk pointed to by \a ptr.
 */
typedef void  (*stix_mmgr_free_t)    (stix_mmgr_t* mmgr, void* ptr);

/**
 * The stix_mmgr_t type defines the memory management interface.
 * As the type is merely a structure, it is just used as a single container
 * for memory management functions with a pointer to user-defined data. 
 * The user-defined data pointer \a ctx is passed to each memory management 
 * function whenever it is called. You can allocate, reallocate, and free 
 * a memory chunk.
 *
 * For example, a stix_xxx_open() function accepts a pointer of the stix_mmgr_t
 * type and the xxx object uses it to manage dynamic data within the object. 
 */
struct stix_mmgr_t
{
	stix_mmgr_alloc_t   alloc;   /**< allocation function */
	stix_mmgr_realloc_t realloc; /**< resizing function */
	stix_mmgr_free_t    free;    /**< disposal function */
	void*               ctx;     /**< user-defined data pointer */
};

/**
 * The STIX_MMGR_ALLOC() macro allocates a memory block of the \a size bytes
 * using the \a mmgr memory manager.
 */
#define STIX_MMGR_ALLOC(mmgr,size) ((mmgr)->alloc(mmgr,size))

/**
 * The STIX_MMGR_REALLOC() macro resizes a memory block pointed to by \a ptr 
 * to the \a size bytes using the \a mmgr memory manager.
 */
#define STIX_MMGR_REALLOC(mmgr,ptr,size) ((mmgr)->realloc(mmgr,ptr,size))

/** 
 * The STIX_MMGR_FREE() macro deallocates the memory block pointed to by \a ptr.
 */
#define STIX_MMGR_FREE(mmgr,ptr) ((mmgr)->free(mmgr,ptr))


#if defined(_WIN32) || defined(__WATCOMC__)
#	define STIX_IMPORT __declspec(dllimport)
#	define STIX_EXPORT __declspec(dllexport)
#	define STIX_PRIVATE 
#elif defined(__GNUC__) && (__GNUC__>=4)
#	define STIX_IMPORT __attribute__((visibility("default")))
#	define STIX_EXPORT __attribute__((visibility("default")))
#	define STIX_PRIVATE __attribute__((visibility("hidden")))
/*#	define STIX_PRIVATE __attribute__((visibility("internal")))*/
#else
#	define STIX_IMPORT
#	define STIX_EXPORT
#	define STIX_PRIVATE
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__>=199901L)
#	define STIX_INLINE inline
#	define STIX_HAVE_INLINE
#elif defined(__GNUC__) && defined(__GNUC_GNU_INLINE__)
		/* gcc disables inline when -std=c89 or -ansi is used. 
		 * so use __inline__ supported by gcc regardless of the options */
#	define STIX_INLINE /*extern*/ __inline__
#	define STIX_HAVE_INLINE
#else
#	define STIX_INLINE 
#	undef STIX_HAVE_INLINE
#endif


/* ========================================================================== */

/**
 * The stix_errnum_t type defines the error codes.
 */
enum stix_errnum_t
{
	STIX_ENOERR,  /**< no error */
	STIX_EOTHER,  /**< other error */
	STIX_ENOIMPL, /**< not implemented */
	STIX_ESYSERR, /**< subsystem error */
	STIX_EINTERN, /**< internal error */
	STIX_ENOMEM,  /**< insufficient memory */
	STIX_EINVAL,  /**< invalid parameter or data */
	STIX_ENOENT   /**< no matching entry */
};
typedef enum stix_errnum_t stix_errnum_t;

enum stix_option_t
{
	STIX_TRAIT,
	STIX_DFL_SYMTAB_SIZE,
	STIX_DFL_SYSDIC_SIZE
};
typedef enum stix_option_t stix_option_t;

enum stix_trait_t
{
	/* perform no garbage collection when the heap is full. 
	 * you still can use stix_gc() explicitly. */
	STIX_NOGC = (1 << 0)
};
typedef enum stix_trait_t stix_trait_t;

/* NOTE: sizeof(stix_oop_t) must be equal to sizeof(stix_oow_t) */
typedef stix_uintptr_t           stix_oow_t;
typedef struct stix_obj_t        stix_obj_t;
typedef struct stix_obj_t*       stix_oop_t;

/* these are more specialized types for stix_obj_t */
typedef struct stix_obj_oop_t     stix_obj_oop_t;
typedef struct stix_obj_char_t    stix_obj_char_t;
typedef struct stix_obj_uint8_t   stix_obj_uint8_t;
typedef struct stix_obj_uint16_t  stix_obj_uint16_t;

/* these are more specialized types for stix_oop_t */
typedef struct stix_obj_oop_t*    stix_oop_oop_t;
typedef struct stix_obj_char_t*   stix_oop_char_t;
typedef struct stix_obj_uint8_t*  stix_oop_uint8_t;
typedef struct stix_obj_uint16_t* stix_oop_uint16_t;

#define STIX_OOW_BITS (STIX_SIZEOF(stix_oow_t) * 8)
#define STIX_OOP_BITS (STIX_SIZEOF(stix_oop_t) * 8)

/*
 * The Smalltalk-80 Bytecodes
 * Range     Bits               Function
 * -------------------------------------------------------------
 * 0-15      0000iiii           Push Receiver Variable #iiii
 * 16-31     0001iiii           Push Temporary Location #iiii
 * 32-63     001iiiii           Push Literal Constant #iiiii
 * 64-95     010iiiii           Push Literal Variable #iiiii
 * 96-103    01100iii           Pop and Store Receiver Variable #iii
 * 104-111   01101iii           Pop and Store Temporary Location #iii
 * 112-119   01110iii           Push (receiver, _true, _false, _nil, -1, 0, 1, 2) [iii]
 * 120-123   011110ii           Return (receiver, _true, _false, _nil) [ii] From Message
 * 124-125   0111110i           Return Stack Top From (Message, Block) [i]
 * 126-127   0111111i           unused
 * 128       10000000 jjkkkkkk  Push (Receiver Variable, Temporary Location, Literal Constant, Literal Variable) [jj] #kkkkkk
 * 129       10000001 jjkkkkkk  Store (Receiver Variable, Temporary Location, Illegal, Literal Variable) [jj] #kkkkkk
 * 130       10000010 jjkkkkkk  Pop and Store (Receiver Variable, Temporary Location, Illegal, Literal Variable) [jj] #kkkkkk
 * 131       10000011 jjjkkkkk  Send Literal Selector #kkkkk With jjj Arguments
 * 132       10000100 jjjjjjjj kkkkkkkk     Send Literal Selector #kkkkkkkk With jjjjjjjj Arguments
 * 133       10000101 jjjkkkkk  Send Literal Selector #kkkkk To Superclass With jjj Arguments
 * 134       10000110 jjjjjjjj kkkkkkkk     Send Literal Selector #kkkkkkkk To Superclass With jjjjjjjj Arguments
 * 135       10000111           Pop Stack Top
 * 136       10001000           Duplicate Stack Top
 * 137       10001001           Push Active Context
 * 138-143   unused
 * 144-151   10010iii           Jump iii + 1 (i.e., 1 through 8)
 * 152-159   10011iii           Pop and Jump On False iii +1 (i.e., 1 through 8)
 * 160-167   10100iii jjjjjjjj  Jump(iii - 4) *256+jjjjjjjj
 * 168-171   101010ii jjjjjjjj  Pop and Jump On True ii *256+jjjjjjjj
 * 172-175   101011ii jjjjjjjj  Pop and Jump On False ii *256+jjjjjjjj
 * 176-191   1011iiii           Send Arithmetic Message #iiii
 * 192-207   1100iiii           Send Special Message #iiii
 * 208-223   1101iiii           Send Literal Selector #iiii With No Arguments
 * 224-239   1110iiii           Send Literal Selector #iiii With 1 Argument
 * 240-255   1111iiii           Send Literal Selector #iiii With 2 Arguments  
 */

/**
 * The stix_code_t type defines byte-code enumerators.
 */
enum stix_code_t
{
	/* 0-15 */
	STIX_PUSH_RECEIVER_VARIABLE            = 0x00,

	/* 16-31 */
	STIX_PUSH_TEMPORARY_LOCATION           = 0x10,

	/* 32-63 */
	STIX_PUSH_LITERAL_CONSTANT             = 0x20,

	/* 64-95 */
	STIX_PUSH_LITERAL_VARIABLE             = 0x40,

	/* 96-103 */
	STIX_POP_STORE_RECEIVER_VARIABLE       = 0x60,

	/* 104-111 */
	STIX_POP_STORE_TEMPORARY_LOCATION      = 0x68,

	/*  112-119 */
	STIX_PUSH_RECEIVER                     = 0x70,
	STIX_PUSH_TRUE                         = 0x71,
	STIX_PUSH_FALSE                        = 0x72,
	STIX_PUSH_NIL                          = 0x73,
	STIX_PUSH_MINUSONE                     = 0x74,
	STIX_PUSH_ZERO                         = 0x75,
	STIX_PUSH_ONE                          = 0x76,
	STIX_PUSH_TWO                          = 0x77,

	/* 120-123 */
	STIX_RETURN_RECEIVER                   = 0x78,
	STIX_RETURN_TRUE                       = 0x79,
	STIX_RETURN_FALSE                      = 0x7A,
	STIX_RETURN_NIL                        = 0x7B,

	/* 124-125 */
	STIX_RETURN_FROM_MESSAGE               = 0x7C,
	STIX_RETURN_FROM_BLOCK                 = 0x7D,

	/* 128 */ 
	STIX_PUSH_EXTENDED                     = 0x80,

	/* 129 */ 
	STIX_STORE_EXTENDED                    = 0x81,

	/* 130 */ 
	STIX_POP_STORE_EXTENDED                = 0x82,

	/* 131 */
	STIX_SEND_TO_SELF                      = 0x83,

	/* 132 */
	STIX_SEND_TO_SUPER                     = 0x84,

	/* 133 */
	STIX_SEND_TO_SELF_EXTENDED             = 0x85,

	/* 134 */
	STIX_SEND_TO_SUPER_EXTENDED            = 0x86,

	/* 135 */
	STIX_POP_STACK_TOP                     = 0x87,

	/* 136 */
	STIX_DUP_STACK_TOP                     = 0x88,

	/* 137 */
	STIX_PUSH_ACTIVE_CONTEXT               = 0x89,

	/* 138 */
	STIX_DO_PRIMITIVE                      = 0x8A,

	/* 144-151 */
	STIX_JUMP                              = 0x90,

	/* 152-159 */
	STIX_POP_JUMP_ON_FALSE                 = 0x98,

	/* 160-167 */
	STIX_JUMP_EXTENDED                     = 0xA0,

	/* 168-171 */
	STIX_POP_JUMP_ON_TRUE_EXTENDED         = 0xA8,

	/* 172-175 */
	STIX_POP_JUMP_ON_FALSE_EXTENDED        = 0xAC,

#if 0
	STIX_PUSH_RECEIVER_VARIABLE_EXTENDED   = 0x60
	STIX_PUSH_TEMPORARY_LOCATION_EXTENDED  = 0x61
	STIX_PUSH_LITERAL_CONSTANT_EXTENDED    = 0x62
	STIX_PUSH_LITERAL_VARIABLE_EXTENDED    = 0x63
	STIX_STORE_RECEIVER_VARIABLE_EXTENDED  = 0x64
	STIX_STORE_TEMPORARY_LOCATION_EXTENDED = 0x65

	STIX_POP_STACK_TOP                     = 0x67
	STIX_DUPLICATE_STACK_TOP               = 0x68
	STIX_PUSH_ACTIVE_CONTEXT               = 0x69
	STIX_PUSH_NIL                          = 0x6A
	STIX_PUSH_TRUE                         = 0x6B
	STIX_PUSH_FALSE                        = 0x6C
	STIX_PUSH_RECEIVER                     = 0x6D

	STIX_SEND_TO_SELF                      = 0x70
	STIX_SEND_TO_SUPER                     = 0x71
	STIX_SEND_TO_SELF_EXTENDED             = 0x72
	STIX_SEND_TO_SUPER_EXTENDED            = 0x73

	STIX_RETURN_RECEIVER                   = 0x78
	STIX_RETURN_TRUE                       = 0x79
	STIX_RETURN_FALSE                      = 0x7A
	STIX_RETURN_NIL                        = 0x7B
	STIX_RETURN_FROM_MESSAGE               = 0x7C
	STIX_RETURN_FROM_BLOCK                 = 0x7D

	STIX_DO_PRIMITIVE                      = 0xF0
#endif
};

typedef enum stix_code_t stix_code_t;




/* 
 * OOP encoding
 * An object pointer(OOP) is an ordinary pointer value to an object.
 * but some simple numeric values are also encoded into OOP using a simple
 * bit-shifting and masking.
 *
 * A real OOP is stored without any bit-shifting while a non-OOP value encoded
 * in an OOP is bit-shifted to the left by 2 and the 2 least-significant bits
 * are set to 1 or 2.
 * 
 * This scheme works because the object allocators aligns the object size to
 * a multiple of sizeof(stix_oop_t). This way, the 2 least-significant bits
 * of a real OOP are always 0s.
 */

#define STIX_OOP_TAG_BITS  2
#define STIX_OOP_TAG_SMINT 1
#define STIX_OOP_TAG_CHAR  2

#define STIX_OOP_IS_NUMERIC(oop) (((stix_oow_t)oop) & (STIX_OOP_TAG_SMINT | STIX_OOP_TAG_CHAR))
#define STIX_OOP_IS_POINTER(oop) (!STIX_OOP_IS_NUMERIC(oop))
#define STIX_OOP_GET_TAG(oop) (((stix_oow_t)oop) & STIX_LBMASK(stix_oow_t, STIX_OOP_TAG_BITS))

#define STIX_OOP_IS_SMINT(oop) (((stix_oow_t)oop) & STIX_OOP_TAG_SMINT)
#define STIX_OOP_IS_CHAR(oop) (((stix_oow_t)oop) & STIX_OOP_TAG_CHAR)
#define STIX_OOP_FROM_SMINT(num) ((stix_oop_t)((((stix_oow_t)(num)) << STIX_OOP_TAG_BITS) | STIX_OOP_TAG_SMINT))
#define STIX_OOP_TO_SMINT(oop) (((stix_oow_t)oop) >> STIX_OOP_TAG_BITS)
#define STIX_OOP_FROM_CHAR(num) ((stix_oop_t)((((stix_oow_t)(num)) << STIX_OOP_TAG_BITS) | STIX_OOP_TAG_CHAR))
#define STIX_OOP_TO_CHAR(oop) (((stix_oow_t)oop) >> STIX_OOP_TAG_BITS)

/*
 * Object structure
 */
enum stix_obj_type_t
{
	STIX_OBJ_TYPE_OOP,
	STIX_OBJ_TYPE_CHAR,
	STIX_OBJ_TYPE_UINT8, 
	STIX_OBJ_TYPE_UINT16

/* NOTE: you can have STIX_OBJ_SHORT, STIX_OBJ_INT
 * STIX_OBJ_LONG, STIX_OBJ_FLOAT, STIX_OBJ_DOUBLE, etc 
 * type type field being 6 bits long, you can have up to 64 different types.

	STIX_OBJ_TYPE_SHORT,
	STIX_OBJ_TYPE_INT,
	STIX_OBJ_TYPE_LONG,
	STIX_OBJ_TYPE_FLOAT,
	STIX_OBJ_TYPE_DOUBLE */
};
typedef enum stix_obj_type_t stix_obj_type_t;

/* -------------------------------------------------------------------------
 * Object header structure 
 * 
 * _flags:
 *   type: the type of a payload item. 
 *         one of STIX_OBJ_TYPE_OOP, STIX_OBJ_TYPE_CHAR, 
 *                STIX_OBJ_TYPE_UINT8, STIX_OBJ_TYPE_UINT16
 *   unit: the size of a payload item in bytes. 
 *   extra: 0 or 1. 1 indicates that the payload contains 1 more
 *          item than the value of the size field. mostly used for a 
 *          terminating null in a variable-char object.
 *   kernel: 0 or 1. indicates that the object is a kernel object.
 *           VM disallows layout changes of a kernel object.
 *   moved: 0 or 1. used by GC.
 *
 * _size: the number of payload items in an object.
 *        it doesn't include the header size.
 * 
 * The total number of bytes occupied by an object can be calculated
 * with this fomula:
 *    sizeof(stix_obj_t) + ALIGN((size + extra) * unit), sizeof(stix_oop_t))
 * 
 * The ALIGN() macro is used above since allocation adjusts the payload
 * size to a multiple of sizeof(stix_oop_t). it assumes that sizeof(stix_obj_t)
 * is a multiple of sizeof(stix_oop_t). See the STIX_BYTESOF() macro.
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
 * ------------------------------------------------------------------------- */
#define STIX_OBJ_FLAGS_TYPE_BITS   6
#define STIX_OBJ_FLAGS_UNIT_BITS   5
#define STIX_OBJ_FLAGS_EXTRA_BITS  1
#define STIX_OBJ_FLAGS_KERNEL_BITS 1
#define STIX_OBJ_FLAGS_MOVED_BITS  1

#define STIX_OBJ_GET_FLAGS_TYPE(oop)     STIX_GETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_UNIT_BITS + STIX_OBJ_FLAGS_EXTRA_BITS + STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS), STIX_OBJ_FLAGS_TYPE_BITS)
#define STIX_OBJ_GET_FLAGS_UNIT(oop)     STIX_GETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_EXTRA_BITS + STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS),                            STIX_OBJ_FLAGS_UNIT_BITS)
#define STIX_OBJ_GET_FLAGS_EXTRA(oop)    STIX_GETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS),                                                        STIX_OBJ_FLAGS_EXTRA_BITS)
#define STIX_OBJ_GET_FLAGS_KERNEL(oop)   STIX_GETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_MOVED_BITS),                                                                                     STIX_OBJ_FLAGS_KERNEL_BITS)
#define STIX_OBJ_GET_FLAGS_MOVED(oop)    STIX_GETBITS(stix_oow_t, (oop)->_flags, 0,                                                                                                               STIX_OBJ_FLAGS_MOVED_BITS)

#define STIX_OBJ_SET_FLAGS_TYPE(oop,v)   STIX_SETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_UNIT_BITS + STIX_OBJ_FLAGS_EXTRA_BITS + STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS), STIX_OBJ_FLAGS_TYPE_BITS,   v)
#define STIX_OBJ_SET_FLAGS_UNIT(oop,v)   STIX_SETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_EXTRA_BITS + STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS),                            STIX_OBJ_FLAGS_UNIT_BITS,   v)
#define STIX_OBJ_SET_FLAGS_EXTRA(oop,v)  STIX_SETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS),                                                        STIX_OBJ_FLAGS_EXTRA_BITS,  v)
#define STIX_OBJ_SET_FLAGS_KERNEL(oop,v) STIX_SETBITS(stix_oow_t, (oop)->_flags, (STIX_OBJ_FLAGS_MOVED_BITS),                                                                                     STIX_OBJ_FLAGS_KERNEL_BITS, v)
#define STIX_OBJ_SET_FLAGS_MOVED(oop,v)  STIX_SETBITS(stix_oow_t, (oop)->_flags, 0,                                                                                                               STIX_OBJ_FLAGS_MOVED_BITS,  v)

#define STIX_OBJ_GET_SIZE(oop) ((oop)->_size)
#define STIX_OBJ_GET_CLASS(oop) ((oop)->_class)

#define STIX_OBJ_SET_SIZE(oop,v) ((oop)->_size = (v))
#define STIX_OBJ_SET_CLASS(oop,c) ((oop)->_class = (c))

#define STIX_OBJ_BYTESOF(oop) ((STIX_OBJ_GET_SIZE(oop) + STIX_OBJ_GET_FLAGS_EXTRA(oop)) * STIX_OBJ_GET_FLAGS_UNIT(oop))

/* this macro doesn't check the range of the actual value.
 * make sure that the value of each bit fields given fall within the number
 * of defined bits */
#define STIX_OBJ_MAKE_FLAGS(t,u,e,k,m) ( \
	(((stix_oow_t)(t)) << (STIX_OBJ_FLAGS_UNIT_BITS + STIX_OBJ_FLAGS_EXTRA_BITS + STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS)) | \
	(((stix_oow_t)(u)) << (STIX_OBJ_FLAGS_EXTRA_BITS + STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS)) | \
	(((stix_oow_t)(e)) << (STIX_OBJ_FLAGS_KERNEL_BITS + STIX_OBJ_FLAGS_MOVED_BITS)) | \
	(((stix_oow_t)(k)) << (STIX_OBJ_FLAGS_MOVED_BITS)) | \
	(((stix_oow_t)(m)) << 0) \
)

#define STIX_OBJ_HEADER \
	stix_oow_t _flags; \
	stix_oow_t _size; \
	stix_oop_t _class

struct stix_obj_t
{
	STIX_OBJ_HEADER;
};

struct stix_obj_oop_t
{
	STIX_OBJ_HEADER;
	stix_oop_t slot[1];
};

struct stix_obj_char_t
{
	STIX_OBJ_HEADER;
	stix_char_t slot[1];
};

struct stix_obj_uint8_t
{
	STIX_OBJ_HEADER;
	stix_uint8_t slot[1];
};

struct stix_obj_uint16_t
{
	STIX_OBJ_HEADER;
	stix_uint16_t slot[1];
};

#define STIX_CLASS_NAMED_INSTVARS 8
struct stix_class_t
{
	STIX_OBJ_HEADER;

	stix_oop_t      spec;         /* SmallInteger */
	stix_oop_t      superclass;   /* Another class */
	stix_oop_t      subclasses;   /* Array of subclasses */
	stix_oop_char_t name;         /* Symbol */
	stix_oop_char_t instvars;     /* String or Array? */
	stix_oop_char_t classvars;    /* String or Array? */

	stix_oop_oop_t  instmthds;    /* instance methods, MethodDictionary */
	stix_oop_oop_t  classmthds;   /* class methods, MethodDictionary */

	/* indexed part afterwards */
	stix_oop_t      classvar[1];  /* most classes have not class variables. better to be 0 */
};
typedef struct stix_class_t stix_class_t;
typedef struct stix_class_t* stix_oop_class_t;

#define STIX_SET_NAMED_INSTVARS 2
struct stix_set_t
{
	STIX_OBJ_HEADER;
	stix_oop_t     tally;  /* SmallInteger */
	stix_oop_oop_t bucket; /* Array */
};
typedef struct stix_set_t stix_set_t;
typedef struct stix_set_t* stix_oop_set_t;

#define STIX_ASSOCIATION_NAMED_INSTVARS 2
struct stix_association_t
{
	STIX_OBJ_HEADER;
	stix_oop_t key;
	stix_oop_t value;
};
typedef struct stix_association_t stix_association_t;
typedef struct stix_association_t* stix_oop_association_t;

#if 0
/* -----------------------------------------
 * class structures for classes known to VM  
 * ----------------------------------------- */
enum stix_class_desc_t
{
	/* STIX_XXX_SIZE represents the size of the class. other 
	 * enumerators represent the index of instance variables of 
	 * the class */

	STIX_ASSOCIATION_KEY = 0,
	STIX_ASSOCIATION_VALUE,
	STIX_ASSOCIATION_SIZE,
 
	STIX_DICTIONARY_TALLY = 0,
	STIX_DICTIONARY_BUCKET,
	STIX_DICTIONARY_SIZE,

	STIX_BEHAVIOR_SPEC = 0,
	STIX_BEHAVIOR_METHODS,
	STIX_BEHAVIOR_SUPERCLASS,
	STIX_BEHAVIOR_SUBCLASSES,
	STIX_BEHAVIOR_SIZE,

	STIX_CLASS_SPEC = 0,
	STIX_CLASS_METHODS,
	STIX_CLASS_SUPERCLASS,
	STIX_CLASS_SUBCLASSES,
	STIX_CLASS_NAME,
	STIX_CLASS_INSTANCE_VARIABLES,
	STIX_CLASS_CLASS_VARIABLES,
	STIX_CLASS_POOL_DICTIONARIES,
	STIX_CLASS_SIZE,

	STIX_METACLASS_SPEC = 0,
	STIX_METACLASS_METHODS,
	STIX_METACLASS_SUPERCLASS,
	STIX_METACLASS_SUBCLASSES,
	STIX_METACLASS_INSTANCE_CLASS,
	STIX_METACLASS_INSTANCE_VARIABLES,
	STIX_METACLASS_SIZE,

	STIX_BLOCK_CONTEXT = 0,
	STIX_BLOCK_ARG_COUNT,
	STIX_BLOCK_ARG_LOC,
	STIX_BLOCK_BYTE_POINTER,
	STIX_BLOCK_SIZE,

	STIX_CONTEXT_STACK = 0,
	STIX_CONTEXT_STACK_TOP,
	STIX_CONTEXT_RECEIVER,
	STIX_CONTEXT_PC,
	STIX_CONTEXT_METHOD,
	STIX_CONTEXT_SIZE,

	STIX_METHOD_TEXT = 0,
	STIX_METHOD_SELECTOR,
	STIX_METHOD_BYTECODES,
	STIX_METHOD_TMPCOUNT,
	STIX_METHOD_ARGCOUNT,
	STIX_METHOD_SIZE,

	STIX_SYMTAB_TALLY = 0,
	STIX_SYMTAB_BUCKET,
	STIX_SYMTAB_SIZE,

	STIX_SYSDIC_TALLY = STIX_DICTIONARY_TALLY,
	STIX_SYSDIC_BUCKET = STIX_DICTIONARY_BUCKET,
	STIX_SYSDIC_SIZE = STIX_DICTIONARY_SIZE
};
#endif

/**
 * The STIX_CLASSOF() macro return the class of an object including a numeric
 * object encoded into a pointer.
 */
#define STIX_CLASSOF(stix,oop) ( \
	STIX_OOP_IS_SMINT(oop)? (stix)->_small_integer: \
	STIX_OOP_IS_CHAR(oop)? (stix)->_character: STIX_OBJ_GET_CLASS(oop) \
)

/**
 * The STIX_BYTESOF() macro returns the size of the payload of
 * an object in bytes. If the pointer given encodes a numeric value, 
 * it returns the size of #stix_oow_t in bytes.
 */
#define STIX_BYTESOF(stix,oop) \
	(STIX_OOP_IS_NUMERIC(oop)? STIX_SIZEOF(stix_oow_t): STIX_OBJ_BYTESOF(oop))

typedef struct stix_heap_t stix_heap_t;

struct stix_heap_t
{
	stix_uint8_t* base;  /* start of a heap */
	stix_uint8_t* limit; /* end of a heap */
	stix_uint8_t* ptr;   /* next allocation pointer */
};

typedef struct stix_t stix_t;

struct stix_t
{
	stix_mmgr_t*  mmgr;
	stix_errnum_t errnum;

	struct
	{
		int trait;
		stix_oow_t dfl_symtab_size;
		stix_oow_t dfl_sysdic_size;
	} option;

	/* ========================= */

	stix_heap_t* permheap; /* TODO: put kernel objects to here */
	stix_heap_t* curheap;
	stix_heap_t* newheap;

	/* ========================= */
	stix_oop_t _nil;  /* pointer to the nil object */
	stix_oop_t _true;
	stix_oop_t _false;

	/* == NEVER CHANGE THE ORDER OF FIELDS BELOW == */
	stix_oop_t _stix; /* Stix */
	stix_oop_t _nil_object; /* NilObject */
	stix_oop_t _class; /* Class */
	stix_oop_t _object; /* Object */
	stix_oop_t _symbol; /* Symbol */
	stix_oop_t _array; /* Array */
	stix_oop_t _symbol_set; /* SymbolSet */
	stix_oop_t _system_dictionary; /* SystemDictionary */
	stix_oop_t _association; /* Association */
	stix_oop_t _true_class; /* True */
	stix_oop_t _false_class; /* False */
	stix_oop_t _character; /* Character */
	stix_oop_t _small_integer; /* SmallInteger */
	/* == NEVER CHANGE THE ORDER OF FIELDS ABOVE == */

	stix_oop_set_t symtab; /* system-wide symbol table. instance of SymbolSet */
	stix_oop_set_t sysdic; /* system dictionary. instance of SystemDictionary */

	stix_oop_t* tmp_stack[100]; /* stack for temporaries */
	stix_oow_t tmp_count;
};


#if defined(__cplusplus)
extern "C" {
#endif

STIX_EXPORT stix_t* stix_open (
	stix_mmgr_t*   mmgr,
	stix_size_t    xtnsize,
	stix_size_t    heapsize,
	stix_errnum_t* errnum
);

STIX_EXPORT void stix_close (
	stix_t* vm
);

STIX_EXPORT int stix_init (
	stix_t*      vm,
	stix_mmgr_t* mmgr,
	stix_size_t  heapsz
);

STIX_EXPORT void stix_fini (
	stix_t* vm
);

/**
 * The stix_getoption() function gets the value of an option
 * specified by \a id into the buffer pointed to by \a value.
 *
 * \return 0 on success, -1 on failure
 */
STIX_EXPORT int stix_getoption (
	stix_t*        vm,
	stix_option_t  id,
	void*          value
);

/**
 * The stix_setoption() function sets the value of an option 
 * specified by \a id to the value pointed to by \a value.
 *
 * \return 0 on success, -1 on failure
 */
STIX_EXPORT int stix_setoption (
	stix_t*       vm,
	stix_option_t id,
	const void*   value
);

/**
 * The stix_gc() function performs garbage collection.
 * It is not affected by #STIX_NOGC.
 */
STIX_EXPORT void stix_gc (
	stix_t* vm
);

/**
 * The stix_findclass() function searchs the root system dictionary
 * for a class named \a name. The class object pointer is set to the 
 * memory location pointed to by \a oop.
 *
 * \return 0 if found, -1 if not found.
 */
STIX_EXPORT int stix_findclass (
	stix_t*            vm,
	const stix_char_t* name,
	stix_oop_t*        oop
);


/**
 * The stix_instantiate() function creates a new object of the class 
 * \a _class. The size of the fixed part is taken from the information
 * contained in the class defintion. The \a vlen parameter specifies 
 * the length of the variable part. The \a vptr parameter points to 
 * the memory area to copy into the variable part of the new object.
 * If \a vptr is #STIX_NULL, the variable part is initialized to 0 or
 * an equivalent value depending on the type.
 */
STIX_EXPORT stix_oop_t stix_instantiate (
	stix_t*          stix,
	stix_oop_t       _class,
	const void*      vptr,
	stix_oow_t       vlen
);

/**
 * The stix_ignite() function creates key initial objects.
 */
STIX_EXPORT int stix_ignite (
	stix_t* stix
);


/**
 * Temporary OOP management 
 */
STIX_EXPORT void stix_pushtmp (
	stix_t*     stix,
	stix_oop_t* oop_ptr
);

STIX_EXPORT void stix_poptmp (
	stix_t*     stix
);

STIX_EXPORT void stix_poptmps (
	stix_t*     stix,
	stix_oow_t  count
);

#if defined(__cplusplus)
}
#endif


#endif
