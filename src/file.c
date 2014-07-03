#include "clover.h"
#include "common.h"
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLFile);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_file_object(sCLClass* klass)
{
    CLObject obj;
    unsigned int size;

    size = object_size();
    obj = alloc_heap_mem(size, klass);

    return obj;
}

CLObject create_file_object(sCLClass* klass)
{
    CLObject obj;

    obj = alloc_file_object(klass);

    CLFILE(obj)->mFD = -1;

    return obj;
}

static void free_file_object(CLObject obj)
{
    if(CLFILE(obj)->mFD >= 0) { (void)close(CLFILE(obj)->mFD); }
}

void initialize_hidden_class_method_of_file(sCLClass* klass)
{
    klass->mFreeFun = free_file_object;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_file_object;
}

