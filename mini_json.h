#ifndef _MINI_JSON_H__
#define _MINI_JSON_H__

#include <stddef.h> /* size_t */

typedef enum { MINI_NULL, MINI_FALSE, MINI_TRUE, MINI_NUMBER, MINI_STRING, MINI_ARRAY, MINI_OBJECT } mini_type;

typedef struct mini_value mini_value;

struct mini_value {
    union {
        struct { mini_value* e; size_t size; }a; /* array */
        struct { char* s; size_t len; }s;  /* string: null-terminated string, string length */
        double n;                          /* number */
    }u;
    mini_type type;
};

enum {
    MINI_PARSE_OK = 0,
    MINI_PARSE_EXPECT_VALUE,
    MINI_PARSE_INVALID_VALUE,
    MINI_PARSE_ROOT_NOT_SINGULAR,
    MINI_PARSE_NUMBER_TOO_BIG,
    MINI_PARSE_MISS_QUOTATION_MARK,
    MINI_PARSE_INVALID_STRING_ESCAPE,
    MINI_PARSE_INVALID_STRING_CHAR,
    MINI_PARSE_INVALID_UNICODE_HEX,
    MINI_PARSE_INVALID_UNICODE_SURROGATE,
    MINI_PARSE_MISS_COMMA_OR_SQUARE_BEACKET
};

#define mini_init(v) do { (v)->type = MINI_NULL; } while(0)

int mini_parse(mini_value* v, const char* json);

void mini_free(mini_value* v);

mini_type mini_get_type(const mini_value* v);

#define mini_set_null(v) mini_free(v)

int mini_get_boolean(const mini_value* v);
void mini_set_boolean(mini_value* v, int b);

double mini_get_number(const mini_value* v);
void mini_set_number(mini_value* v, double n);

const char* mini_get_string(const mini_value* v);
size_t mini_get_string_length(const mini_value* v);
void mini_set_string(mini_value* v, const char* s, size_t len);

size_t mini_get_array_size(const mini_value* v);
mini_value* mini_get_array_element(const mini_value* v, size_t index);

#endif //_MINI_JSON_H__
