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
#define DBG_PRINT(...)	do { printf("%s:%d (%s) - ", __FILE__, __LINE__, __func__); printf(__VA_ARGS__); } while (0);
#else
#define DBG_PRINT(...)
#endif

#define RETURN_IF_NULL(ch)	if (0 == ch) { return JTOKE_ERROR; }

#define ADV_AND_CHECK(ptr)	do { \
	char ch = *ptr++; \
	RETURN_IF_NULL(ch); \
	} while(0);

#define JSON_NULL	"null"
#define JSON_TRUE	"true"
#define JSON_FALSE	"false"

#define JSON_WHITESPACE	" \t\r\n"

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

static char advance_until_end_quote(const char** c)
{
	char ch = **c;
	int skip = 0;
	while (ch) {
		if (skip) {
			// process escaped val
			skip = 0;
		}
		else if (ch == '\\') {
			// found escape start
			skip = 1;
		}
		else if (ch == '"') {
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

static jtoke_type_t detect_type(const char* str, unsigned int str_len)
{
	int i;
	if (0 == str_len) return JTOKE_INVALID;

	if (str_len == 4 && 
		str[0] == 'n' && 
		str[1] == 'u' && 
		str[2] == 'l' && 
		str[3] == 'l') {
			return JTOKE_NULL;
		}

	if (str_len == 4 && 
		str[0] == 't' && 
		str[1] == 'r' && 
		str[2] == 'u' && 
		str[3] == 'e') {
			return JTOKE_TRUE;
		}

	if (str_len == 5 && 
		str[0] == 'f' && 
		str[1] == 'a' && 
		str[2] == 'l' && 
		str[3] == 's' && 
		str[4] == 'e') {
			return JTOKE_FALSE;
		}

	for (i=0; i<str_len; i++) {
		if (!in_charset("-0123456789", str[i])) {
			break;
		}
	}
	if (i == str_len) return JTOKE_INT;

	for (i=0; i<str_len; i++) {
		if (!in_charset("-0123456789.eE+", str[i])) {
			break;
		}
	}
	if (i == str_len) return JTOKE_REAL;

	return JTOKE_INVALID;
}

jtoke_type_t jtoke_parse(jtoke_context_t* ctx, const char* json, jtoke_item_t* item)
{
	jtoke_type_t type;
	char ch;

	if (0 == ctx->pos) {
		ctx->pos = json;
		// Allow (but don't enforce) an open brace, but only at the start of 
		// the string.
		ch = advance_past(JSON_WHITESPACE "{", &ctx->pos);
		RETURN_IF_NULL(ch);
	}

	DBG_PRINT("checking for open quote\n");
	ch = advance_past(JSON_WHITESPACE, &ctx->pos);
	DBG_PRINT("first char: %c\n", ch);
	RETURN_IF_NULL(ch);
	if (ch != '\"') {
		return JTOKE_ERROR;
	}

	ADV_AND_CHECK(ctx->pos);
	item->name = ctx->pos;

	DBG_PRINT("checking for close quote\n");
	ch = advance_until("\"", &ctx->pos);
	RETURN_IF_NULL(ch);

	item->name_len = ctx->pos - item->name;
	ADV_AND_CHECK(ctx->pos);
	DBG_PRINT("name_len is %u, next char is '%c'\n", item->name_len, *ctx->pos);

	DBG_PRINT("advancing past colon\n");
	ch = advance_past(JSON_WHITESPACE ":", &ctx->pos);
	RETURN_IF_NULL(ch);

	if (ch == '"') {
		type = JTOKE_STRING;
		ADV_AND_CHECK(ctx->pos);
		item->val = ctx->pos;
		DBG_PRINT("found string. advancing until end.\n");
		ch = advance_until_end_quote(&ctx->pos);
		RETURN_IF_NULL(ch);
		item->val_len = ctx->pos - item->val;
		// Move past the quote character itself
		// Note we cannot use advance_past() here because it may chew up 
		// the start of the next quote
		ADV_AND_CHECK(ctx->pos);
		ch = advance_past(JSON_WHITESPACE ",", &ctx->pos);
		DBG_PRINT("end of string char: %c (%c)\n", ch, *ctx->pos);
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
		ch = advance_until(JSON_WHITESPACE ",}", &ctx->pos);
		RETURN_IF_NULL(ch);
		item->val_len = ctx->pos - item->val;

		type = detect_type(item->val, item->val_len);
		if (type > 0) {
			// Found a valid type, proceed.
		}
		else {
			// Unknown type?
			DBG_PRINT("unknown type\n");
			return JTOKE_ERROR;
		}

		// Move to the char following the value
		ADV_AND_CHECK(ctx->pos);
	}

	DBG_PRINT("item complete.\n");
	return type;
}

