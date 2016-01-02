#include "clover.h"
#include "common.h"

static unsigned int items_object_size(int table_size)
{
    unsigned int size;

    size = sizeof(sCLHashData) - sizeof(sCLHashDataItem) * DUMMY_ARRAY_SIZE + sizeof(sCLHashDataItem) * table_size;

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
        entry_exception_object_with_class_name(info, "MethodMissingException", "can't get \"hashValue\" method of %s", CLTYPEOBJECT(key)->mClass);
        return FALSE;
    }

    info->stack_ptr->mObjectValue.mValue = key;
    info->stack_ptr++;

    if(!cl_excute_method(method, CLTYPEOBJECT(type_object)->mClass, CLTYPEOBJECT(type_object)->mClass, info, &result_value)) 
    {
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
        entry_exception_object_with_class_name(info, "MethodMissingException", "can't get \"==\" method of %s", CLTYPEOBJECT(left_key)->mClass);
        return FALSE;
    }

    info->stack_ptr->mObjectValue.mValue = left_key;
    info->stack_ptr++;
    info->stack_ptr->mObjectValue.mValue = right_key;
    info->stack_ptr++;

    if(!cl_excute_method(method, CLTYPEOBJECT(type_object)->mClass, CLTYPEOBJECT(type_object)->mClass, info, &result_value)) 
    {
        return FALSE;
    }

    info->stack_ptr-=2;

    *result = CLBOOL(result_value)->mValue;

    return TRUE;
}

static BOOL rehash(CLObject self, sVMInfo* info, int new_size) 
{
    int i;
    CLObject new_data;
    CLObject old_data;
    CLObject self_clone_hash;
    CLObject self_clone_hash_data;
    MVALUE* keys;
    MVALUE* elements;
    int num_elements;

    /// make clone of self ///
    keys = CALLOC(1, sizeof(MVALUE)*CLHASH(self)->mLen);
    elements = CALLOC(1, sizeof(MVALUE)*CLHASH(self)->mLen);

    old_data = CLHASH(self)->mData;
    
    num_elements = 0;

    for(i=0; i<CLHASH(self)->mSize; i++) {
        int hash_value;

        hash_value = CLHASH_DATA(old_data)->mItems[i].mHashValue;

        if(hash_value) {
            CLObject key;
            CLObject item;

            key = CLHASH_DATA(old_data)->mItems[i].mKey;
            item = CLHASH_DATA(old_data)->mItems[i].mItem;

            keys[num_elements].mObjectValue.mValue = key;
            elements[num_elements].mObjectValue.mValue = item;
            num_elements++;
        }
    }

    if(!create_hash_object(&self_clone_hash, CLOBJECT_HEADER(self)->mType, keys, elements, CLHASH(self)->mLen, info)) {
        FREE(keys);
        FREE(elements);
        return FALSE;
    }

    FREE(keys);
    FREE(elements);

    push_object(self_clone_hash, info);

    /// make new self ///
    ASSERT(new_size > CLHASH(self)->mSize);

    CLHASH(self)->mSize = new_size;
    new_data = alloc_hash_items(new_size);
    CLHASH(self)->mData = new_data;
    CLHASH(self)->mLen = 0;

    memset(CLHASH_DATA(new_data)->mItems, 0, sizeof(sCLHashDataItem)*new_size);

    self_clone_hash_data = CLHASH(self_clone_hash)->mData;

    for(i=0; i<CLHASH(self_clone_hash)->mSize; i++) {
        int hash_value;

        hash_value = CLHASH_DATA(self_clone_hash_data)->mItems[i].mHashValue;

        if(hash_value) {
            CLObject key;
            CLObject item;

            key = CLHASH_DATA(self_clone_hash_data)->mItems[i].mKey;
            item = CLHASH_DATA(self_clone_hash_data)->mItems[i].mItem;
            if(!add_item_to_hash(self, key, item, info)) {
                pop_object_except_top(info);
                return FALSE;
            }
        }
    }

    pop_object(info);

    return TRUE;
}

BOOL add_item_to_hash(CLObject self, CLObject key, CLObject item, sVMInfo* info)
{
    int hash_value;
    int hash_value2;
    CLObject items;
    sCLHashDataItem* p;

    if(CLHASH(self)->mSize < CLHASH(self)->mLen * 3) {
        if(!rehash(self, info, CLHASH(self)->mSize * 3)) {
            return FALSE;
        }
    }

    items = CLHASH(self)->mData;

    if(!get_hash_value(key, info, &hash_value))
    {
        return FALSE;
    }

    hash_value2 = hash_value % CLHASH(self)->mSize;

    p = CLHASH_DATA(items)->mItems + hash_value2;

    while(1) {
        if(p->mHashValue) {
            if((p->mHashValue % CLHASH(self)->mSize) == hash_value2) {
                BOOL result;
                int p_offset;

                p_offset = p - CLHASH_DATA(items)->mItems;

                if(!equalibility_of_key(p->mKey, key, info, &result)) {
                    return FALSE;
                }

                p = CLHASH_DATA(items)->mItems + p_offset; // In this point p is invalid address because running GC on equalibility_of_key

                if(result) {
                    p->mHashValue = hash_value;
                    p->mKey = key;
                    p->mItem = item;
                    return TRUE;
                }
            }

            p++;
            
            if(p == CLHASH_DATA(items)->mItems + CLHASH(self)->mSize) {
                p = CLHASH_DATA(items)->mItems;
            }
            else if(p == CLHASH_DATA(items)->mItems + hash_value2) {
                entry_exception_object_with_class_name(info, "Exception", "require to rehash to add key and item to hash");
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

// if there is not the item of key, this set 0 on hash_item
static BOOL get_hash_item(CLObject self, CLObject key, CLObject* hash_item, sVMInfo* info)
{
    int hash_value;
    int hash_value2;
    CLObject items;
    sCLHashDataItem* p;

    *hash_item = 0;

    items = CLHASH(self)->mData;

    if(!get_hash_value(key, info, &hash_value))
    {
        return FALSE;
    }

    hash_value2 = hash_value % CLHASH(self)->mSize;

    p = CLHASH_DATA(items)->mItems + hash_value2;
    
    while(1) {
        if(p->mHashValue) {
            if((p->mHashValue % CLHASH(self)->mSize) == hash_value2) {
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
            else if(p == CLHASH_DATA(items)->mItems + hash_value2) {
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

static BOOL erase_item(CLObject self, CLObject key, sVMInfo* info)
{
    int hash_value;
    int hash_value2;
    CLObject items;
    sCLHashDataItem* p;

    items = CLHASH(self)->mData;

    if(!get_hash_value(key, info, &hash_value))
    {
        return FALSE;
    }

    hash_value2 = hash_value % CLHASH(self)->mSize;

    p = CLHASH_DATA(items)->mItems + hash_value2;

    while(1) {
        if(p->mHashValue) {
            if((p->mHashValue % CLHASH(self)->mSize) == hash_value2) {
                BOOL result;
                int p_offset;

                p_offset = p - CLHASH_DATA(items)->mItems;

                if(!equalibility_of_key(p->mKey, key, info, &result)) {
                    return FALSE;
                }

                p = CLHASH_DATA(items)->mItems + p_offset; // In this point p is invalid address because running GC on equalibility_of_key

                if(result) {
                    memset(p, 0, sizeof(sCLHashDataItem));
                    CLHASH(self)->mLen--;
                    return TRUE;
                }
            }

            p++;
            
            if(p == CLHASH_DATA(items)->mItems + CLHASH(self)->mSize) {
                p = CLHASH_DATA(items)->mItems;
            }
            else if(p == CLHASH_DATA(items)->mItems + hash_value2) {
                break;
            }
        }
        else {
            memset(p, 0, sizeof(sCLHashDataItem));
            CLHASH(self)->mLen--;
            break;
        }
    }

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

    if(object2 != 0) {
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
}

void initialize_hidden_class_method_of_hash(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = mark_hash_object;
    klass->mCreateFun = create_hash_object_for_new;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gHashClass = klass;
        gHashTypeObject = create_type_object(gHashClass);
    }
}

BOOL Hash_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject value;
    int i;
    CLObject new_data;
    CLObject value_data;

    vm_mutex_lock();
    self = lvar->mObjectValue.mValue;

    if(!check_type_without_generics(self, gHashTypeObject, info, FALSE)) {
        vm_mutex_unlock();
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type_without_generics(value, gHashTypeObject, info, FALSE)) 
    {
        vm_mutex_unlock();
        return FALSE;
    }

    /// make new self ///
    new_data = alloc_hash_items(CLHASH(value)->mSize);
    CLHASH(self)->mData = new_data;
    CLHASH(self)->mSize = CLHASH(value)->mSize;
    CLHASH(self)->mLen = 0;

    memset(CLHASH_DATA(new_data)->mItems, 0, sizeof(sCLHashDataItem)*CLHASH(value)->mSize);

    value_data = CLHASH(value)->mData;

    for(i=0; i<CLHASH(value)->mSize; i++) {
        int hash_value;

        hash_value = CLHASH_DATA(value_data)->mItems[i].mHashValue;

        if(hash_value) {
            CLObject key;
            CLObject item;

            key = CLHASH_DATA(value_data)->mItems[i].mKey;
            item = CLHASH_DATA(value_data)->mItems[i].mItem;

            if(!add_item_to_hash(self, key, item, info)) {
                vm_mutex_unlock();
                return FALSE;
            }
        }
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Hash_put(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject key;
    CLObject key_type_object;
    CLObject item;
    CLObject item_type_object;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_without_generics(self, gHashTypeObject, info, FALSE)) {
        vm_mutex_unlock();
        return FALSE;
    }

    key_type_object = CLTYPEOBJECT(CLOBJECT_HEADER(self)->mType)->mGenericsTypes[0];

    key = (lvar+1)->mObjectValue.mValue;
    if(!check_type(key, key_type_object, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    item_type_object = CLTYPEOBJECT(CLOBJECT_HEADER(self)->mType)->mGenericsTypes[1];

    item = (lvar+2)->mObjectValue.mValue;
    if(!check_type(item, item_type_object, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(CLHASH(self)->mSize < CLHASH(self)->mLen * 3) {
        if(!rehash(self, info, CLHASH(self)->mSize * 3)) {
            vm_mutex_unlock();
            return FALSE;
        }
    }

    if(!add_item_to_hash(self, key, item, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = item;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Hash_assoc(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject key_type_object;
    CLObject item_type_object;
    CLObject key;
    CLObject item;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_without_generics(self, gHashTypeObject, info, FALSE)) {
        vm_mutex_unlock();
        return FALSE;
    }

    key_type_object = CLTYPEOBJECT(CLOBJECT_HEADER(self)->mType)->mGenericsTypes[0];
    item_type_object = CLTYPEOBJECT(CLOBJECT_HEADER(self)->mType)->mGenericsTypes[1];

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
        CLObject tuple;
        CLObject type_object;

        type_object = create_type_object_with_class_name("Tuple$2");
        push_object(type_object, info);

        CLTYPEOBJECT(type_object)->mGenericsTypesNum = 2;
        CLTYPEOBJECT(type_object)->mGenericsTypes[0] = key_type_object;
        CLTYPEOBJECT(type_object)->mGenericsTypes[1] = item_type_object;

        if(!create_user_object(type_object, &tuple, vm_type, NULL, 0, info)) 
        {
            pop_object(info);
            entry_exception_object_with_class_name(info, "Exception", "can't create user object\n");
            vm_mutex_unlock();
            return FALSE;
        }

        pop_object(info);

        CLUSEROBJECT(tuple)->mFields[0].mObjectValue.mValue = key;
        CLUSEROBJECT(tuple)->mFields[1].mObjectValue.mValue = item;

        (*stack_ptr)->mObjectValue.mValue = tuple;
        (*stack_ptr)++;
    }
    else {
        (*stack_ptr)->mObjectValue.mValue = create_null_object();
        (*stack_ptr)++;
    }

    vm_mutex_unlock();

    return TRUE;
}

BOOL Hash_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_without_generics(self, gHashTypeObject, info, FALSE)) {
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(CLHASH(self)->mLen);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Hash_each(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject block;
    BOOL result_existance;
    CLObject data;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_without_generics(self, gHashTypeObject, info, FALSE)) {
        vm_mutex_unlock();
        return FALSE;
    }

    block = (lvar+1)->mObjectValue.mValue;

    result_existance = FALSE;

    data = CLHASH(self)->mData;
    
    for(i=0; i<CLHASH(self)->mSize; i++) {
        int hash_value;

        hash_value = CLHASH_DATA(data)->mItems[i].mHashValue;

        if(hash_value) {
            CLObject caller;
            CLObject key;
            CLObject item;

            caller = self;

            key = CLHASH_DATA(data)->mItems[i].mKey;
            item = CLHASH_DATA(data)->mItems[i].mItem;

            (*stack_ptr)->mObjectValue.mValue = caller;         // caller
            (*stack_ptr)++;

            (*stack_ptr)->mObjectValue.mValue = key;
            (*stack_ptr)++;

            (*stack_ptr)->mObjectValue.mValue = item;
            (*stack_ptr)++;

            if(!cl_excute_block(block, result_existance, info, vm_type)) {
                vm_mutex_unlock();
                return FALSE;
            }
        }
    }

    (*stack_ptr)->mObjectValue.mValue = self;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL Hash_erase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject key;
    CLObject key_type_object;
    CLObject item;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type_without_generics(self, gHashTypeObject, info, FALSE)) {
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

    if(item == 0) { item = create_null_object(); }

    push_object(item, info);

    if(!erase_item(self, key, info)) {
        pop_object_except_top(info);
        vm_mutex_unlock();
        return FALSE;
    }

    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = item;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

