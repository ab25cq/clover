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

static CLObject alloc_class_name_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    type_object = gClassNameTypeObject;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

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
CLObject create_class_name_object_from_bytecodes(int** pc, sByteCode* code, sConst* constant, sVMInfo* info)
{
    CLObject obj;
    sClassNameCore* class_name_core;

    if(!load_class_name_core(ALLOC &class_name_core, pc, code, constant, info)) {
        return 0;
    }

    obj = alloc_class_name_object();

    CLCLASSNAME(obj)->mType = MANAGED class_name_core;

    return obj;
}

static void load_class_name_from_type_object(sClassNameCore** class_name, CLObject type_object)
{
    int i;

    *class_name = CALLOC(1, sizeof(sClassNameCore));

    (*class_name)->mClass = CLTYPEOBJECT(type_object)->mClass;
    (*class_name)->mGenericsTypesNum = CLTYPEOBJECT(type_object)->mGenericsTypesNum;

    for(i=0; i<CLTYPEOBJECT(type_object)->mGenericsTypesNum; i++) {
        load_class_name_from_type_object(&(*class_name)->mGenericsTypes[i], CLTYPEOBJECT(type_object)->mGenericsTypes[i]);
    }
}

// result: (non 0) --> created object
CLObject create_class_name_object(CLObject type_object)
{
    CLObject obj;
    sClassNameCore* class_name_core;

    load_class_name_from_type_object(ALLOC &class_name_core, type_object);

    obj = alloc_class_name_object();

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

static class_name_to_string_core(char* buf, int size, sClassNameCore* class_name)
{
    xstrncat(buf, REAL_CLASS_NAME(class_name->mClass), size);

    if(class_name->mGenericsTypesNum > 0) {
        int i;

        xstrncat(buf, "<", size);
        for(i=0; i<class_name->mGenericsTypesNum; i++) {
            class_name_to_string_core(buf, size, class_name->mGenericsTypes[i]);
            if(i != class_name->mGenericsTypesNum-1) {
                xstrncat(buf, ",", size);
            }
        }
        xstrncat(buf, ">", size);
    }
}

BOOL ClassName_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gClassNameTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    *buf = 0;
    class_name_to_string_core(buf, 128, CLCLASSNAME(self)->mType);

    len = strlen(buf);

    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "failed to mbstowcs");
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_string_object(wstr, len);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

static BOOL equals_core(sClassNameCore* left, sClassNameCore* right)
{
    int i;

    if(left->mClass != right->mClass) {
        return FALSE;
    }

    if(left->mGenericsTypesNum != right->mGenericsTypesNum) {
        return FALSE;
    }

    for(i=0; i<left->mGenericsTypesNum; i++) {
        if(!equals_core(left->mGenericsTypes[i],right->mGenericsTypes[i]))
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL ClassName_equals(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject value;
    CLObject new_obj;
    BOOL result;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self
    value = (lvar+1)->mObjectValue.mValue;      // value

    if(!check_type(self, gClassNameTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(!check_type(value, gClassNameTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    result = equals_core(CLCLASSNAME(self)->mType, CLCLASSNAME(value)->mType);

    new_obj = create_bool_object(result);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

