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
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAfRRANTIES
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

/* -------------------------------------------------------------------------------- 
 * COMPARISON
 * -------------------------------------------------------------------------------- */
 
moo_pfrc_t moo_pf_identical (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, b;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	b = (rcv == arg)? moo->_true: moo->_false;

	MOO_STACK_SETRET (moo, nargs, b);
	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_not_identical (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg, b;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	b = (rcv != arg)? moo->_true: moo->_false;

	MOO_STACK_SETRET (moo, nargs, b);
	return MOO_PF_SUCCESS;
}

static int _equal_objects (moo_t* moo, moo_oop_t rcv, moo_oop_t arg)
{
	int rtag;

	if (rcv == arg) return 1; /* identical. so equal */

	rtag = MOO_OOP_GET_TAG(rcv);
	if (rtag != MOO_OOP_GET_TAG(arg)) return 0;

	switch (rtag)
	{
		case MOO_OOP_TAG_SMOOI:
			return MOO_OOP_TO_SMOOI(rcv) == MOO_OOP_TO_SMOOI(arg)? 1: 0;

		case MOO_OOP_TAG_SMPTR:
			return MOO_OOP_TO_SMPTR(rcv) == MOO_OOP_TO_SMPTR(arg)? 1: 0;

		case MOO_OOP_TAG_CHAR:
			return MOO_OOP_TO_CHAR(rcv) == MOO_OOP_TO_CHAR(arg)? 1: 0;

		case MOO_OOP_TAG_ERROR:
			return MOO_OOP_TO_ERROR(rcv) == MOO_OOP_TO_ERROR(arg)? 1: 0;

		default:
		{
			MOO_ASSERT (moo, MOO_OOP_IS_POINTER(rcv));

			if (MOO_OBJ_GET_CLASS(rcv) != MOO_OBJ_GET_CLASS(arg)) return 0; /* different class, not equal */
			MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_TYPE(rcv) == MOO_OBJ_GET_FLAGS_TYPE(arg));

			if (MOO_OBJ_GET_CLASS(rcv) == moo->_class && rcv != arg) 
			{
				/* a class object are supposed to be unique */
				return 0;
			}
			if (MOO_OBJ_GET_SIZE(rcv) != MOO_OBJ_GET_SIZE(arg)) return 0; /* different size, not equal */

			switch (MOO_OBJ_GET_FLAGS_TYPE(rcv))
			{
				case MOO_OBJ_TYPE_BYTE:
				case MOO_OBJ_TYPE_CHAR:
				case MOO_OBJ_TYPE_HALFWORD:
				case MOO_OBJ_TYPE_WORD:
					return (MOO_MEMCMP (MOO_OBJ_GET_BYTE_SLOT(rcv), MOO_OBJ_GET_BYTE_SLOT(arg), MOO_BYTESOF(moo,rcv)) == 0)? 1: 0;

				default:
				{
					if (rcv == moo->_nil) return arg == moo->_nil? 1: 0;
					if (rcv == moo->_true) return arg == moo->_true? 1: 0;
					if (rcv == moo->_false) return arg == moo->_false? 1: 0;

					/* MOO_OBJ_TYPE_OOP, ... */
					MOO_ASSERT (moo, MOO_OBJ_GET_FLAGS_TYPE(rcv) == MOO_OBJ_TYPE_OOP);

				#if 1
					moo_seterrbfmt (moo, MOO_ENOIMPL, "no builtin comparison implemented for %O and %O", rcv, arg); /* TODO: better error code */
					return -1;
				#else
					for (i = 0; i < MOO_OBJ_GET_SIZE(rcv); i++)
					{
						/* TODO: remove recursion */

						/* NOTE: even if the object implements the equality method, 
						 * this primitive method doesn't honor it. */
						if (!_equal_objects(moo, ((moo_oop_oop_t)rcv)->slot[i], ((moo_oop_oop_t)arg)->slot[i])) return 0;
					}
					return 1;
				#endif
				}
			}
		}
	}
}

moo_pfrc_t moo_pf_equal (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg;
	int n;

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	n = _equal_objects (moo, rcv, arg);
	if (n <= -1) return MOO_PF_FAILURE;

	MOO_STACK_SETRET (moo, nargs, (n? moo->_true: moo->_false));
	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_not_equal (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, arg;
	int n;

	rcv = MOO_STACK_GETRCV(moo, nargs);
	arg = MOO_STACK_GETARG(moo, nargs, 0);

	n = _equal_objects (moo, rcv, arg);
	if (n <= -1) return MOO_PF_FAILURE;

	MOO_STACK_SETRET (moo, nargs, (n? moo->_false: moo->_true));
	return MOO_PF_SUCCESS;
}


/* -------------------------------------------------------------------------------- 
 * INSTANTIATION
 * -------------------------------------------------------------------------------- */

moo_pfrc_t moo_pf_basic_new (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_class_t _class;
	moo_oop_t szoop, obj;
	moo_oow_t size = 0; /* size of the variable/indexed part */

	_class = (moo_oop_class_t)MOO_STACK_GETRCV(moo, nargs);
	if (MOO_CLASSOF(moo, _class) != moo->_class) 
	{
		/* the receiver is not a class object */
		moo_seterrbfmt (moo, MOO_EMSGRCV, "non-class receiver - %O", _class);
		return MOO_PF_FAILURE;
	}

	/* check if #limited is set on the class */
	if (MOO_CLASS_SELFSPEC_FLAGS(MOO_OOP_TO_SMOOI(_class->selfspec)) & MOO_CLASS_SELFSPEC_FLAG_LIMITED)
	{
		moo_seterrbfmt (moo, MOO_EPERM, "limited receiver - %O", _class);
		return MOO_PF_FAILURE;
	}

	if (nargs >= 1)
	{
		szoop = MOO_STACK_GETARG(moo, nargs, 0);
		if (moo_inttooow (moo, szoop, &size) <= 0)
		{
			/* integer out of range or not integer */
			moo_seterrbfmt (moo, MOO_EINVAL, "size out of range or not integer - %O", szoop);
			return MOO_PF_FAILURE;
		}
	}

	if (MOO_OOP_IS_SMOOI(((moo_oop_class_t)_class)->trsize)) 
	{
		obj = moo_instantiatewithtrailer (moo, _class, size, MOO_NULL, MOO_OOP_TO_SMOOI(((moo_oop_class_t)_class)->trsize));
	}
	else 
	{
		/* moo_instantiate() will ignore size if the instance specification 
		 * disallows indexed(variable) parts. */
		/* TODO: should i check the specification before calling 
		*       moo_instantiate()? */
		obj = moo_instantiate (moo, _class, MOO_NULL, size);
	}
	if (!obj) return MOO_PF_FAILURE;

	MOO_STACK_SETRET (moo, nargs, obj);
	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_shallow_copy (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, obj;

	MOO_ASSERT (moo, nargs ==  0);
	rcv = MOO_STACK_GETRCV (moo, nargs);

	obj = moo_shallowcopy (moo, rcv);
	if (!obj) return MOO_PF_FAILURE;

	MOO_STACK_SETRET (moo, nargs, obj);
	return MOO_PF_SUCCESS;
}


/* -------------------------------------------------------------------------------- 
 * BASIC ACCESS
 * -------------------------------------------------------------------------------- */

moo_pfrc_t moo_pf_class (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv;
	rcv = MOO_STACK_GETRCV(moo, nargs);
	MOO_STACK_SETRET (moo, nargs, (moo_oop_t)MOO_CLASSOF(moo, rcv));
	return MOO_PF_SUCCESS;
}


moo_pfrc_t moo_pf_basic_size (moo_t* moo, moo_ooi_t nargs)
{
	/* return the number of indexable fields */

	moo_oop_t rcv, sz;

	MOO_ASSERT (moo, nargs == 0);

	rcv = MOO_STACK_GETRCV (moo, nargs);

	if (!MOO_OOP_IS_POINTER(rcv))
	{
		sz = MOO_SMOOI_TO_OOP(0);
	}
	else
	{
		sz = moo_oowtoint (moo, MOO_OBJ_GET_SIZE(rcv));
		if (!sz) return MOO_PF_FAILURE;
	}

	MOO_STACK_SETRET(moo, nargs, sz);
	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_basic_at (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, pos, v;
	moo_oow_t idx;

	MOO_ASSERT (moo, nargs == 1);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	if (!MOO_OOP_IS_POINTER(rcv))
	{
		/* the receiver is a special numeric object, not a normal pointer */
		moo_seterrbfmt (moo, MOO_EMSGRCV, "receiver not indexable - %O", rcv);
		return MOO_PF_FAILURE;
	}

	pos = MOO_STACK_GETARG(moo, nargs, 0);
	if (moo_inttooow (moo, pos, &idx) <= 0)
	{
		/* negative integer or not integer */
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid position - %O", pos);
		return MOO_PF_FAILURE;
	}
	if (idx >= MOO_OBJ_GET_SIZE(rcv))
	{
		/* index out of range */
		moo_seterrbfmt (moo, MOO_ERANGE, "position out of bound - %O", pos);
		return MOO_PF_FAILURE;
	}

	switch (MOO_OBJ_GET_FLAGS_TYPE(rcv))
	{
		case MOO_OBJ_TYPE_BYTE:
			v = MOO_SMOOI_TO_OOP(((moo_oop_byte_t)rcv)->slot[idx]);
			break;

		case MOO_OBJ_TYPE_CHAR:
			v = MOO_CHAR_TO_OOP(((moo_oop_char_t)rcv)->slot[idx]);
			break;

		case MOO_OBJ_TYPE_HALFWORD:
			/* TODO: LargeInteger if the halfword is too large */
			v = MOO_SMOOI_TO_OOP(((moo_oop_halfword_t)rcv)->slot[idx]);
			break;

		case MOO_OBJ_TYPE_WORD:
			v = moo_oowtoint (moo, ((moo_oop_word_t)rcv)->slot[idx]);
			if (!v) return MOO_PF_FAILURE;
			break;

		case MOO_OBJ_TYPE_OOP:
			v = ((moo_oop_oop_t)rcv)->slot[idx];
			break;

		default:
			moo_seterrnum (moo, MOO_EINTERN);
			return MOO_PF_HARD_FAILURE;
	}

	MOO_STACK_SETRET (moo, nargs, v);
	return MOO_PF_SUCCESS;
}

moo_pfrc_t moo_pf_basic_at_put (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, pos, val;
	moo_oow_t idx;

	MOO_ASSERT (moo, nargs == 2);

	rcv = MOO_STACK_GETRCV(moo, nargs);
	if (!MOO_OOP_IS_POINTER(rcv))
	{
		/* the receiver is a special numeric object, not a normal pointer */
		moo_seterrbfmt (moo, MOO_EMSGRCV, "receiver not indexable - %O", rcv);
		return MOO_PF_FAILURE;
	}

	if (MOO_OBJ_GET_FLAGS_RDONLY(rcv))
	{
/* TODO: better error handling */
		moo_seterrbfmt (moo, MOO_EPERM, "now allowed to change a read-only object - %O", rcv);
		return MOO_PF_FAILURE;
	}

	pos = MOO_STACK_GETARG(moo, nargs, 0);
	val = MOO_STACK_GETARG(moo, nargs, 1);

	if (moo_inttooow (moo, pos, &idx) <= 0)
	{
		/* negative integer or not integer */
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid position - %O", pos);
		return MOO_PF_FAILURE;
	}
	if (idx >= MOO_OBJ_GET_SIZE(rcv))
	{
		/* index out of range */
		moo_seterrbfmt (moo, MOO_ERANGE, "position out of bound - %O", pos);
		return MOO_PF_FAILURE;
	}

	switch (MOO_OBJ_GET_FLAGS_TYPE(rcv))
	{
		case MOO_OBJ_TYPE_BYTE:
			if (!MOO_OOP_IS_SMOOI(val))
			{
				/* the value is not a character */
				moo_seterrnum (moo, MOO_EINVAL);
				return MOO_PF_FAILURE;
			}
/* TOOD: must I check the range of the value? */
			((moo_oop_char_t)rcv)->slot[idx] = MOO_OOP_TO_SMOOI(val);
			break;

		case MOO_OBJ_TYPE_CHAR:
			if (!MOO_OOP_IS_CHAR(val))
			{
				/* the value is not a character */
				moo_seterrnum (moo, MOO_EINVAL);
				return MOO_PF_FAILURE;
			}
			((moo_oop_char_t)rcv)->slot[idx] = MOO_OOP_TO_CHAR(val);
			break;

		case MOO_OBJ_TYPE_HALFWORD:
			if (!MOO_OOP_IS_SMOOI(val))
			{
				/* the value is not a number */
				moo_seterrnum (moo, MOO_EINVAL);
				return MOO_PF_FAILURE;
			}

			/* if the small integer is too large, it will get truncated */
			((moo_oop_halfword_t)rcv)->slot[idx] = MOO_OOP_TO_SMOOI(val);
			break;

		case MOO_OBJ_TYPE_WORD:
		{
			moo_oow_t w;

			if (moo_inttooow (moo, val, &w) <= 0)
			{
				/* the value is not a number, out of range, or negative */
				moo_seterrnum (moo, MOO_EINVAL);
				return MOO_PF_FAILURE;
			}
			((moo_oop_word_t)rcv)->slot[idx] = w;
			break;
		}

		case MOO_OBJ_TYPE_OOP:
			((moo_oop_oop_t)rcv)->slot[idx] = val;
			break;

		default:
			moo_seterrnum (moo, MOO_EINTERN);
			return MOO_PF_HARD_FAILURE;
	}

/* TODO: return receiver or value? */
	MOO_STACK_SETRET (moo, nargs, val);
	return MOO_PF_SUCCESS;
}


/* -------------------------------------------------------------------------------- 
 * BASIC QUERY
 * -------------------------------------------------------------------------------- */

moo_pfrc_t moo_pf_is_kind_of (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, _class;

	rcv = MOO_STACK_GETRCV(moo, nargs);
	_class = MOO_STACK_GETARG(moo, nargs, 0);

	if (MOO_CLASSOF(moo, _class) == moo->_class &&
	    moo_iskindof(moo, rcv, (moo_oop_class_t)_class))
	{
		MOO_STACK_SETRET (moo, nargs, moo->_true);
	}
	else
	{
		MOO_STACK_SETRET (moo, nargs, moo->_false);
	}

	return MOO_PF_SUCCESS;
}


moo_pfrc_t moo_pf_responds_to (moo_t* moo, moo_ooi_t nargs)
{
	moo_oop_t rcv, selector;
	moo_oocs_t mthname;

	rcv = MOO_STACK_GETRCV(moo, nargs);
	selector = MOO_STACK_GETARG(moo, nargs, 0);

	/*if (MOO_CLASSOF(moo,selector) != moo->_symbol)*/
	if (!MOO_OBJ_IS_CHAR_POINTER(selector))
	{
		moo_seterrbfmt (moo, MOO_EINVAL, "invalid argument - %O", selector);
		return MOO_PF_FAILURE;
	}

	mthname.ptr = MOO_OBJ_GET_CHAR_SLOT(selector);
	mthname.len = MOO_OBJ_GET_SIZE(selector);
	if (moo_findmethod (moo, rcv, &mthname, 0))
	{
		MOO_STACK_SETRET (moo, nargs, moo->_true);
	}
	else
	{
		MOO_STACK_SETRET (moo, nargs, moo->_false);
	}

	return MOO_PF_SUCCESS;
}
