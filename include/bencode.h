#ifndef BENCODE_H
#define BENCODE_H

#include <stddef.h>

typedef struct Bencode Bencode;

typedef enum {
	BENCODE_STRING,
	BENCODE_INTEGER,
	BENCODE_LIST,
	BENCODE_DICTIONARY,
} BencodeType;

struct Bencode {
	BencodeType type;
	union {
		long long integer;
		struct { const char* data; size_t length; } string;
		struct { Bencode** items; size_t count; } list;
		struct { Bencode** keys; Bencode** values; size_t count; } dictionary;
	};
};

/**
 * Parses a Bencoded buffer into a Bencode data structure.
 * 
 * This function takes a raw buffer containing Bencoded data 
 * and returns a pointer to a dynamically allocated `Bencode` structure
 * representing the parsed data.
 * 
 * The caller is responsible for freeing the returned structure using
 * `bencode_free()` to avoid memory leaks.
 * 
 * @param buffer A pointer to the input buffer containing Bencoded data.
 *               The buffer does not need to be null-terminated.
 * 
 * @param length The length (in bytes) of the buffer. 
 * 
 * @return A pointer to a `Bencode` structure on success, or `NULL` on error.
 */
Bencode *bencode_parse(const char *buffer, size_t length);

/**
 * Recursively prints the contents of a Bencode structure to 'stdout'
 * in a human-readable format.
 * 
 * @param root A pointer to the root of the Bencode structure to print.
 *             If `root` is NULL, the function prints nothing.
 * 
 * @param indent The number of spaces to indent for nested structures.
 *               Typically starts at 0.
 */
void print_bencode(Bencode *root, int indent);

/**
 * Frees a Bencode data structure and all of its nodes.
 * 
 * @param root A pointer to the root of the Bencode structure to free.
 *             If `root` is NULL, the function does nothing.
 */
void bencode_free(Bencode *root);

#endif 