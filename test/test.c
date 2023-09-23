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

#ifdef JTOKE_DEBUG
#define DBG_PRINT(...)	do { printf("%s:%d (%s) - ", __FILE__, __LINE__, __func__); printf(__VA_ARGS__); } while (0);
#else
#define DBG_PRINT(...)
#endif

typedef struct
{
	const char* name;
	const char* val;
	jtoke_type_t type;
} test_case_field_t;

typedef struct 
{
	// Fields represented as a json string
	const char* json;
	// List of fields for the test case
	// Terminate with an entry where 'type == TEST_TERM'
	test_case_field_t* fields;
} test_case_t;

#define TEST_TERM	4242

static test_case_t cases[] = {
	{
		.fields = (test_case_field_t []) {
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
			{ .type = JTOKE_STRING, .val = ", my test, str" }, // basic string
			{ .type = JTOKE_STRING, .val = "" },			// basic string
			{ .type = JTOKE_STRING, .val = " " },			// basic string
			{ .type = JTOKE_STRING, .val = "123" },			// string that looks like an int
			{ .type = JTOKE_STRING, .val = "1.23" },		// string that looks like a float
			{ .type = JTOKE_STRING, .val = "true" },		// string that looks like a bool
			{ .type = JTOKE_STRING, .val = "extra \\\"escapes\\\"" },
			{ .type = JTOKE_STRING, .val = "multi\nline\nstring" },
			{ .type = JTOKE_STRING, .val = "string\twith\ttabs" },
			{ .type = TEST_TERM }, // end marker
		},
	},

	// Negative cases dealing with types. These should all result in no fields.
	{ .json = "{ foo : \"bar\" }" },
	{ .json = "{ \"foo : bar\" }" },
	{ .json = "{ foo : bar }" },
	{ .json = "{ \"foo\" : \"bar }" },
	{ .json = "{ \"foo : \"bar\" }" },
	{ .json = "{ \"foo\" : tru3 }" },
	{ .json = "{ \"foo\" : fals3 }" },
	{ .json = "{ \"foo\" : 3f }" },
	{ .json = "{ \"foo\" : 3.f }" },

	// Negative cases dealing with truncated or malformed json.
	{ .json = "{ \"foo" },
	{ .json = "{ \"foo\"" },
	{ .json = "{ \"foo\" : " },
	{ .json = "{ \"foo\" : \"b" },

	// Incomplete/invalid json, but sufficient to handle.
	{ 
		.json = "{ \"foo\" : \"bar\"",
		.fields = (test_case_field_t []) {
			{ .type = JTOKE_STRING, .name = "foo", .val = "bar" },
			{ .type = TEST_TERM }, // end marker
		},
	},

	{ 
		.json = "{ \"foo\" : 123, \"",
		.fields = (test_case_field_t []) {
			{ .type = JTOKE_INT, .name = "foo", .val = "123" },
			{ .type = TEST_TERM }, // end marker
		},
	},

	{ 
		.json = "{ \"foo\" : 123, \"bar\" : ",
		.fields = (test_case_field_t []) {
			{ .type = JTOKE_INT, .name = "foo", .val = "123" },
			{ .type = TEST_TERM }, // end marker
		},
	},

	// Special characters in strings
	{ 
		.fields = (test_case_field_t []) {
			{ .type = JTOKE_STRING, .name = "foo bar", .val = "foo bar" },
			{ .type = JTOKE_STRING, .name = "foo\tbar", .val = "foo\tbar" },
			{ .type = JTOKE_STRING, .name = "foo\\\"", .val = "foo\\\"" },
			{ .type = TEST_TERM }, // end marker
		},
	},
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

const char* build_json_from_test(test_case_field_t* fields)
{
	// Big scratch array to hold the test string
	static char json[1024];

	if (NULL == fields) {
		return NULL;
	}

	memset(json, 0, sizeof(json));

	strncat(json, "{", 1);
	for (unsigned i=0; fields[i].type != TEST_TERM; i++) {
		if (!fields[i].name) {
			// Allocate a buffer for name
			char* name = malloc(32);
			snprintf(name, 32, "test_%u", i);
			fields[i].name = name;
		}

		snprintf(&json[strlen(json)], sizeof(json) - strlen(json) - 1,
			(JTOKE_STRING == fields[i].type) ? 
				// string types are quoted
				"\"%s\": \"%s\", " :
				// no quotes for non-string types
				"\"%s\": %s, ",
			fields[i].name, fields[i].val);
	}
	strncat(json, "}", 1);

	return json;
}

void runtest(test_case_t* test_case)
{
	test_case_field_t* fields = test_case->fields;
	const char* json = test_case->json;

	// Build the json if not already set
	if (NULL == json) {
		json = build_json_from_test(fields);
	}
	DBG_PRINT("testing json: %s\n", json);

	jtoke_context_t ctx;
	jtoke_item_t item;
	jtoke_type_t type;

	// Always init the context before parsing a new string
	ctx = JTOKE_CONTEXT_INIT;

	if (fields) {
		// Loop through the test fields. They should be reported in order.
		for (unsigned i=0; fields[i].type != TEST_TERM; i++) {
			type = jtoke_parse(&ctx, json, &item);

			DBG_PRINT("found type %d (%s)\n", type, lookup_type(type));

			if (type != fields[i].type) {
				printf("expected type %d (%s), but found %d (%s)\n", 
					fields[i].type, lookup_type(fields[i].type), type, lookup_type(type));
				assert(false);
			}

			if (type > 0) {
				DBG_PRINT("name '%.*s' has value '%.*s' (len=%u, type=%s)\n",
					item.name_len, item.name, item.val_len, item.val, item.val_len, lookup_type(type));

				if (item.name_len != strlen(fields[i].name)) {
					printf("expected name len %lu (%s), but found %u\n", 
						strlen(fields[i].name), fields[i].name, item.name_len);
					assert(false);
				}
				else if (memcmp(fields[i].name, item.name, item.name_len)) {
					printf("unexpected value. expected %s, but found %.*s\n", 
						fields[i].name, item.name_len, item.name);
					assert(false);
				}

				if (item.val_len != strlen(fields[i].val)) {
					printf("expected val len %lu, but found %u\n", strlen(fields[i].val), item.val_len);
					assert(false);
				}
				else if (memcmp(fields[i].val, item.val, item.val_len)) {
					printf("unexpected value. expected %s, but found %.*s\n", 
						fields[i].val, item.val_len, item.val);
					assert(false);
				}
			}
		}

		DBG_PRINT("end fields.");
	}

	// Check that nothing remains
	type = jtoke_parse(&ctx, json, &item);
	if (type > 0) {
		printf("expected empty json but found type %d (%s)\n", type, lookup_type(type));
		assert(false);
	}
}

int main(void)
{
	for (int i=0; i<ARRAY_SIZE(cases); i++) {
		runtest(&cases[i]);
	}

	printf("tests passed.\n");
	return 0;
}
