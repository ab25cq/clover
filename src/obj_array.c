#include "clover.h"
#include "common.h"

static void show_array_object_to_stdout(CLObject obj)
{
}

static unsigned int items_object_size(int mvalue_num)
{
    unsigned int size;

    size = sizeof(sCLArrayItems) - sizeof(MVALUE) * DUMMY_ARRAY_SIZE + sizeof(MVALUE) * mvalue_num;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_array_items(int mvalue_num)
{
    int item_heap_size;

    item_heap_size = items_object_size(mvalue_num);

    return alloc_heap_mem(item_heap_size, 0);
}

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLArray);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_array_object(CLObject type_object, int mvalue_num, sVMInfo* info)
{
    int heap_size;
    CLObject obj;
    int item_heap_size;
    volatile CLObject items;

    heap_size = object_size();
    obj = alloc_heap_mem(heap_size, type_object);
    push_object(obj, info);

    CLARRAY(obj)->mSize = mvalue_num;
    items = alloc_array_items(mvalue_num);
    CLARRAY(obj)->mData = items;

    pop_object(info);

    return obj;
}

CLObject create_array_object_with_element_class_name(char* element_class_name, MVALUE elements[], int num_elements, sVMInfo* info)
{
    CLObject result;
    CLObject array_type_object;
    CLObject type_object;

    array_type_object = create_type_object_with_class_name("Array$1");
    push_object(array_type_object, info);

    type_object = create_type_object_with_class_name(element_class_name);
    CLTYPEOBJECT(array_type_object)->mGenericsTypes[0] = type_object;
    CLTYPEOBJECT(array_type_object)->mGenericsTypesNum = 1;

    result = create_array_object(array_type_object, elements, num_elements, info);
    
    pop_object(info);

    return result;
}

CLObject create_array_object(CLObject type_object, MVALUE elements[], int num_elements, sVMInfo* info)
{
    CLObject obj;
    MVALUE* data;

    const int mvalue_num = (num_elements + 1) * 2;

    obj = alloc_array_object(type_object, mvalue_num, info);

    CLARRAY(obj)->mLen = num_elements;

    if(num_elements > 0) {
        int j;

        data = CLARRAY_ITEMS(obj)->mItems;
        for(j=0; j<num_elements; j++) {
            data[j] = elements[j];
        }
    }

    return obj;
}

CLObject create_array_object2(CLObject type_object, CLObject elements[], int num_elements, sVMInfo* info)
{
    CLObject obj;
    MVALUE* data;

    const int mvalue_num = (num_elements + 1) * 2;

    obj = alloc_array_object(type_object, mvalue_num, info);

    CLARRAY(obj)->mLen = num_elements;

    if(num_elements > 0) {
        int j;

        data = CLARRAY_ITEMS(obj)->mItems;
        for(j=0; j<num_elements; j++) {
            data[j].mObjectValue.mValue = elements[j];
        }
    }

    return obj;
}

static CLObject create_array_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;

    self = create_array_object(type_object, NULL, 0, info);
    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

static void mark_array_object(CLObject object, unsigned char* mark_flg)
{
    int i;

    CLObject object2 = CLARRAY(object)->mData;
    mark_object(object2, mark_flg);

    for(i=0; i<CLARRAY(object)->mLen; i++) {
        CLObject object3 = CLARRAY_ITEMS2(object, i).mObjectValue.mValue;

        mark_object(object3, mark_flg);
    }
}

void add_to_array(CLObject self, CLObject item, sVMInfo* info)
{
    CLObject data;
    MVALUE* items;

    push_object(item, info);

    if(CLARRAY(self)->mLen >= CLARRAY(self)->mSize) {
        CLObject old_data;
        int new_mvalue_num;
        CLObject new_data;

        push_object(self, info);
        push_object(CLARRAY(self)->mData, info);
        
        new_mvalue_num = (CLARRAY(self)->mSize+1) * 2;
        old_data = CLARRAY(self)->mData;

        new_data = alloc_array_items(new_mvalue_num);

        memcpy(CLARRAY_DATA(new_data)->mItems, CLARRAY_DATA(old_data)->mItems, sizeof(MVALUE)*CLARRAY(self)->mLen);

        CLARRAY(self)->mData = new_data;
        CLARRAY(self)->mSize = new_mvalue_num;

        pop_object(info);
        pop_object(info);
    }

    pop_object(info);

    data = CLARRAY(self)->mData;
    CLARRAY_DATA(data)->mItems[CLARRAY(self)->mLen].mObjectValue.mValue = item;
    CLARRAY(self)->mLen++;
}

static void put_to_array(CLObject self, int index, CLObject item)
{
    CLObject data;

    data = CLARRAY(self)->mData;
    CLARRAY_DATA(data)->mItems[index].mObjectValue.mValue = item;
}

void initialize_hidden_class_method_of_array(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = mark_array_object;
    klass->mCreateFun = create_array_object_for_new;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gArrayClass = klass;
        gArrayTypeObject = create_type_object(gArrayClass);
    }
}

BOOL Array_add(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject item;
    CLObject item_type_object;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_without_generics(self, gArrayTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    item = (lvar+1)->mObjectValue.mValue;

    item_type_object = CLTYPEOBJECT(CLOBJECT_HEADER(self)->mType)->mGenericsTypes[0];

    /// check t ype with dynamic typing for anonymous type 
    if(!check_type_with_dynamic_typing(item, item_type_object, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    add_to_array(self, item, info);

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Array_items(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int index;
    CLObject ovalue1;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_without_generics(self, gArrayTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    ovalue1 = (lvar+1)->mObjectValue.mValue;
    if(!check_type(ovalue1, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    index = CLINT(ovalue1)->mValue;

    if(index < 0) { index += CLARRAY(self)->mLen; }

    if(index >= 0 && index < CLARRAY(self)->mLen) {
        (*stack_ptr)->mObjectValue.mValue = CLARRAY_ITEMS2(self, index).mObjectValue.mValue;
        (*stack_ptr)++;
    }
    else {
        (*stack_ptr)->mObjectValue.mValue = create_null_object();
        (*stack_ptr)++;
    }

    vm_mutex_unlock();

    return TRUE;
}

BOOL Array_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_without_generics(self, gArrayTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(CLARRAY(self)->mLen);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Array_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject data;
    int array_size;
    int array_len;
    CLObject value;
    int i;

    vm_mutex_lock();
    self = lvar->mObjectValue.mValue;

    if(!check_type_without_generics(self, gArrayTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_without_generics(value, gArrayTypeObject, info)) 
    {
        vm_mutex_unlock();
        return FALSE;
    }

    array_size = CLARRAY(value)->mSize;
    array_len = CLARRAY(value)->mLen;

    CLARRAY(self)->mSize = array_size;
    CLARRAY(self)->mLen = 0;
    data = alloc_array_items(array_size);
    CLARRAY(self)->mData = data;

    for(i=0; i<array_len; i++) {
        add_to_array(self, CLARRAY_ITEMS2(value, i).mObjectValue.mValue, info);
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL Array_setItem(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject index;
    CLObject item;
    CLObject new_obj;
    int index2;
    CLObject item_type_object;

    self = lvar->mObjectValue.mValue;           // self

    if(!check_type_without_generics(self, gArrayTypeObject, info)) {
        return FALSE;
    }

    index = (lvar+1)->mObjectValue.mValue;      // index

    if(!check_type(index, gIntTypeObject, info)) {
        return FALSE;
    }

    index2 = CLINT(index)->mValue;

    if(index2 < 0) { index2 += CLARRAY(self)->mLen; }
    if(index2 < 0) { index2 = 0; }

    if(index2 < CLARRAY(self)->mLen) {
        item = (lvar+2)->mObjectValue.mValue;       // item

        item_type_object = CLTYPEOBJECT(CLOBJECT_HEADER(self)->mType)->mGenericsTypes[0];

        // check type with dynamic typing for Array<anonymous>
        if(!check_type_with_dynamic_typing(item, item_type_object, info)) {
            return FALSE;
        }

        put_to_array(self, index2, item);
    }
    else {
        int count;
        int i;

        count = index2-CLARRAY(self)->mLen;
        for(i=0; i<count; i++) {
            CLObject null_object;

            null_object = create_null_object();

            add_to_array(self, null_object, info);
        }

        item = (lvar+2)->mObjectValue.mValue;       // item

        item_type_object = CLTYPEOBJECT(CLOBJECT_HEADER(self)->mType)->mGenericsTypes[0];

        // check type with dynamic typing for Array<anonymous>
        if(!check_type_with_dynamic_typing(item, item_type_object, info)) {
            return FALSE;
        }

        add_to_array(self, item, info);
    }

    return TRUE;
}

