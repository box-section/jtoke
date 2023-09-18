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

// TODO: this is more of an example than a test.
//       add proper self-checks.

// To run this code, use the runtests.sh script and observe the output.

#include "jtoke.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)	(sizeof(arr)/sizeof(arr[0]))
#endif

#ifdef JTOKE_TEST_DEBUG
#define DBG_PRINT	printf
#else
#define DBG_PRINT(...)
#endif

typedef struct
{
	const char* val;
	jtoke_type_t type;
} test_case_t;

static const test_case_t t0[] = {
	{ .type = JTOKE_INT, .val = "42" },
	{ .type = JTOKE_INT, .val = "424242424242" },

	{ .type = JTOKE_REAL, .val = "3.14" },
	{ .type = JTOKE_REAL, .val = "-3.14" },
	{ .type = JTOKE_REAL, .val = "1.0E+2" },
	{ .type = JTOKE_REAL, .val = "-1.0E+2" },
	{ .type = JTOKE_REAL, .val = "9.876E-05" },
	{ .type = JTOKE_REAL, .val = "-9.876E-05" },

	{ .type = JTOKE_TRUE,  .val = "true" },
	{ .type = JTOKE_FALSE, .val = "false" },
	{ .type = JTOKE_NULL,  .val = "null" },

	{ .type = JTOKE_STRING, .val = "test" },		// basic string
	{ .type = JTOKE_STRING, .val = "test str" },	// basic string
	{ .type = JTOKE_STRING, .val = "" },			// basic string
	{ .type = JTOKE_STRING, .val = " " },			// basic string
	{ .type = JTOKE_STRING, .val = "123" },			// string that looks like an int
	{ .type = JTOKE_STRING, .val = "1.23" },		// string that looks like a float
	{ .type = JTOKE_STRING, .val = "true" },		// string that looks like a bool
	{ .type = JTOKE_STRING, .val = "extra \\\"escapes\\\"" },
	{ .type = JTOKE_STRING, .val = "multi\nline\nstring" },
	{ .type = JTOKE_STRING, .val = "string\twith\ttabs" },
};

const char* lookup_type(jtoke_type_t type)
{
	switch (type) {
		case JTOKE_INVALID:	return "invalid";
		case JTOKE_NULL:	return "null";
		case JTOKE_TRUE:	return "true";
		case JTOKE_FALSE:	return "false";
		case JTOKE_INT:		return "int";
		case JTOKE_REAL:	return "real";
		case JTOKE_STRING:	return "string";
		case JTOKE_OBJ:		return "obj";
		case JTOKE_ARRAY:	return "array";
		default:
			break;
	}
	return "unknown";
}

const char* build_json_from_test(const test_case_t* test_case, unsigned test_case_count)
{
	// Big scratch array to hold the test string
	static char json[1024];
	// Temporary buffer for names
	char nameval[256];

	memset(json, 0, sizeof(json));

	strncat(json, "{", sizeof(json));
	for (unsigned i=0; i<test_case_count; i++) {
		snprintf(nameval, sizeof(nameval), 
			(JTOKE_STRING == test_case[i].type) ? 
				// string types are quoted
				"\"test_%u\": \"%s\", " :
				// no quotes for non-string types
				"\"test_%u\": %s, ",
			i, test_case[i].val);
		strncat(json, nameval, sizeof(json));
	}
	strncat(json, "}", sizeof(json));

	return json;
}

void runtest(const test_case_t* test_case, unsigned test_case_count, const char* json)
{
	if (NULL == json) {
		json = build_json_from_test(test_case, test_case_count);
	}
	DBG_PRINT("test json: %s\n", json);

	jtoke_context_t ctx;
	jtoke_item_t item;
	jtoke_type_t type;

	// Always wipe the context before parsing a new string
	memset(&ctx, 0, sizeof(ctx));

	// Loop through the test fields. They should be reported in order.
	for (unsigned i=0; i<test_case_count; i++) {
		type = jtoke_parse(&ctx, json, &item);
		if (type > 0) {
			DBG_PRINT("name '%.*s' has value '%.*s' (len=%u, type=%s)\n",
				item.name_len, item.name, item.val_len, item.val, item.val_len, lookup_type(type));

			if (type != test_case[i].type) {
				printf("expected type %u (%s), but found %u (%s)\n", 
					type, lookup_type(type), test_case[i].type, lookup_type(test_case[i].type));
				assert(false);
			}
			else if (item.val_len != strlen(test_case[i].val)) {
				printf("expected len %u, but found %u\n", strlen(test_case[i].val), item.val_len);
				assert(false);
			}
			else if (memcmp(test_case[i].val, item.val, item.val_len)) {
				printf("unexpected value. expected %s, but found %.*s", 
					test_case[i].val, item.val_len, item.val);
				assert(false);
			}
		}
		else {
			assert(false && "invalid");
		}
	}

	// Check that nothing remains
	assert(jtoke_parse(&ctx, json, &item) < 0);
}

int main(void)
{
	runtest(t0, ARRAY_SIZE(t0), NULL);
	printf("tests passed.\n");
	return 0;
}
