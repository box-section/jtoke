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

#ifndef JTOKE_DOT_H
#define JTOKE_DOT_H

// Combined type and status enum to reduce footprint
typedef enum
{
	// memset to 0 creates invalid object
	JTOKE_INVALID = 0,

	// values >0 are valid types
	JTOKE_NULL,
	JTOKE_TRUE,
	JTOKE_FALSE,
	JTOKE_INT,
	JTOKE_REAL,
	JTOKE_STRING,
	JTOKE_OBJ,
	JTOKE_ARRAY,

	// values <0 are errors
	JTOKE_ERROR = -1
} jtoke_type_t;

// Context/state information. Callers should consider this opaque.
typedef struct {
	const char* pos;
} jtoke_context_t;

// Initialize all jtoke_context_t to this prior to processing a new string.
// Assume this is opaque for forward-compatibility.
#define JTOKE_CONTEXT_INIT	(jtoke_context_t){ 0 }

// Result struct, arranged to avoid need to pack
typedef struct {
	const char* name;			// pointer to field name, after quote. *not null-terminated*
	const char* val;			// pointer to field value as a string. *not null-terminated*
	unsigned short name_len;	// length of name string, in bytes
	unsigned short val_len;		// length of value string, in bytes
} jtoke_item_t;

/**
 * @brief Parse a name/value pair from a json string.
 * 
 * Each call to this routine provides a new field name and value in item. When 
 * the end of the json object is reached, it returns a negative number.
 *
 * While the strings in item result are not null-terminated, the json string
 * must end with null. This double standard is a neccessary evil.
 *
 * @param ctx    Opaque context structure to track progress through the item.
 *               Set the context struct to JTOKE_CONTEXT_INIT before processing
 *               a new string.
 * @param json   JSON string to parse
 * @param item   Resulting output
 *
 * @return Zero on success or (negative) error code otherwise.
 */
jtoke_type_t jtoke_parse(jtoke_context_t* ctx, const char* json, jtoke_item_t* item);

#endif // JTOKE_DOT_H
