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

static CLObject alloc_type_object(sCLClass* klass)
{
    CLObject obj;
    unsigned int size;

    size = object_size();
    obj = alloc_heap_mem(size, klass);

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

    CLTYPEOBJECT(object)->mGenericsTypesNum = ivalue2;

    for(i=0; i<ivalue2; i++) {
        CLObject object2;

        object2 = alloc_type_object(gTypeClass);
        push_object(object2, info);
        CLTYPEOBJECT(object)->mGenericsTypes[i] = object2;
        pop_object(info);

        if(!load_type_object_core(object2, pc, code, constant, info))
        {
            return FALSE;
        }
    }

    return TRUE;
}

// result: (0) --> class not found (non 0) --> created object
CLObject create_type_object(int** pc, sByteCode* code, sConst* constant, sVMInfo* info)
{
    CLObject obj;

    obj = alloc_type_object(gTypeClass);
    push_object(obj, info);

    if(!load_type_object_core(obj, pc, code, constant, info)) {
        pop_object(info);
        return 0;
    }
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

void initialize_hidden_class_method_of_type(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = mark_type_object;
    klass->mCreateFun = NULL;
}

