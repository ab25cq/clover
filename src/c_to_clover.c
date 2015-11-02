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
    else {
        result = 0;
    }

    return result;
}

CLObject create_clover_array_from_c_array(void* c_array, int num_elements, size_t element_size, char* element_class_name, sVMInfo* info)
{
    CLObject clover_array;
    int i;
    char* p;

    clover_array = create_array_object_with_element_class_name(element_class_name, NULL, 0, info);
    push_object(clover_array, info);

    p = c_array;

    for(i=0; i<num_elements; i++) {
        unsigned long value;
        CLObject element;

        if(element_size == 1) {
            value = *p;
        }
        else if(element_size == 2) {
            value = *(unsigned short*)p;
        }
        else if(element_size == 4) {
            value = *(unsigned int*)p;
        }
        else if(element_size == 8) {
            value = *(unsigned long*)p;
        }
        else {
            fprintf(stderr, "unexpected error\n");
            assert(0);
        }

        element = create_number_object_from_size(element_size, value, element_class_name, info);

        add_to_array(clover_array, element, info);

        p += element_size;
    }

    pop_object(info);

    return clover_array;
}
