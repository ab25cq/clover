#include "clover.h"
#include "common.h"

sCLNodeType* gIntType;      // foudamental classes
sCLNodeType* gByteType;
sCLNodeType* gShortType;
sCLNodeType* gUIntType;
sCLNodeType* gLongType;
sCLNodeType* gCharType;
sCLNodeType* gDoubleType;
sCLNodeType* gFloatType;
sCLNodeType* gVoidType;
sCLNodeType* gBoolType;
sCLNodeType* gNullType;

sCLNodeType* gObjectType;
sCLNodeType* gStringType;
sCLNodeType* gBytesType;
sCLNodeType* gArrayType;
sCLNodeType* gTupleType[CL_GENERICS_CLASS_PARAM_MAX+1];
sCLNodeType* gRangeType;
sCLNodeType* gHashType;
sCLNodeType* gBlockType;
sCLNodeType* gExceptionType;
sCLNodeType* gThreadType;
sCLNodeType* gTypeType;
sCLNodeType* gRegexType;

sCLNodeType* gGParamTypes[CL_GENERICS_CLASS_PARAM_MAX];
sCLNodeType* gAnonymousType;

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
        gShortType = alloc_node_type();
        gUIntType = alloc_node_type();
        gLongType = alloc_node_type();
        gCharType = alloc_node_type();
        gDoubleType = alloc_node_type();
        gFloatType = alloc_node_type();
        gVoidType = alloc_node_type();
        gBoolType = alloc_node_type();
        gNullType = alloc_node_type();
        gObjectType = alloc_node_type();
        gStringType = alloc_node_type();
        gBytesType = alloc_node_type();
        gArrayType = alloc_node_type();
        gRangeType = alloc_node_type();
        gArrayType->mGenericsTypesNum = 1;
        gArrayType->mGenericsTypes[0] = alloc_node_type();
        gHashType = alloc_node_type();
        gHashType->mGenericsTypesNum = 2;
        gHashType->mGenericsTypes[0] = alloc_node_type();
        gHashType->mGenericsTypes[1] = alloc_node_type();
        gBlockType = alloc_node_type();
        gThreadType = alloc_node_type();
        gExceptionType = alloc_node_type();
        gTypeType = alloc_node_type();
        gRegexType = alloc_node_type();

        for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
            int j;

            gTupleType[i] = alloc_node_type();
            gTupleType[i]->mGenericsTypesNum = i+1;

            for(j=0; j<i+1; j++) {
                gTupleType[i]->mGenericsTypes[j] = alloc_node_type();
            }
        }

        for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
            gGParamTypes[i] = alloc_node_type();
            //gMAnonymousType[i] = alloc_node_type();
        }

        gAnonymousType = alloc_node_type();
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
            gNodeTypes = xxrealloc(gNodeTypes, sizeof(sCLNodeType*)*gSizePageNodeTypes, sizeof(sCLNodeType*)*new_size);
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

    node_type2->mStar = node_type->mStar;

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

    node_type->mStar = cl_type->mStar;

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
            if(node_type->mClass == gGParamClass[i]) {
                if(i < type_->mGenericsTypesNum) {
                    *result = ALLOC clone_node_type(type_->mGenericsTypes[i]);
                    return TRUE;
                }
                else {
                    *result = ALLOC clone_node_type(node_type); // no solve
                    return FALSE; // error
                }
            }
        }

        *result = alloc_node_type();
        (*result)->mClass = node_type->mClass;

        (*result)->mGenericsTypesNum = node_type->mGenericsTypesNum;

        (*result)->mStar = node_type->mStar;

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

static BOOL substitution_posibility_for_super_class(sCLNodeType* left_type, sCLNodeType* right_type, sCLNodeType* type_)
{
    sCLNodeType* current_type;
    int i;

    current_type = type_;

    for(i=right_type->mClass->mNumSuperClasses-1; i>=0; i--) {
        sCLNodeType* super_class;
        sCLNodeType* solved_super_class;

        super_class = ALLOC create_node_type_from_cl_type(&right_type->mClass->mSuperClasses[i], right_type->mClass);

        ASSERT(super_class->mClass != NULL);
        

        if(!solve_generics_types_for_node_type(super_class, &solved_super_class, current_type))
        {
            return FALSE;
        }

        current_type = solved_super_class;

        if(substitution_posibility(left_type, solved_super_class)) {
            return TRUE;                // found
        }
    }

    return FALSE;
}

// left_type is stored type. right_type is value type.
BOOL substitution_posibility(sCLNodeType* left_type, sCLNodeType* right_type)
{
    ASSERT(left_type != NULL);
    ASSERT(right_type != NULL);
    ASSERT(left_type->mClass != NULL);
    ASSERT(right_type->mClass != NULL);

    if(is_dynamic_typing_class(left_type->mClass) || is_dynamic_typing_class(right_type->mClass))
    {
        return TRUE;
    }
    else {
        int i;

        if(left_type->mClass != right_type->mClass) {
            if(substitution_posibility_for_super_class(left_type, right_type, right_type))
            {
                return TRUE;
            }
            else {
                if(!search_for_implemeted_interface(right_type->mClass, left_type->mClass))
                {
                    return FALSE;
                }

                if(left_type->mGenericsTypesNum != right_type->mGenericsTypesNum) {
                    return FALSE;
                }

                for(i=0; i<left_type->mGenericsTypesNum; i++) {
                    if(!substitution_posibility(left_type->mGenericsTypes[i], right_type->mGenericsTypes[i])) 
                    {
                        return FALSE;
                    }
                }
            }
        }
        else {
            if(left_type->mGenericsTypesNum != right_type->mGenericsTypesNum) {
                return FALSE;
            }

            for(i=0; i<left_type->mGenericsTypesNum; i++) {
                if(!substitution_posibility(left_type->mGenericsTypes[i], right_type->mGenericsTypes[i])) 
                {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

// left_type is stored type. right_type is value type.
BOOL substitution_posibility_with_solving_generics(sCLNodeType* left_type, sCLNodeType* right_type, sCLClass* caller_class, sCLMethod* caller_method)
{
    if(!substitution_posibility(left_type, right_type)) {
        if((is_generics_param_class(left_type->mClass) && caller_class != NULL))
        {
            sCLGenericsParamTypes* generics_param_types;
            sCLNodeType* extends_type;
            int num_implements_types;
            sCLNodeType* implements_types[CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX];

            generics_param_types = get_generics_param_types(left_type->mClass, caller_class, caller_method);

            if(generics_param_types == NULL) {
                return FALSE;
            }

            if(!get_type_patterns_from_generics_param_type(caller_class, generics_param_types, &extends_type, implements_types, &num_implements_types))
            {
                return FALSE;
            }

            if(extends_type) {
                if(!substitution_posibility(extends_type, right_type)) {
                    return FALSE;
                }
            }
            else if(num_implements_types > 0) {
                int j;
                BOOL flg;

                flg = TRUE;
                for(j=0; j<num_implements_types; j++) {
                    if(!substitution_posibility(implements_types[j], right_type)) {
                        flg = FALSE;
                        break;
                    }
                }

                if(!flg) {
                    return FALSE;
                }
            }
            else {
                return FALSE;
            }
        }
        else if(is_generics_param_class(right_type->mClass) && caller_class != NULL)
        {
            sCLGenericsParamTypes* generics_param_types;
            sCLNodeType* extends_type;
            int num_implements_types;
            sCLNodeType* implements_types[CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX];

            generics_param_types = get_generics_param_types(right_type->mClass, caller_class, caller_method);

            if(generics_param_types == NULL) {
                return FALSE;
            }

            if(!get_type_patterns_from_generics_param_type(caller_class, generics_param_types, &extends_type, implements_types, &num_implements_types))
            {
                return FALSE;
            }

            if(extends_type) {
                if(!substitution_posibility(left_type, extends_type)) {
                    return FALSE;
                }
            }
            else if(num_implements_types > 0) {
                int j;
                BOOL flg;

                flg = FALSE;
                for(j=0; j<num_implements_types; j++) {
                    if(substitution_posibility(left_type, implements_types[j])) {
                        flg = TRUE;
                        break;
                    }
                }

                if(!flg) {
                    return FALSE;
                }
            }
            else {
                return FALSE;
            }
        }
        else {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL get_type_patterns_from_generics_param_type(sCLClass* klass, sCLGenericsParamTypes* generics_param_types, sCLNodeType** extends_type, sCLNodeType** implements_types, int* num_implements_types)
{
    if(generics_param_types->mExtendsType.mClassNameOffset != 0) {

        *extends_type = ALLOC create_node_type_from_cl_type(&generics_param_types->mExtendsType, klass);
        *num_implements_types = 0;
    }
    else if(generics_param_types->mNumImplementsTypes > 0) {
        int j;

        *extends_type = NULL;
        *num_implements_types = 0;

        for(j=0; j<generics_param_types->mNumImplementsTypes; j++) {
            implements_types[*num_implements_types] = ALLOC create_node_type_from_cl_type(&generics_param_types->mImplementsTypes[j], klass);

            (*num_implements_types)++;
            
            if(*num_implements_types >= CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX) {
                return FALSE;
            }
        }
    }
    else {
        *extends_type = NULL;
        *num_implements_types = 0;
    }

    return TRUE;
}

BOOL check_valid_generics_type(sCLNodeType* type, char* sname, int* sline, int* err_num, sCLClass* caller_class, sCLMethod* caller_method)
{
    int i;

    /// check T<T2, T3> pattern. This is invalid generics type.
    if(is_generics_param_class(type->mClass)) {
        for(i=0; i<type->mGenericsTypesNum; i++) {
            if(is_generics_param_class(type->mGenericsTypes[i]->mClass)) {
                parser_err_msg_format(sname, *sline, "Generics Parametor Class can't have generics parametor");
                (*err_num)++;
                return TRUE;
            }
        }

    }

    /// check T<int, float> pattern. This is invalid generics type. ///
    if((is_generics_param_class(type->mClass)) && type->mClass->mGenericsTypesNum > 0) {
        parser_err_msg_format(sname, *sline, "Invalid generics class. Clover can't take generics class params on a generics class");
        (*err_num)++;
        return TRUE;
    }

    /// check the generics class param number ///
    if(type->mGenericsTypesNum != type->mClass->mGenericsTypesNum) {
        parser_err_msg_format(sname, *sline, "Invalid generics class param number");
        (*err_num)++;
        return TRUE;
    }

    if(type->mClass == NULL) {
        parser_err_msg_format(sname, *sline, "Invalid generics class. This is NULL pointer");
        puts("");
        (*err_num)++;
        return TRUE;
    }

    /// check assignement of generics type ///
    for(i=0; i<type->mClass->mGenericsTypesNum; i++) {
        sCLGenericsParamTypes* generics_param_types;
        int j;
        sCLNodeType* extends_type;
        int num_implements_types;
        sCLNodeType* implements_types[CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX];

        generics_param_types = type->mClass->mGenericsTypes + i;
        if(!get_type_patterns_from_generics_param_type(type->mClass, generics_param_types, &extends_type, implements_types, &num_implements_types))
        {
            parser_err_msg_format(sname, *sline, "Overflow implements number");
            (*err_num)++;
            return FALSE;
        }

        if(type->mGenericsTypes[i]->mClass == NULL) {
            parser_err_msg_format(sname, *sline, "Generics Parametor class doesn't exist");
            (*err_num)++;
            return FALSE;
        }

        if(extends_type) {
            sCLNodeType* solved_extends_type;

            if(!solve_generics_types_for_node_type(extends_type, ALLOC &solved_extends_type, type))
            {
                return FALSE;
            }

            if(type->mGenericsTypes[i]->mClass == NULL) {
                parser_err_msg_format(sname, *sline, "Type error. Invalid generics class param");
                printf("Generics type is ");
                show_node_type(solved_extends_type);
                printf(". Parametor type is NULL class pointer.");
                puts("");
                (*err_num)++;
                return TRUE;
            }

            if(is_generics_param_class(type->mGenericsTypes[i]->mClass))
            {
                sCLNodeType* extends_type2;
                int num_implements_types2;
                sCLNodeType* implements_types2[CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX];


                generics_param_types = get_generics_param_types(type->mGenericsTypes[i]->mClass, caller_class, caller_method);


                if(generics_param_types == NULL) {
                    parser_err_msg_format(sname, *sline, "Invalid anonymous type number");
                    (*err_num)++;
                    return FALSE;
                }

                if(!get_type_patterns_from_generics_param_type(caller_class, generics_param_types, &extends_type2, implements_types2, &num_implements_types2))
                {
                    parser_err_msg_format(sname, *sline, "Overflow implements number");
                    (*err_num)++;
                    return FALSE;
                }

                if(extends_type2) {
                    if(!substitution_posibility(solved_extends_type, extends_type2)) {
                        parser_err_msg_format(sname, *sline, "Type error. Invalid generics class param");
                        printf("Generics type is ");
                        show_node_type(solved_extends_type);
                        printf(". Parametor type is ");
                        show_node_type(extends_type2);
                        puts("");
                        (*err_num)++;
                    }
                }
                else if(num_implements_types2 > 0) {
                    parser_err_msg_format(sname, *sline, "Type error. Invalid generics class param");
                    printf("Generics type is ");
                    show_node_type(solved_extends_type);
                    printf(". Parametor type is ");
                    show_node_type(type->mGenericsTypes[i]);
                    puts("");
                    (*err_num)++;
                }
                else {
                    if(!substitution_posibility(solved_extends_type, type->mGenericsTypes[i])) {
                        parser_err_msg_format(sname, *sline, "Type error. Invalid generics class param");
                        printf("Generics type is ");
                        show_node_type(solved_extends_type);
                        printf(". Parametor type is ");
                        show_node_type(type->mGenericsTypes[i]);
                        puts("");
                        (*err_num)++;
                    }
                }
            }
            else {
                if(!substitution_posibility(solved_extends_type, type->mGenericsTypes[i])) {
                    parser_err_msg_format(sname, *sline, "Type error. Invalid generics class param");
                    printf("Generics type is ");
                    show_node_type(solved_extends_type);
                    printf(". Parametor type is ");
                    show_node_type(type->mGenericsTypes[i]);
                    puts("");
                    (*err_num)++;
                }
            }
        }
        else if(num_implements_types > 0) {
            if(is_generics_param_class(type->mGenericsTypes[i]->mClass))
            {
                if(caller_class == NULL) {
                    parser_err_msg_format(sname, *sline, "Type error. caller class is null. Clover can't get info of this anonymous class");
                    (*err_num)++;
                }
                else {
                    sCLNodeType* extends_type2;
                    int num_implements_types2;
                    sCLNodeType* implements_types2[CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX];

                    generics_param_types = get_generics_param_types(type->mGenericsTypes[i]->mClass, caller_class, caller_method);

                    if(generics_param_types == NULL) {
                        parser_err_msg_format(sname, *sline, "Ivalid anonymous type number");
                        (*err_num)++;
                        return FALSE;
                    }

                    if(!get_type_patterns_from_generics_param_type(caller_class, generics_param_types, &extends_type2, implements_types2, &num_implements_types2))
                    {
                        parser_err_msg_format(sname, *sline, "Overflow implements number");
                        (*err_num)++;
                        return FALSE;
                    }

                    if(extends_type2) {
                        for(j=0; j<num_implements_types; j++) {
                            if(!check_implemented_interface2(extends_type2->mClass, implements_types[j]))
                            {
                                parser_err_msg_format(sname, *sline, "Type error. This class(%s) is not implemented this interface(%s)", REAL_CLASS_NAME(extends_type2->mClass), REAL_CLASS_NAME(implements_types[j]->mClass));
                                (*err_num)++;
                            }
                        }
                    }
                    else if(num_implements_types2 > 0) {
                        int k;
                        for(j=0; j<num_implements_types; j++) {
                            BOOL flg;

                            flg = FALSE;
                            for(k=0; k<num_implements_types2; k++) {
                                if(substitution_posibility(implements_types[j], implements_types2[k])) 
                                {
                                    flg = TRUE;
                                    break;
                                }
                            }

                            if(!flg) {
                                parser_err_msg_format(sname, *sline, "Type error. Invalid generics class param");
                                printf("Generics type is ");
                                show_node_type(implements_types[j]);
                                puts("");
                                (*err_num)++;
                            }
                        }
                    }
                    else {
                        for(j=0; j<num_implements_types; j++) {
                            if(!check_implemented_interface2(type->mGenericsTypes[i]->mClass, implements_types[j]))
                            {
                                parser_err_msg_format(sname, *sline, "Type error. This class(%s) is not implemented this interface(%s)", REAL_CLASS_NAME(type->mGenericsTypes[i]->mClass), REAL_CLASS_NAME(implements_types[j]->mClass));
                                (*err_num)++;
                            }
                        }
                    }
                }
            }
            else {
                for(j=0; j<num_implements_types; j++) {
                    if(!check_implemented_interface2(type->mGenericsTypes[i]->mClass, implements_types[j]))
                    {
                        parser_err_msg_format(sname, *sline, "Type error. This class(%s) is not implemented this interface(%s)", REAL_CLASS_NAME(type->mGenericsTypes[i]->mClass), REAL_CLASS_NAME(implements_types[j]->mClass));
                        (*err_num)++;
                    }
                }
            }
        }
        else {
            return TRUE; // no check
        }
    }

    return TRUE;
}

BOOL check_valid_star_type(sCLClass* klass)
{
    int i;

    if(!(klass->mFlags & CLASS_FLAGS_STRUCT)) {
        if(get_clone_method(klass) == NULL) {
            return FALSE;
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

sCLNodeType* create_node_type_from_class_name(char* class_name)
{
    sCLNodeType* result;

    result = alloc_node_type();
    
    result->mClass = cl_get_class(class_name);
    ASSERT(result->mClass != NULL);
    result->mGenericsTypesNum = 0;

    return result;
}

BOOL operand_posibility_with_class_name(sCLNodeType* left_type, char* class_name)
{
    sCLNodeType* right_type;

    right_type = alloc_node_type();

    right_type->mClass = cl_get_class(class_name);
    ASSERT(right_type->mClass != NULL);
    right_type->mGenericsTypesNum = 0;

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

    if(type1->mStar != type2->mStar) {
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

BOOL type_identity_without_star(sCLNodeType* type1, sCLNodeType* type2)
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

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(node_type->mClass), CLASS_NAME(node_type->mClass), node_type->mGenericsTypesNum);

    cl_type->mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name, FALSE);
    cl_type->mStar = node_type->mStar;

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
    
    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(node_type->mClass), CLASS_NAME(node_type->mClass), node_type->mGenericsTypesNum);

    cl_type->mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name, FALSE);

    cl_type->mStar = node_type->mStar;

    cl_type->mGenericsTypesNum = node_type->mGenericsTypesNum;


    for(i=0; i<node_type->mGenericsTypesNum; i++) {
        cl_type->mGenericsTypes[i] = ALLOC create_cl_type_from_node_type(node_type->mGenericsTypes[i], klass);
    }
}

ALLOC char* node_type_to_buffer(sCLNodeType* node_type)
{
    int i;
    sBuf buf;

    sBuf_init(&buf);

    if(node_type == NULL) {
        sBuf_append_str(&buf, "NULL");
    }
    else if(node_type->mGenericsTypesNum == 0) {
        sBuf_append_str(&buf, REAL_CLASS_NAME(node_type->mClass));
    }
    else {
        if(node_type->mClass == NULL) {
            sBuf_append_str(&buf, "NULL<");
        }
        else {
            sBuf_append_str(&buf, REAL_CLASS_NAME(node_type->mClass));
            sBuf_append_str(&buf, "<");
        }
        for(i=0; i<node_type->mGenericsTypesNum; i++) {
            char* result;

            result = node_type_to_buffer(node_type->mGenericsTypes[i]);

            sBuf_append_str(&buf, result);

            FREE(result);
            if(i != node_type->mGenericsTypesNum-1) { sBuf_append_str(&buf, ","); }
        }
        sBuf_append_str(&buf, ">");
    }

    return buf.mBuf;
}
