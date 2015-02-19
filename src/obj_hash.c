#include "clover.h"
#include "common.h"

static unsigned int items_object_size(int table_size)
{
    unsigned int size;

    size = sizeof(sCLHashData) - sizeof(sCLHashDataItem) * DUMMY_ARRAY_SIZE
                + sizeof(sCLHashDataItem) * table_size;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_hash_items(int mvalue_num)
{
    int item_heap_size;

    item_heap_size = items_object_size(mvalue_num);

    return alloc_heap_mem(item_heap_size, 0);
}

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLHash);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_hash_object(CLObject type_object, int mvalue_num, sVMInfo* info)
{
    int heap_size;
    CLObject obj;
    int item_heap_size;
    volatile CLObject items;

    heap_size = object_size();
    obj = alloc_heap_mem(heap_size, type_object);
    push_object(obj, info);

    CLHASH(obj)->mSize = mvalue_num;
    items = alloc_hash_items(mvalue_num);
    CLHASH(obj)->mData = items;

    memset(CLHASH_DATA(items)->mItems, 0, sizeof(sCLHashDataItem)*mvalue_num);

    pop_object(info);

    return obj;
}

static BOOL get_hash_value(CLObject key, sVMInfo* info, int* hash_value)
{
    sCLMethod* method;
    sCLClass* founded_class;
    CLObject type_object;
    CLObject result_value;

    type_object = CLOBJECT_HEADER(key)->mType;

    method = get_virtual_method_with_params(type_object, "hashValue", NULL, 0, &founded_class, FALSE, 0, 0, NULL, 0, info);

    if(method == NULL) {
        entry_exception_object(info, gExMethodMissingClass, "can't get \"hashValue\" method of %s", CLTYPEOBJECT(key)->mClass);
        return FALSE;
    }

    info->stack_ptr->mObjectValue.mValue = key;
    info->stack_ptr++;

    if(!cl_excute_method(method, CLTYPEOBJECT(type_object)->mClass, info, &result_value)) {
        return FALSE;
    }

    info->stack_ptr--;

    *hash_value = CLINT(result_value)->mValue;

    return TRUE;
}

static BOOL equalibility_of_key(CLObject left_key, CLObject right_key, sVMInfo* info, BOOL* result)
{
    sCLMethod* method;
    sCLClass* founded_class;
    CLObject params[CL_METHOD_PARAM_MAX];
    CLObject type_object;
    CLObject result_value;

    params[0] = CLOBJECT_HEADER(right_key)->mType;

    type_object = CLOBJECT_HEADER(left_key)->mType;

    method = get_virtual_method_with_params(type_object, "==", params, 1, &founded_class, FALSE, 0, 0, NULL, 0, info);

    if(method == NULL) {
        entry_exception_object(info, gExMethodMissingClass, "can't get \"==\" method of %s", CLTYPEOBJECT(left_key)->mClass);
        return FALSE;
    }

    info->stack_ptr->mObjectValue.mValue = left_key;
    info->stack_ptr++;
    info->stack_ptr->mObjectValue.mValue = right_key;
    info->stack_ptr++;

    if(!cl_excute_method(method, CLTYPEOBJECT(type_object)->mClass, info, &result_value)) 
    {
        return FALSE;
    }

    info->stack_ptr-=2;

    *result = CLBOOL(result_value)->mValue;

    return TRUE;
}

static BOOL add_item_to_hash(CLObject self, CLObject key, CLObject item, sVMInfo* info)
{
    int hash_value;
    CLObject items;
    sCLHashDataItem* p;

    items = CLHASH(self)->mData;

    if(!get_hash_value(key, info, &hash_value))
    {
        return FALSE;
    }

    hash_value = hash_value % CLHASH(self)->mSize;

    p = CLHASH_DATA(items)->mItems + hash_value;
    
    while(1) {
        if(p->mHashValue) {
            if(p->mHashValue == hash_value) {
                BOOL result;

                if(!equalibility_of_key(p->mKey, key, info, &result)) {
                    return FALSE;
                }

                if(result) {
                    p->mKey = key;
                    p->mItem = item;
                    return TRUE;
                }
            }

            p++;
            
            if(p == CLHASH_DATA(items)->mItems + CLHASH(self)->mSize) {
                p = CLHASH_DATA(items)->mItems;
            }
            else if(p == CLHASH_DATA(items)->mItems + hash_value) {
                return FALSE;
            }
        }
        else {
            p->mHashValue = hash_value;
            p->mKey = key;
            p->mItem = item;
            break;
        }
    }

    CLHASH(self)->mLen++;

    return TRUE;
}

static BOOL get_hash_item(CLObject self, CLObject key, CLObject* hash_item, sVMInfo* info)
{
    int hash_value;
    CLObject items;
    sCLHashDataItem* p;

    *hash_item = 0;

    items = CLHASH(self)->mData;

    if(!get_hash_value(key, info, &hash_value))
    {
        return FALSE;
    }

    hash_value %= CLHASH(self)->mSize;

    p = CLHASH_DATA(items)->mItems + hash_value;
    
    while(1) {
        if(p->mHashValue) {
            if(p->mHashValue == hash_value) {
                BOOL result;

                if(!equalibility_of_key(p->mKey, key, info, &result)) {
                    return FALSE;
                }

                if(result) {
                    *hash_item =  p->mItem;
                    return TRUE;
                }
            }

            p++;
            
            if(p == CLHASH_DATA(items)->mItems + CLHASH(self)->mSize) {
                p = CLHASH_DATA(items)->mItems;
            }
            else if(p == CLHASH_DATA(items)->mItems + hash_value) {
                break;
            }
        }
        else {
            break;
        }
    }

    *hash_item = 0;

    return TRUE;
}

BOOL create_hash_object(CLObject* obj, CLObject type_object, MVALUE keys[], MVALUE elements[], int num_elements, sVMInfo* info)
{
    MVALUE* data;

    const int mvalue_num = (num_elements + 1) * 3;

    *obj = alloc_hash_object(type_object, mvalue_num, info);

    CLHASH(*obj)->mLen = 0;

    if(num_elements > 0) {
        int i;

        push_object(*obj, info);

        for(i=0; i<num_elements; i++) {
            if(!add_item_to_hash(*obj, keys[i].mObjectValue.mValue, elements[i].mObjectValue.mValue, info))
            {
                pop_object_except_top(info);
                return FALSE;
            }
        }

        pop_object(info);
    }

    return TRUE;
}

static CLObject create_hash_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;

    (void)create_hash_object(&self, type_object, NULL, NULL, 0, info);
    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

static void mark_hash_object(CLObject object, unsigned char* mark_flg)
{
    int i;
    CLObject object2;

    object2 = CLHASH(object)->mData;
    mark_object(object2, mark_flg);

    for(i=0; i<CLHASH(object)->mSize; i++) {
        CLObject object3;
        unsigned int hash_value;

        hash_value = CLHASH_DATA(object2)->mItems[i].mHashValue;

        if(hash_value) {
            object3 = CLHASH_DATA(object2)->mItems[i].mKey;
            mark_object(object3, mark_flg);

            object3 = CLHASH_DATA(object2)->mItems[i].mItem;
            mark_object(object3, mark_flg);
        }
    }
}

void initialize_hidden_class_method_of_hash(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = mark_hash_object;
    klass->mCreateFun = create_hash_object_for_new;
}

BOOL Hash_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    return TRUE;
}

BOOL Hash_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    return TRUE;
}

BOOL Hash_add(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    return TRUE;
}

BOOL Hash_get(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject key_type_object;
    CLObject key;
    CLObject item;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_without_generics(self, gHashTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    key_type_object = CLTYPEOBJECT(CLOBJECT_HEADER(self)->mType)->mGenericsTypes[0];

    key = (lvar+1)->mObjectValue.mValue;
    if(!check_type(key, key_type_object, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(!get_hash_item(self, key, &item, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(item) {
        (*stack_ptr)->mObjectValue.mValue = item;
        (*stack_ptr)++;
    }
    else {
        (*stack_ptr)->mObjectValue.mValue = create_null_object();
        (*stack_ptr)++;
    }

    vm_mutex_unlock();

    return TRUE;
}

BOOL Hash_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_without_generics(self, gHashTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(CLHASH(self)->mLen);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Hash_setItem(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    return TRUE;
}
