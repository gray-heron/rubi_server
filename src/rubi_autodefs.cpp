
#include <inttypes.h>

#include "exceptions.h"
#include "rubi_autodefs.h"

uint8_t rubi_type_size(uint8_t typecode)
{
    switch (typecode)
    {
    case _RUBI_TYPECODES_void:
        return 0;
    case _RUBI_TYPECODES_int32_t:
        return 4;
    case _RUBI_TYPECODES_int16_t:
        return 2;
    case _RUBI_TYPECODES_int8_t:
        return 1;
    case _RUBI_TYPECODES_uint32_t:
        return 4;
    case _RUBI_TYPECODES_uint16_t:
        return 2;
    case _RUBI_TYPECODES_uint8_t:
        return 1;
    case _RUBI_TYPECODES_float:
        return 4;
    case _RUBI_TYPECODES_shortstring:
        return 32;
    case _RUBI_TYPECODES_longstring:
        return 255;
    case _RUBI_TYPECODES_bool:
        return 1;
    case _RUBI_TYPECODES_RUBI_ENUM1:
        return 1;
    default:
        ASSERT(0);
    }

    return 0;
}
