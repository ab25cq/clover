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

