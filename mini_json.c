#include <assert.h>
#include <stdlib.h>
#include "mini_json.h"


int mini_parse(mini_value *v,const char *json){
    mini_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = MINI_NULL;
    mini_parse_whitespace(&c);
    if((ret == mini_parse_value(&c, v)) == MINI_PARSE_OK) {
	    mini_parse_whitespace(&c);
	    if(*c.json != '\0')
	    	ret = MINI_PARSE_ROOT_NOT_SINGULAR;
    }
    return ret;
}

mini_type mini_get_type(const mini_value *v) {
	assert(v != NULL);
	return v->type;
}

static void mini_parse_whitespace(mini_context* c){
    const char *p = c->json;
    while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int mini_parse_null(mini_context* c, mini_value* v){
   EXPECT(c,'n');
   if(c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
       return MINI_PARSE_INVALID_VALUE;
   c->json += 3;
   v->type = MINI_NULL;
   return MINI_PARSE_OK;
}

static int mini_parse_false(mini_context* c, mini_value* v){
   EXPECT(c,'f');
   if(c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
       return MINI_PARSE_INVALID_VALUE;
   c->json += 4;
   v->type = MINI_FALSE;
   return MINI_PARSE_OK;
}

static int mini_parse_true(mini_context* c, mini_value* v){
   EXPECT(c,'t');
   if(c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
       return MINI_PARSE_INVALID_VALUE;
   c->json += 3;
   v->type = MINI_TRUE;
   return MINI_PARSE_OK;
}

static int mini_parse_value(mini_context* c,mini_value* v){
    switch(*c->json){
	    case 't' : return mini_parse_true(c, v);
	    case 'f' : return mini_parse_false(c, v);
        case 'n' : return mini_parse_null(c, v);
        case '\0' : return MINI_PARSE_EXPECT_VALUE;
        defalult : return MINI_PARSE_INVALID_VALUE;
    }
}
