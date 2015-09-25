#include "clover.h"
#include "common.h"

static BOOL is_parent_class_by_name(sCLClass* klass1, char* parent_class_name)
{
    int i;
    for(i=0; i<klass1->mNumSuperClasses; i++) {
        char* real_class_name;
        
        real_class_name = CONS_str(&klass1->mConstPool, klass1->mSuperClasses[i].mClassNameOffset);

        if(strcmp(real_class_name, parent_class_name) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

void entry_native_enum_fields(sCLClass* klass, int num_fields, int values[])
{
    if(klass->mNumFields == num_fields) {
        if(is_parent_class_by_name(klass, "int")) {
            int i;
            for(i=0; i<num_fields; i++) {
                klass->mFields[i].uValue.mStaticField.mObjectValue.mValue = create_int_object(values[i]);
            }
        }
        else if(is_parent_class_by_name(klass, "uint")) {
            int i;
            for(i=0; i<num_fields; i++) {
                klass->mFields[i].uValue.mStaticField.mObjectValue.mValue = create_uint_object(values[i]);
            }
        }
        else if(is_parent_class_by_name(klass, "byte")) {
            int i;
            for(i=0; i<num_fields; i++) {
                klass->mFields[i].uValue.mStaticField.mObjectValue.mValue = create_byte_object(values[i]);
            }
        }
        else if(is_parent_class_by_name(klass, "short")) {
            int i;
            for(i=0; i<num_fields; i++) {
                klass->mFields[i].uValue.mStaticField.mObjectValue.mValue = create_short_object(values[i]);
            }
        }
        else if(is_parent_class_by_name(klass, "long")) {
            int i;
            for(i=0; i<num_fields; i++) {
                klass->mFields[i].uValue.mStaticField.mObjectValue.mValue = create_long_object(values[i]);
            }
        }
        else {
            int i;
            for(i=0; i<num_fields; i++) {
                klass->mFields[i].uValue.mStaticField.mObjectValue.mValue = create_int_object(values[i]);
            }
        }
    }
    else {
        fprintf(stderr, "Clover can't initialize fileds of %s class", REAL_CLASS_NAME(klass));
        exit(2);
    }
}

BOOL enum_to_hash(CLObject hash, sCLClass* klass, sVMInfo* info)
{
    int i;

    for(i=0; i<klass->mNumFields; i++) {
        CLObject key;
        sCLField* field;
        char* field_name;
        CLObject field_value;

        field = klass->mFields + i;

        if(field->mFlags & CL_STATIC_FIELD) {
            CLObject type_object;

            field_name = CONS_str(&klass->mConstPool, field->mNameOffset);

            if(!create_string_object_from_ascii_string(&key, field_name, gStringTypeObject, info))
            {
                return FALSE;
            }

            push_object(key, info);

            field_value = field->uValue.mStaticField.mObjectValue.mValue;

            if(!add_item_to_hash(hash, key, field_value, info)) {
                pop_object_except_top(info);
                return FALSE;
            }

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
    CLTYPEOBJECT(hash_type_object)->mGenericsTypes[1] = gAnonymousTypeObject;

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

