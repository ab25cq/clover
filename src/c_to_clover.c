#include "clover.h"
#include "common.h"

CLObject create_number_object_from_size(size_t size, unsigned long value, char* type_name, sVMInfo* info)
{
    CLObject result;
    CLObject type_object;

    if(size == 1) {
        result = create_byte_object((unsigned char)value);
    }
    else if(size == 2) {
        result = create_short_object((unsigned short)value);
    }
    else if(size == 4) {
        result = create_uint_object((unsigned int)value);
    }
    else if(size == 8) {
        result = create_long_object((unsigned long)value);
    }

    push_object(result, info);

    type_object = create_type_object_with_class_name(type_name);

    CLOBJECT_HEADER(result)->mType = type_object;

    pop_object(info);

    return result;
}

unsigned long get_value_with_size(size_t size, CLObject number)
{
    unsigned long result;

    if(size == 1) {
        result = CLBYTE(number)->mValue;
    }
    else if(size == 2) {
        result = CLSHORT(number)->mValue;
    }
    else if(size == 4) {
        result = CLUINT(number)->mValue;
    }
    else if(size == 8) {
        result = CLLONG(number)->mValue;
    }

    return result;
}


