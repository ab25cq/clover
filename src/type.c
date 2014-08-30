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

    /// null type is special ///
    if(right_type == gNullClass) {
        if(search_for_super_class(left_type, gObjectClass)) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    else {
        if(left_type != right_type) {
            if(!search_for_super_class(right_type, left_type) && !search_for_implemeted_interface(right_type, left_type)) 
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}
BOOL type_identity_of_cl_type(sCLClass* klass1, sCLType* type1, sCLClass* klass2, sCLType* type2)
{
    sCLClass* left_class;
    char* real_class_name;
    sCLClass* right_class;
    int i;

    real_class_name = CONS_str(&klass1->mConstPool, type1->mClassNameOffset);
    left_class = cl_get_class(real_class_name);

    ASSERT(left_class != NULL);

    real_class_name = CONS_str(&klass2->mConstPool, type2->mClassNameOffset);
    right_class= cl_get_class(real_class_name);

    ASSERT(right_class != NULL);

    if(left_class != right_class) {
        return FALSE;
    }

    if(type1->mGenericsTypesNum != type2->mGenericsTypesNum) {
        return FALSE;
    }

    for(i=0; i<type1->mGenericsTypesNum; i++) {
        if(!type_identity_of_cl_type(klass1, type1->mGenericsTypes[i], klass2, type2->mGenericsTypes[i]))
        {
            return FALSE;
        }
    }

    return TRUE;
}
