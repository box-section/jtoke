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

int main(void)
{
	static const char jobj[] = "{ "
		"\"name_str\"	 : \"test str\", "
		"\"name_int\":42, "
		"\"name_real\" : 3.14 , "
		"\"name_real0\" : -3.14 , \n"
		"\"name_real1\" : 1.0E+2 ,"
		"\"name_real2\" : 9.876E-05,"
		"\"name_null\" : null , "
		"\"name_true\":  true "
		" }";

	jtoke_context_t ctx;
	jtoke_item_t item;
	jtoke_type_t type;

	// Always wipe the context before parsing a new string
	memset(&ctx, 0, sizeof(ctx));

	// Loop through all fields
	while ((type = jtoke_parse(&ctx, jobj, &item)) > 0) {
		printf("name '%.*s' has value '%.*s' (type=%s)",
			item.name_len, item.name, item.val_len, item.val, lookup_type(type));

		if (JTOKE_INT == type) {
			// Use strtol to convert integers from strings
			int val = strtol(item.val, NULL, 0);
			printf(" val=%d", val);
		}
		else if (JTOKE_REAL == type) {
			// Use strtof to convert integers from strings
			float val = strtof(item.val, NULL);
			printf(" val=%f", val);
		}
		printf("\n");
	}
}
