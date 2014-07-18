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

static CLObject alloc_regular_file_object(sCLClass* klass)
{
    CLObject obj;
    unsigned int size;

    size = object_size();
    obj = alloc_heap_mem(size, klass);

    return obj;
}

CLObject create_regular_file_object(sCLClass* klass)
{
    CLObject obj;

    obj = alloc_regular_file_object(klass);

    CLFILE(obj)->mFD = -1;

    return obj;
}

static void free_reqular_file_object(CLObject obj)
{
    if(CLFILE(obj)->mFD >= 0) { (void)close(CLFILE(obj)->mFD); }
}

BOOL RegularFile_RegularFile(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info)
{
    CLObject self;
    CLObject file_name;
    CLObject mode;
    int permission;
    char file_name_mbs[PATH_MAX];
    char mode_mbs[128];
    int oflag;
    char* p;
    
    int fd;

    vm_mutex_lock();

    self = lvar->mObjectValue;
    file_name = (lvar+1)->mObjectValue;                 // String
    mode = (lvar+2)->mObjectValue;                      // String
    permission = (lvar+3)->mIntValue;                   // int

    if((int)wcstombs(file_name_mbs, CLSTRING(file_name)->mChars, PATH_MAX) < 0) 
    {
        entry_exception_object(info, gExConvertingStringCodeType.mClass, "error wcstombs on file_name");
        vm_mutex_unlock();
        return FALSE;
    }

    if((int)wcstombs(mode_mbs, CLSTRING(mode)->mChars, 128) < 0) 
    {
        entry_exception_object(info, gExConvertingStringCodeType.mClass, "error wcstombs on mode");
        vm_mutex_unlock();
        return FALSE;
    }

    oflag = 0;

    p = mode_mbs;
    if(*p == 'r') {
        oflag = O_RDONLY;
    }
    else if(*p == 'w') {
        oflag = O_WRONLY|O_CREAT|O_TRUNC;
    }
    else if(*p == 'a') {
        oflag = O_WRONLY|O_CREAT|O_APPEND;
    }
    else {
        entry_exception_object(info, gExceptionType.mClass, "ivalid file mode");
        vm_mutex_unlock();
        return FALSE;
    }
    p++;
    
    if(*p == '+') {
        p++;
        oflag |= O_RDWR;
    }

    if(*p != 0) {
        entry_exception_object(info, gExceptionType.mClass, "ivalid file mode");
        vm_mutex_unlock();
        return FALSE;
    }

    fd = open(file_name_mbs, oflag, permission);

    if(fd < 0) {
        entry_exception_object(info, gExIOType.mClass, "can't open file");
        vm_mutex_unlock();
        return FALSE;
    }

    CLFILE(self)->mFD = fd;

    (*stack_ptr)->mObjectValue = self;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

void initialize_hidden_class_method_of_regular_file(sCLClass* klass)
{
    klass->mFreeFun = free_reqular_file_object;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_regular_file_object;
}

