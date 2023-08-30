/*
Copyright (c) 2023 Paul Adelsbach

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/

#include "jtoke.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>

#ifdef JTOKE_DEBUG
#define DBG_PRINT	printf
#else
#define DBG_PRINT(...)
#endif

#define RETURN_IF_NULL(ch)	if (0 == ch) { return JTOKE_ERROR; }

#define JSON_NULL	"null"
#define JSON_TRUE	"true"
#define JSON_FALSE	"false"

static char in_charset(const char* charset, char c)
{
	char ch = *charset;
	while (ch) {
		if (ch == c) {
			return ch;
		}
		ch = *charset++;
	}
	return 0;
}

static char advance_until(const char* charset, const char** c)
{
	char ch = **c;
	while (ch) {
		if (in_charset(charset, ch)) {
			return ch;
		}
		(*c)++;
		ch = **c;
	}
	return 0;
}

static char advance_past(const char* charset, const char** c)
{
	char ch = **c;
	while (ch) {
		if (!in_charset(charset, ch)) {
			return ch;
		}
		(*c)++;
		ch = **c;
	}
	return 0;
}

static int is_integer(const char* str, unsigned int str_len) 
{
	for (unsigned int i=0; i<str_len; i++) {
		if (!in_charset("-0123456789", str[i])) {
			return 0;
		}
	}
	return 1;
}

static int is_real(const char* str, unsigned int str_len) 
{
	for (unsigned int i=0; i<str_len; i++) {
		if (!in_charset("-0123456789.eE+", str[i])) {
			return 0;
		}
	}
	return 1;
}

jtoke_type_t jtoke_parse(jtoke_context_t* ctx, const char* json, jtoke_item_t* item)
{
	jtoke_type_t type;
	char ch;

	if (0 == ctx->pos) {
		ctx->pos = json;
	}

	DBG_PRINT("checking for open quote\n");
	ch = advance_until("\"", &ctx->pos);
	RETURN_IF_NULL(ch);

	ctx->pos++;
	item->name = ctx->pos;

	DBG_PRINT("checking for close quote\n");
	ch = advance_until("\"", &ctx->pos);
	RETURN_IF_NULL(ch);

	item->name_len = ctx->pos - item->name;
	ctx->pos++;
	DBG_PRINT("name_len is %u, next char is '%c'\n", item->name_len, *ctx->pos);

	DBG_PRINT("advancing past colon\n");
	ch = advance_past(" \t\r\n:", &ctx->pos);
	RETURN_IF_NULL(ch);

	if (ch == '"') {
		type = JTOKE_STRING;
		ctx->pos++;
		item->val = ctx->pos;
		DBG_PRINT("advancing until value end (string)\n");
		ch = advance_until("\"", &ctx->pos);
		RETURN_IF_NULL(ch);
		item->val_len = ctx->pos - item->val;
	}
	else if (ch == '{') {
		// TODO: handle sub-object
		type = JTOKE_OBJ;
		return -1;
	}
	else if (ch == '[') {
		// TODO: handle arrays
		type = JTOKE_ARRAY;
		return -1;
	}
	else {
		item->val = ctx->pos;
		DBG_PRINT("advancing until value end\n");
		ch = advance_until(" \t\r\n,}", &ctx->pos);
		RETURN_IF_NULL(ch);
		item->val_len = ctx->pos - item->val;

		if (item->val_len == strlen(JSON_NULL) && !memcmp(item->val, JSON_NULL, item->val_len)) {
			type = JTOKE_NULL;
		}
		else if (item->val_len == strlen(JSON_TRUE) && !memcmp(item->val, JSON_TRUE, item->val_len)) {
			type = JTOKE_TRUE;
		}
		else if (item->val_len == strlen(JSON_FALSE) && !memcmp(item->val, JSON_FALSE, item->val_len)) {
			type = JTOKE_FALSE;
		}
		else if (is_integer(item->val, item->val_len)) {
			type = JTOKE_INT;
		}
		else if (is_real(item->val, item->val_len)) {
			type = JTOKE_REAL;
		}
		else {
			// Unknown type?
			DBG_PRINT("unknown type\n");
			return JTOKE_ERROR;
		}
	}

	ctx->pos++;
	DBG_PRINT("item complete.\n");
	return type;
}

