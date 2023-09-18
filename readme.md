# jtoke: a truly minimal json tokenizer

This is a rudimentary parser for simple json strings. It is designed for embedded systems that need to decode particular known fields, into a struct for example. It is not meant for general purpose json processing of arbitrary fields.

The main goal is to minimize the RAM footprint. Many/most parsers allocate or require numerous tracking structures.

## Usage

```
jtoke_context_t ctx;
jtoke_item_t item;
jtoke_type_t type;
const char jobj[] = "{ \"foo\": 42, \"bar\": true}";

// Always init the context before parsing a new string
ctx = JTOKE_CONTEXT_INIT;

// Loop through all fields until end of string
while ((type = jtoke_parse(&ctx, jobj, &item)) > 0) {

	// Use string precision (.*) to specify the length when printing
	printf("found name '%.*s' ", item.name_len, item.name);

	// Process field using item
	if (JTOKE_INT == type) {
		// Use strtol to convert to int
		int val = strtol(item.val, NULL, 0);
	}

	else if (JTOKE_REAL == type) {
		// Use strtof to convert to float
		float val = strtof(item.val, NULL);
	}
	
}
```


## Design
This design uses the source json object as data wherever possible, and leaves it to the caller to decode into other buffers, if needed. The source json is not modified and can even be in `const` memory (flash, ROM, etc).

Some maintainers have [questioned](https://github.com/DaveGamble/cJSON/issues/253#issuecomment-378895161) whether a malloc-less approach is even possible.

This module has the following features:
* no memory allocation
* no callbacks
* no recursion
* no `static` data (and thus theoretically thread-safe)
* requires only two structs to be created by the caller
* 1 header file and 1 source file

Note that this means streaming is not supported since it requires complete json fields to be present.

## What is the catch?
The module returns pointers into the original json object, so the strings are not null-terminated. Consider this json object:

```
{ "foo": 42, "bar": true }
```

The module provides a pointer to `foo`, and its length. But the `\0` termination is not found until the end of the string. 

Care must be taken when operating on the output strings.

