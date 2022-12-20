#pragma once

#define RUBI_READONLY 1
#define RUBI_WRITEONLY 2
#define RUBI_READWRITE 3

#define _RUBI_TYPECODES_void 1
#define _RUBI_TYPECODES_int32_t 2
#define _RUBI_TYPECODES_int16_t 3
#define _RUBI_TYPECODES_int8_t 4
#define _RUBI_TYPECODES_uint32_t 5
#define _RUBI_TYPECODES_uint16_t 6
#define _RUBI_TYPECODES_uint8_t 7
#define _RUBI_TYPECODES_float 8
#define _RUBI_TYPECODES_shortstring 9
#define _RUBI_TYPECODES_longstring 10
#define _RUBI_TYPECODES_bool 11
#define _RUBI_TYPECODES_RUBI_ENUM1 12

#define _RUBI_IS_VOID_void(is) is
#define _RUBI_IS_VOID_int32_t(is)
#define _RUBI_IS_VOID_int18_t(is)
#define _RUBI_IS_VOID_int8_t(is)
#define _RUBI_IS_VOID_uint32_t(is)
#define _RUBI_IS_VOID_uint18_t(is)
#define _RUBI_IS_VOID_uint8_t(is)
#define _RUBI_IS_VOID_float(is)
#define _RUBI_IS_VOID_shortstring(is)
#define _RUBI_IS_VOID_longstring(is)
#define _RUBI_IS_VOID_bool(is)
#define _RUBI_IS_VOID_RUBI_ENUM1(is)

#define __RUBI_REPEAT0(X)
#define __RUBI_REPEAT1(X) X
#define __RUBI_REPEAT2(X) __RUBI_REPEAT1(X), X
#define __RUBI_REPEAT3(X) __RUBI_REPEAT2(X), X
#define __RUBI_REPEAT4(X) __RUBI_REPEAT3(X), X
#define __RUBI_REPEAT5(X) __RUBI_REPEAT4(X), X
#define __RUBI_REPEAT6(X) __RUBI_REPEAT5(X), X
#define __RUBI_REPEAT7(X) __RUBI_REPEAT6(X), X
#define __RUBI_REPEAT8(X) __RUBI_REPEAT7(X), X
#define __RUBI_REPEAT9(X) __RUBI_REPEAT8(X), X
#define __RUBI_REPEAT10(X) __RUBI_REPEAT9(X), X

// typedef enum __RUBI_ENUM1 RUBI_ENUM1;

typedef struct shortstring
{
    uint8_t str[32];
} shortstring;
typedef struct longstring
{
    uint8_t str[256];
} longstring;

// typedef uint8_t shortstring;

uint8_t rubi_type_size(uint8_t typecode);
