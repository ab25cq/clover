#include "clover.h"
#include "common.h"

static sCLType* gHeadCLType;

sCLType* allocate_cl_type()
{
    sCLType* type_;

    type_ = CALLOC(1, sizeof(sCLType));

    type_->mNext = gHeadCLType;
    gHeadCLType = type_;

    return type_;
}

void free_cl_types()
{
    sCLType* it;
    sCLType* free_one;

    it = gHeadCLType;

    while(it) {
        free_one = it;
        it = it->mNext;
        FREE(free_one);
    }
}

// left_type is stored calss. right_type is class of value.
BOOL substitution_posibility_of_class(sCLClass* left_type, sCLClass* right_type)
{
ASSERT(left_type != NULL);
ASSERT(right_type != NULL);

    if(left_type != right_type) {
        if(!search_for_super_class(right_type, left_type) && !search_for_implemeted_interface(right_type, left_type)) 
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL substitution_posibility_of_type_object(CLObject left_type, CLObject right_type)
{
    int i;
    sCLClass* left_class;
    sCLClass* right_class;

    ASSERT(left_type != 0);
    ASSERT(right_type != 0);

    left_class = CLTYPEOBJECT(left_type)->mClass;
    right_class = CLTYPEOBJECT(right_type)->mClass;

    /// dollar anonymous is special ///
    if(left_class == gDAnonymousClass || right_class == gDAnonymousClass) 
    {
        return TRUE;
    }
    else {
        int i;

        if(left_class != right_class) {
            int i;

            if(!search_for_super_class(right_class, left_class) && !search_for_implemeted_interface(right_class, left_class)) 
            {
                return FALSE;
            }

            if(CLTYPEOBJECT(left_type)->mGenericsTypesNum != CLTYPEOBJECT(right_type)->mGenericsTypesNum)
            {
                return FALSE;
            }

            for(i=0; i<CLTYPEOBJECT(left_type)->mGenericsTypesNum; i++) {
                if(!substitution_posibility_of_type_object(CLTYPEOBJECT(left_type)->mGenericsTypes[i], CLTYPEOBJECT(right_type)->mGenericsTypes[i]))
                {
                    return FALSE;
                }
            }
        }
        else {
            int i;

            if(CLTYPEOBJECT(left_type)->mGenericsTypesNum != CLTYPEOBJECT(right_type)->mGenericsTypesNum)
            {
                return FALSE;
            }

            for(i=0; i<CLTYPEOBJECT(left_type)->mGenericsTypesNum; i++) {
                if(!substitution_posibility_of_type_object(CLTYPEOBJECT(left_type)->mGenericsTypes[i], CLTYPEOBJECT(right_type)->mGenericsTypes[i]))
                {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

BOOL substitution_posibility_of_type_object_without_generics(CLObject left_type, CLObject right_type)
{
    int i;
    sCLClass* left_class;
    sCLClass* right_class;

    ASSERT(left_type != 0);
    ASSERT(right_type != 0);

    left_class = CLTYPEOBJECT(left_type)->mClass;
    right_class = CLTYPEOBJECT(right_type)->mClass;

    /// dollar anonymous is special ///
    if(left_class == gDAnonymousClass || right_class == gDAnonymousClass) 
    {
        return TRUE;
    }
    else {
        int i;

        if(left_class != right_class) {
            int i;

            if(!search_for_super_class(right_class, left_class) && !search_for_implemeted_interface(right_class, left_class)) 
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL check_type(CLObject ovalue1, CLObject type_object, sVMInfo* info)
{
    if(ovalue1 == 0) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        return FALSE;
    }
    if(!substitution_posibility_of_type_object(type_object, CLOBJECT_HEADER(ovalue1)->mType))
    {
        char buf1[1024];
        char buf2[1024];

        write_type_name_to_buffer(buf1, 1024, CLOBJECT_HEADER(ovalue1)->mType);
        write_type_name_to_buffer(buf2, 1024, type_object);

        entry_exception_object(info, gExceptionClass, "This is %s type. But requiring type is %s.", buf1, buf2);
        return FALSE;
    }

    return TRUE;
}

BOOL check_type_without_generics(CLObject ovalue1, CLObject type_object, sVMInfo* info)
{
    if(ovalue1 == 0) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        return FALSE;
    }
    if(!substitution_posibility_of_type_object_without_generics(type_object, CLOBJECT_HEADER(ovalue1)->mType))
    {
        char buf1[1024];
        char buf2[1024];

        write_type_name_to_buffer(buf1, 1024, CLOBJECT_HEADER(ovalue1)->mType);
        write_type_name_to_buffer(buf2, 1024, type_object);

        entry_exception_object(info, gExceptionClass, "This is %s type. But requiring type is %s.", buf1, buf2);
        return FALSE;
    }

    return TRUE;
}

ALLOC sCLType* clone_cl_type(sCLType* cl_type2, sCLClass* klass, sCLClass* klass2)
{
    sCLType* self;
    int i;

    self = ALLOC allocate_cl_type();

    self->mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, CONS_str(&klass2->mConstPool, cl_type2->mClassNameOffset));

    for(i=0; i<cl_type2->mGenericsTypesNum; i++) {
        self->mGenericsTypes[i] = ALLOC clone_cl_type(cl_type2->mGenericsTypes[i], klass, klass2);
    }

    return self;
}

void clone_cl_type2(sCLType* self, sCLType* cl_type2, sCLClass* klass, sCLClass* klass2)
{
    int i;

    self->mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, CONS_str(&klass2->mConstPool, cl_type2->mClassNameOffset));

    self->mGenericsTypesNum = cl_type2->mGenericsTypesNum;

    for(i=0; i<cl_type2->mGenericsTypesNum; i++) {
        self->mGenericsTypes[i] = ALLOC clone_cl_type(cl_type2->mGenericsTypes[i], klass, klass2);
    }
}
