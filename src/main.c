#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h> 

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

Bencode *bencode_parse(const char **cursor_ptr, const char *end_ptr);

typedef struct {
	bool ok;
	bool negative;
	long long value;
} IntegerParseResult;


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
        Bencode *item = bencode_parse(cursor_ptr, end_ptr);
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
        Bencode *key = bencode_parse(cursor_ptr, end_ptr);
        if (!key || key->type != BENCODE_STRING) break;

        Bencode *value = bencode_parse(cursor_ptr, end_ptr);
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

Bencode *bencode_parse(const char **cursor_ptr, const char *end_ptr) {
	if (*cursor_ptr >= end_ptr) return NULL;
	char c = **cursor_ptr;

	if (isdigit(c)) return _parse_string(cursor_ptr, end_ptr);
	if ('i' == c) return _parse_integer(cursor_ptr, end_ptr);
	if ('l' == c) return _parse_list(cursor_ptr, end_ptr);
	if ('d' == c) return _parse_dictionary(cursor_ptr, end_ptr);

	return NULL;
}

void print_indent(int n) {
	for (int i = 0; i < n; i++) printf("  ");
}

void print_bencode(Bencode *root, int indent) {
	switch (root->type) {
		case BENCODE_INTEGER:
			print_indent(indent);
			printf("Int: %lld\n", root->integer);
			break;
		case BENCODE_STRING:
            print_indent(indent);
			printf("String (%ld): ", root->string.length);
			fwrite(root->string.data, 1, root->string.length, stdout);
			printf("\n");
			break;
		case BENCODE_LIST:
			print_indent(indent);
			printf("List:\n");
			for (size_t i = 0; i < root->list.count; i++)
				print_bencode(root->list.items[i], indent + 1);
			break;
		case BENCODE_DICTIONARY:
			print_indent(indent);
			printf("Dict:\n");
			for (size_t i = 0; i < root->dictionary.count; i++) {
				print_indent(indent + 1);
				printf("Key:\n");
				print_bencode(root->dictionary.keys[i], indent + 2);
				print_indent(indent + 1);
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


unsigned char *read_file(const char *filename, size_t *out_size) {
	unsigned char *buffer = NULL;

	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		perror("fopen");
		return NULL;
	}

	if (fseek(fp, 0, SEEK_END) !=  0) {
		perror("fseek");
		goto close_and_return;
	}

	long size = ftell(fp);
	if (size < 0) {
        perror("ftell");
        goto close_and_return;
    }

	rewind(fp);

	buffer = malloc(size);
    if (!buffer) {
        perror("malloc");
        goto close_and_return;
    }

	size_t bytes_read = fread(buffer, 1, size, fp);
    if (bytes_read != size) {
        fprintf(stderr, "fread: expected %ld, got %zu\n", size, bytes_read);
        free(buffer);
		buffer = NULL;
        goto close_and_return;
    }

    if (out_size) *out_size = size;

close_and_return:
	fclose(fp);
	return buffer;
}


int main(int argc, char** argv) {

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	size_t size = 0;
	unsigned char *file_data = read_file(argv[1], &size);
	if (!file_data) {
		fprintf(stderr, "Failed to read the file\n");
		return 1;
	}

	//printf("File content: \n%.*s\n", (int)size, file_data);

	//free(file_data);

	//const char* bencode = "d4:name6:Ubuntu3:agei20e5:filesl5:file15:file2ee";
	//const char* bencode = "d8:announce41:http://bttracker.debian.org:6969/announce7:comment35:\"Debian CD from cdimage.debian.org\"13:creation datei1573903810e9:httpseedsl145:https://cdimage.debian.org/cdimage/release/10.2.0//srv/cdbuilder.debian.org/dst/deb-cd/weekly-builds/amd64/iso-cd/debian-10.2.0-amd64-netinst.iso145:https://cdimage.debian.org/cdimage/archive/10.2.0//srv/cdbuilder.debian.org/dst/deb-cd/weekly-builds/amd64/iso-cd/debian-10.2.0-amd64-netinst.isoe4:infod6:lengthi351272960e4:name31:debian-10.2.0-amd64-netinst.iso12:piece lengthi262144e6:pieces26800:�����PS�^�� (binary blob of the hashes of each piece)ee";
	//size_t size = strlen(bencode);
	const char* cursor_ptr = file_data;
	const char* end_ptr = file_data + size;

	Bencode* parsed_bencode = bencode_parse(&cursor_ptr, end_ptr);

	print_bencode(parsed_bencode, 0);
	bencode_free(parsed_bencode);
	free(file_data);

	return 0;
}