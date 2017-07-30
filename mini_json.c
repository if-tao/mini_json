#include "mini_json.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
<<<<<<< HEAD
#include <stdlib.h>  /* NULL, strtod() */
=======
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */

#ifndef MINI_PARSE_STACK_INIT_SIZE
#define MINI_PARSE_STACK_INIT_SIZE 256
#endif
>>>>>>> develop

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
<<<<<<< HEAD

typedef struct {
    const char* json;
}mini_context;

=======
#define PUTC(c, ch)         do { *(char*)mini_context_push(c, sizeof(char)) = (ch); } while(0)

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
}mini_context;

static void* mini_context_push(mini_context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            c->size = MINI_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1;  /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* mini_context_pop(mini_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

>>>>>>> develop
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
<<<<<<< HEAD
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

=======
    if (*p == '-') p++;
    if (*p == '0') p++;
    else {
        if (!ISDIGIT1TO9(*p)) return MINI_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return MINI_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return MINI_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    errno = 0;
    v->u.n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return MINI_PARSE_NUMBER_TOO_BIG;
    v->type = MINI_NUMBER;
    c->json = p;
    return MINI_PARSE_OK;
}

static int mini_parse_string(mini_context* c, mini_value* v) {
    size_t head = c->top, len;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                len = c->top - head;
                mini_set_string(v, (const char*)mini_context_pop(c, len), len);
                c->json = p;
                return MINI_PARSE_OK;
            case '\\':
                switch (*p++) {
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/':  PUTC(c, '/' ); break;
                    case 'b':  PUTC(c, '\b'); break;
                    case 'f':  PUTC(c, '\f'); break;
                    case 'n':  PUTC(c, '\n'); break;
                    case 'r':  PUTC(c, '\r'); break;
                    case 't':  PUTC(c, '\t'); break;
                    default:
                        c->top = head;
                        return MINI_PARSE_INVALID_STRING_ESCAPE;
                }
                break;
            case '\0':
                c->top = head;
                return MINI_PARSE_MISS_QUOTATION_MARK;
            default:
                if ((unsigned char)ch < 0x20) { 
                    c->top = head;
                    return MINI_PARSE_INVALID_STRING_CHAR;
                }
                PUTC(c, ch);
        }
    }
}

>>>>>>> develop
static int mini_parse_value(mini_context* c, mini_value* v) {
    switch (*c->json) {
        case 't':  return mini_parse_literal(c, v, "true", MINI_TRUE);
        case 'f':  return mini_parse_literal(c, v, "false", MINI_FALSE);
        case 'n':  return mini_parse_literal(c, v, "null", MINI_NULL);
        default:   return mini_parse_number(c, v);
<<<<<<< HEAD
=======
        case '"':  return mini_parse_string(c, v);
>>>>>>> develop
        case '\0': return MINI_PARSE_EXPECT_VALUE;
    }
}

int mini_parse(mini_value* v, const char* json) {
    mini_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
<<<<<<< HEAD
    v->type = MINI_NULL;
=======
    c.stack = NULL;
    c.size = c.top = 0;
    mini_init(v);
>>>>>>> develop
    mini_parse_whitespace(&c);
    if ((ret = mini_parse_value(&c, v)) == MINI_PARSE_OK) {
        mini_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = MINI_NULL;
            ret = MINI_PARSE_ROOT_NOT_SINGULAR;
        }
    }
<<<<<<< HEAD
    return ret;
}

=======
    assert(c.top == 0);
    free(c.stack);
    return ret;
}

void mini_free(mini_value* v) {
    assert(v != NULL);
    if (v->type == MINI_STRING)
        free(v->u.s.s);
    v->type = MINI_NULL;
}

>>>>>>> develop
mini_type mini_get_type(const mini_value* v) {
    assert(v != NULL);
    return v->type;
}

<<<<<<< HEAD
double mini_get_number(const mini_value* v) {
    assert(v != NULL && v->type == MINI_NUMBER);
    return v->n;
=======
int mini_get_boolean(const mini_value* v) {
    assert(v != NULL && (v->type == MINI_TRUE || v->type == MINI_FALSE));
    return v->type == MINI_TRUE;
}

void mini_set_boolean(mini_value* v, int b) {
    mini_free(v);
    v->type = b ? MINI_TRUE : MINI_FALSE;
}

double mini_get_number(const mini_value* v) {
    assert(v != NULL && v->type == MINI_NUMBER);
    return v->u.n;
}

void mini_set_number(mini_value* v, double n) {
    mini_free(v);
    v->u.n = n;
    v->type = MINI_NUMBER;
}

const char* mini_get_string(const mini_value* v) {
    assert(v != NULL && v->type == MINI_STRING);
    return v->u.s.s;
}

size_t mini_get_string_length(const mini_value* v) {
    assert(v != NULL && v->type == MINI_STRING);
    return v->u.s.len;
}

void mini_set_string(mini_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    mini_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = MINI_STRING;
>>>>>>> develop
}
