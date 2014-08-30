#include "clover.h"
#include "common.h"

unsigned int gIntType;      // foudamental classes
unsigned int gByteType;
unsigned int gFloatType;
unsigned int gVoidType;
unsigned int gBoolType;
unsigned int gNullType;

unsigned int gObjectType;
unsigned int gStringType;
unsigned int gBytesType;
unsigned int gArrayType;
unsigned int gHashType;
unsigned int gBlockType;
unsigned int gExceptionType;
unsigned int gClassNameType;
unsigned int gThreadType;

unsigned int gAnonymousType[CL_GENERICS_CLASS_PARAM_MAX];

sCLNodeType* gNodeTypes = NULL;
static int gUsedNodeTypes = 0;
static int gSizeNodeTypes = 0;

void init_node_types()
{
    const int size_node_type = 32;

    if(gUsedNodeTypes == 0) {
        int i;

        gNodeTypes = CALLOC(1, sizeof(sCLNodeType)*size_node_type);
        gSizeNodeTypes = size_node_type;
        gUsedNodeTypes = 1;  // 0 for null node type

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
        gHashType = alloc_node_type();
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
    int i;

    if(gUsedNodeTypes > 0) {
        FREE(gNodeTypes);

        gSizeNodeTypes = 0;
        gUsedNodeTypes = 0;
    }
}

unsigned int alloc_node_type()
{
    ASSERT(gNodeTypes != NULL && gSizeNodeTypes > 0); // Is the node types initialized ?

    if(gSizeNodeTypes == gUsedNodeTypes) {
        int new_size;

        new_size = (gSizeNodeTypes+1) * 2;
        gNodeTypes = REALLOC(gNodeTypes, sizeof(sCLNodeType)*new_size);
        memset(gNodeTypes + gSizeNodeTypes, 0, sizeof(sCLNodeType)*(new_size - gSizeNodeTypes));

        gSizeNodeTypes = new_size;
    }

    return gUsedNodeTypes++;
}

ALLOC unsigned int clone_node_type(unsigned int node_type)
{
    unsigned int node_type2;
    int i;

    node_type2 = alloc_node_type();

    gNodeTypes[node_type2].mClass = gNodeTypes[node_type].mClass;
    gNodeTypes[node_type2].mGenericsTypesNum = gNodeTypes[node_type].mGenericsTypesNum;

    for(i=0; i<gNodeTypes[node_type].mGenericsTypesNum; i++) {
        gNodeTypes[node_type2].mGenericsTypes[i] = clone_node_type(gNodeTypes[node_type].mGenericsTypes[i]);
    }

    return node_type2;
}

ALLOC unsigned int create_node_type_from_cl_type(sCLType* cl_type, sCLClass* klass)
{
    unsigned int node_type;
    int i;

    node_type = alloc_node_type();

    gNodeTypes[node_type].mClass = cl_get_class(CONS_str(&klass->mConstPool, cl_type->mClassNameOffset));

    ASSERT(gNodeTypes[node_type].mClass != NULL);

    gNodeTypes[node_type].mGenericsTypesNum = cl_type->mGenericsTypesNum;

    for(i=0; i<cl_type->mGenericsTypesNum; i++) {
        gNodeTypes[node_type].mGenericsTypes[i] = ALLOC create_node_type_from_cl_type(cl_type->mGenericsTypes[i], klass);
    }

    return node_type;
}

// result is setted on (sCLClass** result_class)
// result (TRUE) success on solving or not solving (FALSE) error on solving the generic type
BOOL solve_generics_types(sCLClass* klass, unsigned int type_, sCLClass** result_class)
{
    int i;
    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
        if(klass == gNodeTypes[gAnonymousType[i]].mClass) { 
            if(i < gNodeTypes[type_].mGenericsTypesNum) {
                *result_class = gNodeTypes[gNodeTypes[type_].mGenericsTypes[i]].mClass;
                return TRUE;
            }
            else {
                *result_class = klass; // !!!
                return FALSE;
            }
        }
    }

    *result_class = klass;
    return TRUE;
}

// left_type is stored type. right_type is value type.
BOOL substitution_posibility(unsigned int left_type, unsigned int right_type)
{
ASSERT(left_type != 0);
ASSERT(right_type != 0);

    /// null type is special ///
    if(right_type == gNullType) {
        if(search_for_super_class(gNodeTypes[left_type].mClass, gNodeTypes[gObjectType].mClass)) 
        {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }
    else {
        int i;

        if(gNodeTypes[left_type].mClass != gNodeTypes[right_type].mClass) {
            if(!search_for_super_class(gNodeTypes[right_type].mClass, gNodeTypes[left_type].mClass) && !search_for_implemeted_interface(gNodeTypes[right_type].mClass, gNodeTypes[left_type].mClass)) 
            {
                return FALSE;
            }
        }
        if(gNodeTypes[left_type].mGenericsTypesNum != gNodeTypes[right_type].mGenericsTypesNum) {
            return FALSE;
        }

        for(i=0; i<gNodeTypes[left_type].mGenericsTypesNum; i++) {
            if(!substitution_posibility(gNodeTypes[left_type].mGenericsTypes[i], gNodeTypes[right_type].mGenericsTypes[i])) 
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL operand_posibility(unsigned int left_type, unsigned int right_type)
{
    if(!type_identity(left_type, right_type)) {
        return FALSE;
    }

    return TRUE;
}

BOOL type_identity(unsigned int type1, unsigned int type2)
{
    int i;

    if(gNodeTypes[type1].mClass != gNodeTypes[type2].mClass) {
        return FALSE;
    }

    if(gNodeTypes[type1].mGenericsTypesNum != gNodeTypes[type2].mGenericsTypesNum) {
        return FALSE;
    }

    for(i=0; i<gNodeTypes[type1].mGenericsTypesNum; i++) {
        if(!type_identity(gNodeTypes[type1].mGenericsTypes[i], gNodeTypes[type2].mGenericsTypes[i]))
        {
            return FALSE;
        }
    }

    return TRUE;
}

ALLOC sCLType* create_cl_type_from_node_type(unsigned int node_type, sCLClass* klass)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    sCLType* cl_type;
    int i;
    
    cl_type = allocate_cl_type();

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(gNodeTypes[node_type].mClass), CLASS_NAME(gNodeTypes[node_type].mClass));

    cl_type->mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);

    cl_type->mGenericsTypesNum = gNodeTypes[node_type].mGenericsTypesNum;

    for(i=0; i<gNodeTypes[node_type].mGenericsTypesNum; i++) {
        cl_type->mGenericsTypes[i] = ALLOC create_cl_type_from_node_type(gNodeTypes[node_type].mGenericsTypes[i], klass);
    }

    return cl_type;
}

void create_cl_type_from_node_type2(sCLType* cl_type, unsigned int node_type, sCLClass* klass)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    int i;
    
    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(gNodeTypes[node_type].mClass), CLASS_NAME(gNodeTypes[node_type].mClass));

    cl_type->mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name);

    cl_type->mGenericsTypesNum = gNodeTypes[node_type].mGenericsTypesNum;

    for(i=0; i<gNodeTypes[node_type].mGenericsTypesNum; i++) {
        cl_type->mGenericsTypes[i] = ALLOC create_cl_type_from_node_type(gNodeTypes[node_type].mGenericsTypes[i], klass);
    }
}
