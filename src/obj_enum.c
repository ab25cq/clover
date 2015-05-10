#include "clover.h"
#include "common.h"

BOOL enum_to_hash(CLObject hash, sCLClass* klass, sVMInfo* info)
{
    int i;

    for(i=0; i<klass->mNumFields; i++) {
        CLObject key;
        CLObject item;
        sCLField* field;
        char* field_name;
        CLObject field_value;

        field = klass->mFields + i;

        if(field->mFlags & CL_STATIC_FIELD) {
            field_name = CONS_str(&klass->mConstPool, field->mNameOffset);

            if(!create_string_object_from_ascii_string(&key, field_name, gStringTypeObject, info))
            {
                return FALSE;
            }

            push_object(key, info);

            field_value = field->uValue.mStaticField.mObjectValue.mValue;

            if(!check_type(field_value, gIntTypeObject, info)) {
                pop_object_except_top(info);
                return FALSE;
            }

            item = create_int_object(CLINT(field_value)->mValue);

            push_object(item, info);

            if(!add_item_to_hash(hash, key, item, info)) {
                pop_object_except_top(info);
                pop_object_except_top(info);
                return FALSE;
            }

            pop_object(info);
            pop_object(info);
        }
    }

    return TRUE;
}

BOOL Enum_toHash(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject hash_type_object;
    CLObject type_object;
    CLObject type_object2;
    CLObject hash;
    int i;

    hash_type_object = create_type_object_with_class_name("Hash$2");
    push_object(hash_type_object, info);

    CLTYPEOBJECT(hash_type_object)->mGenericsTypesNum = 2;

    CLTYPEOBJECT(hash_type_object)->mGenericsTypes[0] = gStringTypeObject;
    CLTYPEOBJECT(hash_type_object)->mGenericsTypes[1] = gIntTypeObject;

    if(!create_hash_object(&hash, hash_type_object, NULL, NULL, 0, info))
    {
        pop_object_except_top(info);
        return FALSE;
    }

    push_object(hash, info);

    for(i=0; i<klass->mNumSuperClasses; i++) {
        sCLClass* super;
        char* real_class_name;

        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);

        super = cl_get_class(real_class_name);

        ASSERT(super != NULL);

        if(super != gObjectClass) {
            if(!enum_to_hash(hash, super, info)) {
                pop_object_except_top(info);
                pop_object_except_top(info);
                return FALSE;
            }
        }
    }

    if(!enum_to_hash(hash, klass, info)) {
        pop_object_except_top(info);
        pop_object_except_top(info);
        return FALSE;
    }

    pop_object(info);
    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = hash;    // push result
    (*stack_ptr)++;

    return TRUE;
}

