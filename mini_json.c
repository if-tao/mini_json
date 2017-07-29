#include "mini_json.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, strtod() */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
}mini_context;

static void mini_parse_whitespace(mini_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int mini_parse_literal(mini_context* c, mini_value* v, const char* literal, mini_type type) {
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; literal[i + 1]; i++)
        if (c->json[i] != literal[i + 1])
            return MINI_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return MINI_PARSE_OK;
}

static int mini_parse_number(mini_context* c, mini_value* v) {
    const char* p = c->json;
    if(*p == '-') ++p;
    if(*p == '0') ++p;
    else{
        if(!ISDIGIT1TO9(*p)) return MINI_PARSE_INVALID_VALUE;
        for(++p; ISDIGIT(*p); ++p);
    }
    if(*p == '.'){
        ++p;
        if(!ISDIGIT(*p)) return MINI_PARSE_INVALID_VALUE;
        for(++p; ISDIGIT(*p); ++p);
    }
    if(*p == 'e' || *p == 'E'){
        ++p;
        if(*p == '+' || *p == '-') ++p;
        if(!ISDIGIT(*p)) return MINI_PARSE_INVALID_VALUE;
        for(++p; ISDIGIT(*p); ++p);
    }

    errno = 0;
    v->n = strtod(c->json, NULL);
    if(errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
        return MINI_PARSE_NUMBER_TOO_BIG;
    c->json = p;
    v->type = MINI_NUMBER;
    return MINI_PARSE_OK;
}

static int mini_parse_value(mini_context* c, mini_value* v) {
    switch (*c->json) {
        case 't':  return mini_parse_literal(c, v, "true", MINI_TRUE);
        case 'f':  return mini_parse_literal(c, v, "false", MINI_FALSE);
        case 'n':  return mini_parse_literal(c, v, "null", MINI_NULL);
        default:   return mini_parse_number(c, v);
        case '\0': return MINI_PARSE_EXPECT_VALUE;
    }
}

int mini_parse(mini_value* v, const char* json) {
    mini_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = MINI_NULL;
    mini_parse_whitespace(&c);
    if ((ret = mini_parse_value(&c, v)) == MINI_PARSE_OK) {
        mini_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = MINI_NULL;
            ret = MINI_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

mini_type mini_get_type(const mini_value* v) {
    assert(v != NULL);
    return v->type;
}

double mini_get_number(const mini_value* v) {
    assert(v != NULL && v->type == MINI_NUMBER);
    return v->n;
}
