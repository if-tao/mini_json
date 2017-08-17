#include "mini_json.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h>  /* memcpy() */

#ifndef MINI_PARSE_STACK_INIT_SIZE
#define MINI_PARSE_STACK_INIT_SIZE 256
#endif

#ifndef MINI_PARSE_BUILDER_INIT_SIZE
#define MINI_PARSE_BUILDER_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)         do { *(char*)mini_context_push(c, sizeof(char)) = (ch); } while(0)
#define PUTS(c, s, len)     memcpy(mini_context_push(c,len), s, len)

void mini_show_value(const mini_value* v) {
    assert(v != NULL);
    size_t i;
    switch(v->type){
        case MINI_NULL : printf("null"); break;
        case MINI_TRUE : printf("true"); break;
        case MINI_FALSE : printf("false"); break;
        case MINI_NUMBER : printf("%f", mini_get_number(v)); break;
        case MINI_STRING : printf("\"%s\"", mini_get_string(v)); break;
        case MINI_ARRAY : 
                printf("[ ");
                size_t size = mini_get_array_size(v);
                for(i = 0; i < size; ++i) {
                    if(i > 0) printf(" , ");
                    mini_show_value(mini_get_array_element(v, i));
                }
                printf(" ]");
                break;
        case MINI_OBJECT : 
                printf("{ ");
                if(get_map(v) != NULL)
                    map_show(get_map(v), show_item);
                printf(" }");
                break;
    }
}

void mini_add_value_to_array(mini_value* arr, mini_value* v) {
    assert(arr != NULL && v != NULL && arr->type == MINI_ARRAY);
    mini_value* tmp = mini_backup(v);
    size_t size = mini_get_array_size(arr);
    if(size == 0){
        arr->u.a.e = tmp;
    }
    else{
        arr->u.a.e = (mini_value*)realloc(arr->u.a.e, sizeof(mini_value) * (size+1));
        memcpy(arr->u.a.e+size, tmp, sizeof(mini_value));
        free((void*)tmp);
    }
    arr->u.a.size += 1;
}

void mini_add_value_to_object(mini_value* obj, mini_value* key, mini_value* val) {
    assert(obj != NULL && obj->type == MINI_OBJECT && key != NULL && val != NULL);
    if(obj->u.o.pmap == NULL){
        mini_init_object(obj);
    }
    mini_value *mkey = mini_backup(key);
    mini_value *mval = mini_backup(val);
    Item* pitem = new_item(mini_get_string(mkey), mval);
    add_item(obj->u.o.pmap, pitem);
    free(mkey);
    free(mval);
    obj->u.o.size += 1;
}

// for deep copy
mini_value* mini_backup(mini_value* v){
    char *tmp_json = NULL;
    mini_value* ret = NULL;
    size_t len;
    if(MINI_GENERATE_OK == mini_generate(v, &tmp_json, &len)){
        ret = (mini_value*)malloc(sizeof(mini_value));
        if(MINI_PARSE_OK != mini_parse(ret, tmp_json)) {
            free(ret);
            ret = NULL;
        }
    }
    if(tmp_json != NULL) free(tmp_json);
    return ret;
}

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
    for(i = 0; i<4; ++i){
        char ch = *p++;
        *u <<= 4;
        if      (ch >= '0' && ch <= '9') *u |= ch - '0';
        else if (ch >= 'a' && ch <= 'f') *u |= ch - ('a' - 10);
        else if (ch >= 'A' && ch <= 'F') *u |= ch - ('A' - 10);
        else    return NULL;
    }
    return p;
}

static void mini_encode_utf8(mini_context* c, unsigned int u){
    if(u <= 0x7F)
        PUTC(c, u & 0xFF);
    else if(u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
        PUTC(c, 0x80 | ( u       & 0x3F));
    }
    else if(u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >> 6)  & 0x3F));
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
static int mini_parse_string_raw(mini_context* c, char** str, size_t* len) {
    size_t head = c->top;
    unsigned int u, u2;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                *len = c->top - head;
                *str = mini_context_pop(c, *len);
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
                        if(!(p = mini_parse_hex4(p, &u)))
                           STRING_ERROR(MINI_PARSE_INVALID_UNICODE_HEX);
                        if(u >= 0xD800 && u <= 0xDBFF) {
                            if(*p++ != '\\')
                                STRING_ERROR(MINI_PARSE_INVALID_UNICODE_SURROGATE);
                            if(*p++ != 'u')
                                STRING_ERROR(MINI_PARSE_INVALID_UNICODE_SURROGATE);
                            if(!(p = mini_parse_hex4(p, &u2)))
                                STRING_ERROR(MINI_PARSE_INVALID_UNICODE_HEX);
                            if(u2 < 0xDC00 || u2 > 0xDFFF)
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
                if ((unsigned char)ch < 0x20) { 
                    STRING_ERROR(MINI_PARSE_INVALID_STRING_CHAR);
                }
                PUTC(c, ch);
        }
    }
}

static int mini_parse_string(mini_context* c, mini_value* v){
    int ret;
    char* s;
    size_t len;
    if((ret = mini_parse_string_raw(c, &s, &len)) == MINI_PARSE_OK)
        mini_set_string(v, s, len);
    return ret;
}

static int mini_parse_value(mini_context* c, mini_value* v);/* forward declare */

static int mini_parse_array(mini_context* c, mini_value* v) {
    size_t size = 0;
    size_t i;
    int ret;
    EXPECT(c, '[');
    mini_parse_whitespace(c);
    if(*c->json == ']') {
        c->json++;
        v->type = MINI_ARRAY;
        v->u.a.size = 0;
        v->u.a.e = NULL;
        return MINI_PARSE_OK;
    }
    for(;;) {
        mini_value e;
        mini_init(&e);
        if((ret = mini_parse_value(c, &e)) != MINI_PARSE_OK) break;
        memcpy(mini_context_push(c, sizeof(mini_value)), &e, sizeof(mini_value));
        size++;
        mini_parse_whitespace(c);
        if(*c->json == ',') {
            c->json++;
            mini_parse_whitespace(c);
        }
        else if(*c->json == ']') {
            c->json++;
            v->type = MINI_ARRAY;
            v->u.a.size = size;
            size = size * sizeof(mini_value);
            memcpy(v->u.a.e = (mini_value*)malloc(size), mini_context_pop(c, size), size);
            return MINI_PARSE_OK;
        }
        else {
            ret = MINI_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }
    for(i = 0; i < size; i++) {
        mini_free((mini_value*)mini_context_pop(c, sizeof(mini_value)));
    }
    return ret;
}

static int mini_parse_object(mini_context* c, mini_value* v) {
    size_t i,size;
    mini_value key;
    mini_value value;
    int ret;

    EXPECT(c, '{');
    mini_parse_whitespace(c);
    if(*c->json == '}') {
        c->json++;
        v->type = MINI_OBJECT;
        v->u.o.pmap = NULL;
        v->u.o.size = 0;
        return MINI_PARSE_OK;
    }
    v->u.o.pmap = (Map *)malloc(sizeof(Map));
    *(v->u.o.pmap) = map();
    v->type = MINI_OBJECT;
    size = 0;
    for(;;){
        mini_init(&key);
        mini_init(&value);
        /* parse key */
        if(*c->json != '\"'){
            ret = MINI_PARSE_MISS_KEY;
            break;
        }
        if((ret = mini_parse_string(c, &key)) != MINI_PARSE_OK) 
            break;
        /* parse ws colon ws */
        mini_parse_whitespace(c);
        if(*c->json != ':'){
            ret = MINI_PARSE_MISS_COLON;
            break;
        }
        c->json++;
        mini_parse_whitespace(c);
        /* parse value */
        if((ret = mini_parse_value(c, &value)) != MINI_PARSE_OK)
            break;
        Item *pitem = new_item(key.u.s.s, &value);
        add_item(v->u.o.pmap, pitem);
        size++;
        mini_free(&key); //free the memory
        /* parse ws [comma / right-curly-brae] ws */
        mini_parse_whitespace(c);
        if(*c->json == ','){
            c->json++;
            mini_parse_whitespace(c);
        }
        else if(*c->json == '}') {
            c->json++;
            v->u.o.size = size;
            return MINI_PARSE_OK;
        }
        else{
            ret = MINI_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }
    mini_free(&key);
    mini_free(&value);
    mini_free(v);
    v->type = MINI_NULL;
    return ret;
}

static int mini_parse_value(mini_context* c, mini_value* v) {
    switch (*c->json) {
        case 't':  return mini_parse_literal(c, v, "true", MINI_TRUE);
        case 'f':  return mini_parse_literal(c, v, "false", MINI_FALSE);
        case 'n':  return mini_parse_literal(c, v, "null", MINI_NULL);
        case '"':  return mini_parse_string(c, v);
        case '[':  return mini_parse_array(c, v);
        case '{':  return mini_parse_object(c, v);
        case '\0': return MINI_PARSE_EXPECT_VALUE;
        default:   return mini_parse_number(c, v);
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
    size_t i;
    assert(v != NULL);
    switch(v->type) {
        case MINI_STRING:
            free(v->u.s.s);
            break;
        case MINI_ARRAY:
            for(i = 0; i < v->u.a.size; i++)
                mini_free(&v->u.a.e[i]);
            free(v->u.a.e);
            break;
        case MINI_OBJECT:
            if(v->u.o.pmap == NULL) break;
            map_clear(v->u.o.pmap, inner_clear);
            free(v->u.o.pmap);
            break;
        default:
            break;
    }
    v->type = MINI_NULL;
}

mini_type mini_get_type(const mini_value* v) {
    assert(v != NULL);
    return v->type;
}

void mini_set_type(mini_value* v, mini_type type) {
    assert(v != NULL);
    v->type = type;
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

void mini_init_array(mini_value* v) {
    assert(v != NULL);
    v->type = MINI_ARRAY;
    v->u.a.size = 0;
}

size_t mini_get_array_size(const mini_value* v) {
    assert(v != NULL && v->type == MINI_ARRAY);
    return v->u.a.size;
}

mini_value* mini_get_array_element(const mini_value* v, size_t index) {
    assert(v != NULL && v->type == MINI_ARRAY);
    assert(index < v->u.a.size);
    return &v->u.a.e[index];
}

void mini_init_object(mini_value* v) {
    assert(v != NULL);
    v->type = MINI_OBJECT;
    v->u.o.pmap = (Map*)malloc(sizeof(Map));
    *(v->u.o.pmap) = map();
}

size_t mini_get_object_size(const mini_value* v) {
    assert(v != NULL && v->type == MINI_OBJECT);
    return v->u.o.size;
}

mini_value* mini_get_object_value(const mini_value* v, const char* key) {
    assert(v != NULL && v->type == MINI_OBJECT);
    return (mini_value*)value(v->u.o.pmap, key);
}

static void mini_generate_string(mini_context* c, const char* s, size_t len) {
    static const char hex_digits[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    size_t i, size;
    char* head, *p;
    assert(s != NULL);
    p = head = mini_context_push(c, size = len * 6 + 2); /* "\u00xx ... " */
    *p++ = '"';
    for(i = 0; i < len; i++){
        unsigned char ch = (unsigned char)s[i];
        switch(ch) {
            case '\"' : *p++ = '\\', *p++ = '\"';break;
            case '\\' : *p++ = '\\'; *p++ = '\\'; break;
            case '\b' : *p++ = '\\'; *p++ = 'b';  break;
            case '\f' : *p++ = '\\'; *p++ = 'f';  break;
            case '\n' : *p++ = '\\'; *p++ = 'n';  break;
            case '\r' : *p++ = '\\'; *p++ = 'r';  break;
            case '\t' : *p++ = '\\'; *p++ = 't';  break;
            default :
                if( ch < 0x20) {
                    *p++ = '\\'; *p++ = 'u'; *p++ = '0'; *p++ = '0';
                    *p++ = hex_digits[ch >> 4];
                    *p++ = hex_digits[ch & 15];
                }
                else
                    *p++ = s[i];
        }
    }
    *p++ = '"';
    c->top -= size - (p - head);
}

static void mini_generate_value(mini_context* c, const mini_value* v) {
    size_t i;
    size_t len;
    switch(v->type) {
        case MINI_NULL : PUTS(c, "null", 4);break;
        case MINI_TRUE : PUTS(c, "true", 4);break;
        case MINI_FALSE : PUTS(c, "false", 5);break;
        case MINI_NUMBER ://使用sprintf("%.17g",...)来把浮点数转换成文本
                len = sprintf(mini_context_push(c, 32), "%.17g", v->u.n);
                c->top -= 32 - len;
                break;
        case MINI_STRING : mini_generate_string(c, v->u.s.s, v->u.s.len); break;
        case MINI_ARRAY :
                len = mini_get_array_size(v);
                PUTC(c, '[');
                for(i = 0; i < len; ++i){
                    if(i > 0) PUTC(c, ',');
                    mini_generate_value(c, &v->u.a.e[i]);
                }
                PUTC(c, ']');
                break;
        case MINI_OBJECT :
                PUTC(c, '{');
                if(get_map(v) != NULL) //don`t hava {key:value}
                    //mini_traverse_map_for_object(c, v);
                    mini_traverse(c, v);
                PUTC(c, '}');
                break;
    }
}

int mini_generate(const mini_value* v, char** json, size_t* length) {
    mini_context c;
    size_t ret;
    assert(v != NULL);
    assert(json != NULL);
    c.stack = (char*)malloc(c.size = MINI_PARSE_BUILDER_INIT_SIZE);
    c.top = 0;
    mini_generate_value(&c, v);
    if(length)
        *length = c.top;
    PUTC(&c, '\0');
    *json = c.stack;
    return MINI_GENERATE_OK;
}

Map* get_map(const mini_value* v) {
    assert(v != NULL && v->type == MINI_OBJECT);
    return v->u.o.pmap;
}
Item* new_item(const char* key, void *value) {
    Item *p = (Item*)lalloc(sizeof(Item), 1);
    p->key = (char*)lalloc(sizeof(char), strlen(key)+1);
    strcpy(p->key, key);
    p->value = lalloc(sizeof(mini_value), 1);
    memcpy(p->value, (mini_value*)value, sizeof(mini_value));
    return p;
}

void inner_clear(void* p) {
    Item* pitem = (Item *)p;
    mini_free(pitem->value);
    lfree(pitem->key);
    lfree(pitem->value);
    lfree(pitem);
}

void show_item(void *data) {
    Item* pitem = (Item *)data;
    mini_value *value = (mini_value*)pitem->value;
    printf("%s : ",pitem->key);
    mini_show_value(value);
    printf("\n");
}

void traverse_map_to_do(mini_context* c, void *data, size_t* i) {
    if(*i > 0) PUTC(c, ',');
    Item *pitem = (Item*)data;
    mini_value* val = (mini_value*)pitem->value;
    mini_generate_string(c, pitem->key, strlen(pitem->key));
    PUTC(c, ':');
    mini_generate_value(c, val);
    *i += 1;
}

void _traverse(Node* p, Node* tail, mini_context* c, size_t* i, void(*func)(mini_context* , void *, size_t*)) {
    if(p->left != tail) _traverse(p->left, tail, c, i, func);
    func(c, p->data, i);
    if(p->right != tail) _traverse(p->right, tail, c, i, func);
}

void mini_traverse(mini_context* c, const mini_value* v){
    Map* pmap = get_map(v);
    size_t i = 0;
    _traverse(pmap->tree->root, pmap->tree->tail, c, &i, traverse_map_to_do);
}

