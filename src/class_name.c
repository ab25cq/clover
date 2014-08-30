#include "clover.h"
#include "common.h"
#include <limits.h>
#include <wchar.h>

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLClassName);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_class_name_object(sCLClass* klass)
{
    CLObject obj;
    unsigned int size;

    size = object_size();
    obj = alloc_heap_mem(size, klass);

    return obj;
}

static BOOL load_class_name_core(sClassNameCore** class_name, int** pc, sByteCode* code, sConst* constant, sVMInfo* info)
{
    int ivalue1, ivalue2;
    char* real_class_name;
    sCLClass* klass;
    int i;

    *class_name = CALLOC(1, sizeof(sClassNameCore));

    ivalue1 = **pc;          // offset
    (*pc)++;

    real_class_name = CONS_str(constant, ivalue1);
    klass = cl_get_class(real_class_name);

    if(klass == NULL) {
        entry_exception_object(info, gExClassNotFoundClass, "can't get a class named %s\n", real_class_name);
        FREE(*class_name);
        return FALSE;
    }

    (*class_name)->mClass = klass;

    ivalue2 = **pc;          // num gneric types
    (*pc)++;

    (*class_name)->mGenericsTypesNum = ivalue2;

    for(i=0; i<ivalue2; i++) {
        if(!load_class_name_core(&(*class_name)->mGenericsTypes[i], pc, code, constant, info))
        {
            FREE(*class_name);
            return FALSE;
        }
    }

    return TRUE;
}

// result: (0) --> class not found (non 0) --> created object
CLObject create_class_name_object(int** pc, sByteCode* code, sConst* constant, sVMInfo* info)
{
    CLObject obj;
    sClassNameCore* class_name_core;

    if(!load_class_name_core(ALLOC &class_name_core, pc, code, constant, info)) {
        return 0;
    }

    obj = alloc_class_name_object(gClassNameClass);

    CLCLASSNAME(obj)->mType = MANAGED class_name_core;

    return obj;
}

static void show_classname_object(sVMInfo* info, CLObject obj)
{
    unsigned int obj_size_;
    int size;
    char* str;

    obj_size_ = object_size();
    VMLOG(info, "class name %s object size %d generics type num %d\n", REAL_CLASS_NAME(CLCLASSNAME(obj)->mType->mClass), obj_size_, CLCLASSNAME(obj)->mType->mGenericsTypesNum);
}

static void free_class_name_core(sClassNameCore* self)
{
    int i;

    for(i=0; i<self->mGenericsTypesNum; i++) {
        free_class_name_core(self->mGenericsTypes[i]);
    }

    FREE(self);
}

static void free_class_name_object(CLObject obj)
{
    free_class_name_core(CLCLASSNAME(obj)->mType);
}

void initialize_hidden_class_method_of_class_name(sCLClass* klass)
{
    klass->mFreeFun = free_class_name_object;
    klass->mShowFun = show_classname_object;
    klass->mMarkFun = NULL;
    klass->mCreateFun = NULL;
}

BOOL ClassName_to_string(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue; // self

    len = snprintf(buf, 128, "%s<", REAL_CLASS_NAME(CLCLASSNAME(self)->mType->mClass));
    for(i=0; i<CLCLASSNAME(self)->mType->mGenericsTypesNum; i++) {
        len += snprintf(buf + len, 128-len, "%s", REAL_CLASS_NAME(CLCLASSNAME(self)->mType->mGenericsTypes[i]->mClass));
        if(i != CLCLASSNAME(self)->mType->mGenericsTypesNum-1) len += snprintf(buf + len, 128-len, ",");
    }
    len += snprintf(buf + len, 128-len, ">");

    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "failed to mbstowcs");
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_string_object(gStringClass, wstr, len);

    (*stack_ptr)->mObjectValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

