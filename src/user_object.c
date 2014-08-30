#include "clover.h"
#include "common.h"

static unsigned int object_size(sCLClass* klass)
{
    unsigned int size;
    int fields_num;

    fields_num = get_field_num_including_super_classes_without_class_field(klass);
    size = sizeof(sCLUserObject) - sizeof(MVALUE) * DUMMY_ARRAY_SIZE;
    size += (unsigned int)sizeof(MVALUE) * fields_num;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

// result (TRUE): success (FALSE): threw exception
BOOL create_user_object(sCLClass* klass, CLObject* obj)
{
    unsigned int size;

    ASSERT(klass != NULL);

    size = object_size(klass);

    *obj = alloc_heap_mem(size, klass);

    /// run initializers of fields ///
    if(!run_fields_initializer(*obj, klass)) {
        return FALSE;
    }

    return TRUE;
}

static void mark_user_object(CLObject object, unsigned char* mark_flg)
{
    int i;

    for(i=0; i<get_field_num_including_super_classes(CLOBJECT_HEADER(object)->mClass); i++) {
        CLObject obj2;

        obj2 = CLUSEROBJECT(object)->mFields[i].mObjectValue;

        mark_object(obj2, mark_flg);
    }
}

static void show_user_object(sVMInfo* info, CLObject obj)
{
    int j;
    sCLClass* klass;

    klass = CLOBJECT_HEADER(obj)->mClass;

    VMLOG(info, " class name (%s)\n", REAL_CLASS_NAME(klass));

    for(j=0; j<get_field_num_including_super_classes(klass); j++) {
        VMLOG(info, " field#%d %d\n", j, CLUSEROBJECT(obj)->mFields[j].mIntValue);
    }
}

void initialize_hidden_class_method_of_user_object(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = show_user_object;
    klass->mMarkFun = mark_user_object;
    klass->mCreateFun = NULL;
}

BOOL Object_class_name(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    sCLClass* klass;
    CLObject new_obj;
    char buf[128];
    wchar_t wstr[128];
    int len;

    vm_mutex_lock();

    self = lvar->mObjectValue;

    klass = CLOBJECT_HEADER(self)->mClass;

    if(klass) {
        len = snprintf(buf, 128, "%s", REAL_CLASS_NAME(klass));
        if((int)mbstowcs(wstr, buf, len+1) < 0) {
            entry_exception_object(info, gExConvertingStringCodeClass, "error mbstowcs on converting string");
            vm_mutex_unlock();
            return FALSE;
        }
    }
    else {
        len = snprintf(buf, 128, "no class of this object");
        if((int)mbstowcs(wstr, buf, len+1) < 0) {
            entry_exception_object(info, gExConvertingStringCodeClass, "error mbstowcs on converting string");
            vm_mutex_unlock();
            return FALSE;
        }
    }

    new_obj = create_string_object(gStringClass, wstr, len);

    (*stack_ptr)->mObjectValue = new_obj;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Object_instanceof(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject class_name;
    sCLClass* klass;
    char buf[128];

    vm_mutex_lock();

    self = lvar->mObjectValue;
    class_name = (lvar+1)->mObjectValue;

    klass = CLOBJECT_HEADER(self)->mClass;

    (*stack_ptr)->mIntValue = (klass == CLCLASSNAME(class_name)->mType->mClass);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Object_is_child(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject class_name;
    sCLClass* klass;
    char buf[128];

    vm_mutex_lock();

    self = lvar->mObjectValue;
    class_name = (lvar+1)->mObjectValue;

    klass = CLOBJECT_HEADER(self)->mClass;

    (*stack_ptr)->mIntValue = substitution_posibility_of_class(CLCLASSNAME(class_name)->mType->mClass, klass);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

