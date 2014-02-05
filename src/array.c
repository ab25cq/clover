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

static CLObject alloc_array_object(unsigned int size_item)
{
    unsigned int heap_size;
    CLObject obj;

    heap_size = object_size();
    obj = alloc_heap_mem(heap_size);
    CLOBJECT_HEADER(obj)->mHeapMemSize = heap_size;

    CLARRAY(obj)->mItems = alloc_heap_mem(sizeof(MVALUE)*size_item);

    return obj;
}

CLObject create_array_object(MVALUE elements[], unsigned int num_elements)
{
    CLObject obj;
    MVALUE* data;

    const unsigned int size_item = (num_elements + 1) * 2;

    obj = alloc_array_object(size_item);

    CLOBJECT_HEADER(obj)->mExistence = 0;
    CLOBJECT_HEADER(obj)->mClass = gArrayType.mClass;
    CLARRAY(obj)->mSize = size_item;
    CLARRAY(obj)->mLen = num_elements;

    data = object_to_ptr(CLARRAY(obj)->mItems);

    memcpy(data, elements, sizeof(MVALUE)*num_elements);

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

    printf(" class name (Array) ");
    printf(" (size %d) (len %d)\n", CLARRAY(obj)->mSize, CLARRAY(obj)->mLen);

    for(j=0; j<CLARRAY(obj)->mLen; j++) {
        printf("item##%d %d\n", j, CLARRAY_ITEMS(obj, j).mIntValue);
    }
}

void initialize_hidden_class_method_of_array(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = show_array_object;
    klass->mMarkFun = mark_array_object;
}

BOOL Array_Array(MVALUE* stack_ptr, MVALUE* lvar)
{
    return TRUE;
}

BOOL Array_add(MVALUE* stack_ptr, MVALUE* lvar)
{
    return TRUE;
}

BOOL Array_get(MVALUE* stack_ptr, MVALUE* lvar)
{
    return TRUE;
}

