#include "clover.h"
#include "common.h"

sCLNodeType* gIntType;      // foudamental classes
sCLNodeType* gByteType;
sCLNodeType* gFloatType;
sCLNodeType* gVoidType;
sCLNodeType* gBoolType;
sCLNodeType* gNullType;

sCLNodeType* gObjectType;
sCLNodeType* gStringType;
sCLNodeType* gBytesType;
sCLNodeType* gArrayType;
sCLNodeType* gHashType;
sCLNodeType* gBlockType;
sCLNodeType* gExceptionType;
sCLNodeType* gClassNameType;
sCLNodeType* gThreadType;

sCLNodeType* gAnonymousType[CL_GENERICS_CLASS_PARAM_MAX];

static sCLNodeType** gNodeTypes = NULL;
static int gUsedPageNodeTypes = 0;
static int gSizePageNodeTypes = 0;
static int gUsedNodeTypes = 0;

#define NODE_TYPE_PAGE_SIZE 64

void init_node_types()
{
    const int size_page_node_types = 4;

    if(gSizePageNodeTypes == 0) {
        int i;

        gNodeTypes = CALLOC(1, sizeof(sCLNodeType*)*size_page_node_types);
        for(i=0; i<size_page_node_types; i++) {
            gNodeTypes[i] = CALLOC(1, sizeof(sCLNodeType)*NODE_TYPE_PAGE_SIZE);
        }

        gSizePageNodeTypes = size_page_node_types;
        gUsedPageNodeTypes = 0;
        gUsedNodeTypes = 0;

        gIntType = alloc_node_type();
        gByteType = alloc_node_type();
        gFloatType = alloc_node_type();
        gVoidType = alloc_node_type();
        gBoolType = alloc_node_type();
        gNullType = alloc_node_type();
        gObjectType = alloc_node_type();
        gStringType = alloc_node_type();
        gBytesType = alloc_node_type();
        gArrayType = alloc_node_type();
        gArrayType->mGenericsTypesNum = 1;
        gArrayType->mGenericsTypes[0] = alloc_node_type();
        gHashType = alloc_node_type();
        gHashType->mGenericsTypesNum = 1;
        gHashType->mGenericsTypes[0] = alloc_node_type();
        gBlockType = alloc_node_type();
        gClassNameType = alloc_node_type();
        gThreadType = alloc_node_type();
        gExceptionType = alloc_node_type();

        for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
            gAnonymousType[i] = alloc_node_type();
        }
    }
}

void free_node_types()
{
    if(gSizePageNodeTypes > 0) {
        int i;

        for(i=0; i<gSizePageNodeTypes; i++) {
            FREE(gNodeTypes[i]);
        }
        FREE(gNodeTypes);

        gSizePageNodeTypes = 0;
        gUsedPageNodeTypes = 0;
        gUsedNodeTypes = 0;
    }
}

sCLNodeType* alloc_node_type()
{
    ASSERT(gNodeTypes != NULL && gSizePageNodeTypes > 0); // Is the node types initialized ?

    if(gUsedNodeTypes == NODE_TYPE_PAGE_SIZE) {
        gUsedNodeTypes = 0;
        gUsedPageNodeTypes++;

        if(gUsedPageNodeTypes == gSizePageNodeTypes) {
            int new_size;
            int i;

            new_size = (gSizePageNodeTypes+1) * 2;
            gNodeTypes = REALLOC(gNodeTypes, sizeof(sCLNodeType*)*new_size);
            memset(gNodeTypes + gSizePageNodeTypes, 0, sizeof(sCLNodeType*)*(new_size - gSizePageNodeTypes));

            for(i=gSizePageNodeTypes; i<new_size; i++) {
                gNodeTypes[i] = CALLOC(1, sizeof(sCLNodeType)*NODE_TYPE_PAGE_SIZE);
            }

            gSizePageNodeTypes = new_size;
        }
    }

    return &gNodeTypes[gUsedPageNodeTypes][gUsedNodeTypes++];
}

ALLOC sCLNodeType* clone_node_type(sCLNodeType* node_type)
{
    sCLNodeType* node_type2;
    int i;

    node_type2 = alloc_node_type();

    node_type2->mClass = node_type->mClass;
    node_type2->mGenericsTypesNum = node_type->mGenericsTypesNum;

    for(i=0; i<node_type->mGenericsTypesNum; i++) {
        node_type2->mGenericsTypes[i] = ALLOC clone_node_type(node_type->mGenericsTypes[i]);
    }

    return node_type2;
}

ALLOC sCLNodeType* create_node_type_from_cl_type(sCLType* cl_type, sCLClass* klass)
{
    sCLNodeType* node_type;
    int i;

    node_type = alloc_node_type();

    node_type->mClass = cl_get_class(CONS_str(&klass->mConstPool, cl_type->mClassNameOffset));

    ASSERT(node_type->mClass != NULL);

    node_type->mGenericsTypesNum = cl_type->mGenericsTypesNum;

    for(i=0; i<cl_type->mGenericsTypesNum; i++) {
        node_type->mGenericsTypes[i] = ALLOC create_node_type_from_cl_type(cl_type->mGenericsTypes[i], klass);
    }

    return node_type;
}

BOOL solve_generics_types_for_node_type(sCLNodeType* node_type, ALLOC sCLNodeType** result, sCLNodeType* type_)
{
    int i;
    int j;

    if(type_) {
        for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
            if(node_type->mClass == gAnonymousClass[i]) {
                if(i < type_->mGenericsTypesNum) {
                    *result = ALLOC clone_node_type(type_->mGenericsTypes[i]);
                    return TRUE;
                }
                else {
                    *result = ALLOC clone_node_type(node_type); // error
                    return FALSE;
                }
            }
        }

        *result = alloc_node_type();
        (*result)->mClass = node_type->mClass;

        (*result)->mGenericsTypesNum = node_type->mGenericsTypesNum;

        for(j=0; j<node_type->mGenericsTypesNum; j++) {
            if(!solve_generics_types_for_node_type(node_type->mGenericsTypes[j], &(*result)->mGenericsTypes[j], type_))
            {
                return FALSE;
            }
        }
    }
    else {
        *result = clone_node_type(node_type); // no solve
    }

    return TRUE;
}

// left_type is stored type. right_type is value type.
BOOL substitution_posibility(sCLNodeType* left_type, sCLNodeType* right_type)
{
    ASSERT(left_type != NULL);
    ASSERT(right_type != NULL);

    /// null type is special ///
    if(type_identity(right_type, gNullType)) {
        if(search_for_super_class(left_type->mClass, gObjectType->mClass))
        {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    else {
        int i;

        if(left_type->mClass != right_type->mClass) {
            if(!search_for_super_class(right_type->mClass, left_type->mClass) && !search_for_implemeted_interface(right_type->mClass, left_type->mClass))
            {
                return FALSE;
            }
        }
        else {
            if(left_type->mGenericsTypesNum != right_type->mGenericsTypesNum) {
                return FALSE;
            }

            for(i=0; i<left_type->mGenericsTypesNum; i++) {
                //if(!is_anonymous_class(left_type->mGenericsTypes[i]->mClass)) {
                    if(!substitution_posibility(left_type->mGenericsTypes[i], right_type->mGenericsTypes[i])) 
                    {
                        return FALSE;
                    }
                //}
            }
        }
    }

    return TRUE;
}

BOOL operand_posibility(sCLNodeType* left_type, sCLNodeType* right_type)
{
    if(!type_identity(left_type, right_type)) {
        return FALSE;
    }

    return TRUE;
}

BOOL type_identity(sCLNodeType* type1, sCLNodeType* type2)
{
    int i;

    if(type1->mClass != type2->mClass) {
        return FALSE;
    }

    if(type1->mGenericsTypesNum != type2->mGenericsTypesNum) {
        return FALSE;
    }

    for(i=0; i<type1->mGenericsTypesNum; i++) {
        if(!type_identity(type1->mGenericsTypes[i], type2->mGenericsTypes[i]))
        {
            return FALSE;
        }
    }

    return TRUE;
}

ALLOC sCLType* create_cl_type_from_node_type(sCLNodeType* node_type, sCLClass* klass)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    sCLType* cl_type;
    int i;
    
    cl_type = allocate_cl_type();

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(node_type->mClass), CLASS_NAME(node_type->mClass));

    cl_type->mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);

    cl_type->mGenericsTypesNum = node_type->mGenericsTypesNum;

    for(i=0; i<node_type->mGenericsTypesNum; i++) {
        cl_type->mGenericsTypes[i] = ALLOC create_cl_type_from_node_type(node_type->mGenericsTypes[i], klass);
    }

    return cl_type;
}

void create_cl_type_from_node_type2(sCLType* cl_type, sCLNodeType* node_type, sCLClass* klass)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    int i;
    
    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(node_type->mClass), CLASS_NAME(node_type->mClass));

    cl_type->mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);

    cl_type->mGenericsTypesNum = node_type->mGenericsTypesNum;

    for(i=0; i<node_type->mGenericsTypesNum; i++) {
        cl_type->mGenericsTypes[i] = ALLOC create_cl_type_from_node_type(node_type->mGenericsTypes[i], klass);
    }
}

