/*
 * $Id$
 *
    Copyright (c) 2014-2018 Chung, Hyung-Hwan. All rights reserved.

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

void moo_dumpsymtab (moo_t* moo)
{
	moo_oow_t i;
	moo_oop_char_t symbol;

	MOO_DEBUG0 (moo, "--------------------------------------------\n");
	MOO_DEBUG1 (moo, "MOO Symbol Table %zu\n", MOO_OBJ_GET_SIZE(moo->symtab->bucket));
	MOO_DEBUG0 (moo, "--------------------------------------------\n");

	for (i = 0; i < MOO_OBJ_GET_SIZE(moo->symtab->bucket); i++)
	{
		symbol = (moo_oop_char_t)MOO_OBJ_GET_OOP_VAL(moo->symtab->bucket, i);
		if ((moo_oop_t)symbol != moo->_nil)
		{
			MOO_DEBUG2 (moo, " %07zu %O\n", i, symbol);
		}
	}

	MOO_DEBUG0 (moo, "--------------------------------------------\n");
}

void moo_dumpdic (moo_t* moo, moo_oop_dic_t dic, const moo_bch_t* title)
{
	moo_oow_t i;
	moo_oop_association_t ass;

	MOO_DEBUG0 (moo, "--------------------------------------------\n");
	MOO_DEBUG2 (moo, "%s %zu\n", title, MOO_OBJ_GET_SIZE(dic->bucket));
	MOO_DEBUG0 (moo, "--------------------------------------------\n");

	for (i = 0; i < MOO_OBJ_GET_SIZE(dic->bucket); i++)
	{
		ass = (moo_oop_association_t)MOO_OBJ_GET_OOP_VAL(dic->bucket, i);
		if ((moo_oop_t)ass != moo->_nil)
		{
			MOO_DEBUG2 (moo, " %07zu %O\n", i, ass->key);
		}
	}
	MOO_DEBUG0 (moo, "--------------------------------------------\n");
}


/* TODO: moo_loaddbginfofromimage() -> load debug information from compiled image?
moo_storedbginfotoimage()? -> store debug information to compiled image?
moo_compactdbginfo()? -> compact debug information by scaning dbginfo data. find class and method. if not found, drop the portion.
*/

int moo_initdbginfo (moo_t* moo, moo_oow_t capa)
{
	moo_dbginfo_t* tmp;

	if (capa < MOO_SIZEOF(*tmp)) capa = MOO_SIZEOF(*tmp);

	tmp = (moo_dbginfo_t*)moo_callocmem(moo, capa);
	if (!tmp) return -1;

	tmp->_capa = capa;
	tmp->_len = MOO_SIZEOF(*tmp);
	tmp->_last_class = 0;
	tmp->_last_file = 0;

	moo->dbginfo = tmp;
	return 0;
}

void moo_finidbginfo (moo_t* moo)
{
	if (moo->dbginfo)
	{
		moo_freemem (moo, moo->dbginfo);
		moo->dbginfo = MOO_NULL;
	}
}

static MOO_INLINE secure_dbginfo_space (moo_t* moo, moo_oow_t req_bytes)
{
	if (moo->dbginfo->_capa - moo->dbginfo->_len < req_bytes)
	{
		moo_dbginfo_t* tmp;
		moo_oow_t newcapa;

		newcapa = moo->dbginfo->_len + req_bytes;
		newcapa = MOO_ALIGN_POW2(newcapa, 65536); /* TODO: make the align value configurable */
		tmp = moo_reallocmem(moo, moo->dbginfo, newcapa);
		if (!tmp) return -1;

		moo->dbginfo = tmp;
		moo->dbginfo->_capa = newcapa;
	}

	return 0;
}

int moo_addclasstodbginfo (moo_t* moo, const moo_ooch_t* class_name, moo_oow_t* start_offset)
{
	moo_oow_t name_len, name_bytes, name_bytes_aligned, req_bytes;
	moo_dbginfo_class_t* di;

	if (!moo->dbginfo) return 0; /* debug information is disabled*/

	name_len = moo_count_oocstr(class_name);
	name_bytes = name_len * MOO_SIZEOF(*class_name);
	name_bytes_aligned = MOO_ALIGN_POW2(name_bytes, MOO_SIZEOF_OOW_T);
	req_bytes = MOO_SIZEOF(moo_dbginfo_class_t) + name_bytes_aligned;

	if (secure_dbginfo_space(moo, req_bytes) <= -1) return -1;

	di = (moo_dbginfo_class_t*)&((moo_uint8_t*)moo->dbginfo)[moo->dbginfo->_len];
	di->_type = MOO_DBGINFO_MAKE_TYPE(MOO_DBGINFO_TYPE_CODE_CLASS, 0);
	di->_len = name_len;
	di->_next = moo->dbginfo->_last_class;

	MOO_MEMCPY (di + 1, class_name, name_len * MOO_SIZEOF(*class_name));
/* TODO: set '\0' after file names? */

	moo->dbginfo->_last_class = moo->dbginfo->_len;
	moo->dbginfo->_len += req_bytes;

	if (start_offset) *start_offset = moo->dbginfo->_last_class;
	return 0;
}

int moo_addfiletodbginfo (moo_t* moo, const moo_ooch_t* file_name, moo_oow_t* start_offset)
{
	moo_oow_t name_len, name_bytes, name_bytes_aligned, req_bytes;
	moo_dbginfo_file_t* di;

	if (!moo->dbginfo) return 0; /* debug information is disabled*/

	name_len = moo_count_oocstr(file_name);
	name_bytes = name_len * MOO_SIZEOF(*file_name);
	name_bytes_aligned = MOO_ALIGN_POW2(name_bytes, MOO_SIZEOF_OOW_T);
	req_bytes = MOO_SIZEOF(moo_dbginfo_file_t) + name_bytes_aligned;

	if (secure_dbginfo_space(moo, req_bytes) <= -1) return -1;

	di = (moo_dbginfo_file_t*)&((moo_uint8_t*)moo->dbginfo)[moo->dbginfo->_len];
	di->_type = MOO_DBGINFO_MAKE_TYPE(MOO_DBGINFO_TYPE_CODE_FILE, 0);
	di->_len = name_len;
	di->_next = moo->dbginfo->_last_file;

	MOO_MEMCPY (di + 1, file_name, name_len * MOO_SIZEOF(*file_name));
/* TODO: set '\0' after file names? */

	moo->dbginfo->_last_file = moo->dbginfo->_len;
	moo->dbginfo->_len += req_bytes;

	if (start_offset) *start_offset = moo->dbginfo->_last_file;
	return 0;
}

#if 0
int moo_addmethodtodbginfo (moo_t* moo, const moo_ooch_t* class_name, const moo_ooch_t* method_name, const moo_ooch_t* file_name)
{
	moo_oow_t name_len, name_bytes, name_bytes_aligned, req_bytes;
	moo_dbginfo_file_t* di;

	if (!moo->dbginfo) return 0; /* debug information is disabled*/

	name_len = moo_count_oocstr(file_name);
	name_bytes = name_len * MOO_SIZEOF(*file_name);
	name_bytes_aligned = MOO_ALIGN_POW2(name_bytes, MOO_SIZEOF_OOW_T);
	req_bytes = MOO_SIZEOF(moo_dbginfo_file_t) + name_bytes_aligned;

	if (secure_dbginfo_space(moo, req_bytes) <= -1) return -1;

	di = (moo_dbginfo_file_t*)&((moo_uint8_t*)moo->dbginfo)[moo->dbginfo->_len];
	di->_type = MOO_DBGINFO_MAKE_TYPE(MOO_DBGINFO_TYPE_CODE_FILE, 0);
	di->_len = name_len;
	di->_next = moo->dbginfo->_last_file;

	MOO_MEMCPY (di + 1, file_name, name_len * MOO_SIZEOF(*file_name));
/* TODO: set '\0' after file names? */

	moo->dbginfo->_last_file = moo->dbginfo->_len;
	moo->dbginfo->_len += req_bytes;

	if (start_offset) *start_offset = moo->dbginfo->_last_file;
	return 0;
}
#endif