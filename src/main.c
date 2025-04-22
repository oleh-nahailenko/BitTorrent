#include <stdio.h>
#include <stdlib.h>

#include "bencode.h"

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

	Bencode* bencode = bencode_parse(file_data, size);

	print_bencode(bencode, 0);
	bencode_free(bencode);
	free(file_data);

	return 0;
}