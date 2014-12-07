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
        CLTYPEOBJECT(object)->mGenericsTypesNum++;
        pop_object(info);

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

static void load_type_object_from_cl_type_core(CLObject object, sCLClass* klass, sCLType* cl_type, sVMInfo* info)
{
    char* real_class_name;
    int i;
    sCLClass* klass2;

    real_class_name = CONS_str(&klass->mConstPool, cl_type->mClassNameOffset);
    klass2 = cl_get_class(real_class_name);

    ASSERT(klass2 != NULL);

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

BOOL substitution_posibility_of_type_object(CLObject left_type, CLObject right_type)
{
    int i;
    sCLClass* left_class;
    sCLClass* right_class;

    ASSERT(left_type != 0);
    ASSERT(right_type != 0);

    left_class = CLTYPEOBJECT(left_type)->mClass;
    right_class = CLTYPEOBJECT(right_type)->mClass;

    /// anonymous is special ///
    if(left_class == gDAnonymousClass || right_class == gDAnonymousClass) 
    {
        return TRUE;
    }
    /// Null class is special ///
    else if(left_class == gNullClass || right_class == gNullClass) {
        return TRUE;
    }
    else {
        int i;

        if(left_class != right_class) {
            int i;

            if(!search_for_super_class(right_class, left_class) && !search_for_implemeted_interface(right_class, left_class)) 
            {
                return FALSE;
            }

            if(CLTYPEOBJECT(left_type)->mGenericsTypesNum != CLTYPEOBJECT(right_type)->mGenericsTypesNum)
            {
                return FALSE;
            }

            for(i=0; i<CLTYPEOBJECT(left_type)->mGenericsTypesNum; i++) {
                if(!substitution_posibility_of_type_object(CLTYPEOBJECT(left_type)->mGenericsTypes[i], CLTYPEOBJECT(right_type)->mGenericsTypes[i]))
                {
                    return FALSE;
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
                if(!substitution_posibility_of_type_object(CLTYPEOBJECT(left_type)->mGenericsTypes[i], CLTYPEOBJECT(right_type)->mGenericsTypes[i]))
                {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

BOOL substitution_posibility_of_type_object_without_generics(CLObject left_type, CLObject right_type)
{
    int i;
    sCLClass* left_class;
    sCLClass* right_class;

    ASSERT(left_type != 0);
    ASSERT(right_type != 0);

    left_class = CLTYPEOBJECT(left_type)->mClass;
    right_class = CLTYPEOBJECT(right_type)->mClass;

    /// anonymous is special ///
    if(left_class == gDAnonymousClass || right_class == gDAnonymousClass) 
    {
        return TRUE;
    }
    /// Null class is special ///
    else if(left_class == gNullClass || right_class == gNullClass) {
        return TRUE;
    }
    else {
        int i;

        if(left_class != right_class) {
            int i;

            if(!search_for_super_class(right_class, left_class) && !search_for_implemeted_interface(right_class, left_class)) 
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


static type_to_string_core(char* buf, int size, CLObject type_object)
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

BOOL Type_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    char buf[128];
    int len;
    wchar_t wstr[128];
    CLObject new_obj;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    *buf = 0;
    type_to_string_core(buf, 128, self);

    len = strlen(buf);

    if((int)mbstowcs(wstr, buf, len+1) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "failed to mbstowcs");
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_string_object(wstr, len, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

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

BOOL Type_equals(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject value;
    CLObject new_obj;
    BOOL result;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self
    value = (lvar+1)->mObjectValue.mValue;      // value

    if(!check_type(self, gTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(!check_type(value, gTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    result = equals_core(self, value);

    new_obj = create_bool_object(result);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Type_class(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    new_obj = create_type_object(CLTYPEOBJECT(self)->mClass);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Type_genericsParam(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject index;
    int index_value;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    index = (lvar+1)->mObjectValue.mValue;  // index

    if(!check_type(index, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    index_value = CLINT(index)->mValue;

    if(index_value < 0) { index_value += CLTYPEOBJECT(self)->mGenericsTypesNum; }

    if(index_value < 0 || index_value >= CLTYPEOBJECT(self)->mGenericsTypesNum)
    {
        entry_exception_object(info, gExRangeClass, "range exception");
        vm_mutex_unlock();
        return FALSE;
    }

    new_obj = create_type_object_from_other_type_object(CLTYPEOBJECT(self)->mGenericsTypes[index_value], info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Type_parentClassNumber(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject new_obj;
    sCLClass* klass;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass = CLTYPEOBJECT(self)->mClass;

    new_obj = create_int_object(klass->mNumSuperClasses);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Type_parentClass(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject new_obj;
    sCLClass* klass;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    klass = CLTYPEOBJECT(self)->mClass;

    if(klass->mNumSuperClasses > 0) {
        sCLClass* super;
        char* real_class_name;

        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[klass->mNumSuperClasses-1].mClassNameOffset);

        super = cl_get_class(real_class_name);

        ASSERT(super != NULL);

        new_obj = create_type_object(super);
    }
    else {
        new_obj = create_null_object();
    }

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Type_genericsParamNumber(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    new_obj = create_int_object(CLTYPEOBJECT(self)->mGenericsTypesNum);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

