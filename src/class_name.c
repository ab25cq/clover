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

CLObject create_class_name_object(sCLNodeType type_)
{
    CLObject obj;
    int i;

    obj = alloc_class_name_object(gClassNameType.mClass);

    CLCLASSNAME(obj)->mType = type_;

    return obj;
}

static void show_classname_object(sVMInfo* info, CLObject obj)
{
    unsigned int obj_size_;
    int size;
    char* str;

    obj_size_ = object_size();
    VMLOG(info, "class name %s object size %d generics type num %d\n", REAL_CLASS_NAME(CLCLASSNAME(obj)->mType.mClass), obj_size_, CLCLASSNAME(obj)->mType.mGenericsTypesNum);
}

void initialize_hidden_class_method_of_class_name(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = show_classname_object;
    klass->mMarkFun = NULL;
    klass->mCreateFun = NULL;
}

BOOL ClassName_to_str(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue; // self

    len = snprintf(buf, 128, "%s<", REAL_CLASS_NAME(CLCLASSNAME(self)->mType.mClass));
    for(i=0; i<CLCLASSNAME(self)->mType.mGenericsTypesNum; i++) {
        len += snprintf(buf + len, 128-len, "%s", REAL_CLASS_NAME(CLCLASSNAME(self)->mType.mGenericsTypes[i]));
        if(i != CLCLASSNAME(self)->mType.mGenericsTypesNum-1) len += snprintf(buf + len, 128-len, ",");
    }
    len += snprintf(buf + len, 128-len, ">");

    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(info, gExConvertingStringCodeType.mClass, "failed to mbstowcs");
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_string_object(gStringType.mClass, wstr, len);

    (*stack_ptr)->mObjectValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

