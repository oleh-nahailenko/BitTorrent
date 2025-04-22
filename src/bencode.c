#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h> 

#include "bencode.h"

typedef struct {
	bool ok;
	bool negative;
	long long value;
} IntegerParseResult;

static Bencode *_parse(const char **cursor_ptr, const char *end_ptr);

static IntegerParseResult _parse_int_until(const char **cursor_ptr, 
                                           const char *end_ptr,
                                           char delimiter) {

	IntegerParseResult result = { .ok = false };

	if (*cursor_ptr >= end_ptr) return result;

	long long value = 0;
	bool negative = false;

	if ('-' == **cursor_ptr) {
		negative = true;
		(*cursor_ptr)++;
	}

	while (*cursor_ptr < end_ptr && **cursor_ptr != delimiter) {
		if (!isdigit(**cursor_ptr)) return result; 

		int digit = **cursor_ptr - '0';
		if (value > (LLONG_MAX - digit) / 10) return result;

		value = value * 10 + digit;
		(*cursor_ptr)++;
	}

	if (*cursor_ptr >= end_ptr || **cursor_ptr != delimiter) return result;
	(*cursor_ptr)++;

	result.ok = true;
	result.negative = negative;
	result.value = negative ? -value : value;
	return result;
}

static Bencode *_parse_string(const char **cursor_ptr, const char *end_ptr) {
	IntegerParseResult result = _parse_int_until(cursor_ptr, end_ptr, ':');
	if (!result.ok || result.negative) return NULL;

	size_t length = result.value;
	if ((*cursor_ptr + length) > end_ptr) return NULL;

	Bencode *node_ptr = malloc(sizeof(Bencode));
	if (!node_ptr) return NULL;

	node_ptr->type = BENCODE_STRING;
	node_ptr->string.data = *cursor_ptr;
	node_ptr->string.length = length;

	*cursor_ptr += length;

	return node_ptr;
}

static Bencode *_parse_integer(const char **cursor_ptr, const char *end_ptr) {
    (*cursor_ptr)++; // skip 'i'
    
    IntegerParseResult result = _parse_int_until(cursor_ptr, end_ptr, 'e');
	if (!result.ok) return NULL;

    Bencode *node_ptr = malloc(sizeof(Bencode));
    if (!node_ptr) return NULL;

    node_ptr->type = BENCODE_INTEGER;
    node_ptr->integer = result.value;
    return node_ptr;
}

static Bencode *_parse_list(const char **cursor_ptr, const char *end_ptr) {
    (*cursor_ptr)++; // skip 'l'

    Bencode **items = NULL;
    size_t count = 0;

    while (*cursor_ptr < end_ptr && **cursor_ptr != 'e') {
        Bencode *item = _parse(cursor_ptr, end_ptr);
        if (!item) break;

        Bencode **tmp = realloc(items, sizeof(Bencode *) * (count + 1));
        if (!tmp) break;
        items = tmp;
        items[count++] = item;
    }

    if (*cursor_ptr >= end_ptr || **cursor_ptr != 'e') {
        free(items);
        return NULL;
    }

    (*cursor_ptr)++; // skip 'e'

    Bencode *node_ptr = malloc(sizeof(Bencode));
    if (!node_ptr) return NULL;

    node_ptr->type = BENCODE_LIST;
    node_ptr->list.items = items;
    node_ptr->list.count = count;

    return node_ptr;
}

static Bencode *_parse_dictionary(const char **cursor_ptr, const char *end_ptr) {
    (*cursor_ptr)++; // skip 'd'

    Bencode **keys = NULL, **values = NULL;
    size_t count = 0;

    while (*cursor_ptr < end_ptr && **cursor_ptr != 'e') {
        Bencode *key = _parse(cursor_ptr, end_ptr);
        if (!key || key->type != BENCODE_STRING) break;

        Bencode *value = _parse(cursor_ptr, end_ptr);
        if (!value) break;

        keys = realloc(keys, sizeof(Bencode *) * (count + 1));
        values = realloc(values, sizeof(Bencode *) * (count + 1));
        if (!keys || !values) break;

        keys[count] = key;
        values[count] = value;
        count++;
    }

    if (*cursor_ptr >= end_ptr || **cursor_ptr != 'e') {
        free(keys);
        free(values);
        return NULL;
    }

    (*cursor_ptr)++; // skip 'e'

    Bencode *node_ptr = malloc(sizeof(Bencode));
    if (!node_ptr) return NULL;

    node_ptr->type = BENCODE_DICTIONARY;
    node_ptr->dictionary.keys = keys;
    node_ptr->dictionary.values = values;
    node_ptr->dictionary.count = count;

    return node_ptr;
}

static Bencode *_parse(const char **cursor_ptr, const char *end_ptr) {
	if (*cursor_ptr >= end_ptr) return NULL;
	char c = **cursor_ptr;

	if (isdigit(c)) return _parse_string(cursor_ptr, end_ptr);
	if ('i' == c) return _parse_integer(cursor_ptr, end_ptr);
	if ('l' == c) return _parse_list(cursor_ptr, end_ptr);
	if ('d' == c) return _parse_dictionary(cursor_ptr, end_ptr);

	return NULL;
}

static void _print_indent(int n) {
	for (int i = 0; i < n; i++) printf("  ");
}

Bencode *bencode_parse(const char *buffer, size_t length) {
	if (!buffer) {
		fprintf(stderr, "[bencode] Invalid argument: buffer=%p\n", (void*)buffer);
		return NULL;
	}

	const char* cursor = buffer;
	const char* end = buffer + length;

	return _parse(&cursor, end);
}

void print_bencode(Bencode *root, int indent) {
	switch (root->type) {
		case BENCODE_INTEGER:
			_print_indent(indent);
			printf("Int: %lld\n", root->integer);
			break;
		case BENCODE_STRING:
            _print_indent(indent);
			printf("String (%ld): ", root->string.length);
			fwrite(root->string.data, 1, root->string.length, stdout);
			printf("\n");
			break;
		case BENCODE_LIST:
			_print_indent(indent);
			printf("List:\n");
			for (size_t i = 0; i < root->list.count; i++)
				print_bencode(root->list.items[i], indent + 1);
			break;
		case BENCODE_DICTIONARY:
			_print_indent(indent);
			printf("Dict:\n");
			for (size_t i = 0; i < root->dictionary.count; i++) {
				_print_indent(indent + 1);
				printf("Key:\n");
				print_bencode(root->dictionary.keys[i], indent + 2);
				_print_indent(indent + 1);
				printf("Value:\n");
				print_bencode(root->dictionary.values[i], indent + 2);
			}
			break;
	}
}

void bencode_free(Bencode *root) {
	if (!root) return;

	switch (root->type) {
		case BENCODE_STRING: break;
		case BENCODE_INTEGER: break;
		case BENCODE_LIST:
			for (size_t i = 0; i < root->list.count; i++)
				bencode_free(root->list.items[i]);
		
			free(root->list.items);
			break;
		
		case BENCODE_DICTIONARY:
			for (size_t i = 0; i < root->dictionary.count; i++) {
				bencode_free(root->dictionary.keys[i]);
				bencode_free(root->dictionary.values[i]);
			}

			free(root->dictionary.keys);
			free(root->dictionary.values);
			break;
		default:
			fprintf(stderr, "Unsupported value of BencodeType\n");
			exit(EXIT_FAILURE);
			break;
	}

	free(root);
}
