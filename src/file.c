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

BOOL File_write(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject data;
    int fd;
    int len;
    char* str;

    self = lvar->mObjectValue;              // File
    data = (lvar+1)->mObjectValue;          // Bytes

    fd = CLFILE(self)->mFD;
    len = CLBYTES(data)->mLen;
    str = CLBYTES(data)->mChars;

    if(fd == -1) {
        entry_exception_object(info, gExceptionType.mClass, "This file is not opened");
        vm_mutex_unlock();
        return FALSE;
    }

    if(write(fd, &len, sizeof(int)) < 0) {
        entry_exception_object(info, gExIOType.mClass, "write error");
        vm_mutex_unlock();
        return FALSE;
    }

    if(write(fd, str, len) < 0) {
        entry_exception_object(info, gExIOType.mClass, "write error");
        vm_mutex_unlock();
        return FALSE;
    }

    vm_mutex_unlock();

    return TRUE;
}
