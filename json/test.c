#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mini_json.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")


static void test_parse_null() {
    mini_value v;
    mini_init(&v);
    mini_set_boolean(&v, 0);
    EXPECT_EQ_INT(MINI_PARSE_OK, mini_parse(&v, "null"));
    EXPECT_EQ_INT(MINI_NULL, mini_get_type(&v));
    mini_free(&v);
}

static void test_parse_true() {
    mini_value v;
    mini_init(&v);
    mini_set_boolean(&v, 0);
    EXPECT_EQ_INT(MINI_PARSE_OK, mini_parse(&v, "true"));
    EXPECT_EQ_INT(MINI_TRUE, mini_get_type(&v));
    mini_free(&v);
}

static void test_parse_false() {
    mini_value v;
    mini_init(&v);
    mini_set_boolean(&v, 1);
    EXPECT_EQ_INT(MINI_PARSE_OK, mini_parse(&v, "false"));
    EXPECT_EQ_INT(MINI_FALSE, mini_get_type(&v));
    mini_free(&v);
}

#define TEST_NUMBER(expect, json)\
    do {\
        mini_value v;\
        mini_init(&v);\
        EXPECT_EQ_INT(MINI_PARSE_OK, mini_parse(&v, json));\
        EXPECT_EQ_INT(MINI_NUMBER, mini_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, mini_get_number(&v));\
        mini_free(&v);\
    } while(0)

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define TEST_STRING(expect, json)\
    do {\
        mini_value v;\
        mini_init(&v);\
        EXPECT_EQ_INT(MINI_PARSE_OK, mini_parse(&v, json));\
        EXPECT_EQ_INT(MINI_STRING, mini_get_type(&v));\
        EXPECT_EQ_STRING(expect, mini_get_string(&v), mini_get_string_length(&v));\
        mini_free(&v);\
    } while(0)

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
    TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

#define TEST_ERROR(error, json)\
    do {\
        mini_value v;\
        mini_init(&v);\
        v.type = MINI_FALSE;\
        EXPECT_EQ_INT(error, mini_parse(&v, json));\
        EXPECT_EQ_INT(MINI_NULL, mini_get_type(&v));\
        mini_free(&v);\
    } while(0)

static void test_parse_expect_value() {
    TEST_ERROR(MINI_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(MINI_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(MINI_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(MINI_PARSE_INVALID_VALUE, "?");

    /* invalid number */
    TEST_ERROR(MINI_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(MINI_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(MINI_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(MINI_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(MINI_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(MINI_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(MINI_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(MINI_PARSE_INVALID_VALUE, "nan");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(MINI_PARSE_ROOT_NOT_SINGULAR, "null x");

    /* invalid number */
    TEST_ERROR(MINI_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
    TEST_ERROR(MINI_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(MINI_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {
    TEST_ERROR(MINI_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(MINI_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_missing_quotation_mark() {
    TEST_ERROR(MINI_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(MINI_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(MINI_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(MINI_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(MINI_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(MINI_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(MINI_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(MINI_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_HEX, "\"\\u 123\"");
}

static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(MINI_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_array() {
    size_t i,j;
    mini_value v;
    mini_init(&v);
    EXPECT_EQ_INT(MINI_PARSE_OK, mini_parse(&v, "[ ]"));
    EXPECT_EQ_INT(MINI_ARRAY, mini_get_type(&v));
    EXPECT_EQ_SIZE_T(0, mini_get_array_size(&v));
    mini_free(&v);

    mini_init(&v);
    EXPECT_EQ_INT(MINI_PARSE_OK, mini_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(MINI_ARRAY, mini_get_type(&v));
    EXPECT_EQ_SIZE_T(5, mini_get_array_size(&v));
    EXPECT_EQ_INT(MINI_NULL,   mini_get_type(mini_get_array_element(&v, 0)));
    EXPECT_EQ_INT(MINI_FALSE,  mini_get_type(mini_get_array_element(&v, 1)));
    EXPECT_EQ_INT(MINI_TRUE,   mini_get_type(mini_get_array_element(&v, 2)));
    EXPECT_EQ_INT(MINI_NUMBER, mini_get_type(mini_get_array_element(&v, 3)));
    EXPECT_EQ_INT(MINI_STRING, mini_get_type(mini_get_array_element(&v, 4)));
    EXPECT_EQ_DOUBLE(123.0, mini_get_number(mini_get_array_element(&v, 3)));
    EXPECT_EQ_STRING("abc", mini_get_string(mini_get_array_element(&v, 4)), mini_get_string_length(mini_get_array_element(&v, 4)));
    mini_free(&v);

    mini_init(&v);
    EXPECT_EQ_INT(MINI_PARSE_OK, mini_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(MINI_ARRAY, mini_get_type(&v));
    EXPECT_EQ_SIZE_T(4, mini_get_array_size(&v));
    for(i = 0; i < 4; i++) {
        mini_value* a = mini_get_array_element(&v, i);
        EXPECT_EQ_INT(MINI_ARRAY, mini_get_type(a));
        EXPECT_EQ_SIZE_T(i, mini_get_array_size(a));
        for(j = 0; j < i; j++){
            mini_value* e = mini_get_array_element(a, j);
            EXPECT_EQ_INT(MINI_NUMBER, mini_get_type(e));
            EXPECT_EQ_DOUBLE((double)j, mini_get_number(e));
        }
    }
    mini_free(&v);
}

static void test_parse_object() {
    mini_value v;
    size_t i;

    mini_init(&v);
    EXPECT_EQ_INT(MINI_PARSE_OK, mini_parse(&v, " { } "));
    EXPECT_EQ_INT(MINI_OBJECT, mini_get_type(&v));
    EXPECT_EQ_SIZE_T(0, mini_get_object_size(&v));
    mini_free(&v);

    mini_init(&v);
    EXPECT_EQ_INT(MINI_PARSE_OK, mini_parse(&v,
                " { "
                "\"n\" : null , "
                "\"f\" : false , "
                "\"t\" : true , "
                "\"i\" : 123 , "
                "\"s\" : \"abc\" , "
                "\"a\" : [ 1, 2, 3 ], "
                "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
                " } "
                ));
    EXPECT_EQ_INT(MINI_OBJECT, mini_get_type(&v));
    EXPECT_EQ_SIZE_T(7, mini_get_object_size(&v));
    EXPECT_EQ_INT(MINI_NULL, mini_get_type(mini_get_object_value(&v, "n")));
    EXPECT_EQ_INT(MINI_TRUE, mini_get_type(mini_get_object_value(&v, "t")));
    EXPECT_EQ_INT(MINI_FALSE, mini_get_type(mini_get_object_value(&v, "f")));
    EXPECT_EQ_INT(MINI_NUMBER, mini_get_type(mini_get_object_value(&v, "i")));
    EXPECT_EQ_DOUBLE(123.0, mini_get_number(mini_get_object_value(&v, "i")));
    EXPECT_EQ_INT(MINI_STRING, mini_get_type(mini_get_object_value(&v, "s")));
    EXPECT_EQ_STRING("abc", mini_get_string(mini_get_object_value(&v, "s")), mini_get_string_length(mini_get_object_value(&v, "s")));
    EXPECT_EQ_INT(MINI_ARRAY, mini_get_type(mini_get_object_value(&v, "a")));
    EXPECT_EQ_SIZE_T(3, mini_get_array_size(mini_get_object_value(&v, "a")));
    for (i = 0; i < 3; i++) {
        mini_value* e = mini_get_array_element(mini_get_object_value(&v, "a"), i);
        EXPECT_EQ_INT(MINI_NUMBER, mini_get_type(e));
        EXPECT_EQ_DOUBLE(i + 1.0, mini_get_number(e));
    }

    mini_value* o = mini_get_object_value(&v, "o");
    EXPECT_EQ_INT(MINI_OBJECT, mini_get_type(o));
    EXPECT_EQ_SIZE_T(3, mini_get_object_size(o));
    /*
    for(i = 0; i < 3; i++) {
        mini_value* ov = mini_get_object_value(o, "1");
        EXPECT_EQ_INT(MINI_NUMBER, mini_get_type(ov));
        EXPECT_EQ_DOUBLE(i + 1.0, mini_get_number(ov));
    }
    */
    mini_free(&v);
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();
    test_parse_array();
    test_parse_object();
}

static void test_access_null() {
    mini_value v;
    mini_init(&v);
    mini_set_string(&v, "a", 1);
    mini_set_null(&v);
    EXPECT_EQ_INT(MINI_NULL, mini_get_type(&v));
    mini_free(&v);
}

static void test_access_boolean() {
    mini_value v;
    mini_init(&v);
    mini_set_string(&v, "a", 1);
    mini_set_boolean(&v, 1);
    EXPECT_TRUE(mini_get_boolean(&v));
    mini_set_boolean(&v, 0);
    EXPECT_FALSE(mini_get_boolean(&v));
    mini_free(&v);
}

static void test_access_number() {
    mini_value v;
    mini_init(&v);
    mini_set_string(&v, "a", 1);
    mini_set_number(&v, 1234.5);
    EXPECT_EQ_DOUBLE(1234.5, mini_get_number(&v));
    mini_free(&v);
}

static void test_access_string() {
    mini_value v;
    mini_init(&v);
    mini_set_string(&v, "", 0);
    EXPECT_EQ_STRING("", mini_get_string(&v), mini_get_string_length(&v));
    mini_set_string(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", mini_get_string(&v), mini_get_string_length(&v));
    mini_free(&v);
}

static void test_access() {
    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
}

int main() {
    test_parse();
    test_access();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
