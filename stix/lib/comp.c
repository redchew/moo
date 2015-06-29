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

#include "stix-prv.h"

#define TOKEN_NAME_ALIGN     256
#define CLASS_BUFFER_ALIGN   8 /* 256 */ /*TODO: change 8 to 256 */
#define LITERAL_BUFFER_ALIGN 8 /* 256 */
#define CODE_BUFFER_ALIGN    8 /* 256 */
#define BALIT_BUFFER_ALIGN   8 /* 256 */
#define ARLIT_BUFFER_ALIGN   8 /* 256 */

/* initial method dictionary size */
#define INSTANCE_METHOD_DICTIONARY_SIZE 256 /* TODO: choose the right size */
#define CLASS_METHOD_DICTIONARY_SIZE 128 /* TODO: choose the right size */

enum class_mod_t
{
	CLASS_INDEXED   = (1 << 0),
	CLASS_EXTENDED  = (1 << 1)
};

enum mth_type_t
{
	MTH_INSTANCE,
	MTH_CLASS
};

enum var_type_t
{
	/* NEVER Change the order and the value of 3 items below.
	 * stix->c->cls.vars and stix->c->cls.var_count relies on them. */
	VAR_INSTANCE   = 0,
	VAR_CLASS      = 1,
	VAR_CLASSINST  = 2,
	/* NEVER Change the order and the value of 3 items above. */

	VAR_GLOBAL,
	VAR_ARGUMENT,
	VAR_TEMPORARY
};
typedef enum var_type_t var_type_t;

struct var_info_t
{
	var_type_t       type;
	stix_ssize_t     pos;
	stix_oop_class_t cls; /* useful if type is VAR_CLASS. note STIX_NULL indicates the self class. TODO: use it for GLOBAL?? */
};
typedef struct var_info_t var_info_t;

static struct voca_t
{
	stix_oow_t len;
	stix_uch_t str[11];
} vocas[] = {
	{  4, { 'b','y','t','e'                                               } },
	{  9, { 'c','h','a','r','a','c','t','e','r'                           } },
	{  5, { 'c','l','a','s','s'                                           } },
	{  9, { 'c','l','a','s','s','i','n','s','t'                           } },
	{  3, { 'd','c','l'                                                   } },
	{  7, { 'd','e','c','l','a','r','e'                                   } },
	{  5, { 'f','a','l','s','e'                                           } },
	{  7, { 'i','n','c','l','u','d','e'                                   } },
	{  4, { 'm','a','i','n'                                               } },
	{  6, { 'm','e','t','h','o','d'                                       } },
	{  3, { 'm','t','h'                                                   } },
	{  3, { 'n','i','l'                                                   } },
	{  7, { 'p','o','i','n','t','e','r'                                   } },
	{ 10, { 'p','r','i','m','i','t','i','v','e',':'                       } },
	{  4, { 's','e','l','f'                                               } },
	{  5, { 's','u','p','e','r'                                           } },
	{ 11, { 't','h','i','s','C','o','n','t','e','x','t'                   } },
	{  4, { 't','r','u','e'                                               } },
	{  4, { 'w','o','r','d'                                               } },

	{  1, { '|'                                                           } },
	{  1, { '>'                                                           } },
	{  1, { '<'                                                           } },

	{  5, { '<','E','O','F','>'                                           } }
};

enum voca_id_t
{
	VOCA_BYTE,
	VOCA_CHARACTER,
	VOCA_CLASS,
	VOCA_CLASSINST,
	VOCA_DCL,
	VOCA_DECLARE,
	VOCA_FALSE,
	VOCA_INCLUDE,
	VOCA_MAIN,
	VOCA_METHOD,
	VOCA_MTH,
	VOCA_NIL,
	VOCA_POINTER,
	VOCA_PRIMITIVE_COLON,
	VOCA_SELF,
	VOCA_SUPER,
	VOCA_THIS_CONTEXT,
	VOCA_TRUE,
	VOCA_WORD,

	VOCA_VBAR,
	VOCA_GT,
	VOCA_LT,

	VOCA_EOF
};
typedef enum voca_id_t voca_id_t;

static int compile_block_statement (stix_t* stix);
static int compile_method_statement (stix_t* stix);
static int compile_method_expression (stix_t* stix, int pop);
static int add_literal (stix_t* stix, stix_oop_t lit, stix_size_t* index);

static STIX_INLINE int is_spacechar (stix_uci_t c)
{
	/* TODO: handle other space unicode characters */
	switch (c)
	{
		case ' ':
		case '\f': /* formfeed */
		case '\n': /* linefeed */
		case '\r': /* carriage return */
		case '\t': /* horizon tab */
		case '\v': /* vertical tab */
			return 1;

		default:
			return 0;
	}
}

static STIX_INLINE int is_alphachar (stix_uci_t c)
{
/* TODO: support full unicode */
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static STIX_INLINE int is_digitchar (stix_uci_t c)
{
/* TODO: support full unicode */
	return (c >= '0' && c <= '9');
}

static STIX_INLINE int is_alnumchar (stix_uci_t c)
{
/* TODO: support full unicode */
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static STIX_INLINE int is_binselchar (stix_uci_t c)
{
	/*
	 * binary-selector-character :=
	 * 	'!' | '%' | '&' | '*' | '+' | ',' | 
	 * 	'/' | '<' | '>' | '=' | '?' | '@' | 
	 * 	'\' | '~' | '|' | '-'
	 */

	switch (c)
	{
		case '!':
		case '%':
		case '&':
		case '*':
		case '+':
		case ',':
		case '/': 
		case '<':
		case '>':
		case '=':
		case '?':
		case '@':
		case '\\':
		case '|':
		case '~':
		case '-':
			return 1;

		default:
			return 0;
	}
}

static STIX_INLINE int is_closing_char (stix_uci_t c)
{
	switch (c)
	{
		case '.':
		case ']':
		case ')':
		case ';':
		case '\"':
		case '\'':
			return 1;
	
		default:
			return 0;
	}
}

static STIX_INLINE int does_token_name_match (stix_t* stix, voca_id_t id)
{
	return stix->c->tok.name.len == vocas[id].len &&
	       stix_equalchars(stix->c->tok.name.ptr, vocas[id].str, vocas[id].len);
}

static STIX_INLINE int is_token_symbol (stix_t* stix, voca_id_t id)
{
	return stix->c->tok.type == STIX_IOTOK_SYMLIT && does_token_name_match(stix, id);
}

static STIX_INLINE int is_token_ident (stix_t* stix, voca_id_t id)
{
	return stix->c->tok.type == STIX_IOTOK_IDENT && does_token_name_match(stix, id);
}

static STIX_INLINE int is_token_binsel (stix_t* stix, voca_id_t id)
{
	return stix->c->tok.type == STIX_IOTOK_BINSEL && does_token_name_match(stix, id);
}

static STIX_INLINE int is_token_keyword (stix_t* stix, voca_id_t id)
{
	return stix->c->tok.type == STIX_IOTOK_KEYWORD && does_token_name_match(stix, id);
}

static int begin_include (stix_t* fsc);
static int end_include (stix_t* fsc);

static void set_syntax_error (stix_t* stix, stix_synerrnum_t num, const stix_ioloc_t* loc, const stix_ucs_t* tgt)
{
	stix->errnum = STIX_ESYNTAX;
	stix->c->synerr.num = num;
	stix->c->synerr.loc = loc? *loc: stix->c->tok.loc;
	if (tgt) stix->c->synerr.tgt = *tgt;
	else 
	{
		stix->c->synerr.tgt.ptr = STIX_NULL;
		stix->c->synerr.tgt.len = 0;
	}
}

static int copy_string_to (stix_t* stix, const stix_ucs_t* src, stix_ucs_t* dst, stix_size_t* dst_capa, int append, stix_uch_t add_delim)
{
	stix_size_t len, pos;

	if (append)
	{
		pos = dst->len;
		len = dst->len + src->len;
		if (add_delim != '\0') len++;
	}
	else
	{
		pos = 0;
		len = src->len;
	}

	if (len > *dst_capa)
	{
		stix_uch_t* tmp;
		stix_size_t capa;

		capa = STIX_ALIGN(len, CLASS_BUFFER_ALIGN);

		tmp = stix_reallocmem (stix, dst->ptr, STIX_SIZEOF(*tmp) * capa);
		if (!tmp)  return -1;

		dst->ptr = tmp;
		*dst_capa = capa;
	}

	if (append && add_delim) dst->ptr[pos++] = add_delim;
	stix_copychars (&dst->ptr[pos], src->ptr, src->len);
	dst->len = len;
	return 0;
}

static stix_ssize_t find_word_in_string (const stix_ucs_t* haystack, const stix_ucs_t* name)
{
	/* this function is inefficient. but considering the typical number
	 * of arguments and temporary variables, the inefficiency can be 
	 * ignored in my opinion. the overhead to maintain the reverse lookup
	 * table from a name to an index should be greater than this simple
	 * inefficient lookup */

	stix_uch_t* t, * e;
	stix_ssize_t index;
	stix_size_t i;

	t = haystack->ptr;
	e = t + haystack->len;
	index = 0;

	while (t < e)
	{
		while (t < e && is_spacechar(*t)) t++;

		for (i = 0; i < name->len; i++)
		{
			if (t >= e || name->ptr[i] != *t) goto unmatched;
			t++;
		}
		if (t >= e || is_spacechar(*t)) return index;

	unmatched:
		while (t < e)
		{
			if (is_spacechar(*t))
			{
				t++;
				break;
			}
			t++;
		}

		index++;
	}

	return -1;
}

#define CHAR_TO_NUM(c,base) \
	((c >= '0' && c <= '9')? ((c - '0' < base)? (c - '0'): base): \
	 (c >= 'A' && c <= 'Z')? ((c - 'A' + 10 < base)? (c - 'A' + 10): base): \
	 (c >= 'a' && c <= 'z')? ((c - 'a' + 10 < base)? (c - 'a' + 10): base): base)

static int string_to_smint (stix_t* stix, stix_ucs_t* str, int radixed, stix_ooi_t* num)
{
	/* it is not a generic conversion function.
	 * it assumes a certain pre-sanity check on the string
	 * done by the lexical analyzer */

/* TODO: handle floating point numbers, etc, handle radix */
	int v, negsign, base;
	const stix_uch_t* ptr, * end;
	stix_oow_t value, old_value;

	negsign = 0;
	ptr = str->ptr,
	end = str->ptr + str->len;

	STIX_ASSERT (ptr < end);

	if (*ptr == '+' || *ptr == '-')
	{
		negsign = *ptr - '+';
		ptr++;
	}

	if (radixed)
	{
		STIX_ASSERT (ptr < end);

		base = 0;
		do
		{
			base = base * 10 + CHAR_TO_NUM(*ptr, 10);
			ptr++;
		}
		while (*ptr != 'r');

		ptr++;
	}
	else base = 10;

	STIX_ASSERT (ptr < end);

	value = old_value = 0;
	while (ptr < end && (v = CHAR_TO_NUM(*ptr, base)) < base)
	{
		value = value * base + v;
		if (value < old_value) 
		{
			/* overflow must have occurred */
			stix->errnum = STIX_ERANGE;
			return -1;
		}
		old_value = value;
		ptr++;
	}

	if (ptr < end)
	{
		/* trailing garbage? */
		stix->errnum = STIX_EINVAL;
		return -1;
	}

	if (negsign)
	{
		/*if (value > -STIX_SMINT_MIN) */
		if (value > ((stix_oow_t)STIX_SMINT_MAX + 1)) 
		{
			stix->errnum = STIX_ERANGE;
			return -1;
		}
		*num = value;
		*num *= -1;
	}
	else
	{
		if (value > STIX_SMINT_MAX) 
		{
			stix->errnum = STIX_ERANGE;
			return -1;
		}
		*num = value;
	}

	return 0;
}

/* ---------------------------------------------------------------------
 * Tokenizer 
 * --------------------------------------------------------------------- */

#define GET_CHAR(stix) \
	do { if (get_char(stix) <= -1) return -1; } while (0)

#define GET_CHAR_TO(stix,c) \
	do { \
		if (get_char(stix) <= -1) return -1; \
		c = (stix)->c->lxc.c; \
	} while(0)


#define GET_TOKEN(stix) \
	do { if (get_token(stix) <= -1) return -1; } while (0)

#define GET_TOKEN_WITH_ERRRET(stix, v_ret) \
	do { if (get_token(stix) <= -1) return v_ret; } while (0)

#define ADD_TOKEN_STR(stix,s,l) \
	do { if (add_token_str(stix, s, l) <= -1) return -1; } while (0)

#define ADD_TOKEN_CHAR(stix,c) \
	do { if (add_token_char(stix, c) <= -1) return -1; } while (0)


static STIX_INLINE int add_token_str (stix_t* stix, const stix_uch_t* ptr, stix_size_t len)
{
	stix_ucs_t tmp;

	tmp.ptr = (stix_uch_t*)ptr;
	tmp.len = len;
	return copy_string_to (stix, &tmp, &stix->c->tok.name, &stix->c->tok.name_capa, 1, '\0');
}

static STIX_INLINE int add_token_char (stix_t* stix, stix_uch_t c)
{
	stix_ucs_t tmp;

	tmp.ptr = &c;
	tmp.len = 1;
	return copy_string_to (stix, &tmp, &stix->c->tok.name, &stix->c->tok.name_capa, 1, '\0');
}

static STIX_INLINE void unget_char (stix_t* stix, const stix_iolxc_t* c)
{
	/* Make sure that the unget buffer is large enough */
	STIX_ASSERT (stix->c->nungots < STIX_COUNTOF(stix->c->ungot));
	stix->c->ungot[stix->c->nungots++] = *c;
}

static int get_char (stix_t* stix)
{
	stix_ssize_t n;
	stix_uci_t lc, ec;

	if (stix->c->nungots > 0)
	{
		/* something in the unget buffer */
		stix->c->lxc = stix->c->ungot[--stix->c->nungots];
		return 0;
	}

	if (stix->c->curinp->b.state == -1) 
	{
		stix->c->curinp->b.state = 0;
		return -1;
	}
	else if (stix->c->curinp->b.state == 1) 
	{
		stix->c->curinp->b.state = 0;
		goto return_eof;
	}

	if (stix->c->curinp->b.pos >= stix->c->curinp->b.len)
	{
		n = stix->c->impl (stix, STIX_IO_READ, stix->c->curinp);
		if (n <= -1) return -1;
		
		if (n == 0)
		{
		return_eof:
			stix->c->curinp->lxc.c = STIX_UCI_EOF;
			stix->c->curinp->lxc.l.line = stix->c->curinp->line;
			stix->c->curinp->lxc.l.colm = stix->c->curinp->colm;
			stix->c->curinp->lxc.l.file = stix->c->curinp->name;
			stix->c->lxc = stix->c->curinp->lxc;

			/* indicate that EOF has been read. lxc.c is also set to EOF. */
			return 0; 
		}

		stix->c->curinp->b.pos = 0;
		stix->c->curinp->b.len = n;
	}

	if (stix->c->curinp->lxc.c == STIX_UCI_NL)
	{
		/* if the previous charater was a newline,
		 * increment the line counter and reset column to 1.
		 * incrementing it line number here instead of
		 * updating inp->lxc causes the line number for
		 * TOK_EOF to be the same line as the lxc newline. */
		stix->c->curinp->line++;
		stix->c->curinp->colm = 1;
	}

	lc = stix->c->curinp->buf[stix->c->curinp->b.pos++];
	if (lc == '\n' || lc == '\r')
	{
		/* handle common newline conventions.
		 *   LF+CR
		 *   CR+LF
		 *   LF
		 *   CR
		 */
		if (stix->c->curinp->b.pos >= stix->c->curinp->b.len)
		{
			n = stix->c->impl (stix, STIX_IO_READ, stix->c->curinp);
			if (n <= -1) 
			{
				stix->c->curinp->b.state = -1;
				goto done;
			}
			else if (n == 0)
			{
				stix->c->curinp->b.state = 1;
				goto done;
			}
			else
			{
				stix->c->curinp->b.pos = 0;
				stix->c->curinp->b.len = n;
			}
		}

		ec = (lc == '\n')? '\r': '\n';
		if (stix->c->curinp->buf[stix->c->curinp->b.pos] == ec) stix->c->curinp->b.pos++;

	done:
		lc = STIX_UCI_NL;
	}

	stix->c->curinp->lxc.c = lc;
	stix->c->curinp->lxc.l.line = stix->c->curinp->line;
	stix->c->curinp->lxc.l.colm = stix->c->curinp->colm++;
	stix->c->curinp->lxc.l.file = stix->c->curinp->name;
	stix->c->lxc = stix->c->curinp->lxc;

	return 1; /* indicate that a normal character has been read */
}

static int skip_spaces (stix_t* stix)
{
	while (is_spacechar(stix->c->curinp->lxc.c)) GET_CHAR (stix);
	return 0;
}

static int skip_comment (stix_t* stix)
{
	stix_uci_t c = stix->c->lxc.c;
	stix_iolxc_t lc;

	if (c == '"')
	{
		/* skip up to the closing " */
		do 
		{
			GET_CHAR_TO (stix, c); 
			if (c == STIX_UCI_EOF)
			{
				/* unterminated comment */
				set_syntax_error (stix, STIX_SYNERR_CMTNC, &stix->c->lxc.l, STIX_NULL);
				return -1;
			}
		}
		while (c != '"');

		if (c == '"') GET_CHAR (stix);
		return 1; /* double-quoted comment */
	}

	/* handle #! or ## */
	if (c != '#') return 0; /* not a comment */

	/* save the last character */
	lc = stix->c->lxc;
	/* read a new character */
	GET_CHAR_TO (stix, c);

	if (c == '!' || c == '#') 
	{
		do 
		{
			GET_CHAR_TO (stix, c);
			if (c == STIX_UCI_EOF)
			{
				break;
			}
			else if (c == STIX_UCI_NL)
			{
				GET_CHAR (stix);
				break;
			}
		} 
		while (1);

		return 1; /* single line comment led by ## or #! */
	}

	/* unget '#' */
	unget_char (stix, &stix->c->lxc);
	/* restore the previous state */
	stix->c->lxc = lc;

	return 0;
}

static int get_ident (stix_t* stix, stix_uci_t char_read_ahead)
{
	/*
	 * identifier := alpha-char (alpha-char | digit-char)*
	 * keyword := identifier ":"
	 */

	stix_uci_t c;

	c = stix->c->lxc.c;
	stix->c->tok.type = STIX_IOTOK_IDENT;

	if (char_read_ahead != STIX_UCI_EOF)
	{
		ADD_TOKEN_CHAR(stix, char_read_ahead);
	}

get_more:
	do 
	{
		ADD_TOKEN_CHAR (stix, c);
		GET_CHAR_TO (stix, c);
	} 
	while (is_alnumchar(c));

	if (c == ':') 
	{
		ADD_TOKEN_CHAR (stix, c);
		stix->c->tok.type = STIX_IOTOK_KEYWORD;
		GET_CHAR_TO (stix, c);

		if (stix->c->in_array && is_alnumchar(c)) 
		{
			/* [NOTE]
			 * for an input like #(abc:def 1 2 3), abc:def is returned as
			 * a keyword. it would not be a real keyword even if it were
			 * prefixed with #. it is because it doesn't end with a colon. 
			 */
			goto get_more;
		}
	}
	else
	{
		/* handle reserved words */
		if (is_token_ident(stix, VOCA_SELF))
		{
			stix->c->tok.type = STIX_IOTOK_SELF;
		}
		else if (is_token_ident(stix, VOCA_SUPER))
		{
			stix->c->tok.type = STIX_IOTOK_SUPER;
		}
		else if (is_token_ident(stix, VOCA_NIL))
		{
			stix->c->tok.type = STIX_IOTOK_NIL;
		}
		else if (is_token_ident(stix, VOCA_TRUE))
		{
			stix->c->tok.type = STIX_IOTOK_TRUE;
		}
		else if (is_token_ident(stix, VOCA_FALSE))
		{
			stix->c->tok.type = STIX_IOTOK_FALSE;
		}
		else if (is_token_ident(stix, VOCA_THIS_CONTEXT))
		{
			stix->c->tok.type = STIX_IOTOK_THIS_CONTEXT;
		}
	}

	return 0;
}

static int get_numlit (stix_t* stix, int negated)
{
	/* 
	 * number-literal := number | ("-" number)
	 * number := integer | float | scaledDecimal
	 * integer := decimal-integer  | radix-integer
	 * decimal-integer := digit-char+
	 * radix-integer := radix-specifier "r" radix-digit+
	 * radix-specifier := digit-char+
	 * radix-digit := digit-char | upper-alpha-char
	 *
	 * float :=  mantissa [exponentLetter exponent]
	 * mantissa := digit-char+ "." digit-char+
	 * exponent := ['-'] decimal-integer
	 * exponentLetter := 'e' | 'd' | 'q'
	 * scaledDecimal := scaledMantissa 's' [fractionalDigits]
	 * scaledMantissa := decimal-integer | mantissa
	 * fractionalDigits := decimal-integer
	 */

	stix_uci_t c;
	int radix = 0, r;

	c = stix->c->lxc.c;
	stix->c->tok.type = STIX_IOTOK_NUMLIT;

/*TODO: support a complex numeric literal */
	do 
	{
		if (radix <= 36)
		{
			/* collect the potential radix specifier */
			r = CHAR_TO_NUM (c, 10);
			STIX_ASSERT (r < 10);
			radix = radix * 10 + r;
		}

		ADD_TOKEN_CHAR(stix, c);
		GET_CHAR_TO (stix, c);
	} 
	while (is_digitchar(c));

	if (c == 'r')
	{
		/* radix specifier */

		if (radix < 2 || radix > 36)
		{
			/* no digit after the radix specifier */
			set_syntax_error (stix, STIX_SYNERR_RADIX, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		ADD_TOKEN_CHAR(stix, c);
		GET_CHAR_TO (stix, c);

		if (CHAR_TO_NUM(c, radix) >= radix)
		{
			/* no digit after the radix specifier */
			set_syntax_error (stix, STIX_SYNERR_RADNUMLIT, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		do
		{
			ADD_TOKEN_CHAR(stix, c);
			GET_CHAR_TO (stix, c);
		}
		while (CHAR_TO_NUM(c, radix) < radix);

		stix->c->tok.type = STIX_IOTOK_RADNUMLIT;
	}

/*
 * TODO: handle floating point number
 */
	return 0;
}

static int get_charlit (stix_t* stix)
{
	/* 
	 * character-literal := "$" character
	 * character := normal-character | "'"
	 */

	stix_uci_t c = stix->c->lxc.c; /* even a new-line or white space would be taken */
	if (c == STIX_UCI_EOF) 
	{
		set_syntax_error (stix, STIX_SYNERR_CLTNT, &stix->c->lxc.l, STIX_NULL);
		return -1;
	}

	stix->c->tok.type = STIX_IOTOK_CHARLIT;
	ADD_TOKEN_CHAR(stix, c);
	GET_CHAR (stix);
	return 0;
}

static int get_strlit (stix_t* stix)
{
	/* 
	 * string-literal := single-quote string-character* single-quote
	 * string-character := normal-character | (single-quote single-quote)
	 * single-quote := "'"
	 * normal-character := character-except-single-quote
	 */

	stix_uci_t c = stix->c->lxc.c;
	stix->c->tok.type = STIX_IOTOK_STRLIT;

	do 
	{
		do 
		{
			ADD_TOKEN_CHAR (stix, c);
			GET_CHAR_TO (stix, c);

			if (c == STIX_UCI_EOF) 
			{
				/* string not closed */
				set_syntax_error (stix, STIX_SYNERR_STRNC, &stix->c->tok.loc /*&stix->c->lxc.l*/, STIX_NULL);
				return -1;
			}
		} 
		while (c != '\'');

		GET_CHAR_TO (stix, c);
	} 
	while (c == '\'');

	return 0;
}

static int get_string (stix_t* stix, stix_uch_t end_char, stix_uch_t esc_char, int regex, stix_size_t preescaped)
{
	stix_uci_t c;
	stix_size_t escaped = preescaped;
	stix_size_t digit_count = 0;
	stix_uci_t c_acc = 0;

	stix->c->tok.type = STIX_IOTOK_STRLIT;

	while (1)
	{
		GET_CHAR_TO (stix, c);

		if (c == STIX_UCI_EOF)
		{
			set_syntax_error (stix, STIX_SYNERR_STRNC, &stix->c->tok.loc /*&stix->c->lxc.l*/, STIX_NULL);
			return -1;
		}

		if (escaped == 3)
		{
			if (c >= '0' && c <= '7')
			{
				c_acc = c_acc * 8 + c - '0';
				digit_count++;
				if (digit_count >= escaped) 
				{
					/* should i limit the max to 0xFF/0377? 
					 * if (c_acc > 0377) c_acc = 0377;*/
					ADD_TOKEN_CHAR (stix, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				ADD_TOKEN_CHAR (stix, c_acc);
				escaped = 0;
			}
		}
		else if (escaped == 2 || escaped == 4 || escaped == 8)
		{
			if (c >= '0' && c <= '9')
			{
				c_acc = c_acc * 16 + c - '0';
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (stix, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= 'A' && c <= 'F')
			{
				c_acc = c_acc * 16 + c - 'A' + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (stix, c_acc);
					escaped = 0;
				}
				continue;
			}
			else if (c >= 'a' && c <= 'f')
			{
				c_acc = c_acc * 16 + c - 'a' + 10;
				digit_count++;
				if (digit_count >= escaped) 
				{
					ADD_TOKEN_CHAR (stix, c_acc);
					escaped = 0;
				}
				continue;
			}
			else
			{
				stix_uch_t rc;

				rc = (escaped == 2)? 'x':
				     (escaped == 4)? 'u': 'U';
				if (digit_count == 0) 
					ADD_TOKEN_CHAR (stix, rc);
				else ADD_TOKEN_CHAR (stix, c_acc);

				escaped = 0;
			}
		}

		if (escaped == 0 && c == end_char)
		{
			/* terminating quote */
			/*GET_CHAR_TO (stix, c);*/
			GET_CHAR (stix);
			break;
		}

		if (escaped == 0 && c == esc_char)
		{
			escaped = 1;
			continue;
		}

		if (escaped == 1)
		{
			if (c == 'n') c = '\n';
			else if (c == 'r') c = '\r';
			else if (c == 't') c = '\t';
			else if (c == 'f') c = '\f';
			else if (c == 'b') c = '\b';
			else if (c == 'v') c = '\v';
			else if (c == 'a') c = '\a';
			else if (c >= '0' && c <= '7' && !regex) 
			{
				/* i don't support the octal notation for a regular expression.
				 * it conflicts with the backreference notation between \1 and \7 inclusive. */
				escaped = 3;
				digit_count = 1;
				c_acc = c - '0';
				continue;
			}
			else if (c == 'x') 
			{
				escaped = 2;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
			else if (c == 'u' && STIX_SIZEOF(stix_uch_t) >= 2) 
			{
				escaped = 4;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
			else if (c == 'U' && STIX_SIZEOF(stix_uch_t) >= 4) 
			{
				escaped = 8;
				digit_count = 0;
				c_acc = 0;
				continue;
			}
			else if (regex) 
			{
				/* if the following character doesn't compose a proper
				 * escape sequence, keep the escape character. 
				 * an unhandled escape sequence can be handled 
				 * outside this function since the escape character 
				 * is preserved.*/
				ADD_TOKEN_CHAR (stix, esc_char);
			}

			escaped = 0;
		}

		ADD_TOKEN_CHAR (stix, c);
	}

	return 0;
}


static int get_binsel (stix_t* stix)
{
	/* 
	 * binary-selector := binary-selector-character+
	 */
	stix_uci_t oc;

	oc = stix->c->lxc.c;
	ADD_TOKEN_CHAR (stix, oc);

	GET_CHAR (stix);
	/* special case if a minus is followed by a digit immediately */
	if (oc == '-' && is_digitchar(stix->c->lxc.c)) return get_numlit (stix, 1);

	/* up to 2 characters only */
	if (is_binselchar (stix->c->lxc.c)) 
	{
		ADD_TOKEN_CHAR (stix, stix->c->lxc.c);
		GET_CHAR (stix);
	}

	/* or up to any occurrences */
	/*
	while (is_binselchar(stix->c->lxc.c)) 
	{
		ADD_TOKEN_CHAR (stix, c);
		GET_CHAR (stix);
	}
	*/

	stix->c->tok.type = STIX_IOTOK_BINSEL;
	return 0;
}

static int get_token (stix_t* stix)
{
	stix_uci_t c;
	int n;

retry:
	do 
	{
		if (skip_spaces(stix) <= -1) return -1;
		if ((n = skip_comment(stix)) <= -1) return -1;
	} 
	while (n >= 1);

	/* clear the token resetting its location */
	stix->c->tok.type = STIX_IOTOK_EOF;  /* is it correct? */
	stix->c->tok.name.len = 0;

	stix->c->tok.loc = stix->c->lxc.l;
	c = stix->c->lxc.c;

	switch (c)
	{
		case STIX_UCI_EOF:
		{
			int n;

			n = end_include (stix);
			if (n <= -1) return -1;
			if (n >= 1) goto retry;

			stix->c->tok.type = STIX_IOTOK_EOF;
			ADD_TOKEN_STR(stix, vocas[VOCA_EOF].str, vocas[VOCA_EOF].len);
			break;
		}

		case '$': /* character literal */
			GET_CHAR (stix);
			if (get_charlit(stix) <= -1) return -1;
			break;

		case '\'': /* string literal */
			GET_CHAR (stix);
			if (get_strlit(stix) <= -1) return -1;
			break;

		case ':':
			stix->c->tok.type = STIX_IOTOK_COLON;
			ADD_TOKEN_CHAR (stix, c);
			GET_CHAR_TO (stix, c);
			if (c == '=') 
			{
				stix->c->tok.type = STIX_IOTOK_ASSIGN;
				ADD_TOKEN_CHAR (stix, c);
				GET_CHAR (stix);
			}
			break;

		case '^':
			stix->c->tok.type = STIX_IOTOK_RETURN;
			ADD_TOKEN_CHAR(stix, c);
			GET_CHAR_TO (stix, c);
#if 0
/* TODO: support explicit block return */
			if (c == '^')
			{
				/* ^^ */
				stix->c->tok.type == STIX_IOTOK_BLKRET;
				ADD_TOKEN_CHAR (stix, c);
			}
#endif
			break;

		case '{': /* extension */
			stix->c->tok.type = STIX_IOTOK_LBRACE;
			goto single_char_token;
		case '}': /* extension */
			stix->c->tok.type = STIX_IOTOK_RBRACE;
			goto single_char_token;
		case '[':
			stix->c->tok.type = STIX_IOTOK_LBRACK;
			goto single_char_token;
		case ']': 
			stix->c->tok.type = STIX_IOTOK_RBRACK;
			goto single_char_token;
		case '(':
			stix->c->tok.type = STIX_IOTOK_LPAREN;
			goto single_char_token;
		case ')':
			stix->c->tok.type = STIX_IOTOK_RPAREN;
			goto single_char_token;
		case '.':
			stix->c->tok.type = STIX_IOTOK_PERIOD;
			goto single_char_token;
		case ';':
			stix->c->tok.type = STIX_IOTOK_SEMICOLON;
			goto single_char_token;

		case '#':  
			/*
			 * The hash sign is not the part of the token name.
			 * ADD_TOKEN_CHAR(stix, c); */
			GET_CHAR_TO (stix, c);
			switch (c)
			{
				case STIX_UCI_EOF:
					set_syntax_error (stix, STIX_SYNERR_HLTNT, &stix->c->lxc.l, STIX_NULL);
					return -1;
				
				case '(':
					/* #( */
					ADD_TOKEN_CHAR(stix, c);
					stix->c->tok.type = STIX_IOTOK_APAREN;
					GET_CHAR (stix);
					break;

				case '[':
					/* #[ - byte array literal */
					ADD_TOKEN_CHAR(stix, c);
					stix->c->tok.type = STIX_IOTOK_BPAREN;
					GET_CHAR (stix);
					break;

				case '\'':
					/* quoted symbol literal */
					GET_CHAR (stix);
					if (get_strlit(stix) <= -1) return -1;
					stix->c->tok.type = STIX_IOTOK_SYMLIT;
					break;

				default:
					/* symbol-literal := "#" symbol-body
					 * symbol-body := identifier | keyword+ | binary-selector | string-literal
					 */ 

					/* unquoted symbol literal */
					if (is_binselchar(c))
					{
						do 
						{
							ADD_TOKEN_CHAR (stix, c);
							GET_CHAR_TO (stix, c);
						} 
						while (is_binselchar(c));
					}
					else if (is_alphachar(c))
					{
						int colon_required = 0;

					nextword:
						do 
						{
							ADD_TOKEN_CHAR (stix, c);
							GET_CHAR_TO (stix, c);
						} 
						while (is_alnumchar(c));

						if (c == ':')
						{
							ADD_TOKEN_CHAR (stix, c);
							GET_CHAR_TO (stix, c);

							if (is_alphachar(c)) 
							{
								/* if a colon is found in the middle of a symbol,
								 * the last charater is expected to be a colon as well */
								colon_required =1;
								goto nextword;
							}
						}
						else if (colon_required)
						{
							/* the last character is not a colon */
							set_syntax_error (stix, STIX_SYNERR_COLON, &stix->c->lxc.l, STIX_NULL);
							return -1;
						}
					}
					else
					{
						set_syntax_error (stix, STIX_SYNERR_HLTNT, &stix->c->lxc.l, STIX_NULL);
						return -1;
					}

					stix->c->tok.type = STIX_IOTOK_SYMLIT;
					break;
			}

			break;

		case 'C': /* a character with a C-style escape sequence */
		case 'S': /* a string with a C-style escape sequences */
		case 'M': /* a symbol with a C-style escape sequences */
		{
			stix_uci_t saved_c = c;

			GET_CHAR_TO (stix, c);
			if (c == '\'')
			{
				/*GET_CHAR (stix);*/
				if (get_string(stix, '\'', '\\', 0, 0) <= -1) return -1;

				if (saved_c == 'C')
				{
					if (stix->c->tok.name.len != 1)
					{
						set_syntax_error (stix, STIX_SYNERR_CHARLIT, &stix->c->tok.loc, &stix->c->tok.name);
						return -1;
					}
					stix->c->tok.type = STIX_IOTOK_CHARLIT;
				}
				else if (saved_c == 'M')
				{
					stix->c->tok.type = STIX_IOTOK_SYMLIT;
				}
			}
			else
			{
				if (get_ident(stix, saved_c) <= -1) return -1;
			}

			break;
		}

	/*	case 'B': TODO: byte string  with a c-style escape sequence? */

	/*  case 'R':
			TODO: regular expression?
			GET_CHAR_TO (stix, c);
			if (c == '\'')
			{
				GET_CHAR (stix);
				if (get_rexlit(stix) <= -1) return -1;
			}
			else
			{
				if (get_ident(stix, 'R') <= -1) return -1;
			}
			break;
	 */

		default:
			if (is_alphachar(c)) 
			{
				if (get_ident(stix, STIX_UCI_EOF) <= -1) return -1;
			}
			else if (is_digitchar(c)) 
			{
				if (get_numlit(stix, 0) <= -1) return -1;
			}
			else if (is_binselchar(c)) 
			{
				/* binary selector */
				if (get_binsel(stix) <= -1) return -1;
			}
			else 
			{
				stix->c->ilchr = (stix_uch_t)c;
				set_syntax_error (stix, STIX_SYNERR_ILCHR, &stix->c->lxc.l, &stix->c->ilchr_ucs);
				return -1;
			}
			break;

		single_char_token:
			ADD_TOKEN_CHAR(stix, c);
			GET_CHAR (stix);
			break;
	}

	return 0;
}


static void clear_io_names (stix_t* stix)
{
	stix_iolink_t* cur;

	STIX_ASSERT (stix->c != STIX_NULL);

	while (stix->c->io_names)
	{
		cur = stix->c->io_names;
		stix->c->io_names = cur->link;
		stix_freemem (stix, cur);
	}
}

static const stix_uch_t* add_io_name (stix_t* stix, const stix_ucs_t* name)
{
	stix_iolink_t* link;
	stix_uch_t* ptr;

	link = (stix_iolink_t*) stix_callocmem (stix, STIX_SIZEOF(*link) + STIX_SIZEOF(stix_uch_t) * (name->len + 1));
	if (!link) return STIX_NULL;

	ptr = (stix_uch_t*)(link + 1);

	stix_copychars (ptr, name->ptr, name->len);
	ptr[name->len] = '\0';

	link->link = stix->c->io_names;
	stix->c->io_names = link;

	return ptr;
}

static int begin_include (stix_t* stix)
{
	stix_ioarg_t* arg;
	const stix_uch_t* io_name;

	io_name = add_io_name (stix, &stix->c->tok.name);
	if (!io_name) return -1;

	arg = (stix_ioarg_t*) stix_callocmem (stix, STIX_SIZEOF(*arg));
	if (!arg) goto oops;

	arg->name = io_name;
	arg->line = 1;
	arg->colm = 1;
	arg->includer = stix->c->curinp;

	if (stix->c->impl (stix, STIX_IO_OPEN, arg) <= -1) goto oops;

	stix->c->curinp = arg;
	/* stix->c->depth.incl++; */

	/* read in the first character in the included file. 
	 * so the next call to get_token() sees the character read
	 * from this file. */
	if (get_char(stix) <= -1 || get_token(stix) <= -1) 
	{
		end_include (stix); 
		/* i don't jump to oops since i've called 
		 * end_include() which frees stix->c->curinp/arg */
		return -1;
	}

	return 0;

oops:
	if (arg) stix_freemem (stix, arg);
	return -1;
}

static int end_include (stix_t* stix)
{
	int x;
	stix_ioarg_t* cur;

	if (stix->c->curinp == &stix->c->arg) return 0; /* no include */

	/* if it is an included file, close it and
	 * retry to read a character from an outer file */

	x = stix->c->impl (stix, STIX_IO_CLOSE, stix->c->curinp);

	/* if closing has failed, still destroy the
	 * sio structure first as normal and return
	 * the failure below. this way, the caller 
	 * does not call STIX_IO_CLOSE on 
	 * stix->c->curinp again. */

	cur = stix->c->curinp;
	stix->c->curinp = stix->c->curinp->includer;

	STIX_ASSERT (cur->name != STIX_NULL);
	stix_freemem (stix, cur);
	/* stix->parse.depth.incl--; */

	if (x != 0)
	{
		/* the failure mentioned above is returned here */
		return -1;
	}

	stix->c->lxc = stix->c->curinp->lxc;
	return 1; /* ended the included file successfully */
}

/* ---------------------------------------------------------------------
 * Byte-Code Generator
 * --------------------------------------------------------------------- */

static STIX_INLINE int emit_byte_instruction (stix_t* stix, stix_byte_t code)
{
	stix_size_t i;

	i = stix->c->mth.code.len + 1;
	if (i > stix->c->mth.code_capa)
	{
		stix_byte_t* tmp;

		i = STIX_ALIGN (i, CODE_BUFFER_ALIGN);

		tmp = stix_reallocmem (stix, stix->c->mth.code.ptr, i * STIX_SIZEOF(*tmp));
		if (!tmp) return -1;

		stix->c->mth.code.ptr = tmp;
		stix->c->mth.code_capa = i;
	}

	stix->c->mth.code.ptr[stix->c->mth.code.len++] = code;
	return 0;
}

static int emit_single_param_instruction (stix_t* stix, int cmd, stix_oow_t param_1)
{
	stix_byte_t bc;

	switch (cmd)
	{
		case BCODE_PUSH_INSTVAR_0:
		case BCODE_STORE_INTO_INSTVAR_0:
		case BCODE_POP_INTO_INSTVAR_0:
		case BCODE_PUSH_TEMPVAR_0:
		case BCODE_STORE_INTO_TEMPVAR_0:
		case BCODE_POP_INTO_TEMPVAR_0:
		case BCODE_PUSH_LITERAL_0:
			if (param_1 < 8)
			{
				bc = (stix_byte_t)(cmd & 0xF8) | (stix_byte_t)param_1;
				goto write_short;
			}
			else
			{
				/* convert the code to a long version */
				bc = cmd | 0x80;
				goto write_long;
			}

		case BCODE_PUSH_OBJECT_0:
		case BCODE_STORE_INTO_OBJECT_0:
		case BCODE_POP_INTO_OBJECT_0:
		case BCODE_JUMP_FORWARD_0:
		case BCODE_JUMP_BACKWARD_0:
		case BCODE_JUMP_IF_TRUE_0:
		case BCODE_JUMP_IF_FALSE_0:
		case BCODE_JUMP_BY_OFFSET_0:
			if (param_1 < 4)
			{
				bc = (stix_byte_t)(cmd & 0xFC) | (stix_byte_t)param_1;
				goto write_short;
			}
			else
			{
				/* convert the code to a long version */
				bc = cmd | 0x80;
				goto write_long;
			}
	}

	stix->errnum = STIX_EINVAL;
	return -1;

write_short:
	if (emit_byte_instruction(stix, bc) <= -1) return -1;
	return 0;

write_long:
	#if (STIX_BCODE_LONG_PARAM_SIZE == 2)
		if (emit_byte_instruction(stix, bc) <= -1 ||
		    emit_byte_instruction(stix, param_1 >> 8) <= -1 ||
		    emit_byte_instruction(stix, param_1 & 0xFF) <= -1) return -1;
	#else
		if (emit_byte_instruction(stix, bc) <= -1 ||
		    emit_byte_instruction(stix, param_1) <= -1) return -1;
	#endif
	return 0;
}


static int emit_double_param_instruction (stix_t* stix, int cmd, stix_size_t param_1, stix_size_t param_2)
{
	stix_byte_t bc;

	switch (cmd)
	{
		case BCODE_STORE_INTO_CTXTEMPVAR_0:
		case BCODE_POP_INTO_CTXTEMPVAR_0:
		case BCODE_PUSH_CTXTEMPVAR_0:
		case BCODE_PUSH_OBJVAR_0:
		case BCODE_STORE_INTO_OBJVAR_0:
		case BCODE_POP_INTO_OBJVAR_0:
		case BCODE_SEND_MESSAGE_0:
		case BCODE_SEND_MESSAGE_TO_SUPER_0:
			if (param_1 < 8 && param_2 < 0xFF)
			{
				/* low 2 bits of the instruction code is the first parameter */
				bc = (stix_byte_t)(cmd & 0xFC) | (stix_byte_t)param_1;
				goto write_short;
			}
			else
			{
				/* convert the code to a long version */
				bc = cmd | 0x80;
				goto write_long;
			}
	}

	stix->errnum = STIX_EINVAL;
	return -1;

write_short:
	if (emit_byte_instruction(stix, bc) <= -1 ||
	    emit_byte_instruction(stix, param_2) <= -1) return -1;
	return 0;

write_long:
	#if (STIX_BCODE_LONG_PARAM_SIZE == 2)
		if (emit_byte_instruction(stix, bc) <= -1 ||
		    emit_byte_instruction(stix, param_1 >> 8) <= -1 ||
		    emit_byte_instruction(stix, param_1 & 0xFF) <= -1 ||
		    emit_byte_instruction(stix, param_2 >> 8) <= -1 ||
		    emit_byte_instruction(stix, param_2 & 0xFF) <= -1) return -1;
	#else
		if (emit_byte_instruction(stix, bc) <= -1 ||
		    emit_byte_instruction(stix, param_1) <= -1 ||
		    emit_byte_instruction(stix, param_2) return -1;
	#endif
	return 0;
}

static int emit_push_smint_literal (stix_t* stix, stix_ooi_t i)
{
	stix_size_t index;

	switch (i)
	{
		case -1:
			return emit_byte_instruction (stix, BCODE_PUSH_NEGONE);

		case 0:
			return emit_byte_instruction (stix, BCODE_PUSH_ZERO);

		case 1:
			return emit_byte_instruction (stix, BCODE_PUSH_ONE);

		case 2:
			return emit_byte_instruction (stix, BCODE_PUSH_TWO);

/* TODO: include some other numbers? like 3 */
	}


	if (add_literal(stix, STIX_OOP_FROM_SMINT(i), &index) <= -1 ||
	    emit_single_param_instruction(stix, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;

	return 0;
}

/* ---------------------------------------------------------------------
 * Compiler
 * --------------------------------------------------------------------- */


static int add_literal (stix_t* stix, stix_oop_t lit, stix_size_t* index)
{
	stix_size_t i;

	for (i = 0; i < stix->c->mth.literal_count; i++) 
	{
		/* 
		 * this removes redundancy of symbols, characters, and small integers. 
		 * more complex redundacy check may be done somewhere else like 
		 * in add_string_literal().
		 */
		if (stix->c->mth.literals[i] == lit) 
		{
			*index = i;
			return i;
		}
	}

	if (stix->c->mth.literal_count >= stix->c->mth.literal_capa)
	{
		stix_oop_t* tmp;
		stix_size_t new_capa;

		new_capa = STIX_ALIGN (stix->c->mth.literal_count + 1, LITERAL_BUFFER_ALIGN);
		tmp = (stix_oop_t*)stix_reallocmem (stix, stix->c->mth.literals, new_capa * STIX_SIZEOF(*tmp));
		if (!tmp) return -1;

		stix->c->mth.literal_capa = new_capa;
		stix->c->mth.literals = tmp;
	}

	*index = stix->c->mth.literal_count;
	stix->c->mth.literals[stix->c->mth.literal_count++] = lit;
	return 0;
}

static STIX_INLINE int add_character_literal (stix_t* stix, stix_uch_t ch, stix_size_t* index)
{
	return add_literal (stix, STIX_OOP_FROM_CHAR(ch), index);
}

static int add_string_literal (stix_t* stix, const stix_ucs_t* str, stix_size_t* index)
{
	stix_oop_t lit;
	stix_size_t i;

	for (i = 0; i < stix->c->mth.literal_count; i++) 
	{
		lit = stix->c->mth.literals[i];

		if (STIX_CLASSOF(stix, lit) == stix->_string && 
		    STIX_OBJ_GET_SIZE(lit) == str->len &&
		    stix_equalchars(((stix_oop_char_t)lit)->slot, str->ptr, str->len)) 
		{
			*index = i;
			return 0;
		}
	}

	lit = stix_instantiate (stix, stix->_string, str->ptr, str->len);
	if (!lit) return -1;

	return add_literal (stix, lit, index);
}

static int add_symbol_literal (stix_t* stix, const stix_ucs_t* str, stix_size_t* index)
{
	stix_oop_t tmp;

	tmp = stix_makesymbol (stix, str->ptr, str->len);
	if (!tmp) return -1;

	return add_literal (stix, tmp, index);
}

static STIX_INLINE int set_class_name (stix_t* stix, const stix_ucs_t* name)
{
	return copy_string_to (stix, name, &stix->c->cls.name, &stix->c->cls.name_capa, 0, '\0');
}

static STIX_INLINE int set_superclass_name (stix_t* stix, const stix_ucs_t* name)
{
	return copy_string_to (stix, name, &stix->c->cls.supername, &stix->c->cls.supername_capa, 0, '\0');
}

static STIX_INLINE int add_class_level_variable (stix_t* stix, var_type_t index, const stix_ucs_t* name)
{
	int n;

	n =  copy_string_to (stix, name, &stix->c->cls.vars[index], &stix->c->cls.vars_capa[index], 1, ' ');
	if (n >= 0) 
	{
		stix->c->cls.var_count[index]++;
/* TODO: check if it exceeds STIX_MAX_NAMED_INSTVARS, STIX_MAX_CLASSVARS, STIX_MAX_CLASSINSTVARS */
	}

	return n;
}

static stix_ssize_t find_class_level_variable (stix_t* stix, stix_oop_class_t self, const stix_ucs_t* name, var_info_t* var)
{
	stix_ssize_t pos;
	stix_oop_t super;
	stix_oop_char_t v;
	stix_oop_char_t* vv;
	stix_ucs_t hs;
	int index;

	if (self)
	{
		STIX_ASSERT (STIX_CLASSOF(stix, self) == stix->_class);

		/* NOTE the loop here assumes the right order of
		 *    instvars
		 *    classvars
		 *    classinstvars
		 */
		vv = &self->instvars;
		for (index = VAR_INSTANCE; index <= VAR_CLASSINST; index++)
		{
			v = vv[index];
			hs.ptr = v->slot;
			hs.len = STIX_OBJ_GET_SIZE(v);

			pos = find_word_in_string(&hs, name);
			if (pos >= 0) 
			{
				super = self->superclass;
				goto done;
			}
		}

		super = self->superclass;
	}
	else
	{
		/* the class definition is not available yet */
		for (index = VAR_INSTANCE; index <= VAR_CLASSINST; index++)
		{
			pos = find_word_in_string(&stix->c->cls.vars[index], name);

			if (pos >= 0) 
			{
				super = stix->c->cls.super_oop;
				goto done;
			}
		}
		super = stix->c->cls.super_oop;
	}

	while (super != stix->_nil)
	{
		STIX_ASSERT (STIX_CLASSOF(stix, super) == stix->_class);

		/* NOTE the loop here assumes the right order of
		 *    instvars
		 *    classvars
		 *    classinstvars
		 */
		vv = &((stix_oop_class_t)super)->instvars;
		for (index = VAR_INSTANCE; index <= VAR_CLASSINST; index++)
		{
			v = vv[index];
			hs.ptr = v->slot;
			hs.len = STIX_OBJ_GET_SIZE(v);

			pos = find_word_in_string(&hs, name);
			if (pos >= 0) 
			{
				super = ((stix_oop_class_t)super)->superclass;
				goto done;
			}
		}

		super = ((stix_oop_class_t)super)->superclass;
	}

	stix->errnum = STIX_ENOMEM;
	return -1;

done:
	/* 'self' may be STIX_NULL if STIX_NULL has been given for it.
	 * the caller must take good care when interpreting the meaning of 
	 * this field */
	var->cls = self; 

	if (super != stix->_nil)
	{
		stix_oow_t spec;

		STIX_ASSERT (STIX_CLASSOF(stix, super) == stix->_class);
		switch (index)
		{
			case VAR_INSTANCE:
				/* each class has the number of named instance variables 
				 * accumulated for inheritance. the position found in the
				 * local variable string can be adjusted by adding the
				 * number in the superclass */
				spec = STIX_OOP_TO_SMINT(((stix_oop_class_t)super)->spec);
				pos += STIX_CLASS_SPEC_NAMED_INSTVAR(spec);
				break;

			case VAR_CLASS:
				/* no adjustment is needed.
				 * a class object is composed of three parts.
				 *  fixed-part | classinst-variables | class-variabes 
				 * the position returned here doesn't consider 
				 * class instance variables that can be potentially
				 * placed before the class variables. */
				break;

			case VAR_CLASSINST:
				spec = STIX_OOP_TO_SMINT(((stix_oop_class_t)super)->selfspec);
				pos += STIX_CLASS_SELFSPEC_CLASSINSTVAR(spec);
				break;
		}
	}

	var->type = index;
	var->pos = pos;
	return pos;
}

static int clone_assignee (stix_t* stix, stix_ucs_t* name)
{
	int n;
	stix_size_t old_len;

	old_len = stix->c->mth.assignees.len;
	n = copy_string_to (stix, name, &stix->c->mth.assignees, &stix->c->mth.assignees_capa, 1, '\0');
	if (n <= -1) return -1;

	/* update the pointer to of the name. its length is the same. */
	name->ptr = stix->c->mth.assignees.ptr + old_len;
	return 0;
}

static int clone_binary_selector (stix_t* stix, stix_ucs_t* name)
{
	int n;
	stix_size_t old_len;

	old_len = stix->c->mth.binsels.len;
	n = copy_string_to (stix, name, &stix->c->mth.binsels, &stix->c->mth.binsels_capa, 1, '\0');
	if (n <= -1) return -1;

	/* update the pointer to of the name. its length is the same. */
	name->ptr = stix->c->mth.binsels.ptr + old_len;
	return 0;
}

static int clone_keyword (stix_t* stix, stix_ucs_t* name)
{
	int n;
	stix_size_t old_len;

	old_len = stix->c->mth.kwsels.len;
	n = copy_string_to (stix, name, &stix->c->mth.kwsels, &stix->c->mth.kwsels_capa, 1, '\0');
	if (n <= -1) return -1;

	/* update the pointer to of the name. its length is the same. */
	name->ptr = stix->c->mth.kwsels.ptr + old_len;
	return 0;
}

static int add_method_name_fragment (stix_t* stix, const stix_ucs_t* name)
{
	/* method name fragments are concatenated without any delimiters */
	return copy_string_to (stix, name, &stix->c->mth.name, &stix->c->mth.name_capa, 1, '\0');
}

static int method_exists (stix_t* stix, const stix_ucs_t* name)
{
	/* check if the current class contains a method of the given name */
	return stix_lookupdic (stix, stix->c->cls.mthdic_oop[stix->c->mth.type], name) != STIX_NULL;
}

static int add_temporary_variable (stix_t* stix, const stix_ucs_t* name)
{
	/* temporary variable names are added to the string with leading
	 * space if it's not the first variable */
	return copy_string_to (stix, name, &stix->c->mth.tmprs, &stix->c->mth.tmprs_capa, 1, ' ');
}

static STIX_INLINE stix_ssize_t find_temporary_variable (stix_t* stix, const stix_ucs_t* name)
{
	return find_word_in_string(&stix->c->mth.tmprs, name);
}

static int compile_class_level_variables (stix_t* stix)
{
	var_type_t dcl_type = VAR_INSTANCE;

	if (stix->c->tok.type == STIX_IOTOK_LPAREN)
	{
		/* process variable modifiers */
		GET_TOKEN (stix);

		if (is_token_symbol(stix, VOCA_CLASS))
		{
			/* #dcl(#class) */
			dcl_type = VAR_CLASS;
			GET_TOKEN (stix);
		}
		else if (is_token_symbol(stix, VOCA_CLASSINST))
		{
			/* #dcl(#classinst) */
			dcl_type = VAR_CLASSINST;
			GET_TOKEN (stix);
		}

		if (stix->c->tok.type != STIX_IOTOK_RPAREN)
		{
			set_syntax_error (stix, STIX_SYNERR_RPAREN, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		GET_TOKEN (stix);
	}

	do
	{
		if (stix->c->tok.type == STIX_IOTOK_IDENT)
		{
			var_info_t var;

			if (find_class_level_variable(stix, STIX_NULL, &stix->c->tok.name, &var) >= 0)
			{
				set_syntax_error (stix, STIX_SYNERR_VARNAMEDUP, &stix->c->tok.loc, &stix->c->tok.name);
				return -1;
			}

/* TOOD: CHECK IF IT CONFLICTS WITH GLOBAL VARIABLE NAMES */
/* TODO: ------------------------------------------------ */

			if (add_class_level_variable(stix, dcl_type, &stix->c->tok.name) <= -1) return -1;
		}
		else
		{
			break;
		}

		GET_TOKEN (stix);
	}
	while (1);

	if (stix->c->tok.type != STIX_IOTOK_PERIOD)
	{
		set_syntax_error (stix, STIX_SYNERR_PERIOD, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	GET_TOKEN (stix);
	return 0;
}

static int compile_unary_method_name (stix_t* stix)
{
	STIX_ASSERT (stix->c->mth.name.len == 0);
	STIX_ASSERT (stix->c->mth.tmpr_nargs == 0);

	if (add_method_name_fragment(stix, &stix->c->tok.name) <= -1) return -1;
	GET_TOKEN (stix);
	return 0;
}

static int compile_binary_method_name (stix_t* stix)
{
	STIX_ASSERT (stix->c->mth.name.len == 0);
	STIX_ASSERT (stix->c->mth.tmpr_nargs == 0);

	if (add_method_name_fragment(stix, &stix->c->tok.name) <= -1) return -1;
	GET_TOKEN (stix);

	/* collect the argument name */
	if (stix->c->tok.type != STIX_IOTOK_IDENT) 
	{
		/* wrong argument name. identifier expected */
		set_syntax_error (stix, STIX_SYNERR_IDENT, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	STIX_ASSERT (stix->c->mth.tmpr_nargs == 0);

	/* no duplication check is performed against class-level variable names.
	 * a duplcate name will shade a previsouly defined variable. */
	if (add_temporary_variable(stix, &stix->c->tok.name) <= -1) return -1;
	stix->c->mth.tmpr_nargs++;

	STIX_ASSERT (stix->c->mth.tmpr_nargs == 1);
	/* this check should not be not necessary
	if (stix->c->mth.tmpr_nargs > MAX_CODE_NARGS)
	{
		set_syntax_error (stix, STIX_SYNERR_ARGFLOOD, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}
	*/

	GET_TOKEN (stix);
	return 0;
}

static int compile_keyword_method_name (stix_t* stix)
{
	STIX_ASSERT (stix->c->mth.name.len == 0);
	STIX_ASSERT (stix->c->mth.tmpr_nargs == 0);

	do 
	{
		if (add_method_name_fragment(stix, &stix->c->tok.name) <= -1) return -1;

		GET_TOKEN (stix);
		if (stix->c->tok.type != STIX_IOTOK_IDENT) 
		{
			/* wrong argument name. identifier is expected */
			set_syntax_error (stix, STIX_SYNERR_IDENT, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		if (find_temporary_variable(stix, &stix->c->tok.name) >= 0)
		{
			set_syntax_error (stix, STIX_SYNERR_ARGNAMEDUP, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		if (add_temporary_variable(stix, &stix->c->tok.name) <= -1) return -1;
		stix->c->mth.tmpr_nargs++;

		GET_TOKEN (stix);
	} 
	while (stix->c->tok.type == STIX_IOTOK_KEYWORD);

	return 0;
}

static int compile_method_name (stix_t* stix)
{
	/* 
	 * method-name := unary-method-name | binary-method-name | keyword-method-name
	 * unary-method-name := unary-selector
	 * binary-method-name := binary-selector selector-argument
	 * keyword-method-name := (keyword selector-argument)+
	 * selector-argument := identifier
	 * unary-selector := identifier
	 */
	int n;

	STIX_ASSERT (stix->c->mth.tmpr_count == 0);

	stix->c->mth.name_loc = stix->c->tok.loc;
	switch (stix->c->tok.type)
	{
		case STIX_IOTOK_IDENT:
			n = compile_unary_method_name(stix);
			break;

		case STIX_IOTOK_BINSEL:
			n = compile_binary_method_name(stix);
			break;

		case STIX_IOTOK_KEYWORD:
			n = compile_keyword_method_name(stix);
			break;

		default:
			/* illegal method name  */
			set_syntax_error (stix, STIX_SYNERR_MTHNAME, &stix->c->tok.loc, &stix->c->tok.name);
			n = -1;
	}

	if (n >= 0)
	{
		if (method_exists(stix, &stix->c->mth.name)) 
 		{
			set_syntax_error (stix, STIX_SYNERR_MTHNAMEDUP, &stix->c->mth.name_loc, &stix->c->mth.name);
			return -1;
 		}
	}

	STIX_ASSERT (stix->c->mth.tmpr_nargs < MAX_CODE_NARGS);
	/* the total number of temporaries is equal to the number of 
	 * arguments after having processed the message pattern. it's because
	 * stix treats arguments the same as temporaries */
	stix->c->mth.tmpr_count = stix->c->mth.tmpr_nargs;
	return n;
}

static int compile_method_temporaries (stix_t* stix)
{
	/* 
	 * method-temporaries := "|" variable-list "|"
	 * variable-list := identifier*
	 */

	if (!is_token_binsel(stix, VOCA_VBAR)) 
	{
		/* return without doing anything if | is not found.
		 * this is not an error condition */
		return 0;
	}

	GET_TOKEN (stix);
	while (stix->c->tok.type == STIX_IOTOK_IDENT) 
	{
		if (find_temporary_variable(stix, &stix->c->tok.name) >= 0)
		{
			set_syntax_error (stix, STIX_SYNERR_TMPRNAMEDUP, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		if (add_temporary_variable(stix, &stix->c->tok.name) <= -1) return -1;
		stix->c->mth.tmpr_count++;

		if (stix->c->mth.tmpr_count > MAX_CODE_NARGS)
		{
			set_syntax_error (stix, STIX_SYNERR_TMPRFLOOD, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		GET_TOKEN (stix);
	}

	if (!is_token_binsel(stix, VOCA_VBAR)) 
	{
		set_syntax_error (stix, STIX_SYNERR_VBAR, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	GET_TOKEN (stix);
	return 0;
}

static int compile_method_primitive (stix_t* stix)
{
	/* 
	 * method-primitive := "<"  "primitive:" integer ">"
	 */
	int prim_no;
	const stix_uch_t* ptr, * end;

	if (!is_token_binsel(stix, VOCA_LT)) 
	{
		/* return if < is not seen. it is not an error condition */
		return 0;
	}

	GET_TOKEN (stix);
	if (!is_token_keyword(stix, VOCA_PRIMITIVE_COLON))
	{
		set_syntax_error (stix, STIX_SYNERR_PRIMITIVE, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}
/* TODO: other modifiers? 
 * <primitive: 10>
 * <some-other-modifier: xxxx>
 */

	GET_TOKEN (stix); /* TODO: only integer */
	if (stix->c->tok.type != STIX_IOTOK_NUMLIT) 
	{
		set_syntax_error (stix, STIX_SYNERR_INTEGER, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

/*TODO: more checks the validity of the primitive number. support nubmer with radix and so on support more extensive syntax */
	ptr = stix->c->tok.name.ptr;
	end = ptr + stix->c->tok.name.len;
	prim_no = 0;
	while (ptr < end && is_digitchar(*ptr)) 
	{
		prim_no = prim_no * 10 + (*ptr - '0');
		if (prim_no > MAX_CODE_PRIMNO)
		{
			set_syntax_error (stix, STIX_SYNERR_PRIMNO, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		ptr++;
	}

	stix->c->mth.prim_no = prim_no;

	GET_TOKEN (stix);
	if (!is_token_binsel(stix, VOCA_GT)) 
	{
		set_syntax_error (stix, STIX_SYNERR_GT, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	GET_TOKEN (stix);
	return 0;
}

static int get_variable_info (stix_t* stix, const stix_ucs_t* name, const stix_ioloc_t* name_loc, var_info_t* var)
{
	stix_ssize_t index;

	STIX_MEMSET (var, 0, STIX_SIZEOF(*var));

	index = find_temporary_variable (stix, name);
	if (index >= 0)
	{
		var->type = (index < stix->c->mth.tmpr_nargs)? VAR_ARGUMENT: VAR_TEMPORARY;
		var->pos = index;
	}
	else 
	{
		if (find_class_level_variable(stix, stix->c->cls.self_oop, name, var) >= 0)
		{
			switch (var->type)
			{
				case VAR_INSTANCE:
					if (stix->c->mth.type == MTH_CLASS)
					{
						/* a class method cannot access an instance variable */
						set_syntax_error (stix, STIX_SYNERR_VARINACC, name_loc, name);
						return -1;
					}
					break;

				case VAR_CLASS:
/* TODO: change code here ... */
					/* a class variable can be access by both instance methods and class methods */
					STIX_ASSERT (var->cls != STIX_NULL);
					STIX_ASSERT (STIX_CLASSOF(stix, var->cls) == stix->_class);

/* TOOD: index must be incremented witht eh number of classinstancevariables counts from var.cls 
 * verify if the below increment is correct*/
					var->pos += STIX_CLASS_NAMED_INSTVARS + 
					            STIX_CLASS_SELFSPEC_CLASSINSTVAR(STIX_OOP_TO_SMINT(var->cls->selfspec));
					break;

				case VAR_CLASSINST:
					/* class instance variable can be accessed by only class methods */
					if (stix->c->mth.type == MTH_INSTANCE)
					{
						/* an instance method cannot access a class-instance variable */
						set_syntax_error (stix, STIX_SYNERR_VARINACC, name_loc, name);
						return -1;
					}

					/* to a class object itself, a class-instance variable is
					 * just an instance variriable. but these are located
					 * after the named instance variables. */
					var->pos += STIX_CLASS_NAMED_INSTVARS;
					break;

				default:
					/* internal error - it must not happen */
					stix->errnum = STIX_EINTERN;
					return -1;
			}
		}
		/* TODO 
		else if (find global variable... )
			var->type = VAR_GLOBAL;
		*/
		else
		{
			/* undeclared identifier */
			set_syntax_error (stix, STIX_SYNERR_VARUNDCL, name_loc, name);
			return -1;
		}
	}

	if (var->pos > MAX_CODE_INDEX)
	{
		/* the assignee is not usable because its index is too large 
		 * to be expressed in byte-codes. */
		set_syntax_error (stix, STIX_SYNERR_VARUNUSE, name_loc, name);
		return -1;
	}

	return 0;
}

static int compile_block_temporaries (stix_t* stix)
{
	/* 
	 * block-temporaries := "|" variable-list "|"
	 * variable-list := identifier*
	 */

	if (!is_token_binsel(stix, VOCA_VBAR)) 
	{
		/* return without doing anything if | is not found.
		 * this is not an error condition */
		return 0;
	}

	GET_TOKEN (stix);
	while (stix->c->tok.type == STIX_IOTOK_IDENT) 
	{
		if (find_temporary_variable(stix, &stix->c->tok.name) >= 0)
		{
			set_syntax_error (stix, STIX_SYNERR_TMPRNAMEDUP, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		if (add_temporary_variable(stix, &stix->c->tok.name) <= -1) return -1;
		stix->c->mth.tmpr_count++;
		if (stix->c->mth.tmpr_count > MAX_CODE_NTMPRS)
		{
			set_syntax_error (stix, STIX_SYNERR_TMPRFLOOD, &stix->c->tok.loc, &stix->c->tok.name); 
			return -1;
		}

		GET_TOKEN (stix);
	}

	if (!is_token_binsel(stix, VOCA_VBAR)) 
	{
		set_syntax_error (stix, STIX_SYNERR_VBAR, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	GET_TOKEN (stix);
	return 0;
}

static int compile_block_expression (stix_t* stix)
{
	stix_size_t jump_inst_pos;
	stix_size_t saved_tmpr_count, saved_tmprs_len;
	stix_size_t block_arg_count, block_tmpr_count;
	stix_size_t block_code_size;
	stix_ioloc_t block_loc, colon_loc, tmpr_loc;

	/*
	 * block-expression := "[" block-body "]"
	 * block-body := (block-argument* "|")? block-temporaries? method-statement*
	 * block-argument := ":" identifier
	 */

	/* this function expects [ not to be consumed away */
	STIX_ASSERT (stix->c->tok.type == STIX_IOTOK_LBRACK);
	block_loc = stix->c->tok.loc;
	GET_TOKEN (stix);

	saved_tmprs_len = stix->c->mth.tmprs.len;
	saved_tmpr_count = stix->c->mth.tmpr_count;

	if (stix->c->tok.type == STIX_IOTOK_COLON) 
	{
		colon_loc = stix->c->tok.loc;

		/* block temporary variables */
		do 
		{
			GET_TOKEN (stix);

			if (stix->c->tok.type != STIX_IOTOK_IDENT) 
			{
				/* wrong argument name. identifier expected */
				set_syntax_error (stix, STIX_SYNERR_IDENT, &stix->c->tok.loc, &stix->c->tok.name);
				return -1;
			}

/* TODO: check conflicting names as well */
			if (find_temporary_variable(stix, &stix->c->tok.name) >= 0)
			{
				set_syntax_error (stix, STIX_SYNERR_BLKARGNAMEDUP, &stix->c->tok.loc, &stix->c->tok.name);
				return -1;
			}

			if (add_temporary_variable(stix, &stix->c->tok.name) <= -1) return -1;
			stix->c->mth.tmpr_count++;
			if (stix->c->mth.tmpr_count > MAX_CODE_NARGS)
			{
				set_syntax_error (stix, STIX_SYNERR_BLKARGFLOOD, &stix->c->tok.loc, &stix->c->tok.name); 
				return -1;
			}

			GET_TOKEN (stix);
		} 
		while (stix->c->tok.type == STIX_IOTOK_COLON);

		if (!is_token_binsel(stix, VOCA_VBAR))
		{
			set_syntax_error (stix, STIX_SYNERR_VBAR, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		GET_TOKEN (stix);
	}

	block_arg_count = stix->c->mth.tmpr_count - saved_tmpr_count;
	if (block_arg_count > MAX_CODE_NBLKARGS)
	{
		/* while an integer object is pused to indicate the number of
		 * block arguments, evaluation which is done by message passing
		 * limits the number of arguments that can be passed. so the
		 * check is implemented */
		set_syntax_error (stix, STIX_SYNERR_BLKARGFLOOD, &colon_loc, STIX_NULL); 
		return -1;
	}

	tmpr_loc = stix->c->tok.loc;
	if (compile_block_temporaries(stix) <= -1) return -1;

	/* this is a block-local temporary count */
	block_tmpr_count = stix->c->mth.tmpr_count - saved_tmpr_count;
	if (block_tmpr_count > MAX_CODE_NBLKTMPRS)
	{
		set_syntax_error (stix, STIX_SYNERR_BLKTMPRFLOOD, &tmpr_loc, STIX_NULL); 
		return -1;
	}

printf ("\tpush_context nargs %d ntmprs %d\n", (int)block_arg_count, (int)stix->c->mth.tmpr_count /*block_tmpr_count*/);
printf ("\tpush smint %d\n", (int)block_arg_count);
printf ("\tpush smint %d\n", (int)stix->c->mth.tmpr_count /*block_tmpr_count*/);
printf ("\tsend_block_copy\n");
	if (emit_byte_instruction(stix, BCODE_PUSH_CONTEXT) <= -1 ||
	    emit_push_smint_literal(stix, block_arg_count) <= -1 ||
	    emit_push_smint_literal(stix, stix->c->mth.tmpr_count/*block_tmpr_count*/) <= -1 ||
	    emit_byte_instruction(stix, BCODE_SEND_BLOCK_COPY) <= -1) return -1;

printf ("\tjump\n");
	/* insert dummy instructions before replacing them with a jump instruction */
	jump_inst_pos = stix->c->mth.code.len;
#if (STIX_BCODE_LONG_PARAM_SIZE == 2)
	if (emit_byte_instruction(stix, BCODE_JUMP_FORWARD_X) <= -1 ||
	    emit_byte_instruction(stix, 0) <= -1 ||
	    emit_byte_instruction(stix, 0) <= -1) return -1;
#else
	if (emit_byte_instruction(stix, BCODE_JUMP_FORWARD_X) <= -1 ||
	    emit_byte_instruction(stix, 0) <= -1) return -1;
#endif

	/* compile statements inside a block */
	if (stix->c->tok.type == STIX_IOTOK_RBRACK)
	{
		/* the block is empty */
		if (emit_byte_instruction (stix, BCODE_PUSH_NIL) <= -1) return -1;
		GET_TOKEN (stix);
	}
	else 
	{
		while (stix->c->tok.type != STIX_IOTOK_EOF)
		{
			if (compile_block_statement(stix) <= -1) return -1;

			if (stix->c->tok.type == STIX_IOTOK_RBRACK) break;
			else if (stix->c->tok.type == STIX_IOTOK_PERIOD)
			{
				GET_TOKEN (stix);
				if (stix->c->tok.type == STIX_IOTOK_RBRACK) break;
				if (emit_byte_instruction(stix, BCODE_POP_STACKTOP) <= -1) return -1;
			}
			else
			{
				set_syntax_error (stix, STIX_SYNERR_RBRACK, &stix->c->tok.loc, &stix->c->tok.name);
				return -1;
			}
		}

printf ("\treturn_from_block\n");
		if (emit_byte_instruction(stix, BCODE_RETURN_FROM_BLOCK) <= -1) return -1;
	}

	block_code_size = stix->c->mth.code.len - jump_inst_pos - (STIX_BCODE_LONG_PARAM_SIZE + 1);
	if (block_code_size > MAX_CODE_BLKCODE)
	{
/* TOOD: increate the max block code size and 
 * if it exceedes the limit for BCODE_JUMP_FORWARD_X, switch to BECODE_JUMP_BY_OFFSET */
		set_syntax_error (stix, STIX_SYNERR_BLKFLOOD, &block_loc, STIX_NULL); 
		return -1;
	}

	/* note that the jump offset is a signed number */
	stix->c->mth.code.ptr[jump_inst_pos + 1] = block_code_size >> 8;
	stix->c->mth.code.ptr[jump_inst_pos + 2] = ((stix_int16_t)block_code_size & 0xFF);

	/* restore the temporary count */
	stix->c->mth.tmpr_count = saved_tmpr_count;
	stix->c->mth.tmprs.len = saved_tmprs_len;

	GET_TOKEN (stix);

	return 0;
}


static int add_to_balit_buffer (stix_t* stix, stix_byte_t b)
{
	if (stix->c->mth.balit_count >= stix->c->mth.balit_capa)
	{
		stix_byte_t* tmp;
		stix_size_t new_capa;

		new_capa = STIX_ALIGN (stix->c->mth.balit_count + 1, BALIT_BUFFER_ALIGN);
		tmp = (stix_byte_t*)stix_reallocmem (stix, stix->c->mth.balit, new_capa * STIX_SIZEOF(*tmp));
		if (!tmp) return -1;

		stix->c->mth.balit_capa = new_capa;
		stix->c->mth.balit = tmp;
	}

	stix->c->mth.balit[stix->c->mth.balit_count++] = b;
	return 0;
}

static int add_to_arlit_buffer (stix_t* stix, stix_oop_t item)
{
	if (stix->c->mth.arlit_count >= stix->c->mth.arlit_capa)
	{
		stix_oop_t* tmp;
		stix_size_t new_capa;

		new_capa = STIX_ALIGN (stix->c->mth.arlit_count + 1, ARLIT_BUFFER_ALIGN);
		tmp = (stix_oop_t*)stix_reallocmem (stix, stix->c->mth.arlit, new_capa * STIX_SIZEOF(*tmp));
		if (!tmp) return -1;

		stix->c->mth.arlit_capa = new_capa;
		stix->c->mth.arlit = tmp;
	}

/* TODO: overflow check of stix->c->mth.arlit_count */
	stix->c->mth.arlit[stix->c->mth.arlit_count++] = item;
	return 0;
}


static int __compile_byte_array_literal (stix_t* stix, stix_oop_t* xlit)
{
	stix_ooi_t tmp;
	stix_oop_t ba;

	stix->c->mth.balit_count = 0;

	while (stix->c->tok.type == STIX_IOTOK_NUMLIT || stix->c->tok.type == STIX_IOTOK_RADNUMLIT)
	{
		/* TODO: check if the number is an integer */

		if (string_to_smint(stix, &stix->c->tok.name, stix->c->tok.type == STIX_IOTOK_RADNUMLIT, &tmp) <= -1)
		{
			/* the token reader reads a valid token. no other errors
			 * than the range error must not occur */
			STIX_ASSERT (stix->errnum == STIX_ERANGE);

printf ("NOT IMPLEMENTED LARGE_INTEGER or ERROR?\n");
			stix->errnum = STIX_ENOIMPL;
			return -1;
		}
		else if (tmp < 0 || tmp > 255)
		{
			set_syntax_error (stix, STIX_SYNERR_BYTERANGE, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		if (add_to_balit_buffer(stix, tmp) <= -1) return -1;
		GET_TOKEN (stix);
	}

	if (stix->c->tok.type != STIX_IOTOK_RBRACK)
	{
		set_syntax_error (stix, STIX_SYNERR_RBRACK, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	ba = stix_instantiate (stix, stix->_byte_array, stix->c->mth.balit, stix->c->mth.balit_count);
	if (!ba) return -1;

	*xlit = ba;
	return 0;
}

struct arlit_info_t
{
	stix_size_t pos;
	stix_size_t len;
};

typedef struct arlit_info_t arlit_info_t;

static int __compile_array_literal (stix_t* stix, stix_oop_t* xlit)
{
	stix_oop_t lit, a;
	stix_size_t i, saved_arlit_count;
	arlit_info_t info;

	info.pos = stix->c->mth.arlit_count;
	info.len = 0;

	do
	{
		switch (stix->c->tok.type)
		{
/* TODO: floating pointer number */

			case STIX_IOTOK_NUMLIT:
			case STIX_IOTOK_RADNUMLIT:
			{
				stix_ooi_t tmp;

				if (string_to_smint(stix, &stix->c->tok.name, stix->c->tok.type == STIX_IOTOK_RADNUMLIT, &tmp) <= -1)
				{
					/* the token reader reads a valid token. no other errors
					 * than the range error must not occur */
					STIX_ASSERT (stix->errnum == STIX_ERANGE);

/* TODO: IMPLMENET LARGE INTEGER */
printf ("LARGE NOT IMPLEMENTED IN COMPILE_ARRAY_LITERAL\n");
					stix->errnum = STIX_ENOIMPL;
					return -1;
				}

				lit = STIX_OOP_FROM_SMINT(tmp);
				break;
			}

			case STIX_IOTOK_CHARLIT:
				STIX_ASSERT (stix->c->tok.name.len == 1);
				lit = STIX_OOP_FROM_CHAR(stix->c->tok.name.ptr[0]);
				break;

			case STIX_IOTOK_STRLIT:
				lit = stix_instantiate (stix, stix->_string, stix->c->tok.name.ptr, stix->c->tok.name.len);
				break;

			case STIX_IOTOK_IDENT:
			case STIX_IOTOK_BINSEL:
			case STIX_IOTOK_KEYWORD:
			case STIX_IOTOK_SYMLIT:
			case STIX_IOTOK_SELF:
			case STIX_IOTOK_SUPER:
			case STIX_IOTOK_THIS_CONTEXT:
				lit = stix_makesymbol (stix, stix->c->tok.name.ptr, stix->c->tok.name.len);
				break;

			case STIX_IOTOK_NIL:
				lit = stix->_nil;
				break;

			case STIX_IOTOK_TRUE:
				lit = stix->_true;
				break;

			case STIX_IOTOK_FALSE:
				lit = stix->_false;
				break;

			case STIX_IOTOK_APAREN: /* #( */
			case STIX_IOTOK_LPAREN: /* ( */
				saved_arlit_count = stix->c->mth.arlit_count;
/* TODO: get rid of recursion?? */
				GET_TOKEN (stix);
				if (__compile_array_literal (stix, &lit) <= -1) return -1;
				stix->c->mth.arlit_count = saved_arlit_count;
				break;

			case STIX_IOTOK_BPAREN: /* #[ */
			case STIX_IOTOK_LBRACK: /* [ */
				GET_TOKEN (stix);
				if (__compile_byte_array_literal (stix, &lit) <= -1) return -1;
				break;

			default:
				goto done;
		}

		if (!lit || add_to_arlit_buffer(stix, lit) <= -1) return -1;
		info.len++;

		GET_TOKEN (stix);
	}
	while (1);

done:
	if (stix->c->tok.type != STIX_IOTOK_RPAREN)
	{
		set_syntax_error (stix, STIX_SYNERR_RPAREN, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	a = stix_instantiate (stix, stix->_array, STIX_NULL, info.len);
	if (!a) return -1;

	for (i = 0; i < info.len; i++)
	{
		((stix_oop_oop_t)a)->slot[i] = stix->c->mth.arlit[info.pos + i];
	}

	*xlit = a;
	return 0;
}

static int compile_byte_array_literal (stix_t* stix)
{
	stix_oop_t lit;
	stix_size_t index;

	GET_TOKEN (stix); /* skip #[ and read the next token */
	if (__compile_byte_array_literal (stix, &lit) <= -1) return -1;

printf ("\tpush_literal byte_array\n");
	if (add_literal (stix, lit, &index) <= -1 ||
	    emit_single_param_instruction (stix, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;

	GET_TOKEN (stix);
	return 0;
}

static int compile_array_literal (stix_t* stix)
{
	stix_oop_t lit;
	stix_size_t index;
	int x;

	stix->c->in_array = 1;
	GET_TOKEN (stix); /* skip #( and read the next token */
	x = __compile_array_literal (stix, &lit);
	stix->c->in_array = 0;
	if (x <= -1) return -1;

printf ("\tpush_literal array\n");
	if (add_literal (stix, lit, &index) <= -1 ||
	    emit_single_param_instruction (stix, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;

	GET_TOKEN (stix);
	return 0;
}

static int compile_expression_primary (stix_t* stix, const stix_ucs_t* ident, const stix_ioloc_t* ident_loc, int* to_super)
{
	/*
	 * expression-primary := identifier | literal | block-constructor | ( "(" method-expression ")" )
	 */

	var_info_t var;
	int read_next_token = 0;
	stix_size_t index;

	*to_super = 0;

	if (ident) 
	{
		/* the caller has read the identifier and the next word */

	handle_ident:
		if (get_variable_info(stix, ident, ident_loc, &var) <= -1) return -1;

		switch (var.type)
		{
			case VAR_ARGUMENT:
			case VAR_TEMPORARY:
				if (emit_single_param_instruction(stix, BCODE_PUSH_TEMPVAR_0, var.pos) <= -1) return -1;
printf ("\tpush tempvar %d\n", (int)var.pos);
				break;

			case VAR_INSTANCE:
			case VAR_CLASSINST:
				if (emit_single_param_instruction(stix, BCODE_PUSH_INSTVAR_0, var.pos) <= -1) return -1;
printf ("\tpush instvar %d\n", (int)var.pos);
				break;

			case VAR_CLASS:
				if (add_literal(stix, (stix_oop_t)var.cls, &index) <= -1 ||
				    emit_double_param_instruction(stix, BCODE_PUSH_OBJVAR_0, var.pos, index) <= -1) return -1;
printf ("\tpush objvar %d %d\n", (int)var.pos, (int)index);
				break;

			case VAR_GLOBAL:
/* TODO: .............................. */
printf ("GLOBAL NOT IMPLMENTED.... \n");
				stix->errnum = STIX_ENOIMPL;
				return -1;

			default:
				stix->errnum = STIX_EINTERN;
				return -1;
		}

		if (read_next_token) GET_TOKEN (stix);
	}
	else
	{
		switch (stix->c->tok.type)
		{
			case STIX_IOTOK_IDENT:
				ident = &stix->c->tok.name;
				ident_loc = &stix->c->tok.loc;
				read_next_token = 1;
				goto handle_ident;

			case STIX_IOTOK_SELF:
printf ("\tpush receiver...\n");
				if (emit_byte_instruction(stix, BCODE_PUSH_RECEIVER) <= -1) return -1;
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_SUPER:
printf ("\tpush receiver(super)...\n");
				if (emit_byte_instruction(stix, BCODE_PUSH_RECEIVER) <= -1) return -1;
				GET_TOKEN (stix);
				*to_super = 1;
				break;

			case STIX_IOTOK_NIL:
printf ("\tpush nil...\n");
				if (emit_byte_instruction(stix, BCODE_PUSH_NIL) <= -1) return -1;
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_TRUE:
printf ("\tpush true...\n");
				if (emit_byte_instruction(stix, BCODE_PUSH_TRUE) <= -1) return -1;
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_FALSE:
printf ("\tpush false...\n");
				if (emit_byte_instruction(stix, BCODE_PUSH_FALSE) <= -1) return -1;
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_THIS_CONTEXT:
printf ("\tpush context...\n");
				if (emit_byte_instruction(stix, BCODE_PUSH_CONTEXT) <= -1) return -1;
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_CHARLIT:
				STIX_ASSERT (stix->c->tok.name.len == 1);
				if (add_character_literal(stix, stix->c->tok.name.ptr[0], &index) <= -1 ||
				    emit_single_param_instruction(stix, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;
printf ("\tpush character literal %d\n", (int)index);
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_STRLIT:
				if (add_string_literal(stix, &stix->c->tok.name, &index) <= -1 ||
				    emit_single_param_instruction(stix, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;
printf ("\tpush string literal %d\n", (int)index);
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_SYMLIT:
				if (add_symbol_literal(stix, &stix->c->tok.name, &index) <= -1 ||
				    emit_single_param_instruction(stix, BCODE_PUSH_LITERAL_0, index) <= -1) return -1;
printf ("\tpush symbol literal %d\n", (int)index);
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_NUMLIT:
			case STIX_IOTOK_RADNUMLIT:
			{
				/* TODO: other types of numbers, negative numbers, etc */
/* TODO: proper numbeic literal handling */
				stix_ooi_t tmp;

				if (string_to_smint(stix, &stix->c->tok.name, stix->c->tok.type == STIX_IOTOK_RADNUMLIT, &tmp) <= -1)
				{
					/* the token reader reads a valid token. no other errors
					 * than the range error must not occur */
					STIX_ASSERT (stix->errnum == STIX_ERANGE);

printf ("NOT IMPLEMENTED LARGE_INTEGER or ERROR?\n");
					stix->errnum = STIX_ENOIMPL;
					return -1;
				}
				else
				{
printf ("\tpush int literal\n");
					if (emit_push_smint_literal(stix, tmp) <= -1) return -1;
				}

				GET_TOKEN (stix);
				break;
			}

			case STIX_IOTOK_BPAREN: /* #[ */
				/*GET_TOKEN (stix);*/
				if (compile_byte_array_literal(stix) <= -1) return -1;
				break;

			case STIX_IOTOK_APAREN: /* #( */
				/*GET_TOKEN (stix);*/
				if (compile_array_literal(stix) <= -1) return -1;
				break;

			/* TODO: dynamic array, non constant array #<> or #{} or what is a better bracket? */

			case STIX_IOTOK_LBRACK: /* [ */
				/*GET_TOKEN (stix);*/
				if (compile_block_expression(stix) <= -1) return -1;
				break;

			case STIX_IOTOK_LPAREN:
				GET_TOKEN (stix);
				if (compile_method_expression(stix, 0) <= -1) return -1;
				if (stix->c->tok.type != STIX_IOTOK_RPAREN)
				{
					set_syntax_error (stix, STIX_SYNERR_RPAREN, &stix->c->tok.loc, &stix->c->tok.name);
					return -1;
				}
				GET_TOKEN (stix);
				break;

			default:
				set_syntax_error (stix, STIX_SYNERR_PRIMARY, &stix->c->tok.loc, &stix->c->tok.name);
				return -1;
		}
	}

	return 0;
}

static stix_byte_t send_message_cmd[] = 
{
	BCODE_SEND_MESSAGE_0,
	BCODE_SEND_MESSAGE_TO_SUPER_0
};

static int compile_unary_message (stix_t* stix, int to_super)
{
	stix_size_t index;

	STIX_ASSERT (stix->c->tok.type == STIX_IOTOK_IDENT);

	do
	{
		if (add_symbol_literal(stix, &stix->c->tok.name, &index) <= -1 ||
		    emit_double_param_instruction(stix, send_message_cmd[to_super], 0, index) <= -1) return -1;
printf ("\tsend unary message %d [", (int)index);
print_ucs (&stix->c->tok.name);
printf ("] with 0 arguments %s\n", (to_super? " to super": ""));

		GET_TOKEN (stix);
	}
	while (stix->c->tok.type == STIX_IOTOK_IDENT);

	return 0;
}

static int compile_binary_message (stix_t* stix, int to_super)
{
	/*
	 * binary-message := binary-selector binary-argument
	 * binary-argument := expression-primary unary-message*
	 */
	stix_size_t index;
	int to_super2;
	stix_ucs_t binsel;
	stix_size_t saved_binsels_len;

	STIX_ASSERT (stix->c->tok.type == STIX_IOTOK_BINSEL);

	do
	{
		binsel = stix->c->tok.name;
		saved_binsels_len = stix->c->mth.binsels.len;

		if (clone_binary_selector(stix, &binsel) <= -1) goto oops;

		GET_TOKEN (stix);

		if (compile_expression_primary(stix, STIX_NULL, STIX_NULL, &to_super2) <= -1) goto oops;

		if (stix->c->tok.type == STIX_IOTOK_IDENT && compile_unary_message(stix, to_super2) <= -1) goto oops;

		if (add_symbol_literal(stix, &binsel, &index) <= -1 ||
		    emit_double_param_instruction(stix, send_message_cmd[to_super], 1, index) <= -1) goto oops;
printf ("\tsend binary message %d [", (int)index);
print_ucs (&binsel);
printf ("] with 1 arguments %s\n", (to_super? " to super": ""));

		stix->c->mth.binsels.len = saved_binsels_len;
	}
	while (stix->c->tok.type == STIX_IOTOK_BINSEL);

	return 0;

oops:
	stix->c->mth.binsels.len = saved_binsels_len;
	return -1;
}

static int compile_keyword_message (stix_t* stix, int to_super)
{
	/*
	 * keyword-message := (keyword keyword-argument)+
	 * keyword-argument := expression-primary unary-message* binary-message*
	 */

	stix_size_t index;
	int to_super2;
	stix_ucs_t kw, kwsel;
	stix_ioloc_t saved_kwsel_loc;
	stix_size_t saved_kwsel_len;
	stix_size_t nargs = 0;

	saved_kwsel_loc = stix->c->tok.loc;
	saved_kwsel_len = stix->c->mth.kwsels.len;

	do 
	{
		kw = stix->c->tok.name;
		if (clone_keyword(stix, &kw) <= -1) goto oops;

		GET_TOKEN (stix);

		if (compile_expression_primary(stix, STIX_NULL, STIX_NULL, &to_super2) <= -1) goto oops;
		if (stix->c->tok.type == STIX_IOTOK_IDENT && compile_unary_message(stix, to_super2) <= -1) goto oops;
		if (stix->c->tok.type == STIX_IOTOK_BINSEL && compile_binary_message(stix, to_super2) <= -1) goto oops;

		if (nargs >= MAX_CODE_NARGS)
		{
			/* 'kw' points to only one segment of the full keyword message. 
			 * if it parses an expression like 'aBlock value: 10 with: 20',
			 * 'kw' may point to 'value:' or 'with:'.
			 */
			set_syntax_error (stix, STIX_SYNERR_ARGFLOOD, &saved_kwsel_loc, &kw); 
			goto oops;
		}

		nargs++;
	} 
	while (stix->c->tok.type == STIX_IOTOK_KEYWORD);

	kwsel.ptr = &stix->c->mth.kwsels.ptr[saved_kwsel_len];
	kwsel.len = stix->c->mth.kwsels.len - saved_kwsel_len;

	if (add_symbol_literal(stix, &kwsel, &index) <= -1 ||
	    emit_double_param_instruction(stix, send_message_cmd[to_super], nargs, index) <= -1) goto oops;

printf ("\tsend keyword message %d [", (int)index);
print_ucs (&kwsel);
printf ("] with %d arguments to %s\n", (int)nargs, (to_super? "super": "self"));
	stix->c->mth.kwsels.len = saved_kwsel_len;
	return 0;

oops:
	stix->c->mth.kwsels.len = saved_kwsel_len;
	return -1;
}

static int compile_message_expression (stix_t* stix, int to_super)
{
	/*
	 * message-expression := single-message cascaded-message
	 * single-message := 
	 *     (unary-message+ binary-message* keyword-message?) |
	 *     (binary-message+ keyword-message?) |
	 *     keyword-message
	 * 
	 * keyword-message := (keyword keyword-argument)+
	 * keyword-argument := expression-primary unary-message* binary-message*
	 * binary-message := binary-selector binary-argument
	 * binary-argument := expression-primary unary-message*
	 * unary-message := unary-selector
	 * cascaded-message := (";" single-message)*
	 */
	stix_size_t noop_pos;

	do
	{
		switch (stix->c->tok.type)
		{
			case STIX_IOTOK_IDENT:
				/* insert NOOP to change to DUP_STACKTOP if there is a 
				 * cascaded message */
				noop_pos = stix->c->mth.code.len;
				if (emit_byte_instruction(stix, BCODE_NOOP) <= -1) return -1;

				if (compile_unary_message(stix, to_super) <= -1) return -1;

				if (stix->c->tok.type == STIX_IOTOK_BINSEL)
				{
	STIX_ASSERT (stix->c->mth.code.len > noop_pos);
	STIX_MEMMOVE (&stix->c->mth.code.ptr[noop_pos], &stix->c->mth.code.ptr[noop_pos + 1], stix->c->mth.code.len - noop_pos - 1);
	stix->c->mth.code.len--;

	noop_pos = stix->c->mth.code.len;
	if (emit_byte_instruction(stix, BCODE_NOOP) <= -1) return -1;
					if (compile_binary_message(stix, to_super) <= -1) return -1;
				}
				if (stix->c->tok.type == STIX_IOTOK_KEYWORD)
				{
	STIX_ASSERT (stix->c->mth.code.len > noop_pos);
	STIX_MEMMOVE (&stix->c->mth.code.ptr[noop_pos], &stix->c->mth.code.ptr[noop_pos + 1], stix->c->mth.code.len - noop_pos - 1);
	stix->c->mth.code.len--;

	noop_pos = stix->c->mth.code.len;
	if (emit_byte_instruction(stix, BCODE_NOOP) <= -1) return -1;
					if (compile_keyword_message(stix, to_super) <= -1) return -1;
				}
				break;

			case STIX_IOTOK_BINSEL:
				noop_pos = stix->c->mth.code.len;
				if (emit_byte_instruction(stix, BCODE_NOOP) <= -1) return -1;

				if (compile_binary_message(stix, to_super) <= -1) return -1;
				if (stix->c->tok.type == STIX_IOTOK_KEYWORD)
				{
	STIX_ASSERT (stix->c->mth.code.len > noop_pos);
	STIX_MEMMOVE (&stix->c->mth.code.ptr[noop_pos], &stix->c->mth.code.ptr[noop_pos + 1], stix->c->mth.code.len - noop_pos - 1);
	stix->c->mth.code.len--;

	noop_pos = stix->c->mth.code.len;
	if (emit_byte_instruction(stix, BCODE_NOOP) <= -1) return -1;

					if (compile_keyword_message(stix, to_super) <= -1) return -1;
				}
				break;

			case STIX_IOTOK_KEYWORD:
				noop_pos = stix->c->mth.code.len;
				if (emit_byte_instruction(stix, BCODE_NOOP) <= -1) return -1;

				if (compile_keyword_message(stix, to_super) <= -1) return -1;
				break;

			default:
				goto done;

		}

		if (stix->c->tok.type == STIX_IOTOK_SEMICOLON)
		{
			printf ("\tdup_stacktop for cascading\n");
			stix->c->mth.code.ptr[noop_pos] = BCODE_DUP_STACKTOP;
			if (emit_byte_instruction(stix, BCODE_POP_STACKTOP) <= -1) return -1;
			GET_TOKEN(stix);
		}
		else 
		{
			/* delete the NOOP instruction inserted  */
			STIX_ASSERT (stix->c->mth.code.len > noop_pos);
			STIX_MEMMOVE (&stix->c->mth.code.ptr[noop_pos], &stix->c->mth.code.ptr[noop_pos + 1], stix->c->mth.code.len - noop_pos - 1);
			stix->c->mth.code.len--;
			goto done;
		}
	}
	while (1);

#if 0
	if (compile_keyword_message(stix, to_super) <= -1) return -1;

	while (stix->c->tok.type == STIX_IOTOK_SEMICOLON) 
	{
		printf ("\tpop_stacktop for cascading\n");
		if (emit_byte_instruction(stix, CODE_POP_STACKTOP) <= -1) return -1;
		GET_TOKEN (stix);
		if (compile_keyword_message(stix, 0) <= -1) return -1;
	}
#endif

done:
	return 0;
}

static int compile_basic_expression (stix_t* stix, const stix_ucs_t* ident, const stix_ioloc_t* ident_loc)
{
	/*
	 * basic-expression := expression-primary message-expression?
	 */
	int to_super;

	if (compile_expression_primary(stix, ident, ident_loc, &to_super) <= -1) return -1;
	if (stix->c->tok.type != STIX_IOTOK_EOF && 
	    stix->c->tok.type != STIX_IOTOK_RBRACE && 
	    stix->c->tok.type != STIX_IOTOK_PERIOD &&
	    stix->c->tok.type != STIX_IOTOK_SEMICOLON)
	{
		if (compile_message_expression(stix, to_super) <= -1) return -1;
	}

	return 0;
}

static int compile_method_expression (stix_t* stix, int pop)
{
	/*
	 * method-expression := method-assignment-expression | basic-expression
	 * method-assignment-expression := identifier ":=" method-expression
	 */

	stix_ucs_t assignee;
	stix_size_t index;
	int ret = 0;

	STIX_ASSERT (pop == 0 || pop == 1);
	STIX_MEMSET (&assignee, 0, STIX_SIZEOF(assignee));

	if (stix->c->tok.type == STIX_IOTOK_IDENT) 
	{
		stix_ioloc_t assignee_loc;
		/* store the assignee name to the internal buffer
		 * to make it valid after the token buffer has been overwritten */
		assignee = stix->c->tok.name;
		if (clone_assignee(stix, &assignee) <= -1) return -1;

		assignee_loc = stix->c->tok.loc;

		GET_TOKEN (stix);
		if (stix->c->tok.type == STIX_IOTOK_ASSIGN) 
		{
			/* assignment expression */
			var_info_t var;

			GET_TOKEN (stix);

printf ("ASSIGNIUNG TO ....");
print_ucs (&assignee);
printf ("\n");

			if (compile_method_expression(stix, 0) <= -1 ||
			    get_variable_info(stix, &assignee, &assignee_loc, &var) <= -1) goto oops;

			switch (var.type)
			{
				case VAR_ARGUMENT:
					/* assigning to an argument is not allowed */
					set_syntax_error (stix, STIX_SYNERR_VARARG, &assignee_loc, &assignee);
					goto oops;

				case VAR_TEMPORARY:
printf ("\tstore_into_tempvar %d\n", (int)var.pos);
/* TODO: if pop is 1, emit BCODE_POP_INTO_TEMPVAR.
ret = pop;
*/
					if (emit_single_param_instruction (stix, BCODE_STORE_INTO_TEMPVAR_0, var.pos) <= -1) goto oops;
					break;

				case VAR_INSTANCE:
				case VAR_CLASSINST:
printf ("\tstore_into_instvar %d\n", (int)var.pos);
/* TODO: if pop is 1, emit BCODE_POP_INTO_INSTVAR 
ret = pop;
*/
					if (emit_single_param_instruction (stix, BCODE_STORE_INTO_INSTVAR_0, var.pos) <= -1) goto oops;
					break;

				case VAR_CLASS:
/* TODO is this correct? */
					if (add_literal (stix, (stix_oop_t)var.cls, &index) <= -1 ||
					    emit_double_param_instruction (stix, BCODE_STORE_INTO_OBJVAR_0, var.pos, index) <= -1) goto oops;
printf ("\tstore_into_objvar %d %d\n", (int)var.pos, (int)index);
					break;

				case VAR_GLOBAL:
					/* TODO: .............................. */
printf ("\tSTORE_INTO_GLOBL NOT IMPLEMENTED YET\n");
					goto oops;

				default:
					stix->errnum = STIX_EINTERN;
					goto oops;
			}
		}
		else 
		{
			if (compile_basic_expression(stix, &assignee, &assignee_loc) <= -1) goto oops;
		}
	}
	else 
	{
		assignee.len = 0;
		if (compile_basic_expression(stix, STIX_NULL, STIX_NULL) <= -1) goto oops;
	}

	stix->c->mth.assignees.len -= assignee.len;
	return ret;

oops:
	stix->c->mth.assignees.len -= assignee.len;
	return -1;
}

static int compile_block_statement (stix_t* stix)
{
	/* compile_block_statement() is a simpler version of
	 * of compile_method_statement(). it doesn't cater for
	 * popping the stack top */
	if (stix->c->tok.type == STIX_IOTOK_RETURN) 
	{
		/* handle the return statement */
		GET_TOKEN (stix);
		if (compile_method_expression(stix, 0) <= -1) return -1;
printf ("\treturn_stacktop\n");
		return emit_byte_instruction (stix, BCODE_RETURN_STACKTOP);
	}
	else
	{
		return compile_method_expression(stix, 0);
	}
}

static int compile_method_statement (stix_t* stix)
{
	/*
	 * method-statement := method-return-statement | method-expression
	 * method-return-statement := "^" method-expression
	 */

	if (stix->c->tok.type == STIX_IOTOK_RETURN) 
	{
		/* handle the return statement */
		GET_TOKEN (stix);
		if (compile_method_expression(stix, 0) <= -1) return -1;
printf ("\treturn_stacktop\n");
		return emit_byte_instruction (stix, BCODE_RETURN_STACKTOP);
	}
	else 
	{
/* TODO: optimization. if expresssion is a literal, no push and pop are required */
		int n;

		/* the second parameter to compile_method_expression() indicates 
		 * that the stack top will eventually be popped off. the compiler
		 * can optimize some instruction sequencese. for example, two 
		 * consecutive store and pop intructions can be transformed to 
		 * a more specialized single pop-and-store instruction. */
		n = compile_method_expression(stix, 1);
		if (n <= -1) return -1;

		/* if n is 1, no stack popping is required */
if (n == 0) printf ("\tpop_stacktop\n");
		return (n == 0)? emit_byte_instruction (stix, BCODE_POP_STACKTOP): 0;
	}
}


static int compile_method_statements (stix_t* stix)
{
	/*
	 * method-statements := method-statement ("." | ("." method-statements))*
	 */

	if (stix->c->tok.type != STIX_IOTOK_EOF &&
	    stix->c->tok.type != STIX_IOTOK_RBRACE)
	{
		do
		{
			if (compile_method_statement(stix) <= -1) return -1;

			if (stix->c->tok.type == STIX_IOTOK_PERIOD) 
			{
				/* period after a statement */
				GET_TOKEN (stix);

				if (stix->c->tok.type == STIX_IOTOK_EOF ||
				    stix->c->tok.type == STIX_IOTOK_RBRACE) break;
			}
			else
			{
				if (stix->c->tok.type == STIX_IOTOK_EOF ||
				    stix->c->tok.type == STIX_IOTOK_RBRACE) break;

				/* not a period, EOF, nor } */
				set_syntax_error (stix, STIX_SYNERR_PERIOD, &stix->c->tok.loc, &stix->c->tok.name);
				return -1;
			}
		}
		while (1);
	}

	/* arrange to return the receiver if execution reached 
	 * the end of the method without explicit return */
printf ("\treturn_receiver\n");
	return emit_byte_instruction (stix, BCODE_RETURN_RECEIVER);
}

static int add_compiled_method (stix_t* stix)
{
	stix_oop_t name; /* selector */
	stix_oop_method_t mth; /* method */
	stix_oop_byte_t code;
	stix_size_t tmp_count = 0;
	stix_size_t i;
	stix_ooi_t preamble_code, preamble_index;

	name = stix_makesymbol (stix, stix->c->mth.name.ptr, stix->c->mth.name.len);
	if (!name) return -1;
	stix_pushtmp (stix, &name); tmp_count++;

	/* The variadic data part passed to stix_instantiate() is not GC-safe */
	mth = (stix_oop_method_t)stix_instantiate (stix, stix->_method, STIX_NULL, stix->c->mth.literal_count);
	if (!mth) goto oops;
	for (i = 0; i < stix->c->mth.literal_count; i++)
	{
		/* let's do the variadic data initialization here */
		mth->slot[i] = stix->c->mth.literals[i];
	}
	stix_pushtmp (stix, (stix_oop_t*)&mth); tmp_count++;

	code = (stix_oop_byte_t)stix_instantiate (stix, stix->_byte_array, stix->c->mth.code.ptr, stix->c->mth.code.len);
	if (!code) goto oops;
	stix_pushtmp (stix, (stix_oop_t*)&code); tmp_count++;

	preamble_code = STIX_METHOD_PREAMBLE_NONE;
	preamble_index = 0;

	if (stix->c->mth.prim_no < 0)
	{
		if (stix->c->mth.code.len <= 0)
		{
			preamble_code = STIX_METHOD_PREAMBLE_RETURN_RECEIVER;
		}
		else
		{
			if (stix->c->mth.code.ptr[0] == BCODE_RETURN_RECEIVER)
			{
				preamble_code = STIX_METHOD_PREAMBLE_RETURN_RECEIVER;
			}
			else if (stix->c->mth.code.len >= 2 &&
			         stix->c->mth.code.ptr[0] == BCODE_PUSH_RECEIVER &&
			         stix->c->mth.code.ptr[1] == BCODE_RETURN_STACKTOP)
			{
				preamble_code = STIX_METHOD_PREAMBLE_RETURN_RECEIVER;
			}
			else
			{
				/* check if the method begins with 'return instavar' instruction */
				if (stix->c->mth.code.ptr[0] >= BCODE_PUSH_INSTVAR_0 &&
				    stix->c->mth.code.ptr[0] <= BCODE_PUSH_INSTVAR_7 &&
				    stix->c->mth.code.ptr[1] == BCODE_RETURN_STACKTOP)
				{
					preamble_code = STIX_METHOD_PREAMBLE_RETURN_INSTVAR;
					preamble_index = stix->c->mth.code.ptr[0] & 0x7; /* low 3 bits */
				}
				else if (stix->c->mth.code.ptr[0] == BCODE_PUSH_INSTVAR_X &&
				         stix->c->mth.code.ptr[STIX_BCODE_LONG_PARAM_SIZE + 1] == BCODE_RETURN_STACKTOP)
				{
					int i;
					preamble_code = STIX_METHOD_PREAMBLE_RETURN_INSTVAR;
					preamble_index = stix->c->mth.code.ptr[1];
					for (i = 2; i <= STIX_BCODE_LONG_PARAM_SIZE; i++)
					{
						preamble_index = (preamble_index << 8) | stix->c->mth.code.ptr[i];
					}
				}
			}
		}
	}
	else
	{
		preamble_code = STIX_METHOD_PREAMBLE_PRIMITIVE;
		preamble_index = stix->c->mth.prim_no;
	}

	STIX_ASSERT (preamble_index >= 0 && preamble_index <= 0xFFFF); /* TODO: replace 0xFFFF by a proper macro name */

	mth->owner = stix->c->cls.self_oop;
	mth->preamble = STIX_OOP_FROM_SMINT(STIX_METHOD_MAKE_PREAMBLE(preamble_code, preamble_index));
	mth->tmpr_count = STIX_OOP_FROM_SMINT(stix->c->mth.tmpr_count);
	mth->tmpr_nargs = STIX_OOP_FROM_SMINT(stix->c->mth.tmpr_nargs);
	mth->code = code;

	/*TODO: preserve source??? mth->text = stix->c->mth.text
the compiler must collect all source method string collected so far.
need to write code to collect string.
*/

	stix_poptmps (stix, tmp_count); tmp_count = 0;

	if (!stix_putatdic (stix, stix->c->cls.mthdic_oop[stix->c->mth.type], name, (stix_oop_t)mth)) goto oops;
	return 0;

oops:
	stix_poptmps (stix, tmp_count);
	return -1;
}

static int compile_method_definition (stix_t* stix)
{
	/* clear data required to compile a method */
	stix->c->mth.type = MTH_INSTANCE;
	stix->c->mth.text.len = 0;
	stix->c->mth.assignees.len = 0;
	stix->c->mth.binsels.len = 0;
	stix->c->mth.kwsels.len = 0;
	stix->c->mth.name.len = 0;
	STIX_MEMSET (&stix->c->mth.name_loc, 0, STIX_SIZEOF(stix->c->mth.name_loc));
	stix->c->mth.tmprs.len = 0;
	stix->c->mth.tmpr_count = 0;
	stix->c->mth.tmpr_nargs = 0;
	stix->c->mth.literal_count = 0;
	stix->c->mth.balit_count = 0;
	stix->c->mth.arlit_count = 0;
	stix->c->mth.code.len = 0;
	stix->c->mth.prim_no = -1;

	if (stix->c->tok.type == STIX_IOTOK_LPAREN)
	{
		/* process method modifiers  */
		GET_TOKEN (stix);

		if (is_token_symbol(stix, VOCA_CLASS))
		{
			/* #method(#class) */
			stix->c->mth.type = MTH_CLASS;
			GET_TOKEN (stix);
		}

		if (stix->c->tok.type != STIX_IOTOK_RPAREN)
		{
			/* ) expected */
			set_syntax_error (stix, STIX_SYNERR_RPAREN, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		GET_TOKEN (stix);
	}

	if (compile_method_name(stix) <= -1) return -1;

printf (">>METHOD ");
print_ucs (&stix->c->mth.name);
printf ("\n");
	if (stix->c->tok.type != STIX_IOTOK_LBRACE)
	{
		/* { expected */
		set_syntax_error (stix, STIX_SYNERR_LBRACE, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	GET_TOKEN (stix);

	if (compile_method_temporaries(stix) <= -1 ||
	    compile_method_primitive(stix) <= -1 ||
	    compile_method_statements(stix) <= -1) return -1;

	if (stix->c->tok.type != STIX_IOTOK_RBRACE)
	{
		/* } expected */
		set_syntax_error (stix, STIX_SYNERR_RBRACE, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}
	GET_TOKEN (stix);

	/* add a compiled method to the method dictionary */
	if (add_compiled_method(stix) <= -1) return -1;

	return 0;
}

static int make_defined_class (stix_t* stix)
{
	/* this function make a class object with no functions/methods */

	stix_oop_t tmp;
	stix_oow_t spec, self_spec;
	int just_made = 0;

	spec = STIX_CLASS_SPEC_MAKE (stix->c->cls.var_count[VAR_INSTANCE],  
	                             ((stix->c->cls.flags & CLASS_INDEXED)? 1: 0),
	                             stix->c->cls.indexed_type);

	self_spec = STIX_CLASS_SELFSPEC_MAKE (stix->c->cls.var_count[VAR_CLASS],
	                                      stix->c->cls.var_count[VAR_CLASSINST]);

#if 0
printf ("MAKING ... ");
print_ucs (&stix->c->cls.name);
printf (" instvars %d classvars %d classinstvars %d\n", (int)stix->c->cls.var_count[VAR_INSTANCE], (int)stix->c->cls.var_count[VAR_CLASS], (int)stix->c->cls.var_count[VAR_CLASSINST]);
#endif
	if (stix->c->cls.self_oop)
	{
		/* this is an internally created class object being defined. */

		STIX_ASSERT (STIX_CLASSOF(stix, stix->c->cls.self_oop) == stix->_class);
		STIX_ASSERT (STIX_OBJ_GET_FLAGS_KERNEL (stix->c->cls.self_oop) == 1);

		if (spec != STIX_OOP_TO_SMINT(stix->c->cls.self_oop->spec) ||
		    self_spec != STIX_OOP_TO_SMINT(stix->c->cls.self_oop->selfspec))
		{
			/* it conflicts with internal definition */
#if 0
printf (" CONFLICTING CLASS DEFINITION %lu %lu %lu %lu\n", 
		(unsigned long)spec, (unsigned long)self_spec,
		(unsigned long)STIX_OOP_TO_SMINT(stix->c->cls.self_oop->spec), (unsigned long)STIX_OOP_TO_SMINT(stix->c->cls.self_oop->selfspec)
);
#endif
			set_syntax_error (stix, STIX_SYNERR_CLASSCONTRA, &stix->c->cls.name_loc, &stix->c->cls.name);
			return -1;
		}
	}
	else
	{
		/* the class variables and class instance variables are placed
		 * inside the class object after the fixed part. */
		tmp = stix_instantiate (stix, stix->_class, STIX_NULL,
		                        stix->c->cls.var_count[VAR_CLASSINST] + stix->c->cls.var_count[VAR_CLASS]);
		if (!tmp) return -1;

		just_made = 1;
		stix->c->cls.self_oop = (stix_oop_class_t)tmp;

		STIX_ASSERT (STIX_CLASSOF(stix, stix->c->cls.self_oop) == stix->_class);

		stix->c->cls.self_oop->spec = STIX_OOP_FROM_SMINT(spec);
		stix->c->cls.self_oop->selfspec = STIX_OOP_FROM_SMINT(self_spec);
	}

/* TODO: check if the current class definition conflicts with the superclass.
 * if superclass is byte variable, the current class cannot be word variable or something else.
*  TODO: TODO: TODO:
 */
	STIX_OBJ_SET_FLAGS_KERNEL (stix->c->cls.self_oop, 2);

	stix->c->cls.self_oop->superclass = stix->c->cls.super_oop;

	tmp = stix_makesymbol (stix, stix->c->cls.name.ptr, stix->c->cls.name.len);
	if (!tmp) return -1;
	stix->c->cls.self_oop->name = (stix_oop_char_t)tmp;

	tmp = stix_makestring (stix, stix->c->cls.vars[0].ptr, stix->c->cls.vars[0].len);
	if (!tmp) return -1;
	stix->c->cls.self_oop->instvars = (stix_oop_char_t)tmp;

	tmp = stix_makestring (stix, stix->c->cls.vars[1].ptr, stix->c->cls.vars[1].len);
	if (!tmp) return -1;
	stix->c->cls.self_oop->classvars = (stix_oop_char_t)tmp;

	tmp = stix_makestring (stix, stix->c->cls.vars[2].ptr, stix->c->cls.vars[2].len);
	if (!tmp) return -1;
	stix->c->cls.self_oop->classinstvars = (stix_oop_char_t)tmp;

/* TOOD: good dictionary size */
	tmp = (stix_oop_t)stix_makedic (stix, stix->_method_dictionary, INSTANCE_METHOD_DICTIONARY_SIZE);
	if (!tmp) return -1;
	stix->c->cls.mthdic_oop[MTH_INSTANCE] = (stix_oop_set_t)tmp;

/* TOOD: good dictionary size */
	tmp = (stix_oop_t)stix_makedic (stix, stix->_method_dictionary, CLASS_METHOD_DICTIONARY_SIZE);
	if (!tmp) return -1;
	stix->c->cls.mthdic_oop[MTH_CLASS] = (stix_oop_set_t)tmp;

/* TODO: initialize more fields??? whatelse. */

/* TODO: update the subclasses field of the superclass if it's not nil */

	if (just_made)
	{
		/* register the class to the system dictionary */
		if (!stix_putatsysdic(stix, (stix_oop_t)stix->c->cls.self_oop->name, (stix_oop_t)stix->c->cls.self_oop)) return -1;
	}

	return 0;
}

static int __compile_class_definition (stix_t* stix)
{
	/* 
	 * class-definition := #class class-modifier? "{" class-body "}"
	 * class-modifier := "(" (#byte | #character | #word | #pointer)? ")"
	 * class-body := variable-definition* method-definition*
	 * 
	 * variable-definition := (#dcl | #declare) variable-modifier? variable-list "."
	 * variable-modifier := "(" (#class | #classinst)? ")"
	 * variable-list := identifier*
	 *
	 * method-definition := (#mth | #method) method-modifier? method-actual-definition
	 * method-modifier := "(" (#class | #instance)? ")"
	 * method-actual-definition := method-name "{" method-tempraries? method-primitive? method-statements* "}"
	 */
	stix_oop_association_t ass;

	if (stix->c->tok.type == STIX_IOTOK_LPAREN)
	{
		/* process class modifiers */

		GET_TOKEN (stix);

		if (is_token_symbol(stix, VOCA_BYTE))
		{
			/* #class(#byte) */
			stix->c->cls.flags |= CLASS_INDEXED;
			stix->c->cls.indexed_type = STIX_OBJ_TYPE_BYTE;
			GET_TOKEN (stix);
		}
		else if (is_token_symbol(stix, VOCA_CHARACTER))
		{
			/* #class(#character) */
			stix->c->cls.flags |= CLASS_INDEXED;
			stix->c->cls.indexed_type = STIX_OBJ_TYPE_CHAR;
			GET_TOKEN (stix);
		}
		else if (is_token_symbol(stix, VOCA_WORD))
		{
			/* #class(#word) */
			stix->c->cls.flags |= CLASS_INDEXED;
			stix->c->cls.indexed_type = STIX_OBJ_TYPE_WORD;
			GET_TOKEN (stix);
		}
		else if (is_token_symbol(stix, VOCA_POINTER))
		{
			/* #class(#pointer) */
			stix->c->cls.flags |= CLASS_INDEXED;
			stix->c->cls.indexed_type = STIX_OBJ_TYPE_OOP;
			GET_TOKEN (stix);
		}

		if (stix->c->tok.type != STIX_IOTOK_RPAREN)
		{
			set_syntax_error (stix, STIX_SYNERR_RPAREN, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		GET_TOKEN (stix);
	}

	if (stix->c->tok.type != STIX_IOTOK_IDENT)
	{
		/* class name expected. */
		set_syntax_error (stix, STIX_SYNERR_IDENT, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	/* copy the class name */
	if (set_class_name(stix, &stix->c->tok.name) <= -1) return -1;
	stix->c->cls.name_loc = stix->c->tok.loc;
	GET_TOKEN (stix); 

	if (stix->c->tok.type == STIX_IOTOK_LPAREN)
	{
		int super_is_nil = 0;

printf ("DEFININING..");
print_ucs (&stix->c->cls.name);
printf ("\n");

		/* superclass is specified. new class defintion.
		 * for example, #class Class(Stix) 
		 */
		GET_TOKEN (stix); /* read superclass name */

		/* TODO: multiple inheritance */

		if (stix->c->tok.type == STIX_IOTOK_NIL)
		{
			super_is_nil = 1;
		}
		else if (stix->c->tok.type != STIX_IOTOK_IDENT)
		{
			/* superclass name expected */
			set_syntax_error (stix, STIX_SYNERR_IDENT, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		if (set_superclass_name(stix, &stix->c->tok.name) <= -1) return -1;
		stix->c->cls.supername_loc = stix->c->tok.loc;

		GET_TOKEN (stix);
		if (stix->c->tok.type != STIX_IOTOK_RPAREN)
		{
			set_syntax_error (stix, STIX_SYNERR_RPAREN, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		GET_TOKEN (stix);

		ass = stix_lookupsysdic(stix, &stix->c->cls.name);
		if (ass)
		{
			if (STIX_CLASSOF(stix, ass->value) != stix->_class  ||
			    STIX_OBJ_GET_FLAGS_KERNEL(ass->value) > 1)
			{
				/* the object found with the name is not a class object 
				 * or the the class object found is a fully defined kernel 
				 * class object */
				set_syntax_error (stix, STIX_SYNERR_CLASSDUP, &stix->c->cls.name_loc, &stix->c->cls.name);
				return -1;
			}
			
			stix->c->cls.self_oop = (stix_oop_class_t)ass->value;
		}
		else
		{
			/* no class of such a name is found. it's a new definition,
			 * which is normal for most new classes. */
			STIX_ASSERT (stix->c->cls.self_oop == STIX_NULL);
		}

		if (super_is_nil)
		{
			stix->c->cls.super_oop = stix->_nil;
		}
		else
		{
			ass = stix_lookupsysdic(stix, &stix->c->cls.supername);
			if (ass &&
			    STIX_CLASSOF(stix, ass->value) == stix->_class &&
			    STIX_OBJ_GET_FLAGS_KERNEL(ass->value) != 1) 
			{
				/* the value found must be a class and it must not be 
				 * an incomplete internal class object */
				stix->c->cls.super_oop = ass->value;
			}
			else
			{
				/* there is no object with such a name. or,
				 * the object found with the name is not a class object. or,
				 * the class object found is a internally defined kernel
				 * class object. */
				set_syntax_error (stix, STIX_SYNERR_CLASSUNDEF, &stix->c->cls.supername_loc, &stix->c->cls.supername);
				return -1;
			}
		}
	}
	else
	{
		/* extending class */
		if (stix->c->cls.flags != 0)
		{
			/* the class definition specified with modifiers cannot extend 
			 * an existing class. the superclass must be specified enclosed
			 * in parentheses. an opening parenthesis is expected to specify
			 * a superclass here. */
			set_syntax_error (stix, STIX_SYNERR_LPAREN, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		stix->c->cls.flags |= CLASS_EXTENDED;

		ass = stix_lookupsysdic(stix, &stix->c->cls.name);
		if (ass && 
		    STIX_CLASSOF(stix, ass->value) == stix->_class &&
		    STIX_OBJ_GET_FLAGS_KERNEL(ass->value) != 1)
		{
			/* the value must be a class object.
			 * and it must be either a user-defined(0) or 
			 * completed kernel built-in(2). 
			 * an incomplete kernel built-in class object(1) can not be
			 * extended */
			stix->c->cls.self_oop = (stix_oop_class_t)ass->value;
		}
		else
		{
			/* only an existing class can be extended. */
			set_syntax_error (stix, STIX_SYNERR_CLASSUNDEF, &stix->c->cls.name_loc, &stix->c->cls.name);
			return -1;
		}

		stix->c->cls.super_oop = stix->c->cls.self_oop->superclass;

		STIX_ASSERT ((stix_oop_t)stix->c->cls.super_oop == stix->_nil || 
		             STIX_CLASSOF(stix, stix->c->cls.super_oop) == stix->_class);
	}

	if (stix->c->tok.type != STIX_IOTOK_LBRACE)
	{
		set_syntax_error (stix, STIX_SYNERR_LBRACE, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	if (stix->c->cls.super_oop != stix->_nil)
	{
		/* adjust the instance variable count and the class instance variable
		 * count to include that of a superclass */
		stix_oop_class_t c;
		stix_oow_t spec, self_spec;

		c = (stix_oop_class_t)stix->c->cls.super_oop;
		spec = STIX_OOP_TO_SMINT(c->spec);
		self_spec = STIX_OOP_TO_SMINT(c->selfspec);
		stix->c->cls.var_count[VAR_INSTANCE] = STIX_CLASS_SPEC_NAMED_INSTVAR(spec);
		stix->c->cls.var_count[VAR_CLASSINST] = STIX_CLASS_SELFSPEC_CLASSINSTVAR(self_spec);
	}

	GET_TOKEN (stix);

	if (stix->c->cls.flags & CLASS_EXTENDED)
	{
		/* when a class is extended, a new variable cannot be added */
		if (is_token_symbol(stix, VOCA_DCL) || is_token_symbol(stix, VOCA_DECLARE))
		{
			set_syntax_error (stix, STIX_SYNERR_DCLBANNED, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		/* use the method dictionary of an existing class object */
		stix->c->cls.mthdic_oop[MTH_INSTANCE] = stix->c->cls.self_oop->mthdic[STIX_CLASS_MTHDIC_INSTANCE];
		stix->c->cls.mthdic_oop[MTH_CLASS] = stix->c->cls.self_oop->mthdic[STIX_CLASS_MTHDIC_CLASS];
	}
	else
	{
		/* a new class including an internally defined class object */

		while (is_token_symbol(stix, VOCA_DCL) || is_token_symbol(stix, VOCA_DECLARE))
		{
			/* variable definition. #dcl or #declare */
			GET_TOKEN (stix);
			if (compile_class_level_variables(stix) <= -1) return -1;
		}

		if (make_defined_class(stix) <= -1) return -1;
	}

	while (is_token_symbol(stix, VOCA_MTH) || is_token_symbol(stix, VOCA_METHOD))
	{
		/* method definition. #mth or #method */
		GET_TOKEN (stix);
		if (compile_method_definition(stix) <= -1) return -1;
	}
	
	if (stix->c->tok.type != STIX_IOTOK_RBRACE)
	{
		set_syntax_error (stix, STIX_SYNERR_RBRACE, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	if (!(stix->c->cls.flags & CLASS_EXTENDED))
	{
/* TODO: anything else to set? */
		stix->c->cls.self_oop->mthdic[STIX_CLASS_MTHDIC_INSTANCE] = stix->c->cls.mthdic_oop[MTH_INSTANCE];
		stix->c->cls.self_oop->mthdic[STIX_CLASS_MTHDIC_CLASS] = stix->c->cls.mthdic_oop[MTH_CLASS];
	}

	GET_TOKEN (stix);
	return 0;
}

static int compile_class_definition (stix_t* stix)
{
	int n;
	stix_size_t i;

	/* reset the structure to hold information about a class to be compiled */
	stix->c->cls.flags = 0;
	stix->c->cls.indexed_type = STIX_OBJ_TYPE_OOP;

	stix->c->cls.name.len = 0;
	stix->c->cls.supername.len = 0;
	STIX_MEMSET (&stix->c->cls.name_loc, 0, STIX_SIZEOF(stix->c->cls.name_loc));
	STIX_MEMSET (&stix->c->cls.supername_loc, 0, STIX_SIZEOF(stix->c->cls.supername_loc));

	STIX_ASSERT (STIX_COUNTOF(stix->c->cls.var_count) == STIX_COUNTOF(stix->c->cls.vars));
	for (i = 0; i < STIX_COUNTOF(stix->c->cls.var_count); i++) 
	{
		stix->c->cls.var_count[i] = 0;
		stix->c->cls.vars[i].len = 0;
	}

	stix->c->cls.self_oop = STIX_NULL;
	stix->c->cls.super_oop = STIX_NULL;
	stix->c->cls.mthdic_oop[MTH_INSTANCE] = STIX_NULL;
	stix->c->cls.mthdic_oop[MTH_CLASS] = STIX_NULL;
	stix->c->mth.literal_count = 0;
	stix->c->mth.balit_count = 0;
	stix->c->mth.arlit_count = 0;

	/* do main compilation work */
	n = __compile_class_definition (stix);

	/* reset these oops plus literal pointers not to confuse gc_compiler() */
	stix->c->cls.self_oop = STIX_NULL;
	stix->c->cls.super_oop = STIX_NULL;
	stix->c->cls.mthdic_oop[MTH_INSTANCE] = STIX_NULL;
	stix->c->cls.mthdic_oop[MTH_CLASS] = STIX_NULL;
	stix->c->mth.literal_count = 0;
	stix->c->mth.balit_count = 0;
	stix->c->mth.arlit_count = 0;

	return n;
}

static int compile_stream (stix_t* stix)
{
	GET_CHAR (stix);
	GET_TOKEN (stix);

	while (stix->c->tok.type != STIX_IOTOK_EOF)
	{
		if (is_token_symbol(stix, VOCA_INCLUDE))
		{
			/* #include 'xxxx' */
			GET_TOKEN (stix);
			if (stix->c->tok.type != STIX_IOTOK_STRLIT)
			{
				set_syntax_error (stix, STIX_SYNERR_STRING, &stix->c->tok.loc, &stix->c->tok.name);
				return -1;
			}
			if (begin_include(stix) <= -1) return -1;
		}
		else if (is_token_symbol(stix, VOCA_CLASS))
		{
			/* #class Selfclass(Superclass) { } */
			GET_TOKEN (stix);
			if (compile_class_definition(stix) <= -1) return -1;
		}
#if 0
		else if (is_token_symbol(stix, VOCA_MAIN))
		{
			/* #main */
			/* TODO: implement this */
			
		}
#endif

		else
		{
			set_syntax_error(stix, STIX_SYNERR_DIRECTIVE, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}
	}

	return 0;
}

static void gc_compiler (stix_t* stix)
{
	/* called when garbage collection is performed */
	if (stix->c)
	{
		stix_size_t i;

		if (stix->c->cls.self_oop) 
			stix->c->cls.self_oop = (stix_oop_class_t)stix_moveoop (stix, (stix_oop_t)stix->c->cls.self_oop);

		if (stix->c->cls.super_oop)
			stix->c->cls.super_oop = stix_moveoop (stix, stix->c->cls.super_oop);

		if (stix->c->cls.mthdic_oop[MTH_INSTANCE])
			stix->c->cls.mthdic_oop[MTH_INSTANCE] = (stix_oop_set_t)stix_moveoop (stix, (stix_oop_t)stix->c->cls.mthdic_oop[MTH_INSTANCE]);

		if (stix->c->cls.mthdic_oop[MTH_CLASS])
			stix->c->cls.mthdic_oop[MTH_CLASS] = (stix_oop_set_t)stix_moveoop (stix, (stix_oop_t)stix->c->cls.mthdic_oop[MTH_CLASS]);

		for (i = 0; i < stix->c->mth.literal_count; i++)
		{
			if (STIX_OOP_IS_POINTER(stix->c->mth.literals[i]))
			{
				stix->c->mth.literals[i] = stix_moveoop (stix, stix->c->mth.literals[i]);
			}
		}

		for (i = 0; i < stix->c->mth.arlit_count; i++)
		{
			if (STIX_OOP_IS_POINTER(stix->c->mth.arlit[i]))
				stix->c->mth.arlit[i] = stix_moveoop (stix, stix->c->mth.arlit[i]);
		}
	}
}

static void fini_compiler (stix_t* stix)
{
	/* called before the stix object is closed */
	if (stix->c)
	{
		stix_size_t i;

		clear_io_names (stix);

		if (stix->c->tok.name.ptr) stix_freemem (stix, stix->c->tok.name.ptr);
		if (stix->c->cls.name.ptr) stix_freemem (stix, stix->c->cls.name.ptr);
		if (stix->c->cls.supername.ptr) stix_freemem (stix, stix->c->cls.supername.ptr);

		for (i = 0; i < STIX_COUNTOF(stix->c->cls.vars); i++)
		{
			if (stix->c->cls.vars[i].ptr) stix_freemem (stix, stix->c->cls.vars[i].ptr);
		}

		if (stix->c->mth.text.ptr) stix_freemem (stix, stix->c->mth.text.ptr);
		if (stix->c->mth.assignees.ptr) stix_freemem (stix, stix->c->mth.assignees.ptr);
		if (stix->c->mth.binsels.ptr) stix_freemem (stix, stix->c->mth.binsels.ptr);
		if (stix->c->mth.kwsels.ptr) stix_freemem (stix, stix->c->mth.kwsels.ptr);
		if (stix->c->mth.name.ptr) stix_freemem (stix, stix->c->mth.name.ptr);
		if (stix->c->mth.tmprs.ptr) stix_freemem (stix, stix->c->mth.tmprs.ptr);
		if (stix->c->mth.code.ptr) stix_freemem (stix, stix->c->mth.code.ptr);
		if (stix->c->mth.literals) stix_freemem (stix, stix->c->mth.literals);
		if (stix->c->mth.balit) stix_freemem (stix, stix->c->mth.balit);
		if (stix->c->mth.arlit) stix_freemem (stix, stix->c->mth.arlit);

		stix_freemem (stix, stix->c);
		stix->c = STIX_NULL;
	}
}

int stix_compile (stix_t* stix, stix_ioimpl_t io)
{
	int n;

	if (!io)
	{
		stix->errnum = STIX_EINVAL;
		return -1;
	}

	if (!stix->c)
	{
		stix_cb_t cb, * cbp;

		STIX_MEMSET (&cb, 0, STIX_SIZEOF(cb));
		cb.gc = gc_compiler;
		cb.fini = fini_compiler;
		cbp = stix_regcb (stix, &cb);
		if (!cbp) return -1;

		stix->c = stix_callocmem (stix, STIX_SIZEOF(*stix->c));
		if (!stix->c) 
		{
			stix_deregcb (stix, cbp);
			return -1;
		}

		stix->c->ilchr_ucs.ptr = &stix->c->ilchr;
		stix->c->ilchr_ucs.len = 1;
	}

	/* Some IO names could have been stored in earlier calls to this function.
	 * I clear such names before i begin this function. i don't clear it
	 * at the end of this function because i may be referenced as an error
	 * location */
	clear_io_names (stix);

	/* initialize some key fields */
	stix->c->impl = io;
	stix->c->nungots = 0;

	/* The name field and the includer field are STIX_NULL 
	 * for the main stream */
	STIX_MEMSET (&stix->c->arg, 0, STIX_SIZEOF(stix->c->arg));
	stix->c->arg.line = 1;
	stix->c->arg.colm = 1;

	/* open the top-level stream */
	n = stix->c->impl (stix, STIX_IO_OPEN, &stix->c->arg);
	if (n <= -1) return -1;

	/* the stream is open. set it as the current input stream */
	stix->c->curinp = &stix->c->arg;

	/* compile the contents of the stream */
	if (compile_stream (stix) <= -1) goto oops;

	/* close the stream */
	STIX_ASSERT (stix->c->curinp == &stix->c->arg);
	stix->c->impl (stix, STIX_IO_CLOSE, stix->c->curinp);

	return 0;

oops:
	/* an error occurred and control has reached here
	 * probably, some included files might not have been 
	 * closed. close them */
	while (stix->c->curinp != &stix->c->arg)
	{
		stix_ioarg_t* prev;

		/* nothing much to do about a close error */
		stix->c->impl (stix, STIX_IO_CLOSE, stix->c->curinp);

		prev = stix->c->curinp->includer;
		STIX_ASSERT (stix->c->curinp->name != STIX_NULL);
		STIX_MMGR_FREE (stix->mmgr, stix->c->curinp);
		stix->c->curinp = prev;
	}

	stix->c->impl (stix, STIX_IO_CLOSE, stix->c->curinp);
	return -1;
}

void stix_getsynerr (stix_t* stix, stix_synerr_t* synerr)
{
	STIX_ASSERT (stix->c != STIX_NULL);
	if (synerr) *synerr = stix->c->synerr;
}
