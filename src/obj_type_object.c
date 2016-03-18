#include "clover.h"
#include "common.h"
#include <limits.h>
#include <ctype.h>
#include <wchar.h>

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLTypeObject);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_type_object()
{
    CLObject obj;
    unsigned int size;

    size = object_size();
    obj = alloc_heap_mem(size, gTypeObject);

    return obj;
}

static BOOL load_type_object_core(CLObject object, int** pc, sByteCode* code, sConst* constant, sVMInfo* info)
{
    int ivalue1, ivalue2;
    char* real_class_name;
    sCLClass* klass;
    int i;

    ivalue1 = **pc;          // offset
    (*pc)++;

    real_class_name = CONS_str(constant, ivalue1);
    klass = cl_get_class(real_class_name);

    if(klass == NULL) {
        entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a class named %s\n", real_class_name);
        return FALSE;
    }

    CLTYPEOBJECT(object)->mClass = klass;

    ivalue2 = **pc;          // num generics types
    (*pc)++;

    CLTYPEOBJECT(object)->mGenericsTypesNum = 0;

    for(i=0; i<ivalue2; i++) {
        CLObject object2;

        object2 = alloc_type_object();
        push_object(object2, info);
        CLTYPEOBJECT(object)->mGenericsTypes[i] = object2;
        pop_object(info);
        CLTYPEOBJECT(object)->mGenericsTypesNum++;

        if(!load_type_object_core(object2, pc, code, constant, info))
        {
            return FALSE;
        }
    }

    return TRUE;
}

sVMInfo* gVMInfo = NULL;

// result: (0) --> class not found (non 0) --> created object
CLObject create_type_object_from_bytecodes(int** pc, sByteCode* code, sConst* constant, sVMInfo* info)
{
    CLObject obj;

    obj = alloc_type_object();
    push_object(obj, info);

    if(!load_type_object_core(obj, pc, code, constant, info)) {
        pop_object_except_top(info);
        return 0;
    }
    pop_object(info);

    return obj;
}

static void skip_spaces_at_type_object(char** p)
{
    while(**p == ' ' || **p == '\t' || **p == '\n') {
        (*p)++;
    }
}

static void get_class_name_or_namespace_name(char* result, char** p)
{
    if(isalpha(**p)) {
        while(isalpha(**p) || **p == '_' || isdigit(**p)) {
            *result = **p;
            result++;
            (*p)++;
        }
    }

    skip_spaces_at_type_object(p);

    *result = 0;
}

static BOOL create_type_object_with_class_name_and_generics_name_core(CLObject object, char** p, sVMInfo* info)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX+1];
    sCLClass* klass;

    get_class_name_or_namespace_name(real_class_name, p);

    if(**p == ':' && *(*p+1) == ':') {
        char real_class_name2[CL_REAL_CLASS_NAME_MAX];

        (*p)+=2;
        skip_spaces_at_type_object(p);

        get_class_name_or_namespace_name(real_class_name2, p);

        xstrncat(real_class_name, "::", CL_REAL_CLASS_NAME_MAX);
        xstrncat(real_class_name, real_class_name2, CL_REAL_CLASS_NAME_MAX);
    }

    CLTYPEOBJECT(object)->mGenericsTypesNum = 0;

    if(**p == '<') {
        int i;
        char number[32];

        (*p)++;
        skip_spaces_at_type_object(p);

        for(i=0; ; i++) {
            CLObject object2;

            object2 = alloc_type_object();
            push_object(object2, info);
            CLTYPEOBJECT(object)->mGenericsTypes[i] = object2;
            pop_object(info);
            CLTYPEOBJECT(object)->mGenericsTypesNum++;

            if(CLTYPEOBJECT(object)->mGenericsTypesNum >= CL_GENERICS_CLASS_PARAM_MAX)
            {
                entry_exception_object_with_class_name(info, "Exception", "invalid class name 1");
                return FALSE;
            }

            if(!create_type_object_with_class_name_and_generics_name_core(object2, p, info))
            {
                return FALSE;
            }

            if(**p == ',') {
                (*p)++;
                skip_spaces_at_type_object(p);
            }
            else if(**p == 0) {
                entry_exception_object_with_class_name(info, "Exception", "invalid class name 2");
                return FALSE;
            }
            else if(**p == '>') {
                (*p)++;
                skip_spaces_at_type_object(p);
                break;
            }
        }

        /// get class from name and generics types number ///
        number[0] = CLTYPEOBJECT(object)->mGenericsTypesNum + '0';
        number[1] = 0;

        xstrncat(real_class_name, "$", CL_REAL_CLASS_NAME_MAX);
        xstrncat(real_class_name, number, CL_REAL_CLASS_NAME_MAX);

        klass = cl_get_class(real_class_name);

        if(klass == NULL) {
            entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a class named (%s)\n", real_class_name);
            return FALSE;
        }

        CLTYPEOBJECT(object)->mClass = klass;

        /// type checking ///
        if(klass->mGenericsTypesNum != CLTYPEOBJECT(object)->mGenericsTypesNum)
        {
            entry_exception_object_with_class_name(info, "Exception", "invalid class name 4");
            return FALSE;
        }
    }
    else if(**p == '$') {
        char number_buf[2];
        int number;

        (*p)++;

        if(**p >= '0' && **p <= '9') {
            number = **p - '0';
            (*p)++;
        }
        else {
            entry_exception_object_with_class_name(info, "Exception", "invalid class name 3");
            return FALSE;
        }

        number_buf[0] = number + '0';
        number_buf[1] = 0;

        /// get class from name and generics types number ///
        xstrncat(real_class_name, "$", CL_REAL_CLASS_NAME_MAX);
        xstrncat(real_class_name, number_buf, CL_REAL_CLASS_NAME_MAX);

        klass = cl_get_class(real_class_name);

        if(klass == NULL) {
            entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a class named (%s)\n", real_class_name);
            return FALSE;
        }

        CLTYPEOBJECT(object)->mClass = klass;
    }
    else if(**p == 0 || **p == ',' || **p == '>') {
        /// get class from name ///
        klass = cl_get_class(real_class_name);

        if(klass == NULL) {
            entry_exception_object_with_class_name(info, "ClassNotFoundException", "can't get a class named (%s)\n", real_class_name);
            return FALSE;
        }

        CLTYPEOBJECT(object)->mClass = klass;

        /// type checking ///
        if(klass->mGenericsTypesNum != CLTYPEOBJECT(object)->mGenericsTypesNum)
        {
            entry_exception_object_with_class_name(info, "Exception", "invalid class name 4");
            return FALSE;
        }
    }
    else {
        entry_exception_object_with_class_name(info, "Exception", "invalid class name 3");
        return FALSE;
    }

    return TRUE;
}

// result: (0) --> class not found (non 0) --> created object
CLObject create_type_object_with_class_name_and_generics_name(char* class_name, sVMInfo* info)
{
    CLObject obj;
    char* p;

    obj = alloc_type_object();
    push_object(obj, info);

    p = class_name;

    if(!create_type_object_with_class_name_and_generics_name_core(obj, &p, info))
    {
        pop_object_except_top(info);
        return 0;
    }

    pop_object(info);

    return obj;
}

CLObject create_type_object(sCLClass* klass)
{
    CLObject obj;

    obj = alloc_type_object();

    CLTYPEOBJECT(obj)->mClass = klass;

    return obj;
}

CLObject create_type_object_with_class_name(char* class_name)
{
    sCLClass* klass;

    klass = cl_get_class(class_name);

    MASSERT(klass != NULL);

    return create_type_object(klass);
}

static CLObject create_type_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;

    self = create_type_object(gNullClass);
    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

static void clone_type_object_core(CLObject type_object1, CLObject type_object2, sVMInfo* info)
{
    int i;

    CLTYPEOBJECT(type_object1)->mClass = CLTYPEOBJECT(type_object2)->mClass;

    CLTYPEOBJECT(type_object1)->mGenericsTypesNum = 0;
    for(i=0; i<CLTYPEOBJECT(type_object2)->mGenericsTypesNum; i++) {
        CLObject obj;

        obj = alloc_type_object();

        push_object(obj, info);

        clone_type_object_core(obj, CLTYPEOBJECT(type_object2)->mGenericsTypes[i], info);

        CLTYPEOBJECT(type_object1)->mGenericsTypes[i] = obj;

        CLTYPEOBJECT(type_object1)->mGenericsTypesNum++;

        pop_object(info);
    }
}

CLObject create_type_object_from_other_type_object(CLObject type_object, sVMInfo* info)
{
    CLObject obj;

    obj = alloc_type_object();

    push_object(obj, info);

    clone_type_object_core(obj, type_object, info);

    pop_object(info);

    return obj;
}

static void load_type_object_from_cl_type_core(CLObject object, sCLClass* klass, sCLType* cl_type, sVMInfo* info)
{
    char* real_class_name;
    int i;
    sCLClass* klass2;

    real_class_name = CONS_str(&klass->mConstPool, cl_type->mClassNameOffset);
    klass2 = cl_get_class(real_class_name);

    MASSERT(klass2 != NULL);

    CLTYPEOBJECT(object)->mClass = klass2;
    CLTYPEOBJECT(object)->mGenericsTypesNum = 0;

    for(i=0; i<cl_type->mGenericsTypesNum; i++) {
        CLObject object2;

        object2 = alloc_type_object();
        push_object(object2, info);
        CLTYPEOBJECT(object)->mGenericsTypes[i] = object2;
        CLTYPEOBJECT(object)->mGenericsTypesNum++;
        pop_object(info);

        load_type_object_from_cl_type_core(object2, klass, cl_type->mGenericsTypes[i], info);
    }
}

CLObject create_type_object_from_cl_type(sCLClass* klass, sCLType* cl_type, sVMInfo* info)
{
    CLObject obj;

    obj = alloc_type_object();

    push_object(obj, info);
    load_type_object_from_cl_type_core(obj, klass, cl_type, info);
    pop_object(info);

    return obj;
}

static void mark_type_object_core(CLObject object, unsigned char* mark_flg)
{
    int i;

    for(i=0; i<CLTYPEOBJECT(object)->mGenericsTypesNum; i++) {
        CLObject object2;
        
        object2 = CLTYPEOBJECT(object)->mGenericsTypes[i];

        mark_object(object2, mark_flg);

        mark_type_object_core(object2, mark_flg);
    }
}

static void mark_type_object(CLObject object, unsigned char* mark_flg)
{
    mark_type_object_core(object, mark_flg);
}

static void show_type_object2(sVMInfo* info, CLObject obj)
{
    VMLOG(info, "Type Object\n");
}

void initialize_hidden_class_method_of_type(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = show_type_object2;
    klass->mMarkFun = mark_type_object;
    klass->mCreateFun = create_type_object_for_new;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gTypeClass = klass;
        gTypeObject = create_type_object(gTypeClass);
    }
}

static CLObject get_type_object_from_cl_type_core(sCLType* cl_type, sCLClass* klass, sVMInfo* info)
{
    int i;
    char* real_class_name;
    CLObject result;
    sCLClass* klass2;

    real_class_name = CONS_str(&klass->mConstPool, cl_type->mClassNameOffset);

    klass2 = cl_get_class(real_class_name);

    MASSERT(klass2 != NULL);

    result = create_type_object(klass2);
    push_object(result, info);

    for(i=0; i<cl_type->mGenericsTypesNum; i++) {
        CLObject object;

        object = get_type_object_from_cl_type_core(cl_type->mGenericsTypes[i], klass, info);
        CLTYPEOBJECT(result)->mGenericsTypes[i] = object;

        CLTYPEOBJECT(result)->mGenericsTypesNum++;
    }

    pop_object(info);

    return result;
}

CLObject get_type_object_from_cl_type(sCLType* cl_type, sCLClass* klass, sVMInfo* info)
{
    return get_type_object_from_cl_type_core(cl_type, klass, info);
}

/// 0: false 1: no solved 2: solved 
static int solve_generics_types_of_type_object_core(CLObject type_object, ALLOC CLObject* solved_type_object, CLObject type_, sVMInfo* info)
{
    int i;

    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
        if(CLTYPEOBJECT(type_object)->mClass == gGParamClass[i]) {
            if(i < CLTYPEOBJECT(type_)->mGenericsTypesNum) {
                CLObject generics_type;
                sCLClass* klass;

                generics_type = CLTYPEOBJECT(type_)->mGenericsTypes[i];
                *solved_type_object = ALLOC create_type_object_from_other_type_object(generics_type, info);

                return 2;
            }
            else {
                *solved_type_object = ALLOC create_type_object_from_other_type_object(type_object, info);
                return 0;
            }
        }
    }

    *solved_type_object = ALLOC create_type_object_from_other_type_object(type_object, info);

    return 1;
}

BOOL solve_generics_types_of_type_object_core2(CLObject type_object, ALLOC CLObject* solved_type_object, CLObject type_, sVMInfo* info)
{
    int i;
    int result;

    result = solve_generics_types_of_type_object_core(type_object, ALLOC solved_type_object, type_, info);

    if(result == 0)
    {
        entry_exception_object_with_class_name(info, "Exception", "can't solve the generics type");
        return FALSE;
    }
    else if(result == 1) {
        push_object(*solved_type_object, info);

        CLTYPEOBJECT(*solved_type_object)->mGenericsTypesNum = 0;

        for(i=0; i<CLTYPEOBJECT(type_object)->mGenericsTypesNum; i++) {
            CLObject object;
            CLObject object2;

            object = CLTYPEOBJECT(type_object)->mGenericsTypes[i];

            if(!solve_generics_types_of_type_object_core2(object, ALLOC &object2, type_, info))
            {
                pop_object_except_top(info);
                return FALSE;
            }

            CLTYPEOBJECT(*solved_type_object)->mGenericsTypes[i] = object2;
            CLTYPEOBJECT(*solved_type_object)->mGenericsTypesNum++;
        }
        pop_object(info);
    }

    return TRUE;
}

BOOL solve_generics_types_of_type_object(CLObject type_object, ALLOC CLObject* solved_type_object, CLObject type_, sVMInfo* info)
{
    return solve_generics_types_of_type_object_core2(type_object, ALLOC solved_type_object, type_, info);
}

BOOL substitution_posibility_of_type_object(CLObject left_type, CLObject right_type, BOOL dynamic_typing)
{
    int i;
    sCLClass* left_class;
    sCLClass* right_class;

    MASSERT(left_type != 0);
    MASSERT(right_type != 0);

    left_class = CLTYPEOBJECT(left_type)->mClass;
    right_class = CLTYPEOBJECT(right_type)->mClass;

    /// Dynamic typing class is special ///
    if(dynamic_typing && (is_dynamic_typing_class(left_class) || is_dynamic_typing_class(right_class)))
    {
        return TRUE;
    }
    /// enum class is special ///
    else if((left_class->mFlags & CLASS_FLAGS_ENUM) && right_class == gIntClass) {
        return TRUE;
    }
    else {
        int i;

        if(left_class != right_class) {
            int i;

            if(search_for_super_class(right_class, left_class)) {
                return TRUE;
            }
            else {
                if(!search_for_super_class(right_class, left_class) && !search_for_implemented_interface(right_class, left_class)) 
                {
                    return FALSE;
                }

                if(CLTYPEOBJECT(left_type)->mGenericsTypesNum != CLTYPEOBJECT(right_type)->mGenericsTypesNum)
                {
                    return FALSE;
                }

                for(i=0; i<CLTYPEOBJECT(left_type)->mGenericsTypesNum; i++) {
                    if(!substitution_posibility_of_type_object(CLTYPEOBJECT(left_type)->mGenericsTypes[i], CLTYPEOBJECT(right_type)->mGenericsTypes[i], dynamic_typing))
                    {
                        return FALSE;
                    }
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
                if(!substitution_posibility_of_type_object(CLTYPEOBJECT(left_type)->mGenericsTypes[i], CLTYPEOBJECT(right_type)->mGenericsTypes[i], dynamic_typing))
                {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}


BOOL substitution_posibility_of_type_object_with_class_name(char* left_type_object_name, CLObject right_type, BOOL dynamic_typing, sVMInfo* info)
{
    CLObject left_type;
    BOOL result;

    left_type = create_type_object_with_class_name(left_type_object_name);

    push_object(left_type, info);

    result = substitution_posibility_of_type_object(left_type, right_type, dynamic_typing);

    pop_object(info);

    return result;
}

BOOL substitution_posibility_of_type_object_without_generics(CLObject left_type, CLObject right_type, BOOL dynamic_typing)
{
    int i;
    sCLClass* left_class;
    sCLClass* right_class;

    MASSERT(left_type != 0);
    MASSERT(right_type != 0);

    left_class = CLTYPEOBJECT(left_type)->mClass;
    right_class = CLTYPEOBJECT(right_type)->mClass;

    /// Dynamic typing class is special ///
    if(dynamic_typing && (is_dynamic_typing_class(left_class) || is_dynamic_typing_class(right_class)))
    {
        return TRUE;
    }
    /// enum class is special ///
    else if((left_class->mFlags & CLASS_FLAGS_ENUM) && right_class == gIntClass) {
        return TRUE;
    }
    else {
        int i;

        if(left_class != right_class) {
            int i;

            if(!search_for_super_class(right_class, left_class) && !search_for_implemented_interface(right_class, left_class)) 
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

// result (0): can't create type object (non 0): success
CLObject get_super_from_type_object(CLObject type_object, sVMInfo* info)
{
    sCLClass* klass;
    sCLType* super_klass_cl_type;
    CLObject super_type_object;
    CLObject solved_super_type_object;

    klass = CLTYPEOBJECT(type_object)->mClass;

    super_klass_cl_type = &klass->mSuperClasses[klass->mNumSuperClasses -1];

    super_type_object = get_type_object_from_cl_type(super_klass_cl_type, klass, info);
    push_object(super_type_object, info);

    if(!solve_generics_types_of_type_object(super_type_object, &solved_super_type_object, type_object, info))
    {
        pop_object(info);
        return 0;
    }
    pop_object(info);

    return solved_super_type_object;
}

BOOL check_type(CLObject ovalue1, CLObject type_object, sVMInfo* info)
{
    if(ovalue1 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }
    if(!substitution_posibility_of_type_object(type_object, CLOBJECT_HEADER(ovalue1)->mType, FALSE))
    {
        char buf1[1024];
        char buf2[1024];

        write_type_name_to_buffer(buf1, 1024, CLOBJECT_HEADER(ovalue1)->mType);
        write_type_name_to_buffer(buf2, 1024, type_object);

        entry_exception_object_with_class_name(info, "Exception", "This is %s type. But requiring type is %s.", buf1, buf2);
        return FALSE;
    }

    return TRUE;
}

BOOL check_type_without_info(CLObject ovalue1, char* class_name)
{
    sCLClass* klass;
    CLObject type_object;

    klass = cl_get_class(class_name);

    MASSERT(klass != NULL);

    type_object = CLOBJECT_HEADER(ovalue1)->mType;

    return substitution_posibility_of_class(klass, CLTYPEOBJECT(type_object)->mClass);
}

BOOL check_type_with_nullable(CLObject ovalue1, CLObject type_object, sVMInfo* info)
{
    if(ovalue1 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }
    if(!substitution_posibility_of_type_object(type_object, CLOBJECT_HEADER(ovalue1)->mType, TRUE))
    {
        char buf1[1024];
        char buf2[1024];

        write_type_name_to_buffer(buf1, 1024, CLOBJECT_HEADER(ovalue1)->mType);
        write_type_name_to_buffer(buf2, 1024, type_object);

        entry_exception_object_with_class_name(info, "Exception", "This is %s type. But requiring type is %s.", buf1, buf2);
        return FALSE;
    }

    return TRUE;
}

BOOL check_type_with_class_name(CLObject ovalue1, char* class_name, sVMInfo* info)
{
    sCLClass* klass;
    CLObject type_object;
    BOOL result;

    klass = cl_get_class(class_name);

    MASSERT(klass != NULL);

    type_object = create_type_object(klass);

//    push_object(type_object, info);

    result = check_type(ovalue1, type_object, info);

//    pop_object_except_top(info);

    return result;
}

BOOL check_type_with_class_name_and_nullable(CLObject ovalue1, char* class_name, sVMInfo* info)
{
    sCLClass* klass;
    CLObject type_object;
    BOOL result;

    klass = cl_get_class(class_name);

    MASSERT(klass != NULL);

    type_object = create_type_object(klass);

//    push_object(type_object, info);

    result = check_type_with_nullable(ovalue1, type_object, info);

//    pop_object_except_top(info);

    return result;
}

BOOL check_type_for_array(CLObject obj, char* generics_param_type, sVMInfo* info)
{
    CLObject type_object;
    CLObject type_object2;
    BOOL result;

    type_object = create_type_object(gArrayClass);

    push_object(type_object, info);

    type_object2 = create_type_object_with_class_name(generics_param_type);

    push_object(type_object2, info);

    CLTYPEOBJECT(type_object)->mGenericsTypesNum = 1;
    CLTYPEOBJECT(type_object)->mGenericsTypes[0] = type_object2;

    result = check_type_with_dynamic_typing(obj, type_object, info);

    if(result) {
        pop_object(info);
        pop_object(info);
    }
    else {
        pop_object_except_top(info);
        pop_object_except_top(info);
    }

    return result;
}

BOOL check_type_without_exception(CLObject ovalue1, CLObject type_object, sVMInfo* info)
{
    if(ovalue1 == 0) {
        return FALSE;
    }

    return substitution_posibility_of_type_object(type_object, CLOBJECT_HEADER(ovalue1)->mType, FALSE);
}

BOOL check_type_without_generics(CLObject ovalue1, CLObject type_object, sVMInfo* info, BOOL dynamic_typing)
{
    if(ovalue1 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }
    if(!substitution_posibility_of_type_object_without_generics(type_object, CLOBJECT_HEADER(ovalue1)->mType, dynamic_typing))
    {
        char buf1[1024];
        char buf2[1024];

        write_type_name_to_buffer(buf1, 1024, CLOBJECT_HEADER(ovalue1)->mType);
        write_type_name_to_buffer(buf2, 1024, type_object);

        entry_exception_object_with_class_name(info, "Exception", "This is %s type. But requiring type is %s.", buf1, buf2);
        return FALSE;
    }

    return TRUE;
}

BOOL check_type_with_dynamic_typing(CLObject ovalue1, CLObject type_object, sVMInfo* info)
{
    if(ovalue1 == 0) {
        entry_exception_object_with_class_name(info, "NullPointerException", "Null pointer exception");
        return FALSE;
    }
    if(!substitution_posibility_of_type_object(type_object, CLOBJECT_HEADER(ovalue1)->mType, TRUE))
    {
        char buf1[1024];
        char buf2[1024];

        write_type_name_to_buffer(buf1, 1024, CLOBJECT_HEADER(ovalue1)->mType);
        write_type_name_to_buffer(buf2, 1024, type_object);

        entry_exception_object_with_class_name(info, "Exception", "This is %s type. But requiring type is %s.", buf1, buf2);
        return FALSE;
    }

    return TRUE;
}

void show_type_object(CLObject type_object)
{
    int i;

    if(type_object == 0) return;

    if(CLTYPEOBJECT(type_object)->mGenericsTypesNum > 0) {
        printf("%s<", REAL_CLASS_NAME(CLTYPEOBJECT(type_object)->mClass));
    }
    else {
        printf("%s", REAL_CLASS_NAME(CLTYPEOBJECT(type_object)->mClass));
    }

    for(i=0; i<CLTYPEOBJECT(type_object)->mGenericsTypesNum; i++) {
        CLObject type_object2;

        type_object2 = CLTYPEOBJECT(type_object)->mGenericsTypes[i];
        show_type_object(type_object2);

        if(i != CLTYPEOBJECT(type_object)->mGenericsTypesNum-1) {
            printf(",");
        }
    }

    if(CLTYPEOBJECT(type_object)->mGenericsTypesNum > 0) {
        printf(">");
    }
}

static void write_type_name_to_buffer_core(char* buf, int size, CLObject type_object)
{
    xstrncat(buf, REAL_CLASS_NAME(CLTYPEOBJECT(type_object)->mClass), size);

    if(CLTYPEOBJECT(type_object)->mGenericsTypesNum > 0) {
        int i;

        xstrncat(buf, "<", size);
        for(i=0; i<CLTYPEOBJECT(type_object)->mGenericsTypesNum; i++) {
            write_type_name_to_buffer_core(buf, size, CLTYPEOBJECT(type_object)->mGenericsTypes[i]);
            if(i != CLTYPEOBJECT(type_object)->mGenericsTypesNum-1) {
                xstrncat(buf, ",", size);
            }
        }
        xstrncat(buf, ">", size);
    }
}

void write_type_name_to_buffer(char* buf, int size, CLObject type_object)
{
    *buf = 0;
    write_type_name_to_buffer_core(buf, size, type_object);
}

static void type_to_string_core(char* buf, int size, CLObject type_object)
{
    xstrncat(buf, REAL_CLASS_NAME(CLTYPEOBJECT(type_object)->mClass), size);

    if(CLTYPEOBJECT(type_object)->mGenericsTypesNum > 0) {
        int i;

        xstrncat(buf, "<", size);
        for(i=0; i<CLTYPEOBJECT(type_object)->mGenericsTypesNum; i++) {
            type_to_string_core(buf, size, CLTYPEOBJECT(type_object)->mGenericsTypes[i]);
            if(i != CLTYPEOBJECT(type_object)->mGenericsTypesNum-1) {
                xstrncat(buf, ",", size);
            }
        }
        xstrncat(buf, ">", size);
    }
}

BOOL Type_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    int i;

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        return FALSE;
    }

    *buf = 0;
    type_to_string_core(buf, 128, self);

    len = strlen(buf);

    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "failed to mbstowcs");
        return FALSE;
    }
    new_obj = create_string_object(wstr, len, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

static BOOL equals_core(CLObject left, CLObject right)
{
    int i;

    if(CLTYPEOBJECT(left)->mClass != CLTYPEOBJECT(right)->mClass) {
        return FALSE;
    }

    if(CLTYPEOBJECT(left)->mGenericsTypesNum != CLTYPEOBJECT(right)->mGenericsTypesNum) {
        return FALSE;
    }

    for(i=0; i<CLTYPEOBJECT(left)->mGenericsTypesNum; i++) {
        if(!equals_core(CLTYPEOBJECT(left)->mGenericsTypes[i], CLTYPEOBJECT(right)->mGenericsTypes[i]))
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL Type_equals(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject value;
    CLObject new_obj;
    BOOL result;

    self = lvar->mObjectValue.mValue; // self
    value = (lvar+1)->mObjectValue.mValue;      // value

    /// null check ///
    if(check_type_without_exception(self, gNullTypeObject, info) || check_type_without_exception(value, gNullTypeObject, info))
    {
        BOOL null_and_null;
        
        null_and_null = check_type_without_exception(self, gNullTypeObject, info) && check_type_without_exception(value, gNullTypeObject, info);

        (*stack_ptr)->mObjectValue.mValue = create_bool_object(null_and_null);  // push result
        (*stack_ptr)++;

        return TRUE;
    }

    if(!check_type(self, gTypeObject, info)) {
        return FALSE;
    }

    if(!check_type(value, gTypeObject, info)) {
        return FALSE;
    }

    result = equals_core(self, value);

    new_obj = create_bool_object(result);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Type_class(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        return FALSE;
    }

    new_obj = create_type_object(CLTYPEOBJECT(self)->mClass);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Type_genericsParam(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject index;
    int index_value;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        return FALSE;
    }

    index = (lvar+1)->mObjectValue.mValue;  // index

    if(!check_type(index, gIntTypeObject, info)) {
        return FALSE;
    }

    index_value = CLINT(index)->mValue;

    if(index_value < 0) { index_value += CLTYPEOBJECT(self)->mGenericsTypesNum; }

    if(index_value < 0 || index_value >= CLTYPEOBJECT(self)->mGenericsTypesNum)
    {
        entry_exception_object_with_class_name(info, "RangeException", "range exception");
        return FALSE;
    }

    new_obj = create_type_object_from_other_type_object(CLTYPEOBJECT(self)->mGenericsTypes[index_value], info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Type_parentClassNumber(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject new_obj;
    sCLClass* klass2;

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        return FALSE;
    }

    klass2 = CLTYPEOBJECT(self)->mClass;

    new_obj = create_int_object(klass2->mNumSuperClasses);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Type_parentClass(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject new_obj;
    sCLClass* klass2;

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        return FALSE;
    }

    klass2 = CLTYPEOBJECT(self)->mClass;

    if(klass2->mNumSuperClasses > 0) {
        sCLClass* super;
        char* real_class_name;

        real_class_name = CONS_str(&klass2->mConstPool, klass2->mSuperClasses[klass2->mNumSuperClasses-1].mClassNameOffset);

        super = cl_get_class(real_class_name);

        MASSERT(super != NULL);

        new_obj = create_type_object(super);
    }
    else {
        new_obj = create_null_object();
    }

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Type_genericsParamNumber(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject new_obj;

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        return FALSE;
    }

    new_obj = create_int_object(CLTYPEOBJECT(self)->mGenericsTypesNum);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Type_toClass(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject new_obj;
    CLObject type_object;
    sCLClass* type_class;

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        return FALSE;
    }

    type_class = cl_get_class("Class");

    MASSERT(type_class != NULL);

    type_object = create_type_object(type_class);

    new_obj = create_class_object(type_object, self);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Type_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject value;
    int i;

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(value, gTypeObject, info)) {
        return FALSE;
    }

    CLTYPEOBJECT(self)->mClass = CLTYPEOBJECT(value)->mClass;

    CLTYPEOBJECT(self)->mGenericsTypesNum = CLTYPEOBJECT(value)->mGenericsTypesNum;

    for(i=0; i<CLTYPEOBJECT(value)->mGenericsTypesNum; i++) {
        CLTYPEOBJECT(self)->mGenericsTypes[i] = CLTYPEOBJECT(value)->mGenericsTypes[i];
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL include_generics_param_type(CLObject type_object)
{
    int i;

    MASSERT(substitution_posibility_of_type_object(gTypeObject, CLOBJECT_HEADER(type_object)->mType, FALSE));

    if(is_generics_param_class(CLTYPEOBJECT(type_object)->mClass)) {
        return TRUE;
    }

    for(i=0; i<CLTYPEOBJECT(type_object)->mGenericsTypesNum; i++) {
        if(include_generics_param_type(CLTYPEOBJECT(type_object)->mGenericsTypes[i]))
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL Type_createFromString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject str;
    CLObject result;
    char* buf;

    str = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(str, "String", info)) {
        return FALSE;
    }

    if(!create_buffer_from_string_object(str, ALLOC &buf, info)) {
        MFREE(buf);
        return FALSE;
    }

    result = create_type_object_with_class_name_and_generics_name(buf, info);
    MFREE(buf);

    if(result == 0) {
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Type_substitutionPosibility(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type)
{
    CLObject self;
    CLObject value;
    CLObject dynamic_typing;
    int i;
    BOOL result;

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(value, gTypeObject, info)) {
        return FALSE;
    }

    dynamic_typing = (lvar+2)->mObjectValue.mValue;

    if(!check_type_with_class_name(dynamic_typing, "bool", info)) {
        return FALSE;
    }

    result = substitution_posibility_of_type_object(self, value, CLBOOL(dynamic_typing)->mValue);

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(result);
    (*stack_ptr)++;

    return TRUE;
}
