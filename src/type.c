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

    /// dynamic typing class is special ///
    if(is_dynamic_typing_class(left_type) || is_dynamic_typing_class(right_type))
    {
        return TRUE;
    }
    else if(left_type != right_type) {
        if(!search_for_super_class(right_type, left_type) && !search_for_implemeted_interface(right_type, left_type)) 
        {
            return FALSE;
        }
    }

    return TRUE;
}


ALLOC sCLType* clone_cl_type(sCLType* cl_type2, sCLClass* klass, sCLClass* klass2)
{
    sCLType* self;
    int i;

    self = ALLOC allocate_cl_type();

    self->mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, CONS_str(&klass2->mConstPool, cl_type2->mClassNameOffset), FALSE);

    for(i=0; i<cl_type2->mGenericsTypesNum; i++) {
        self->mGenericsTypes[i] = ALLOC clone_cl_type(cl_type2->mGenericsTypes[i], klass, klass2);
    }

    return self;
}

void clone_cl_type2(sCLType* self, sCLType* cl_type2, sCLClass* klass, sCLClass* klass2)
{
    int i;

    self->mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, CONS_str(&klass2->mConstPool, cl_type2->mClassNameOffset), FALSE);

    self->mGenericsTypesNum = cl_type2->mGenericsTypesNum;

    for(i=0; i<cl_type2->mGenericsTypesNum; i++) {
        self->mGenericsTypes[i] = ALLOC clone_cl_type(cl_type2->mGenericsTypes[i], klass, klass2);
    }
}

void show_cl_type(sCLType* self, sCLClass* klass, sVMInfo* info)
{
    int i;

    if(self == NULL) {
        cl_print(info, "NULL");
    }
    else if(self->mGenericsTypesNum == 0) {
        cl_print(info, "%s", CONS_str(&klass->mConstPool, self->mClassNameOffset));
    }
    else {
        cl_print(info, "%s<", CONS_str(&klass->mConstPool, self->mClassNameOffset));
        for(i=0; i<self->mGenericsTypesNum; i++) {
            show_cl_type(self->mGenericsTypes[i], klass, info);
            if(i != self->mGenericsTypesNum-1) { cl_print(info, ","); }
        }
        cl_print(info, ">");
    }
}

