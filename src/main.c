#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


typedef struct Bencode Bencode;
Bencode* parse(const char** cursor);

struct Bencode {
    char type; // d,l,i,s
    union {
        long int_val;
        struct { const char* data; int length; } str_val;
        struct { Bencode** items; int count; } list_val;
        struct { Bencode** keys; Bencode** values; int count; } dict_val;
    };
};


static long _parse_int_until(char end, const char** cursor) {
    long value = 0l;
    int sign = 1;
    if ('-' == **cursor) {
       sign = -1;
       (*cursor)++;
    }

    while (**cursor && **cursor != end) {
        value = value * 10 + (**cursor - '0');
        (*cursor)++;
    }

    (*cursor)++; // skip the 'end'
    return sign * value;
}

Bencode* _parse_str(const char** cursor) {
    int length = _parse_int_until(':', cursor);

    Bencode* str = malloc(sizeof(Bencode));
    str->type = 's';
    str->str_val.data = *cursor;
    str->str_val.length = length;
    *cursor += length;

    return str;
}

Bencode* _parse_int(const char** cursor) {
    (*cursor)++; // skip 'i'
    long value =_parse_int_until('e', cursor);

    Bencode* _int = malloc(sizeof(Bencode));
    _int->type = 'i';
    _int->int_val = value;

    return _int;
}

Bencode* _parse_list(const char** cursor) {
    (*cursor)++; // skip 'l'

    //TODO: Implement algorithm for dynamic allocation
    Bencode** items = malloc(sizeof(Bencode*) * 64);

    int count = 0;
    while (**cursor != 'e') {
        items[count] = parse(cursor);
        count++;
    }

    (*cursor)++; // skip 'e'

    Bencode* list = malloc(sizeof(Bencode));
    list->type = 'l';
    list->list_val.items = items;
    list->list_val.count = count;

    return list;
}

Bencode* _parse_dict(const char **cursor) {
    (*cursor)++; // Skip 'd'

    //TODO: Implement algorithm for dynamic allocation
    Bencode **keys = malloc(sizeof(Bencode*) * 64);
    Bencode **values = malloc(sizeof(Bencode*) * 64);

    int count = 0;
    while (**cursor != 'e') {
        keys[count] = parse(cursor);
        values[count] = parse(cursor);
        count++;
    }

    (*cursor)++; // Skip 'e'

    Bencode *dict = malloc(sizeof(Bencode));
    dict->type = 'd';
    dict->dict_val.keys = keys;
    dict->dict_val.values = values;
    dict->dict_val.count = count;

    return dict;
}

Bencode* parse(const char** cursor) {
    if ('i' == **cursor) return _parse_int(cursor);
    else if ('l' == **cursor) return _parse_list(cursor);
    else if ('d' == **cursor) return _parse_dict(cursor);
    else if (isdigit(**cursor)) return _parse_str(cursor);
    else return NULL;
}

void print_indent(int n) {
    for (int i = 0; i < n; i++) printf("  ");
}

void print_bencode(Bencode *val, int indent) {
    switch (val->type) {
        case 'i':
            print_indent(indent);
            printf("Int: %ld\n", val->int_val);
            break;
        case 's':
            print_indent(indent);
            printf("String (%d): ", val->str_val.length);
            fwrite(val->str_val.data, 1, val->str_val.length, stdout);
            printf("\n");
            break;
        case 'l':
            print_indent(indent);
            printf("List:\n");
            for (int i = 0; i < val->list_val.count; i++)
                print_bencode(val->list_val.items[i], indent + 1);
            break;
        case 'd':
            print_indent(indent);
            printf("Dict:\n");
            for (int i = 0; i < val->dict_val.count; i++) {
                print_indent(indent + 1);
                printf("Key:\n");
                print_bencode(val->dict_val.keys[i], indent + 2);
                print_indent(indent + 1);
                printf("Value:\n");
                print_bencode(val->dict_val.values[i], indent + 2);
            }
            break;
    }
}

void free_bencode(Bencode *val) {
    if (!val) return;

    switch (val->type) {
        case 'l':
            for (int i = 0; i < val->list_val.count; i++)
                free_bencode(val->list_val.items[i]);
            free(val->list_val.items);
            break;
        case 'd':
            for (int i = 0; i < val->dict_val.count; i++) {
                free_bencode(val->dict_val.keys[i]);
                free_bencode(val->dict_val.values[i]);
            }
            free(val->dict_val.keys);
            free(val->dict_val.values);
            break;
        default: break;
    }

    free(val);
}

int main(int argc, char** argv) {
    //const char* bencode = "d4:name6:Ubuntu3:agei20e5:filesl5:file15:file2ee";
    const char* bencode = "d8:announce41:http://bttracker.debian.org:6969/announce7:comment35:\"Debian CD from cdimage.debian.org\"13:creation datei1573903810e9:httpseedsl145:https://cdimage.debian.org/cdimage/release/10.2.0//srv/cdbuilder.debian.org/dst/deb-cd/weekly-builds/amd64/iso-cd/debian-10.2.0-amd64-netinst.iso145:https://cdimage.debian.org/cdimage/archive/10.2.0//srv/cdbuilder.debian.org/dst/deb-cd/weekly-builds/amd64/iso-cd/debian-10.2.0-amd64-netinst.isoe4:infod6:lengthi351272960e4:name31:debian-10.2.0-amd64-netinst.iso12:piece lengthi262144e6:pieces26800:�����PS�^�� (binary blob of the hashes of each piece)ee";
    const char* cursor = bencode;

    Bencode* parsed_bencode = parse(&cursor);

    print_bencode(parsed_bencode, 0);
    free_bencode(parsed_bencode);

    return 0;
}