#include "mini_json.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */

#ifndef MINI_PARSE_STACK_INIT_SIZE
#define MINI_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
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

static const char* mini_parse_hex4(const char* p, unsigned int* u) {
    int i;
    *u = 0;
    for (i = 0; i < 4; i++) {
        char ch = *p++;
        *u <<= 4;
        if      (ch >= '0' && ch <= '9')  *u |= ch - '0';
        else if (ch >= 'A' && ch <= 'F')  *u |= ch - ('A' - 10);
        else if (ch >= 'a' && ch <= 'f')  *u |= ch - ('a' - 10);
        else return NULL;
    }
    return p;
}

static void mini_encode_utf8(mini_context* c, unsigned int u) {
    if (u <= 0x7F) 
        PUTC(c, u & 0xFF);
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
        PUTC(c, 0x80 | ( u       & 0x3F));
    }
    else if (u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
    else {
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)

static int mini_parse_string(mini_context* c, mini_value* v) {
    size_t head = c->top, len;
    unsigned int u, u2;
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
                    case 'u':
                        if (!(p = mini_parse_hex4(p, &u)))
                            STRING_ERROR(MINI_PARSE_INVALID_UNICODE_HEX);
                        if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
                            if (*p++ != '\\')
                                STRING_ERROR(MINI_PARSE_INVALID_UNICODE_SURROGATE);
                            if (*p++ != 'u')
                                STRING_ERROR(MINI_PARSE_INVALID_UNICODE_SURROGATE);
                            if (!(p = mini_parse_hex4(p, &u2)))
                                STRING_ERROR(MINI_PARSE_INVALID_UNICODE_HEX);
                            if (u2 < 0xDC00 || u2 > 0xDFFF)
                                STRING_ERROR(MINI_PARSE_INVALID_UNICODE_SURROGATE);
                            u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                        }
                        mini_encode_utf8(c, u);
                        break;
                    default:
                        STRING_ERROR(MINI_PARSE_INVALID_STRING_ESCAPE);
                }
                break;
            case '\0':
                STRING_ERROR(MINI_PARSE_MISS_QUOTATION_MARK);
            default:
                if ((unsigned char)ch < 0x20)
                    STRING_ERROR(MINI_PARSE_INVALID_STRING_CHAR);
                PUTC(c, ch);
        }
    }
}

static int mini_parse_value(mini_context* c, mini_value* v) {
    switch (*c->json) {
        case 't':  return mini_parse_literal(c, v, "true", MINI_TRUE);
        case 'f':  return mini_parse_literal(c, v, "false", MINI_FALSE);
        case 'n':  return mini_parse_literal(c, v, "null", MINI_NULL);
        default:   return mini_parse_number(c, v);
        case '"':  return mini_parse_string(c, v);
        case '\0': return MINI_PARSE_EXPECT_VALUE;
    }
}

int mini_parse(mini_value* v, const char* json) {
    mini_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    mini_init(v);
    mini_parse_whitespace(&c);
    if ((ret = mini_parse_value(&c, v)) == MINI_PARSE_OK) {
        mini_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = MINI_NULL;
            ret = MINI_PARSE_ROOT_NOT_SINGULAR;
        }
    }
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

mini_type mini_get_type(const mini_value* v) {
    assert(v != NULL);
    return v->type;
}

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
}
