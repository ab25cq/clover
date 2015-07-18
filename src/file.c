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

static CLObject alloc_regular_file_object(CLObject type_object, sVMInfo* info)
{
    CLObject obj;
    unsigned int size;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

static CLObject create_regular_file_object(CLObject type_object, sVMInfo* info)
{
    CLObject obj;

    obj = alloc_regular_file_object(type_object, info);

    CLFILE(obj)->mFD = -1;

    return obj;
}

static void free_reqular_file_object(CLObject obj)
{
    if(CLFILE(obj)->mFD >= 0) { (void)close(CLFILE(obj)->mFD); }
}

void initialize_hidden_class_method_of_regular_file(sCLClass* klass)
{
    klass->mFreeFun = free_reqular_file_object;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = create_regular_file_object;
}

BOOL RegularFile_RegularFile(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject file_name;
    CLObject mode;
    CLObject ovalue1;
    int permission;
    char file_name_mbs[PATH_MAX];
    char mode_mbs[128];
    int oflag;
    char* p;
    
    int fd;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;
    file_name = (lvar+1)->mObjectValue.mValue;                 // String
    mode = (lvar+2)->mObjectValue.mValue;                      // String
    ovalue1 = (lvar+3)->mObjectValue.mValue;
    permission = CLINT(ovalue1)->mValue;                       // int

    if((int)wcstombs(file_name_mbs, CLSTRING_DATA(file_name)->mChars, PATH_MAX) < 0) 
    {
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error wcstombs on file_name");
        vm_mutex_unlock();
        return FALSE;
    }

    if((int)wcstombs(mode_mbs, CLSTRING_DATA(mode)->mChars, 128) < 0) 
    {
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error wcstombs on mode");
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
        entry_exception_object_with_class_name(info, "Exception", "ivalid file mode");
        vm_mutex_unlock();
        return FALSE;
    }
    p++;
    
    if(*p == '+') {
        p++;
        oflag |= O_RDWR;
    }

    if(*p != 0) {
        entry_exception_object_with_class_name(info, "Exception", "ivalid file mode");
        vm_mutex_unlock();
        return FALSE;
    }

    fd = open(file_name_mbs, oflag, permission);

    if(fd < 0) {
        entry_exception_object_with_class_name(info, "IOException", "can't open file");
        vm_mutex_unlock();
        return FALSE;
    }

    CLFILE(self)->mFD = fd;

    (*stack_ptr)->mObjectValue.mValue = self;
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}
