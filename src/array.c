#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLArray);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static unsigned int items_object_size(int size_items)
{
    unsigned int size;

    size = sizeof(sCLObjectHeader) + sizeof(MVALUE) * size_items;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_array_object(int size_item)
{
    int heap_size;
    CLObject obj;
    int item_heap_size;

    heap_size = object_size();
    obj = alloc_heap_mem(heap_size, gArrayType.mClass);
    push_object(obj);

    CLARRAY(obj)->mSize = size_item;

    item_heap_size = items_object_size(size_item);
    CLARRAY(obj)->mItems = alloc_heap_mem(item_heap_size, NULL);
    pop_object();

    return obj;
}

CLObject create_array_object(MVALUE elements[], int num_elements)
{
    CLObject obj;
    MVALUE* data;

    const int size_item = (num_elements + 1) * 2;

    obj = alloc_array_object(size_item);
    CLARRAY(obj)->mLen = num_elements;

    if(num_elements > 0) {
        data = CLOBJECT_DATA(CLARRAY(obj)->mItems);
        memcpy(data, elements, sizeof(MVALUE)*num_elements);
    }

    return obj;
}

static void mark_array_object(CLObject object, unsigned char* mark_flg)
{
    int i;

    CLObject object2 = CLARRAY(object)->mItems;
    mark_object(object2, mark_flg);

    for(i=0; i<CLARRAY(object)->mLen; i++) {
        CLObject object3 = CLARRAY_ITEMS(object, i).mObjectValue;

        mark_object(object3, mark_flg);
    }
}

static void show_array_object(CLObject obj)
{
    int j;

    cl_print(" class name (Array) ");
    cl_print(" item id %lu\n", CLARRAY(obj)->mItems);
    cl_print(" (size %d) (len %d)\n", CLARRAY(obj)->mSize, CLARRAY(obj)->mLen);

    for(j=0; j<CLARRAY(obj)->mLen; j++) {
        cl_print("item##%d %d\n", j, CLARRAY_ITEMS(obj, j).mIntValue);
    }
}

static void add_to_array(CLObject self, MVALUE item)
{
    CLObject items;

    if(CLARRAY(self)->mLen >= CLARRAY(self)->mSize) {
        CLObject old_items;
        int new_size;
        CLObject items;
        
        new_size = (CLARRAY(self)->mSize+1) * 2;   // num of MVALUE

        push_object(self);

        old_items = CLARRAY(self)->mItems;
        push_object(old_items);

        items = alloc_heap_mem(items_object_size(new_size), NULL);

        memcpy(CLOBJECT_DATA(items), CLOBJECT_DATA(old_items), sizeof(MVALUE)*CLARRAY(self)->mLen);

        CLARRAY(self)->mItems = items;
        CLARRAY(self)->mSize = new_size;

        pop_object();
        pop_object();
    }

    items = CLARRAY(self)->mItems;

    CLOBJECT_DATA(items)[CLARRAY(self)->mLen] = item;
    CLARRAY(self)->mLen++;
}

/// no check index range
static MVALUE get_item_from_array(CLObject self, int index)
{
    return CLOBJECT_DATA(CLARRAY(self)->mItems)[index];
}

void initialize_hidden_class_method_of_array(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = show_array_object;
    klass->mMarkFun = mark_array_object;
}

BOOL Array_Array(MVALUE** stack_ptr, MVALUE* lvar)
{
    return TRUE;
}

BOOL Array_add(MVALUE** stack_ptr, MVALUE* lvar)
{
    MVALUE* self;
    MVALUE* item;

    self = lvar;
    item = lvar+1;

    add_to_array(self->mObjectValue, *item);

    return TRUE;
}

BOOL Array_items(MVALUE** stack_ptr, MVALUE* lvar)
{
    MVALUE* self;
    MVALUE* index;

    self = lvar;
    index = lvar+1;

    if(index->mIntValue < 0 || index->mIntValue >= CLARRAY(self->mObjectValue)->mLen) {
puts("range");
puts("throw exception");
return FALSE;
    }

    (*stack_ptr)->mObjectValue = get_item_from_array(self->mObjectValue, index->mIntValue).mObjectValue;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Array_length(MVALUE** stack_ptr, MVALUE* lvar)
{
    MVALUE* self;

    self = lvar;

    (*stack_ptr)->mIntValue = CLARRAY(self->mObjectValue)->mLen;
    (*stack_ptr)++;

    return TRUE;
}

