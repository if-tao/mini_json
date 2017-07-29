#ifndef _MINI_JSON_H__
#define _MINI_JSON_H__

 typedef enum{
    MINI_NULL,
    MINI_TRUE,
    MINI_FALSE,
    MINI_NUMBER,
    MINI_STRING,
    MINI_ARRAY,
    MINI_OBJECT
} mini_type;

typedef struct {
	double n;
    mini_type type;
}mini_value;

enum {
    MINI_PARSE_OK = 0,
    MINI_PARSE_EXPECT_VALUE,
    MINI_PARSE_INVALID_VALUE,
    MINI_PARSE_ROOT_NOT_SINGULAR,
    MINI_PARSE_NUMBER_TOO_BIG
};

int mini_parse(mini_value* v, const char* json);

mini_type mini_get_type(const mini_value* v);

double mini_get_number(const mini_value* v);

#endif //_MINI_JSON_H__
