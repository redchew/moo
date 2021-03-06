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

#include "moo-prv.h"

void* moo_allocbytes (moo_t* moo, moo_oow_t size)
{
	moo_uint8_t* ptr;

#if defined(MOO_BUILD_DEBUG)
	if ((moo->option.trait & MOO_TRAIT_DEBUG_GC) && !(moo->option.trait & MOO_TRAIT_NOGC)) moo_gc (moo);
#endif

	if (MOO_UNLIKELY(moo->igniting))
	{
		/* you must increase the size of the permspace if this allocation fails */
		ptr = (moo_uint8_t*)moo_allocheapspace(moo, &moo->heap->permspace, size); 
	}
	else
	{
		ptr = (moo_uint8_t*)moo_allocheapspace(moo, &moo->heap->curspace, size);
		if (!ptr && moo->errnum == MOO_EOOMEM && !(moo->option.trait & MOO_TRAIT_NOGC))
		{
			moo_gc (moo);
			MOO_LOG4 (moo, MOO_LOG_GC | MOO_LOG_INFO,
				"GC completed - current heap ptr %p limit %p size %zd free %zd\n", 
				moo->heap->curspace.ptr, moo->heap->curspace.limit,
				(moo_oow_t)(moo->heap->curspace.limit - moo->heap->curspace.base),
				(moo_oow_t)(moo->heap->curspace.limit - moo->heap->curspace.ptr)
			);
			ptr = (moo_uint8_t*)moo_allocheapspace(moo, &moo->heap->curspace, size);
	/* TODO: grow heap if ptr is still null. */
		}
	}
	return ptr;
}

moo_oop_t moo_allocoopobj (moo_t* moo, moo_oow_t size)
{
	moo_oop_oop_t hdr;
	moo_oow_t nbytes, nbytes_aligned;

	nbytes = size * MOO_SIZEOF(moo_oop_t);

	/* this isn't really necessary since nbytes must be 
	 * aligned already. */
	nbytes_aligned = MOO_ALIGN(nbytes, MOO_SIZEOF(moo_oop_t)); 

	/* making the number of bytes to allocate a multiple of
	 * MOO_SIZEOF(moo_oop_t) will guarantee the starting address
	 * of the allocated space to be an even number. 
	 * see MOO_OOP_IS_NUMERIC() and MOO_OOP_IS_POINTER() */
	hdr = (moo_oop_oop_t)moo_allocbytes(moo, MOO_SIZEOF(moo_obj_t) + nbytes_aligned);
	if (!hdr) return MOO_NULL;

	hdr->_flags = MOO_OBJ_MAKE_FLAGS(MOO_OBJ_TYPE_OOP, MOO_SIZEOF(moo_oop_t), 0, 0, moo->igniting, 0, 0, 0, 0, 0);
	MOO_OBJ_SET_SIZE (hdr, size);
	MOO_OBJ_SET_CLASS (hdr, moo->_nil);

	while (size > 0) 
	{
		size = size - 1;
		MOO_OBJ_SET_OOP_VAL (hdr, size, moo->_nil);
	}

	return (moo_oop_t)hdr;
}

moo_oop_t moo_allocoopobjwithtrailer (moo_t* moo, moo_oow_t size, const moo_oob_t* bptr, moo_oow_t blen)
{
	moo_oop_oop_t hdr;
	moo_oow_t nbytes, nbytes_aligned;
	moo_oow_t i;

	/* +1 for the trailer size of the moo_oow_t type */
	nbytes = (size + 1) * MOO_SIZEOF(moo_oop_t) + blen;
	nbytes_aligned = MOO_ALIGN(nbytes, MOO_SIZEOF(moo_oop_t)); 

	hdr = (moo_oop_oop_t)moo_allocbytes(moo, MOO_SIZEOF(moo_obj_t) + nbytes_aligned);
	if (!hdr) return MOO_NULL;

	hdr->_flags = MOO_OBJ_MAKE_FLAGS(MOO_OBJ_TYPE_OOP, MOO_SIZEOF(moo_oop_t), 0, 0, moo->igniting, 0, 0, 1, 0, 1); /* TRAILER -> 1, UNCOPYABLE -> 1 */
	MOO_OBJ_SET_SIZE (hdr, size);
	MOO_OBJ_SET_CLASS (hdr, moo->_nil);

	for (i = 0; i < size; i++) 
	{
		MOO_OBJ_SET_OOP_VAL (hdr, i, moo->_nil);
	}

	/* [NOTE] this is not converted to a SmallInteger object. it is a special slot handled by GC for an object with the TRAILER bit set */
	MOO_OBJ_SET_OOP_VAL (hdr, size, (moo_oop_t)blen);

	/* the trailer part is just composed of raw bytes from the moo core's perspective. */
	if (bptr)
	{
		MOO_MEMCPY ((moo_oob_t*)MOO_OBJ_GET_OOP_PTR(hdr, size + 1), bptr, blen);
	}
	else
	{
		MOO_MEMSET ((moo_oob_t*)MOO_OBJ_GET_OOP_PTR(hdr, size + 1), 0, blen);
	}

	return (moo_oop_t)hdr;
}

static MOO_INLINE moo_oop_t alloc_numeric_array (moo_t* moo, const void* ptr, moo_oow_t len, moo_obj_type_t type, moo_oow_t unit, int extra)
{
	/* allocate a variable object */

	moo_oop_t hdr;
	moo_oow_t xbytes, nbytes, nbytes_aligned;

	xbytes = len * unit;
	/* 'extra' indicates an extra unit to append at the end.
	 * it's useful to store a string with a terminating null */
	/*nbytes = extra? xbytes + len: xbytes; */
	nbytes = extra? xbytes + unit: xbytes; 
	nbytes_aligned = MOO_ALIGN(nbytes, MOO_SIZEOF(moo_oop_t));
/* TODO: check overflow in size calculation*/

	/* making the number of bytes to allocate a multiple of
	 * MOO_SIZEOF(moo_oop_t) will guarantee the starting address
	 * of the allocated space to be an even number. 
	 * see MOO_OOP_IS_NUMERIC() and MOO_OOP_IS_POINTER() */
	hdr = (moo_oop_t)moo_allocbytes(moo, MOO_SIZEOF(moo_obj_t) + nbytes_aligned);
	if (!hdr) return MOO_NULL;

	hdr->_flags = MOO_OBJ_MAKE_FLAGS(type, unit, extra, 0, moo->igniting, 0, 0, 0, 0, 0); /* TODO: review. ngc and perm flags seems to conflict with each other ... the diff is that ngc is malloc() and perm is allocated in the perm heap */
	hdr->_size = len;
	MOO_OBJ_SET_SIZE (hdr, len);
	MOO_OBJ_SET_CLASS (hdr, moo->_nil);

	if (ptr)
	{
		/* copy data */
		MOO_MEMCPY (hdr + 1, ptr, xbytes);
		MOO_MEMSET ((moo_uint8_t*)(hdr + 1) + xbytes, 0, nbytes_aligned - xbytes);
	}
	else
	{
		/* initialize with zeros when the data pointer is NULL */
		MOO_MEMSET ((hdr + 1), 0, nbytes_aligned);
	}

	return hdr;
}

MOO_INLINE moo_oop_t moo_alloccharobj (moo_t* moo, const moo_ooch_t* ptr, moo_oow_t len)
{
	return alloc_numeric_array(moo, ptr, len, MOO_OBJ_TYPE_CHAR, MOO_SIZEOF(moo_ooch_t), 1);
}

MOO_INLINE moo_oop_t moo_allocbyteobj (moo_t* moo, const moo_oob_t* ptr, moo_oow_t len)
{
	return alloc_numeric_array(moo, ptr, len, MOO_OBJ_TYPE_BYTE, MOO_SIZEOF(moo_oob_t), 0);
}

MOO_INLINE moo_oop_t moo_allochalfwordobj (moo_t* moo, const moo_oohw_t* ptr, moo_oow_t len)
{
	return alloc_numeric_array(moo, ptr, len, MOO_OBJ_TYPE_HALFWORD, MOO_SIZEOF(moo_oohw_t), 0);
}

MOO_INLINE moo_oop_t moo_allocwordobj (moo_t* moo, const moo_oow_t* ptr, moo_oow_t len)
{
	return alloc_numeric_array(moo, ptr, len, MOO_OBJ_TYPE_WORD, MOO_SIZEOF(moo_oow_t), 0);
}

static MOO_INLINE int decode_spec (moo_t* moo, moo_oop_class_t _class, moo_oow_t num_flexi_fields, moo_obj_type_t* type, moo_oow_t* outlen)
{
	moo_oow_t spec;
	moo_oow_t num_fixed_fields;
	moo_obj_type_t indexed_type;

	MOO_ASSERT (moo, MOO_OOP_IS_POINTER(_class));
	MOO_ASSERT (moo, MOO_CLASSOF(moo, _class) == moo->_class);

	MOO_ASSERT (moo, MOO_OOP_IS_SMOOI(_class->spec));
	spec = MOO_OOP_TO_SMOOI(_class->spec);

	num_fixed_fields = MOO_CLASS_SPEC_NAMED_INSTVARS(spec); 
	MOO_ASSERT (moo, num_fixed_fields <= MOO_MAX_NAMED_INSTVARS);

	if (MOO_CLASS_SPEC_IS_INDEXED(spec)) 
	{
		indexed_type = MOO_CLASS_SPEC_INDEXED_TYPE(spec);

		/* the number of the fixed fields for a non-pointer object are supported.
		 * the fixed fields of a pointer object holds named instance variables 
		 * and a non-pointer object is facilitated with the fixed fields of the size
		 * specified in the class description like #byte(5), #word(10).
		 * 
		 * when it comes to spec decoding, there is no difference between a pointer
		 * object and a non-pointer object */

		if (num_flexi_fields > MOO_MAX_INDEXED_INSTVARS(num_fixed_fields))
		{
			moo_seterrbfmt (moo, MOO_EINVAL, "number of flexi-fields(%zu) too big for a class %O", num_flexi_fields, _class);
			return -1;
		}
	}
	else
	{
		/* named instance variables only. treat it as if it is an
		 * indexable class with no variable data */
		indexed_type = MOO_OBJ_TYPE_OOP;

		if (num_flexi_fields > 0)
		{
			moo_seterrbfmt (moo, MOO_EPERM, "flexi-fields(%zu) disallowed for a class %O", num_flexi_fields, _class); 
			return -1;
		}
	}

	MOO_ASSERT (moo, num_fixed_fields + num_flexi_fields <= MOO_OBJ_SIZE_MAX);
	*type = indexed_type;
	*outlen = num_fixed_fields + num_flexi_fields;
	return 0; 
}

moo_oop_t moo_instantiate (moo_t* moo, moo_oop_class_t _class, const void* vptr, moo_oow_t vlen)
{
	moo_oop_t oop;
	moo_obj_type_t type;
	moo_oow_t alloclen;
	moo_oow_t tmp_count = 0;

	MOO_ASSERT (moo, moo->_nil != MOO_NULL);

	if (decode_spec(moo, _class, vlen, &type, &alloclen) <= -1) return MOO_NULL;

	moo_pushvolat (moo, (moo_oop_t*)&_class); tmp_count++;

	switch (type)
	{
		case MOO_OBJ_TYPE_OOP:
			/* both the fixed part(named instance variables) and 
			 * the variable part(indexed instance variables) are allowed. */
			oop = moo_allocoopobj(moo, alloclen);
			if (oop)
			{
				/* initialize named instance variables with default values */
				if (_class->initv[0] != moo->_nil)
				{
					moo_oow_t i = MOO_OBJ_GET_SIZE(_class->initv[0]);

					/* [NOTE] i don't deep-copy initial values.
					 *   if you change the contents of compound values like arrays,
					 *   it affects subsequent instantiation of the class. 
					 *   it's important that the compiler should mark compound initial
					 *   values read-only. */   
					while (i > 0)
					{
						--i;
						MOO_STORE_OOP (moo, MOO_OBJ_GET_OOP_PTR(oop, i), MOO_OBJ_GET_OOP_VAL(_class->initv[0], i));
					}
				}
			}

			MOO_ASSERT (moo, vptr == MOO_NULL);
			/*
			This function is not GC-safe. so i don't want to initialize
			the payload of a pointer object. The caller can call this
			function and initialize payloads then.
			if (oop && vptr && vlen > 0)
			{
				moo_oop_oop_t hdr = (moo_oop_oop_t)oop;
				MOO_MEMCPY (&hdr->slot[named_instvar], vptr, vlen * MOO_SIZEOF(moo_oop_t));
			}

			For the above code to work, it should protect the elements of 
			the vptr array with moo_pushvolat(). So it might be better 
			to disallow a non-NULL vptr when indexed_type is OOP. See
			the assertion above this comment block.
			*/
			break;

		case MOO_OBJ_TYPE_CHAR:
			oop = moo_alloccharobj(moo, vptr, alloclen);
			break;

		case MOO_OBJ_TYPE_BYTE:
			oop = moo_allocbyteobj(moo, vptr, alloclen);
			break;

		case MOO_OBJ_TYPE_HALFWORD:
			oop = moo_allochalfwordobj(moo, vptr, alloclen);
			break;

		case MOO_OBJ_TYPE_WORD:
			oop = moo_allocwordobj(moo, vptr, alloclen);
			break;

		default:
			moo_seterrnum (moo, MOO_EINTERN);
			oop = MOO_NULL;
			break;
	}

	if (oop) 
	{
		moo_ooi_t spec;
		MOO_OBJ_SET_CLASS (oop, (moo_oop_t)_class);
		spec = MOO_OOP_TO_SMOOI(_class->spec);
		if (MOO_CLASS_SPEC_IS_IMMUTABLE(spec)) MOO_OBJ_SET_FLAGS_RDONLY (oop, 1);
		if (MOO_CLASS_SPEC_IS_UNCOPYABLE(spec)) MOO_OBJ_SET_FLAGS_UNCOPYABLE (oop, 1);
	}
	moo_popvolats (moo, tmp_count);
	return oop;
}

moo_oop_t moo_instantiatewithtrailer (moo_t* moo, moo_oop_class_t _class, moo_oow_t vlen, const moo_oob_t* trptr, moo_oow_t trlen)
{
	moo_oop_t oop;
	moo_obj_type_t type;
	moo_oow_t alloclen;
	moo_oow_t tmp_count = 0;

	MOO_ASSERT (moo, moo->_nil != MOO_NULL);

	if (decode_spec(moo, _class, vlen, &type, &alloclen) <= -1) return MOO_NULL;

	moo_pushvolat (moo, (moo_oop_t*)&_class); tmp_count++;

	switch (type)
	{
		case MOO_OBJ_TYPE_OOP:
			oop = moo_allocoopobjwithtrailer(moo, alloclen, trptr, trlen);
			if (oop)
			{
				/* initialize named instance variables with default values */
				if (_class->initv[0] != moo->_nil)
				{
					moo_oow_t i = MOO_OBJ_GET_SIZE(_class->initv[0]);

					/* [NOTE] i don't deep-copy initial values.
					 *   if you change the contents of compound values like arrays,
					 *   it affects subsequent instantiation of the class. 
					 *   it's important that the compiler should mark compound initial
					 *   values read-only. */   
					while (i > 0)
					{
						--i;
						MOO_STORE_OOP (moo, MOO_OBJ_GET_OOP_PTR(oop, i), MOO_OBJ_GET_OOP_VAL(_class->initv[0], i));
					}
				}
			}

			break;

		default:
			MOO_DEBUG3 (moo, "Not allowed to instantiate a non-pointer object of the %.*js class with trailer %zu\n",
				MOO_OBJ_GET_SIZE(_class->name),
				MOO_OBJ_GET_CHAR_SLOT(_class->name),
				trlen);

			moo_seterrnum (moo, MOO_EPERM);
			oop = MOO_NULL;
			break;
	}

	if (oop)
	{
		moo_ooi_t spec;
		MOO_OBJ_SET_CLASS (oop, _class);
		spec = MOO_OOP_TO_SMOOI(_class->spec);
		if (MOO_CLASS_SPEC_IS_IMMUTABLE(spec)) MOO_OBJ_SET_FLAGS_RDONLY (oop, 1);
		/* the object with trailer is to to uncopyable in moo_allocoopobjwithtrailer() so no need to check/set it again here 
		if (MOO_CLASS_SPEC_IS_UNCOPYABLE(spec)) MOO_OBJ_SET_FLAGS_UNCOPYABLE (oop, 1);
 		*/
	}

	moo_popvolats (moo, tmp_count);
	return oop;
}

moo_oop_t moo_oowtoptr (moo_t* moo, moo_oow_t num)
{
	moo_oop_t ret;

	if (MOO_IN_SMPTR_RANGE(num)) return MOO_SMPTR_TO_OOP(num);
	ret = moo_instantiate(moo, moo->_large_pointer, MOO_NULL, 0);
	if (!ret) return MOO_NULL;
	MOO_OBJ_SET_WORD_VAL(ret, 0, num);
	return ret;
}

int moo_ptrtooow (moo_t* moo, moo_oop_t ptr, moo_oow_t* num)
{
	if (MOO_OOP_IS_SMPTR(ptr)) 
	{
		*num = (moo_oow_t)MOO_OOP_TO_SMPTR(ptr);
		return 0;
	}

	if (MOO_CLASSOF(moo, ptr) == moo->_large_pointer)
	{
		MOO_ASSERT (moo, MOO_OBJ_GET_SIZE(ptr) == 1);
		*num = MOO_OBJ_GET_WORD_VAL(ptr, 0);
		return 0;
	}

	moo_seterrnum (moo, MOO_EINVAL);
	return -1;
}
