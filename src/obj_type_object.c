#include "clover.h"
#include "common.h"
#include <limits.h>
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
        entry_exception_object(info, gExClassNotFoundClass, "can't get a class named %s\n", real_class_name);
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

// result: (0) --> class not found (non 0) --> created object
CLObject create_type_object_from_bytecodes(int** pc, sByteCode* code, sConst* constant, sVMInfo* info)
{
    CLObject obj;

    obj = alloc_type_object();
    push_object(obj, info);

    if(!load_type_object_core(obj, pc, code, constant, info)) {
        pop_object(info);
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
    klass->mCreateFun = NULL;
}

static CLObject get_type_object_from_cl_type_core(sCLType* cl_type, sCLClass* klass, sVMInfo* info)
{
    int i;
    char* real_class_name;
    CLObject result;
    sCLClass* klass2;

    real_class_name = CONS_str(&klass->mConstPool, cl_type->mClassNameOffset);

    klass2 = cl_get_class(real_class_name);

    ASSERT(klass2 != NULL);

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

static BOOL solve_generics_types_of_type_object_core(CLObject type_object, ALLOC CLObject* solved_type_object, CLObject type_, sVMInfo* info)
{
    int i;

    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
        if(CLTYPEOBJECT(type_object)->mClass == gAnonymousClass[i]) {
            if(i < CLTYPEOBJECT(type_)->mGenericsTypesNum) {
                CLObject generics_type;
                sCLClass* klass;

                generics_type = CLTYPEOBJECT(type_)->mGenericsTypes[i];
                klass = CLTYPEOBJECT(generics_type)->mClass;

                *solved_type_object = ALLOC create_type_object(klass);
                return TRUE;
            }
            else {
                *solved_type_object = ALLOC create_type_object(CLTYPEOBJECT(type_object)->mClass);
                return FALSE;
            }
        }
    }

    *solved_type_object = ALLOC create_type_object(CLTYPEOBJECT(type_object)->mClass);

    return TRUE;
}

BOOL solve_generics_types_of_type_object_core2(CLObject type_object, ALLOC CLObject* solved_type_object, CLObject type_, sVMInfo* info)
{
    int i;

    if(!solve_generics_types_of_type_object_core(type_object, ALLOC solved_type_object, type_, info))
    {
        return FALSE;
    }

    push_object(*solved_type_object, info);

    CLTYPEOBJECT(*solved_type_object)->mGenericsTypesNum = 0;

    for(i=0; i<CLTYPEOBJECT(type_object)->mGenericsTypesNum; i++) {
        CLObject object;
        CLObject object2;

        object = CLTYPEOBJECT(type_object)->mGenericsTypes[i];

        if(!solve_generics_types_of_type_object_core2(object, ALLOC &object2, type_, info))
        {
            pop_object(info);
            return FALSE;
        }

        CLTYPEOBJECT(*solved_type_object)->mGenericsTypes[i] = object2;
        CLTYPEOBJECT(*solved_type_object)->mGenericsTypesNum++;
    }
    pop_object(info);


    return TRUE;
}

BOOL solve_generics_types_of_type_object(CLObject type_object, ALLOC CLObject* solved_type_object, CLObject type_, sVMInfo* info)
{
    return solve_generics_types_of_type_object_core2(type_object, ALLOC solved_type_object, type_, info);
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

    if(!solve_generics_types_of_type_object(super_type_object, &solved_super_type_object, type_object, info))
    {
        return 0;
    }

    return solved_super_type_object;
}

static void show_type_object(CLObject type_object)
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
