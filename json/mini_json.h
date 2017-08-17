#ifndef _MINI_JSON_H__
#define _MINI_JSON_H__

#include <stddef.h> /* size_t */
#include "../map/map.h"

typedef enum { MINI_NULL, MINI_FALSE, MINI_TRUE, MINI_NUMBER, MINI_STRING, MINI_ARRAY, MINI_OBJECT } mini_type;

typedef struct mini_value mini_value;

struct mini_value {
    union {
        struct { Map* pmap; size_t size; }o; /* object */
        struct { mini_value* e; size_t size; }a; /* array */
        struct { char* s; size_t len; }s;  /* string: null-terminated string, string length */
        double n;                          /* number */
    }u;
    mini_type type;
};

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
}mini_context;

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
    MINI_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    MINI_PARSE_MISS_KEY,
    MINI_PARSE_MISS_COLON,
    MINI_PARSE_MISS_COMMA_OR_CURLY_BRACKET,
    MINI_GENERATE_OK
};

/*****************************************
 *
 *              interface
 *
 *****************************************/
void mini_show_value(const mini_value* v);
void mini_add_value_to_array(mini_value* arr, mini_value* v);
void mini_add_value_to_object(mini_value* obj, mini_value* key, mini_value* value);

int mini_parse(mini_value* v, const char* json);
int mini_generate(const mini_value* v, char** json, size_t* length);
void mini_free(mini_value* v);
//for deep copy
mini_value* mini_backup(mini_value* v);


#define mini_init(v) do { (v)->type = MINI_NULL; } while(0)

mini_type mini_get_type(const mini_value* v);
void mini_set_type(mini_value* v, mini_type type);

#define mini_set_null(v) mini_free(v)

int mini_get_boolean(const mini_value* v);
void mini_set_boolean(mini_value* v, int b);

double mini_get_number(const mini_value* v);
void mini_set_number(mini_value* v, double n);

const char* mini_get_string(const mini_value* v);
size_t mini_get_string_length(const mini_value* v);
void mini_set_string(mini_value* v, const char* s, size_t len);

void mini_init_array(mini_value* v);
size_t mini_get_array_size(const mini_value* v);
mini_value* mini_get_array_element(const mini_value* v, size_t index);

void mini_init_object(mini_value* v);
size_t mini_get_object_size(const mini_value* v);
mini_value* mini_get_object_value(const mini_value* v, const char* key);

/******************************************
 *
 *              map
 * 
 ******************************************/
Map* get_map(const mini_value* v);
Item* new_item(const char* key, void *value);
void inner_clear(void *p);
void show_item(void *data);
void mini_traverse(mini_context* c, const mini_value* v);
#endif //_MINI_JSON_H__
