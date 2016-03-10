#include "clover.h"
#include "common.h"

static unsigned int object_size(sCLClass* klass, int fields_num)
{
    unsigned int size;

    size = sizeof(sCLUserObject) - sizeof(MVALUE) * DUMMY_ARRAY_SIZE;
    size += (unsigned int)sizeof(MVALUE) * fields_num;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

// result (TRUE): success (FALSE): threw exception
BOOL create_user_object(CLObject type_object, CLObject* obj, CLObject vm_type, MVALUE* fields, int num_fields, sVMInfo* info)
{
    unsigned int size;
    sCLClass* klass;
    int i;
    int num_fields2;

    klass = CLTYPEOBJECT(type_object)->mClass;

    MASSERT(klass != NULL);

    num_fields2 = klass->mSumOfNoneClassFields;
    size = object_size(klass, num_fields2);

    *obj = alloc_heap_mem(size, type_object);

    push_object(*obj, info);

    CLUSEROBJECT(*obj)->mNumFields = num_fields2;

    /// run initializers of fields ///
    if(!run_fields_initializer(*obj, klass, vm_type)) {
        pop_object(info);
        return FALSE;
    }

    if(num_fields2 > 0) {
        for(i=0; i<num_fields2 && i<num_fields; i++) {
            CLUSEROBJECT(*obj)->mFields[i] = fields[i];
        }

        CLUSEROBJECT(*obj)->mNumFields = num_fields2;
    }

    pop_object(info);

    return TRUE;
}

BOOL create_user_object_with_class_name(char* class_name, CLObject* obj, CLObject vm_type, sVMInfo* info)
{
    CLObject type_object;

    type_object = create_type_object_with_class_name(class_name);

    push_object(type_object, info);

    if(!create_user_object(type_object, obj, vm_type, NULL, 0, info)) {
        pop_object_except_top(info);
        return FALSE;
    }

    pop_object(info);

    return TRUE;
}

static void mark_user_object(CLObject object, unsigned char* mark_flg)
{
    int i;
    CLObject type_object;
    sCLClass* klass;

    type_object = CLOBJECT_HEADER(object)->mType;
    klass = CLTYPEOBJECT(type_object)->mClass;

    for(i=0; i<CLUSEROBJECT(object)->mNumFields; i++) {
        CLObject obj2;

        obj2 = CLUSEROBJECT(object)->mFields[i].mObjectValue.mValue;

        mark_object(obj2, mark_flg);
    }
}

static void show_user_object(sVMInfo* info, CLObject obj)
{
    int j;
    sCLClass* klass;
    CLObject type_object;

    type_object = CLOBJECT_HEADER(obj)->mType;

    klass = CLTYPEOBJECT(type_object)->mClass;

    VMLOG(info, " class name (%s)\n", REAL_CLASS_NAME(klass));

    for(j=0; j<CLUSEROBJECT(obj)->mNumFields; j++) {
        VMLOG(info, " field#%d %d\n", j, CLUSEROBJECT(obj)->mFields[j].mObjectValue);
    }
}

void initialize_hidden_class_method_of_user_object(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = show_user_object;
    klass->mMarkFun = mark_user_object;
    klass->mCreateFun = NULL;

    if(strcmp(REAL_CLASS_NAME(klass), "Object") == 0) {
        gObjectClass = klass;
    }
    else if(strlen(REAL_CLASS_NAME(klass)) == 14 && strstr(REAL_CLASS_NAME(klass), "GenericsParam") == REAL_CLASS_NAME(klass) && REAL_CLASS_NAME(klass)[13] >= '0' && REAL_CLASS_NAME(klass)[13] <= '7') 
    {
        int number;

        number = (REAL_CLASS_NAME(klass)[13] - '0');

        MASSERT(number >= 0 && number < CL_GENERICS_CLASS_PARAM_MAX);

        gGParamClass[number] = klass;
        klass->mFlags |= CLASS_FLAGS_GENERICS_PARAM;
    }
}

BOOL Object_type(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;
    CLObject type_object;

    self = lvar->mObjectValue.mValue;

    type_object = CLOBJECT_HEADER(self)->mType;

    new_obj = create_type_object_from_other_type_object(type_object, info);
    /// This should be cloned object because object type parametor should not be changed 

    (*stack_ptr)->mObjectValue.mValue = new_obj;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Object_setType(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject type_object;

    self = lvar->mObjectValue.mValue;

    type_object = (lvar+1)->mObjectValue.mValue;

    if(!check_type(type_object, gTypeObject, info)) {
        return FALSE;
    }

    CLOBJECT_HEADER(self)->mType = type_object;

    return TRUE;
}

BOOL Object_ID(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    self = lvar->mObjectValue.mValue;

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)self);
    (*stack_ptr)++;

    return TRUE;
}

BOOL Object_fields(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject number;
    CLObject type_object;
    sCLClass* klass2;
    CLObject result;

    self = lvar->mObjectValue.mValue;

    number = (lvar+1)->mObjectValue.mValue;

    if(!check_type(number, gIntTypeObject, info)) {
        return FALSE;
    }

    type_object = CLOBJECT_HEADER(self)->mType;

    klass2 = CLTYPEOBJECT(type_object)->mClass;

    if(klass2->mFlags & CLASS_FLAGS_NATIVE)
    {
        entry_exception_object_with_class_name(info, "Exception", "This object was created by native class");
        return FALSE;
    }
    else {
        int num_fields;

        num_fields = CLUSEROBJECT(self)->mNumFields;

        if(CLINT(number)->mValue < 0 || CLINT(number)->mValue >= num_fields) {
            entry_exception_object_with_class_name(info, "RangeException", "Range exception");
            return FALSE;
        }

        result = CLUSEROBJECT(self)->mFields[CLINT(number)->mValue].mObjectValue.mValue;
    }

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Object_setField(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject number;
    CLObject object;
    CLObject type_object;
    sCLClass* klass2;
    CLObject result;

    self = lvar->mObjectValue.mValue;

    number = (lvar+1)->mObjectValue.mValue;

    if(!check_type(number, gIntTypeObject, info)) {
        return FALSE;
    }

    object = (lvar+2)->mObjectValue.mValue;

    type_object = CLOBJECT_HEADER(self)->mType;

    klass2 = CLTYPEOBJECT(type_object)->mClass;

    if(klass2->mFlags & CLASS_FLAGS_NATIVE)
    {
        entry_exception_object_with_class_name(info, "Exception", "This object was created by native class");
        return FALSE;
    }
    else {
        int num_fields;
        CLObject object2;
        int index;

        num_fields = CLUSEROBJECT(self)->mNumFields;

        index = CLINT(number)->mValue;

        if(index < 0 || index >= num_fields) {
            entry_exception_object_with_class_name(info, "RangeException", "Range exception");
            return FALSE;
        }

        object2 = CLUSEROBJECT(self)->mFields[index].mObjectValue.mValue;

        if(!check_type_with_dynamic_typing(object, CLOBJECT_HEADER(object2)->mType, info)) {
            return FALSE;
        }

        CLUSEROBJECT(self)->mFields[index].mObjectValue.mValue = object;
    }

    (*stack_ptr)->mObjectValue.mValue = object;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Object_numFields(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject type_object;
    sCLClass* klass2;
    int num_fields;

    self = lvar->mObjectValue.mValue;
    type_object = CLOBJECT_HEADER(self)->mType;
    klass2 = CLTYPEOBJECT(type_object)->mClass;

    if(klass2->mFlags & CLASS_FLAGS_NATIVE)
    {
        num_fields = 0;
    }
    else {
        num_fields = CLUSEROBJECT(self)->mNumFields;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(num_fields);
    (*stack_ptr)++;

    return TRUE;
}

