#ifndef _MINI_JSON_H__
#define _MINI_JSON_H__

//判断c的第一个字母是否是ch
#define EXPECT(c, ch) do { assert(*c->json == (ch)); c->json++; } while(0)

 typedef enum{
    MINI_NULL,
    MINI_TRUE,
    MINI_FALSE,
    MINI_NUMBER,
    MINI_STRING,
    MINI_ARRAY,
    MINI_OBJECT
} mini_type;

typedef struct{
    mini_type type;
}mini_value;

enum {
    MINI_PARSE_OK = 0,
    MINI_PARSE_EXPECT_VALUE,        //若一个 JSON 只含有空白
    MINI_PARSE_INVALID_VALUE,       //若值不是那三种字面值
    MINI_PARSE_ROOT_NOT_SINGULAR    //若一个值之后，在空白之后还有其他字符
};

typedef struct{
    const char *json;
}mini_context;

int mini_parse(mini_value *v,const char *json);

mini_type mini_get_type(const mini_value *v);

static void mini_parse_whitespace(mini_context* c);
static int mini_parse_null(mini_context* c, mini_value* v);
static int mini_parse_false(mini_context* c, mini_value* v);
static int mini_parse_true(mini_context* c, mini_value* v);
static int mini_parse_value(mini_context* c, mini_value* v);

#endif //_MINI_JSON_H__
