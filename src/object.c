#include "clover.h"
#include "common.h"

static unsigned int object_size(sCLClass* klass)
{
    unsigned int size;

    size = sizeof(sCLObject) - sizeof(MVALUE) * DUMMY_ARRAY_SIZE;
    size += (unsigned int)sizeof(MVALUE)*get_field_num_including_super_classes(klass);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

CLObject create_object(sCLClass* klass)
{
    unsigned int size;
    CLObject obj;

    size = object_size(klass);

    obj = alloc_heap_mem(size);
    CLOBJECT_HEADER(obj)->mClass = klass;
    CLOBJECT_HEADER(obj)->mHeapMemSize = size;

    return obj;
}

static void mark_user_object(CLObject object, unsigned char* mark_flg)
{
    int i;

    for(i=0; i<get_field_num_including_super_classes(CLOBJECT_HEADER(object)->mClass); i++) {
        CLObject obj2;

        obj2 = CLOBJECT(object)->mFields[i].mObjectValue;

        mark_object(obj2, mark_flg);
    }
}

static void show_user_object(CLObject obj)
{
    int j;
    sCLClass* klass;

    klass = CLOBJECT_HEADER(obj)->mClass;

    printf(" class name (%s)\n", REAL_CLASS_NAME(klass));

    for(j=0; j<get_field_num_including_super_classes(klass); j++) {
        printf("field#%d %d\n", j, CLOBJECT(obj)->mFields[j].mIntValue);
    }
}

void initialize_hidden_class_method_of_user_object(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = show_user_object;
    klass->mMarkFun = mark_user_object;
}

BOOL Object_show_class(MVALUE** stack_ptr, MVALUE* lvar)
{
    MVALUE* self;
    CLObject ovalue;
    sCLClass* klass;

    self = lvar; // self
    ovalue = lvar->mObjectValue;
    klass = CLOBJECT_HEADER(ovalue)->mClass;
    
    ASSERT(klass != NULL);

    show_class(klass);

    return TRUE;
}

