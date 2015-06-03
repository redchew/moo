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

#define TOKEN_NAME_ALIGN 256
#define CLASS_BUFFER_ALIGN 8 /* 256 */
#define LITERAL_BUFFER_ALIGN 8 /* 256 */
#define CODE_BUFFER_ALIGN 8 /* 256 */

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

	if (stix->c->nungots > 0)
	{
		/* something in the unget buffer */
		stix->c->lxc = stix->c->ungot[--stix->c->nungots];
		return 0;
	}

	if (stix->c->curinp->b.pos >= stix->c->curinp->b.len)
	{
		n = stix->c->impl (stix, STIX_IO_READ, stix->c->curinp);
		if (n <= -1) return -1;

		if (n == 0)
		{
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

	stix->c->curinp->lxc.c = stix->c->curinp->buf[stix->c->curinp->b.pos++];
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

static int get_ident (stix_t* stix)
{
	/*
	 * identifier := alpha-char (alpha-char | digit-char)*
	 * keyword := identifier ":"
	 */

	stix_uci_t c = stix->c->lxc.c;
	stix->c->tok.type = STIX_IOTOK_IDENT;

	do 
	{
		ADD_TOKEN_CHAR (stix, c);
		GET_CHAR (stix);
		c = stix->c->lxc.c;
	} 
	while (is_alnumchar(c));

	if (c == ':') 
	{
		ADD_TOKEN_CHAR (stix, c);
		stix->c->tok.type = STIX_IOTOK_KEYWORD;
		GET_CHAR (stix);
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

	stix_uci_t c = stix->c->lxc.c;
	stix->c->tok.type = STIX_IOTOK_NUMLIT;

/*TODO: support a complex numeric literal */
	do 
	{
		ADD_TOKEN_CHAR(stix, c);
		GET_CHAR (stix);
		c = stix->c->lxc.c;
	} 
	while (is_digitchar(c));

	/* TODO; more */
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

	stix->c->tok.type = STIX_IOTOK_CHRLIT;
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

	/* TODO: C-like string */

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
			ADD_TOKEN_CHAR(stix, c);
			GET_CHAR (stix);

			c = stix->c->lxc.c;
			if (c == '=') 
			{
				stix->c->tok.type = STIX_IOTOK_ASSIGN;
				ADD_TOKEN_CHAR(stix, c);
				GET_CHAR (stix);
			}
			break;

		case '^':
			stix->c->tok.type = STIX_IOTOK_RETURN;
			goto single_char_token;
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
								colon_required =1;
								goto nextword;
							}
						}
						else if (colon_required)
						{
							set_syntax_error (stix, STIX_SYNERR_CLNMS, &stix->c->lxc.l, STIX_NULL);
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

		default:
			if (is_alphachar(c)) 
			{
				if (get_ident (stix) <= -1) return -1;
			}
			else if (is_digitchar(c)) 
			{
				if (get_numlit (stix, 0) <= -1) return -1;
			}
			else if (is_binselchar(c)) 
			{
				/* binary selector */
				if (get_binsel (stix) <= -1) return -1;
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
		 * end_include() where stix->c->curinp/arg is freed. */
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

#define MAKE_CODE(x,y) (((x) << 4) | y)
#define MAX_CODE_INDEX               0xFFFFu
#define MAX_CODE_NARGS               0xFFFFu

#define CMD_EXTEND                       0x0
#define CMD_EXTEND_DOUBLE                0x1
#define CMD_PUSH_INSTVAR                 0x2
#define CMD_PUSH_TEMPVAR                 0x3
#define CMD_PUSH_LITERAL                 0x4
#define CMD_POP_AND_STORE_INTO_INSTVAR   0x5 /* pop and store */
#define CMD_POP_AND_STORE_INTO_CLASSVAR  0x6 /* pop and store */
#define CMD_POP_AND_STORE_INTO_TEMPVAR   0x7 /* pop and store */

#define CMD_SEND_MESSAGE_TO_SELF         0xA
#define CMD_SEND_MESSAGE_TO_SUPER        0xB

/* ---------------------------------- */
#define CODE_PUSH_RECEIVER            0xE0
#define CODE_PUSH_NIL                 0xE1
#define CODE_PUSH_TRUE                0xE2
#define CODE_PUSH_FALSE               0xE3

/* special code */
#define CODE_DUP_STACKTOP             0xF1
#define CODE_POP_STACKTOP             0xF2
#define CODE_RETURN_MESSAGE_STACKTOP  0xF3
#define CODE_RETURN_BLOCK_STACKTOP    0xF4
#define CODE_RETURN_MESSAGE_RECEIVER  0xF5
#define CODE_EXEC_PRIMITIVE           0xFF

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

static int emit_positional_instruction (stix_t* stix, int cmd, stix_size_t index)
{
	STIX_ASSERT (cmd <= 0xF);
	STIX_ASSERT (index <= MAX_CODE_INDEX);

	if (index > 0xFF)
	{
		if (emit_byte_instruction(stix, MAKE_CODE(CMD_EXTEND_DOUBLE, cmd)) <= -1 ||
		    emit_byte_instruction(stix, index >> 8) <= -1 ||
		    emit_byte_instruction(stix, index & 0xFF) <= -1) return -1;
	}
	else if (index > 0xF)
	{
		if (emit_byte_instruction(stix, MAKE_CODE(CMD_EXTEND, cmd)) <= -1 ||
		    emit_byte_instruction(stix, index) <= -1) return -1;
	}
	else
	{
		if (emit_byte_instruction(stix, MAKE_CODE(cmd, index)) <= -1) return -1;
	}

	return 0;

}

static int emit_send_instruction (stix_t* stix, int cmd, stix_size_t nargs, stix_size_t selector)
{
	/* 
	 * 1010JJJJ KKKKKKKK                    Send literal selector K with J arguments to self
	 * 1011JJJJ KKKKKKKK                    Send literal selector K with J arguments to super
 	 * 00001010 JJJJJJJJ KKKKKKKK
	 * 00001011 JJJJJJJJ KKKKKKKK
	 * 00011010 JJJJJJJJ JJJJJJJJ KKKKKKKK KKKKKKKK
	 * 00011011 JJJJJJJJ JJJJJJJJ KKKKKKKK KKKKKKKK
	 */

	STIX_ASSERT (cmd <= 0xF);
	STIX_ASSERT (nargs <= MAX_CODE_NARGS);
	STIX_ASSERT (selector <= MAX_CODE_INDEX);

	if (nargs > 0xFF || selector > 0xFF)
	{
		if (emit_byte_instruction(stix, MAKE_CODE(CMD_EXTEND_DOUBLE, cmd)) <= -1 ||
		    emit_byte_instruction(stix, nargs >> 8) <= -1 ||
		    emit_byte_instruction(stix, nargs & 0xFF) <= -1 ||
		    emit_byte_instruction(stix, selector >> 8) <= -1 ||
		    emit_byte_instruction(stix, selector & 0xFF) <= -1) return -1;
	}
	else if (nargs > 0xF)
	{
		if (emit_byte_instruction(stix, MAKE_CODE(CMD_EXTEND, cmd)) <= -1 ||
		    emit_byte_instruction(stix, nargs) <= -1 ||
		    emit_byte_instruction(stix, selector) <= -1) return -1;
	}
	else
	{
		if (emit_byte_instruction(stix, MAKE_CODE(cmd, nargs)) <= -1 ||
		    emit_byte_instruction(stix, selector) <= -1) return -1;
	}

	return 0;
}


/* ---------------------------------------------------------------------
 * Compiler
 * --------------------------------------------------------------------- */

static int compile_method_statement (stix_t* stix);
static int compile_method_expression (stix_t* stix);

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

/* TODO: add_array_literal, add_byte_array_literal () */

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
				var->cls = (stix_oop_class_t)super;
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
printf ("duplicate variable name type %d pos %lu\n", var.type, var.pos);
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
/* TODO: check if tmpr_nargs exceededs LIMIT (SMINT MAX). also bytecode max */

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

/* TODO: check if tmpr_count exceededs LIMIT (SMINT MAX). also bytecode max */
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
		if (prim_no > 0xFF)
		{
			set_syntax_error (stix, STIX_SYNERR_PRIMITIVENO, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		ptr++;
	}

	if (emit_byte_instruction(stix, CODE_EXEC_PRIMITIVE) <= -1 ||
	    emit_byte_instruction(stix, prim_no) <= -1) return -1;

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

	if (index > MAX_CODE_INDEX)
	{
		/* the assignee is not usable because its index is too large 
		 * to be expressed in byte-codes. */
		set_syntax_error (stix, STIX_SYNERR_VARUNUSE, name_loc, name);
		return -1;
	}

	return 0;
}

static int compile_block_statements (stix_t* stix)
{
	while (stix->c->tok.type != STIX_IOTOK_RBRACK && 
	       stix->c->tok.type != STIX_IOTOK_EOF) 
	{
		if (compile_method_statement(stix) <= -1) return -1;
		if (stix->c->tok.type != STIX_IOTOK_PERIOD) break;
		GET_TOKEN (stix);
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
/*
		if (find_temporary_variable(stix, &stix->c->tok.name) >= 0)
		{
			set_syntax_error (stix, STIX_SYNERR_TMPRNAMEDUP, &stix->c->tok.loc, &stix->c->tok.name);
			return -1;
		}

		if (add_temporary_variable(stix, &stix->c->tok.name) <= -1) return -1;
		stix->c->mth.tmpr_count++;
* */

/* TODO: check if tmpr_count exceededs LIMIT (SMINT MAX). also bytecode max */
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
	/*
	 * block-expression := "[" block-body "]"
	 * block-body := (block-argument* "|")? block-temporaries? method-statement*
	 * block-argument := ":" identifier
	 */

/* TODO: GENERATE JUMP INSTRUCTION */
	if (stix->c->tok.type == STIX_IOTOK_COLON) 
	{
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

			/* TODO : store block arguments */
/* TODO: check conflicting names as well */
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

/* TODO: create a block closure */
	if (compile_block_temporaries(stix) <= -1 ||
	    compile_block_statements(stix) <= -1) return -1;

	if (stix->c->tok.type != STIX_IOTOK_RBRACK) 
	{
		set_syntax_error (stix, STIX_SYNERR_RBRACK, &stix->c->tok.loc, &stix->c->tok.name);
		return -1;
	}

	GET_TOKEN (stix);

	/* TODO: do special treatment for block closures */
/* TODO: GENERATE BLOCK CONTEXT CREATION INSTRUCTION */

	return 0;
}


static int compile_expression_primary (stix_t* stix, const stix_ucs_t* ident, const stix_ioloc_t* ident_loc, int* to_super)
{
	/*
	 * expression-primary := identifier | literal | block-constructor | ( "(" method-expression ")" )
	 */

	var_info_t var;
	int read_next_token = 0;

	*to_super = 0;

	if (ident) 
	{
		/* the caller has read the identifier and the next word */

	handle_ident:
		if (get_variable_info (stix, ident, ident_loc, &var) <= -1) return -1;

		switch (var.type)
		{
			case VAR_ARGUMENT:
			case VAR_TEMPORARY:
printf ("push tempvar %d\n", (int)var.pos);
				if (emit_positional_instruction (stix, CMD_PUSH_TEMPVAR, var.pos) <= -1) return -1;
				break;

			case VAR_INSTANCE:
			case VAR_CLASSINST:
printf ("push instvar %d\n", (int)var.pos);
				if (emit_positional_instruction (stix, CMD_PUSH_INSTVAR, var.pos) <= -1) return -1;
				break;

			case VAR_CLASS:
				/* TODO: what instruction to generate for class variable access... */
				return -1;

			case VAR_GLOBAL:
				/* TODO: .............................. */
				return -1;

			default:
				stix->errnum = STIX_EINTERN;
				return -1;
		}

		if (read_next_token) GET_TOKEN (stix);
	}
	else
	{
		stix_size_t index;

		switch (stix->c->tok.type)
		{
			case STIX_IOTOK_IDENT:
				ident = &stix->c->tok.name;
				ident_loc = &stix->c->tok.loc;
				read_next_token = 1;
				goto handle_ident;

			case STIX_IOTOK_SELF:
printf ("push receiver...\n");
				if (emit_byte_instruction(stix, CODE_PUSH_RECEIVER) <= -1) return -1;
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_SUPER:
printf ("push receiver(super)...\n");
				if (emit_byte_instruction(stix, CODE_PUSH_RECEIVER) <= -1) return -1;
				GET_TOKEN (stix);
				*to_super = 1;
				break;

			case STIX_IOTOK_NIL:
printf ("push nil...\n");
				if (emit_byte_instruction(stix, CODE_PUSH_NIL) <= -1) return -1;
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_TRUE:
printf ("push true...\n");
				if (emit_byte_instruction(stix, CODE_PUSH_TRUE) <= -1) return -1;
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_FALSE:
printf ("push false...\n");
				if (emit_byte_instruction(stix, CODE_PUSH_FALSE) <= -1) return -1;
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_THIS_CONTEXT:
				/* TODO */
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_CHRLIT:
				STIX_ASSERT (stix->c->tok.name.len == 1);
				if (add_character_literal(stix, stix->c->tok.name.ptr[0], &index) <= -1 ||
				    emit_positional_instruction(stix, CMD_PUSH_LITERAL, index) <= -1) return -1;
printf ("push character literal %d\n", (int)index);
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_STRLIT:
				if (add_string_literal(stix, &stix->c->tok.name, &index) <= -1 ||
				    emit_positional_instruction(stix, CMD_PUSH_LITERAL, index) <= -1) return -1;
printf ("push string literal %d\n", (int)index);
				GET_TOKEN (stix);

				break;

			case STIX_IOTOK_SYMLIT:
				if (add_symbol_literal(stix, &stix->c->tok.name, &index) <= -1 ||
				    emit_positional_instruction(stix, CMD_PUSH_LITERAL, index) <= -1) return -1;
printf ("push symbol literal %d\n", (int)index);
				GET_TOKEN (stix);
				break;

			case STIX_IOTOK_NUMLIT:
			{
				/* TODO: other types of numbers, negative numbers, etc */
/*
				stix_word_t tmp;
				STIX_STRTOI (tmp, stix->tok.name.buffer, STIX_NULL, 10);
				literal = STIX_TO_SMALLINT(tmp);

				if (add_literal(stix, literal, &index) <= -1)r eturn -1;

				EMIT_PUSH_LITERAL_CONSTANT (stix, index);
				GET_TOKEN (stix);
*/
				break;
			}

			case STIX_IOTOK_APAREN:
				/* TODO: array literal */
				break;

			case STIX_IOTOK_BPAREN:
				/* TODO: byte array literal */
				break;

			case STIX_IOTOK_LBRACK:
				GET_TOKEN (stix);
				if (compile_block_expression(stix) <= -1) return -1;
				break;

			case STIX_IOTOK_LPAREN:
				GET_TOKEN (stix);
				if (compile_method_expression(stix) <= -1) return -1;
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
	CMD_SEND_MESSAGE_TO_SELF,
	CMD_SEND_MESSAGE_TO_SUPER
};

static int compile_unary_message (stix_t* stix, int to_super)
{
	stix_size_t index;

	while (stix->c->tok.type == STIX_IOTOK_IDENT) 
	{
		if (add_symbol_literal(stix, &stix->c->tok.name, &index) <= -1 ||
		    emit_send_instruction(stix, send_message_cmd[to_super], 0, index) <= -1) return -1;
printf ("send message %d with 0 arguments to %s\n", (int)index, (to_super? "super": "self"));
		GET_TOKEN (stix);
	}

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

	if (compile_unary_message(stix, to_super) <= -1) return -1;

	while (stix->c->tok.type == STIX_IOTOK_BINSEL) 
	{
		binsel = stix->c->tok.name;
		if (clone_binary_selector(stix, &binsel) <= -1) return -1;

		GET_TOKEN (stix);
		if (compile_expression_primary(stix, STIX_NULL, STIX_NULL, &to_super2) <= -1  ||
		    compile_unary_message(stix, to_super2) <= -1 ||
		    add_symbol_literal(stix, &binsel, &index) <= -1 ||
		    emit_send_instruction(stix, send_message_cmd[to_super], 2, index) <= -1) 
		{
			stix->c->mth.binsels.len -= binsel.len;
			return -1;
		}
printf ("send message %d with 2 arguments to %s\n", (int)index, (to_super? "super": "self"));
		stix->c->mth.binsels.len -= binsel.len;
	}

	return 0;
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
	stix_ioloc_t kwsel_loc;
	stix_size_t kwsel_len;
	stix_size_t nargs = 0;

	if (compile_binary_message(stix, to_super) <= -1) return -1;
	if (stix->c->tok.type != STIX_IOTOK_KEYWORD) return 0;

	kwsel_loc = stix->c->tok.loc;
	kwsel_len = stix->c->mth.kwsels.len;

	kw = stix->c->tok.name;
	if (clone_keyword(stix, &kw) <= -1) return -1;

	do 
	{
		GET_TOKEN (stix);
		if (compile_expression_primary(stix, STIX_NULL, STIX_NULL, &to_super2) <= -1 ||
		    compile_binary_message(stix, to_super2) <= -1) goto oops;

		if (nargs >= MAX_CODE_NARGS)
		{
			set_syntax_error (stix, STIX_SYNERR_ARGFLOOD, &kwsel_loc, &kwsel); 
			goto oops;
		}

		nargs++;

		kw = stix->c->tok.name;
		if (clone_keyword(stix, &kw) <= -1) goto oops;
	} 
	while (stix->c->tok.type == STIX_IOTOK_KEYWORD);

	kwsel.ptr = &stix->c->mth.kwsels.ptr[kwsel_len];
	kwsel.len = stix->c->mth.kwsels.len - kwsel_len;

	if (add_symbol_literal(stix, &kwsel, &index) <= -1 ||
	    emit_send_instruction(stix, send_message_cmd[to_super], nargs, index) <= -1) goto oops;
printf ("send message %d with %d arguments to %s\n", (int)index, (int)nargs, (to_super? "super": "self"));
	stix->c->mth.kwsels.len = kwsel_len;
	return 0;

oops:
	stix->c->mth.kwsels.len = kwsel_len;
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
	 * cascaded-message := (";" single-message)*
	 */

	if (compile_keyword_message(stix, to_super) <= -1) return -1;

	while (stix->c->tok.type == STIX_IOTOK_SEMICOLON) 
	{
		/* handle message cascading */
printf ("DoSpecial(DUP_RECEIVER(CASCADE)) ....\n");
/*T ODO: emit code */
		GET_TOKEN (stix);

		if (compile_keyword_message(stix, 0) <= -1) return -1;
printf ("DoSpecial(POP_TOP) ....\n");
/*T ODO: emit code */
	}

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
	    stix->c->tok.type != STIX_IOTOK_PERIOD) 
	{
		if (compile_message_expression(stix, to_super) <= -1) return -1;
	}

	return 0;
}

static int compile_method_expression (stix_t* stix)
{
	/*
	 * method-expression := method-assignment-expression | basic-expression
	 * method-assignment-expression := identifier ":=" method-expression

	 */

	stix_ucs_t assignee;

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

			if (compile_method_expression(stix) <= -1 ||
			    get_variable_info(stix, &assignee, &assignee_loc, &var) <= -1) return -1;

			switch (var.type)
			{
				case VAR_ARGUMENT:
					/* assigning to an argument is not allowed */
					set_syntax_error (stix, STIX_SYNERR_VARARG, &assignee_loc, &assignee);
					return -1;

				case VAR_TEMPORARY:
		printf ("emit pop and store to tempvar %d\n", (int)var.pos);
					if (emit_positional_instruction (stix, CMD_POP_AND_STORE_INTO_TEMPVAR, var.pos) <= -1) return -1;
					break;

				case VAR_INSTANCE:
				case VAR_CLASSINST:
		printf ("emit pop and store to instvar %d\n", (int)var.pos);
					if (emit_positional_instruction (stix, CMD_POP_AND_STORE_INTO_INSTVAR, var.pos) <= -1) return -1;
					break;

				case VAR_CLASS:
					/* TODO: what instruction to generate for class variable access... */

					return -1;

				case VAR_GLOBAL:
					/* TODO: .............................. */
					return -1;

				default:
					stix->errnum = STIX_EINTERN;
					return -1;
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
	return 0;

oops:
	stix->c->mth.assignees.len -= assignee.len;
	return -1;
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
		if (compile_method_expression(stix) <= -1) return -1;
printf ("return message stacktop\n");
		return emit_byte_instruction (stix, CODE_RETURN_MESSAGE_STACKTOP);
	}
	else 
	{
		return compile_method_expression(stix);
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
				else 
				{
					set_syntax_error (stix, STIX_SYNERR_PERIOD, &stix->c->tok.loc, &stix->c->tok.name);
					return -1;
				}
			}
		}
		while (1);
	}

	/* TODO: size optimization. emit code_return_receiver only if it's not previously emitted */
	return emit_byte_instruction (stix, CODE_RETURN_MESSAGE_RECEIVER);
}

static int add_compiled_method (stix_t* stix)
{
	stix_oop_t name; /* selector */
	stix_oop_method_t mth; /* method */
	stix_oop_t code;
	stix_size_t tmp_count = 0;
	stix_size_t i;

	name = stix_makesymbol (stix, stix->c->mth.name.ptr, stix->c->mth.name.len);
	if (!name) return -1;
	stix_pushtmp (stix, &name); tmp_count++;

	/* The variadic data part passed to stix_instantiate() is not GC-safe */
	mth = (stix_oop_method_t)stix_instantiate (stix, stix->_method, STIX_NULL, stix->c->mth.literal_count);
	if (!mth) goto oops;
	for (i = 0; i < stix->c->mth.literal_count; i++)
	{
		/* let's do the variadic data initialization here */
		mth->literal[i] = stix->c->mth.literals[i];
	}
	stix_pushtmp (stix, (stix_oop_t*)&mth); tmp_count++;

	code = stix_instantiate (stix, stix->_byte_array, stix->c->mth.code.ptr, stix->c->mth.code.len);
	if (!code) goto oops;
	stix_pushtmp (stix, &code); tmp_count++;

	mth->owner = stix->c->cls.self_oop;
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
	stix->c->mth.code.len = 0;

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
	self_spec = STIX_CLASS_SELFSPEC_MAKE(stix->c->cls.var_count[VAR_CLASS], stix->c->cls.var_count[VAR_CLASSINST]);

#if 0
printf ("MAKING ... ");
print_ucs (&stix->c->cls.name);
printf (" instvars %d classvars %d classinstvars %d\n", (int)stix->c->cls.var_count[VAR_INSTANCE], (int)stix->c->cls.var_count[VAR_CLASS], (int)stix->c->cls.var_count[VAR_CLASSINST]);
#endif
	if (stix->c->cls.self_oop)
	{
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
	tmp = stix_makedic (stix, stix->_method_dictionary, INSTANCE_METHOD_DICTIONARY_SIZE);
	if (!tmp) return -1;
	stix->c->cls.mthdic_oop[MTH_INSTANCE] = (stix_oop_set_t)tmp;

/* TOOD: good dictionary size */
	tmp = stix_makedic (stix, stix->_method_dictionary, CLASS_METHOD_DICTIONARY_SIZE);
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

printf ("DEFININING..\n");
{
int i;
for (i = 0; i < stix->c->cls.name.len; i++)
{
printf ("%c", stix->c->cls.name.ptr[i]);
}
printf ("\n");
}

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

		ass = (stix_oop_association_t)stix_lookupsysdic(stix, &stix->c->cls.name);
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
			ass = (stix_oop_association_t)stix_lookupsysdic(stix, &stix->c->cls.supername);
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

		ass = (stix_oop_association_t)stix_lookupsysdic(stix, &stix->c->cls.name);
		if (ass && 
		    STIX_CLASSOF(stix, ass->value) != stix->_class &&
		    STIX_OBJ_GET_FLAGS_KERNEL(ass->value) != 1)
		{
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
		stix->c->cls.mthdic_oop[MTH_INSTANCE] = stix->c->cls.self_oop->instmths;
		stix->c->cls.mthdic_oop[MTH_CLASS] = stix->c->cls.self_oop->classmths;
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
		stix->c->cls.self_oop->instmths = stix->c->cls.mthdic_oop[MTH_INSTANCE];
		stix->c->cls.self_oop->classmths = stix->c->cls.mthdic_oop[MTH_CLASS];
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

	stix->c->cls.name.len = 0;
	stix->c->cls.supername.len = 0;
	STIX_MEMSET (&stix->c->cls.name_loc, 0, STIX_SIZEOF(stix->c->cls.name_loc));
	STIX_MEMSET (&stix->c->cls.supername_loc, 0, STIX_SIZEOF(stix->c->cls.supername_loc));

	for (i = 0; i < STIX_COUNTOF(stix->c->cls.var_count); i++) 
		stix->c->cls.var_count[i] = 0;

	stix->c->cls.self_oop = STIX_NULL;
	stix->c->cls.super_oop = STIX_NULL;
	stix->c->cls.mthdic_oop[MTH_INSTANCE] = STIX_NULL;
	stix->c->cls.mthdic_oop[MTH_CLASS] = STIX_NULL;
	stix->c->mth.literal_count = 0;

	/* do main compilation work */
	n = __compile_class_definition (stix);

	/* reset these oops plus literal pointers not to confuse gc_compiler() */
	stix->c->cls.self_oop = STIX_NULL;
	stix->c->cls.super_oop = STIX_NULL;
	stix->c->cls.mthdic_oop[MTH_INSTANCE] = STIX_NULL;
	stix->c->cls.mthdic_oop[MTH_CLASS] = STIX_NULL;
	stix->c->mth.literal_count = 0;

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

	stix->c->impl = io;
	stix->c->arg.line = 1;
	stix->c->arg.colm = 1;
	stix->c->curinp = &stix->c->arg;
	clear_io_names (stix);

	/* open the top-level stream */
	n = stix->c->impl (stix, STIX_IO_OPEN, stix->c->curinp);
	if (n <= -1) return -1;

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
