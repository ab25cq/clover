#include "clover.h"
#include "common.h"
#include <limits.h>
#include <wchar.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include <utime.h>
#include <fnmatch.h>

BOOL System_exit(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject status_code;
    char status_code_value;

    vm_mutex_lock();

    status_code = lvar->mObjectValue.mValue;

    if(!check_type(status_code, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(CLINT(status_code)->mValue <= 0) {
        entry_exception_object_with_class_name(info, "RangeException", "status_code is lesser equals than 0");
        vm_mutex_unlock();
        return FALSE;
    }
    else if(CLINT(status_code)->mValue >= 0xff) {
        /// exception ///
        entry_exception_object_with_class_name(info, "RangeException", "status_code is greater than 255");
        vm_mutex_unlock();
        return FALSE;
    }

    status_code_value = CLINT(status_code)->mValue;

    cl_final();
    exit(status_code_value);

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_getenv(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject env;
    wchar_t* env_wstr;
    char* env_str;
    int size;
    char* str;
    wchar_t* wstr;
    int wcs_len;
    CLObject object;

    vm_mutex_lock();

    /// params ///
    env = lvar->mObjectValue.mValue;

    if(!check_type(env, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    /// go ///
    env_wstr = CLSTRING_DATA(env)->mChars;

    size = (CLSTRING(env)->mLen + 1) * MB_LEN_MAX;
    env_str = MALLOC(size);

    if((int)wcstombs(env_str, env_wstr, size) < 0) {
        FREE(env_str);
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error wcstombs on converting string");
        vm_mutex_unlock();
        return FALSE;
    }

    str = getenv(env_str);

    FREE(env_str);

    wcs_len = strlen(str)+1;
    wstr = MALLOC(sizeof(wchar_t)*wcs_len);

    if((int)mbstowcs(wstr, str, wcs_len) < 0) {
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error mbstowcs on converting string");
        FREE(wstr);
        vm_mutex_unlock();
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_string_object(wstr, wcs_len, gStringTypeObject, info);
    (*stack_ptr)++;

    FREE(wstr);

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_sleep(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject time;
    unsigned int result;

    vm_mutex_lock();

    time = lvar->mObjectValue.mValue;

    if(!check_type(time, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(CLINT(time)->mValue <= 0) {
        entry_exception_object_with_class_name(info, "RangeException", "time is lesser equals than 0");
        vm_mutex_unlock();
        return FALSE;
    }

    result = sleep(CLINT(time)->mValue);

    if(result >= 0x7fffffff) {
        result = 0x7ffffff;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)result);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_msleep(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject time;
    unsigned int result;
    struct timespec req;

    vm_mutex_lock();

    time = lvar->mObjectValue.mValue;

    if(!check_type(time, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(CLINT(time)->mValue <= 0) {
        entry_exception_object_with_class_name(info, "RangeException", "time is lesser equals than 0");
        vm_mutex_unlock();
        return FALSE;
    }

    req.tv_sec = 0;
    req.tv_nsec = 1000000 * CLINT(time)->mValue;     // 1ms

    result = nanosleep(&req, NULL);

    if(result >= 0x7fffffff) {
        result = 0x7ffffff;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)result);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_nanosleep(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject time;
    unsigned int result;
    struct timespec req;

    vm_mutex_lock();

    time = lvar->mObjectValue.mValue;

    if(!check_type(time, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    if(CLINT(time)->mValue <= 0) {
        entry_exception_object_with_class_name(info, "RangeException", "time is lesser equals than 0");
        vm_mutex_unlock();
        return FALSE;
    }

    req.tv_sec = 0;
    req.tv_nsec = 1 * CLINT(time)->mValue;     // 1nano

    result = nanosleep(&req, NULL);

    if(result >= 0x7fffffff) {
        result = 0x7ffffff;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)result);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_srand(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject seed;

    vm_mutex_lock();

    seed = lvar->mObjectValue.mValue;

    if(!check_type(seed, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    srand(CLINT(seed)->mValue);

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_rand(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    int result;

    vm_mutex_lock();

    result = rand();

    (*stack_ptr)->mObjectValue.mValue = create_int_object(result);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL System_execv(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject command;
    CLObject params;
    char* command_name;
    char** param_names;
    int i;
    int len;

    command = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(command, "String", info)) {
        return FALSE;
    }

    params = (lvar+1)->mObjectValue.mValue;

    if(!check_type_for_array(params, "String", info)) {
        return FALSE;
    }

    /// make parametors ///
    if(!create_buffer_from_string_object(command, ALLOC &command_name, info)) {
        return FALSE;
    }

    len = CLARRAY(params)->mLen;
    param_names = CALLOC(sizeof(char*)*(len+2), 1);

    param_names[0] = command_name;

    for(i=0; i<len; i++) {
        CLObject str;

        str = CLARRAY_ITEMS2(params, i).mObjectValue.mValue;

        if(!create_buffer_from_string_object(str, ALLOC &param_names[i+1], info)) {
            int j;
            FREE(command_name);
            for(j=0; j<i; j++) {
                FREE(param_names[j]);
            }
            FREE(param_names);
            return FALSE;
        }
    }

    param_names[i+1] = NULL;

    if(execv(command_name, param_names) < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "execv(2) is failed. error is %s. errno is %d.", strerror(errno), errno);

        FREE(command_name);

        for(i=0; i<len; i++) {
            FREE(param_names[i]);
        }
        FREE(param_names);
        return FALSE;
    }

    FREE(command_name);

    for(i=0; i<len; i++) {
        FREE(param_names[i]);
    }
    FREE(param_names);

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_execvp(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject command;
    CLObject params;
    char* command_name;
    char** param_names;
    int i;
    int len;

    command = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(command, "String", info)) {
        return FALSE;
    }

    params = (lvar+1)->mObjectValue.mValue;

    if(!check_type_for_array(params, "String", info)) {
        return FALSE;
    }

    /// make parametors ///
    if(!create_buffer_from_string_object(command, ALLOC &command_name, info)) {
        return FALSE;
    }

    len = CLARRAY(params)->mLen;
    param_names = CALLOC(sizeof(char*)*(len+2), 1);

    param_names[0] = command_name;

    for(i=0; i<len; i++) {
        CLObject str;

        str = CLARRAY_ITEMS2(params, i).mObjectValue.mValue;

        if(!create_buffer_from_string_object(str, ALLOC &param_names[i+1], info)) {
            int j;
            FREE(command_name);
            for(j=0; j<i; j++) {
                FREE(param_names[j]);
            }
            FREE(param_names);
            return FALSE;
        }
    }

    param_names[i+1] = NULL;

    if(execvp(command_name, param_names) < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "execvp(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);

        FREE(command_name);

        for(i=0; i<len; i++) {
            FREE(param_names[i]);
        }
        FREE(param_names);

        return FALSE;
    }

    FREE(command_name);

    for(i=0; i<len; i++) {
        FREE(param_names[i]);
    }
    FREE(param_names);

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_fork(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    int pid;
    CLObject block;

    block = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(block, "Block", info)) {
        return FALSE;
    }

    /// child process ///
    if((pid = fork()) == 0) {
        BOOL result_existance;

        vm_mutex_unlock();
        new_vm_mutex();         // avoid to dead lock
        vm_mutex_lock();

        result_existance = 0;

        if(!cl_excute_block(block, result_existance, info, vm_type)) 
        {
            vm_mutex_unlock();
            return FALSE;
        }

        vm_mutex_unlock();
        exit(0);
    }

    if(pid < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "fork(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(pid);
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_wait(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    pid_t pid_value;
    int status_value;
    CLObject pid_object;
    CLObject status_object;
    CLObject tuple_object;

    pid_value = wait(&status_value);
    if(pid_value < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "wait(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        return FALSE;
    }

    pid_object = create_number_object_from_size(sizeof(pid_t), pid_value, "pid_t", info);

    push_object(pid_object, info);

    status_object = create_int_object_with_type_name(status_value, "WaitStatus", info);

    push_object(status_object, info);

    if(!create_user_object_with_class_name("Tuple$2", &tuple_object, vm_type, info)) 
    {
        pop_object_except_top(info);
        pop_object_except_top(info);
        return FALSE;
    }

    CLUSEROBJECT(tuple_object)->mFields[0].mObjectValue.mValue = pid_object;
    CLUSEROBJECT(tuple_object)->mFields[1].mObjectValue.mValue = status_object;

    pop_object(info);
    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = tuple_object;
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_open(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject file_name;
    CLObject mode;
    CLObject permission;
    int fd;
    char* file_name_value;
    int mode_value;
    int permission_value;

    file_name = lvar->mObjectValue.mValue;           // file_name

    if(!check_type_with_class_name(file_name, "Path", info)) {
        return FALSE;
    }

    mode = (lvar+1)->mObjectValue.mValue;      // mode

    if(!check_type_with_class_name(mode, "FileMode", info)) {
        return FALSE;
    }

    permission = (lvar+2)->mObjectValue.mValue;

    if(!check_type_with_class_name(permission, "int", info)) {
        return FALSE;
    }

    if(!create_buffer_from_string_object(file_name, ALLOC &file_name_value, info))
    {
        return FALSE;
    }

    mode_value = CLINT(mode)->mValue;
    permission_value = CLINT(permission)->mValue;

    fd = open(file_name_value, mode_value, permission_value);

    if(fd < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "open(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        FREE(file_name_value);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(fd);
    (*stack_ptr)++;

    FREE(file_name_value);

    return TRUE;
}

BOOL System_write(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject fd;
    CLObject data;
    int fd_value;
    char* data_value;
    size_t size;
    ssize_t write_result;

    fd = lvar->mObjectValue.mValue;           // fd

    if(!check_type_with_class_name(fd, "int", info)) {
        return FALSE;
    }

    data = (lvar+1)->mObjectValue.mValue;      // data

    if(!check_type_with_class_name(data, "Bytes", info)) {
        return FALSE;
    }

    fd_value = CLINT(fd)->mValue;
    data_value = CLBYTES(data)->mChars;
    size = CLBYTES(data)->mLen;

    write_result = write(fd_value, data_value, size);

    if(write_result < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "write(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(write_result);
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_close(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject fd;
    int fd_value;
    int close_result;

    fd = lvar->mObjectValue.mValue;           // fd

    if(!check_type_with_class_name(fd, "int", info)) {
        return FALSE;
    }

    fd_value = CLINT(fd)->mValue;

    close_result = close(fd_value);

    if(close_result < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "close(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(close_result);
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_read(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject fd;
    CLObject data;
    CLObject size;
    int fd_value;
    size_t size_value;
    ssize_t result;
    char* buf;

    fd = lvar->mObjectValue.mValue;           // fd

    if(!check_type_with_class_name(fd, "int", info)) {
        return FALSE;
    }

    data = (lvar+1)->mObjectValue.mValue;      // data

    if(!check_type_with_class_name(data, "Bytes", info)) {
        return FALSE;
    }

    size = (lvar+2)->mObjectValue.mValue;      // size

    if(!check_type_with_class_name(size, "int", info)) {
        return FALSE;
    }

    fd_value = CLINT(fd)->mValue;
    size_value = CLINT(size)->mValue;
    buf = MALLOC(size_value+1);

    result = read(fd_value, buf, size_value);

    if(result < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "read(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        FREE(buf);
        return FALSE;
    }

    replace_bytes(data, buf, result);

    (*stack_ptr)->mObjectValue.mValue = create_int_object(result);
    (*stack_ptr)++;

    FREE(buf);

    return TRUE;
}

BOOL System_pipe(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject read_fd;
    CLObject write_fd;
    int result;
    int fds[2];

    read_fd = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(read_fd, "int", info)) {
        return FALSE;
    }

    write_fd = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(write_fd, "int", info)) {
        return FALSE;
    }

    result = pipe(fds);

    CLINT(read_fd)->mValue = fds[0];
    CLINT(write_fd)->mValue = fds[1];

    if(result < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "pipe(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(result);
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_dup2(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject fd1;
    CLObject fd2;
    int fd1_value;
    int fd2_value;
    int result;

    fd1 = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(fd1, "int", info)) {
        return FALSE;
    }

    fd2 = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(fd2, "int", info)) {
        return FALSE;
    }

    fd1_value = CLINT(fd1)->mValue;
    fd2_value = CLINT(fd2)->mValue;

    result = dup2(fd1_value, fd2_value);

    if(result < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "dup2(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(result);
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_getpid(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    pid_t result;

    result = getpid();

    (*stack_ptr)->mObjectValue.mValue = create_int_object(result);
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_getppid(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    pid_t result;

    result = getppid();

    (*stack_ptr)->mObjectValue.mValue = create_int_object(result);
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_getpgid(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    pid_t result;
    CLObject pid;
    int pid_value;

    pid = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(pid, "int", info)) {
        return FALSE;
    }

    pid_value = CLINT(pid)->mValue;

    result = getpgid(pid_value);

    if(result < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "getpgid(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(result);
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_setpgid(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    int result;
    CLObject pid;
    CLObject pgid;
    int pid_value;
    int pgid_value;

    pid = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(pid, "int", info)) {
        return FALSE;
    }

    pgid = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(pgid, "int", info)) {
        return FALSE;
    }

    pid_value = CLINT(pid)->mValue;
    pgid_value = CLINT(pgid)->mValue;

    result = setpgid(pid_value, pgid_value);

    if(result < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "setpgid(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_tcsetpgrp(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    int result;
    CLObject fd;
    CLObject pgid;
    int fd_value;
    int pgid_value;

    fd = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(fd, "int", info)) {
        return FALSE;
    }

    pgid = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(pgid, "int", info)) {
        return FALSE;
    }

    fd_value = CLINT(fd)->mValue;
    pgid_value = CLINT(pgid)->mValue;

    result = tcsetpgrp(fd_value, pgid_value);

    if(result < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "tcsetpgrp(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_stat(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject path;
    CLObject buf_object;
    CLObject type_object;
    int result;
    struct stat stat_buf;
    char* path_value;
    CLObject ovalue;

    path = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(path, "Path", info)) {
        return FALSE;
    }

    buf_object = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(buf_object, "stat", info)) {
        return FALSE;
    }

    if(!create_buffer_from_string_object(path, ALLOC &path_value, info)) 
    {
        return FALSE;
    }

    result = stat(path_value, &stat_buf);

    FREE(path_value);

    if(result < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "stat(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        return FALSE;
    }

    /// create stat Object from buffer ///
    ovalue = create_number_object_from_size(sizeof(stat_buf.st_dev), stat_buf.st_dev, "dev_t", info);
    CLUSEROBJECT(buf_object)->mFields[0].mObjectValue.mValue = ovalue;

    ovalue = create_number_object_from_size(sizeof(stat_buf.st_ino), stat_buf.st_ino, "ino_t", info);
    CLUSEROBJECT(buf_object)->mFields[1].mObjectValue.mValue = ovalue;

    ovalue = create_number_object_from_size(sizeof(stat_buf.st_mode), stat_buf.st_mode, "mode_t", info);
    CLUSEROBJECT(buf_object)->mFields[2].mObjectValue.mValue = ovalue;

    ovalue = create_number_object_from_size(sizeof(stat_buf.st_nlink), stat_buf.st_nlink, "nlink_t", info);
    CLUSEROBJECT(buf_object)->mFields[3].mObjectValue.mValue = ovalue;

    ovalue = create_number_object_from_size(sizeof(stat_buf.st_uid), stat_buf.st_uid, "uid_t", info);
    CLUSEROBJECT(buf_object)->mFields[4].mObjectValue.mValue = ovalue;

    ovalue = create_number_object_from_size(sizeof(stat_buf.st_gid), stat_buf.st_gid, "gid_t", info);
    CLUSEROBJECT(buf_object)->mFields[5].mObjectValue.mValue = ovalue;
    
    ovalue = create_number_object_from_size(sizeof(stat_buf.st_rdev), stat_buf.st_rdev, "dev_t", info);
    CLUSEROBJECT(buf_object)->mFields[6].mObjectValue.mValue = ovalue;

    ovalue = create_number_object_from_size(sizeof(stat_buf.st_size), stat_buf.st_size, "off_t", info);
    CLUSEROBJECT(buf_object)->mFields[7].mObjectValue.mValue = ovalue;

    ovalue = create_number_object_from_size(sizeof(stat_buf.st_blksize), stat_buf.st_blksize, "blksize_t", info);
    CLUSEROBJECT(buf_object)->mFields[8].mObjectValue.mValue = ovalue;

    ovalue = create_number_object_from_size(sizeof(stat_buf.st_blocks), stat_buf.st_blocks, "blkcnt_t", info);
    CLUSEROBJECT(buf_object)->mFields[9].mObjectValue.mValue = ovalue;

    ovalue = create_number_object_from_size(sizeof(stat_buf.st_atime), stat_buf.st_atime, "time_t", info);
    CLUSEROBJECT(buf_object)->mFields[10].mObjectValue.mValue = ovalue;
    
    ovalue = create_number_object_from_size(sizeof(stat_buf.st_mtime), stat_buf.st_mtime, "time_t", info);
    CLUSEROBJECT(buf_object)->mFields[11].mObjectValue.mValue = ovalue;

    ovalue = create_number_object_from_size(sizeof(stat_buf.st_ctime), stat_buf.st_ctime, "time_t", info);
    CLUSEROBJECT(buf_object)->mFields[12].mObjectValue.mValue = ovalue;

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_time(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject result;
    time_t time_value;

    time_value = time(NULL);

    (*stack_ptr)->mObjectValue.mValue = create_number_object_from_size(sizeof(time_t), time_value, "time_t", info);
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_basename(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject path;
    char* path_value;
    char* result_value;
    CLObject result;

    path = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(path, "Path", info)) {
        return FALSE;
    }

    if(!create_buffer_from_string_object(path, ALLOC &path_value, info))
    {
        return FALSE;
    }

    result_value = basename(path_value);

    if(!create_string_object_from_ascii_string_with_class_name(&result, result_value, "Path", info))
    {
        FREE(path_value);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    FREE(path_value);

    return TRUE;
}

BOOL System_dirname(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject path;
    char* path_value;
    char* result_value;
    CLObject result;

    path = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(path, "Path", info)) {
        return FALSE;
    }

    if(!create_buffer_from_string_object(path, ALLOC &path_value, info))
    {
        return FALSE;
    }

    result_value = dirname(path_value);

    if(!create_string_object_from_ascii_string_with_class_name(&result, result_value, "Path", info))
    {
        FREE(path_value);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    FREE(path_value);

    return TRUE;
}

BOOL System_chmod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject path;
    CLObject mode;
    char* path_value;
    int result_chmod;
    mode_t mode_value;

    /// check type ///
    path = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(path, "Path", info)) {
        return FALSE;
    }

    mode = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(mode, "mode_t", info)) {
        return FALSE;
    }

    /// Clover object to C value ///
    if(!create_buffer_from_string_object(path, ALLOC &path_value, info))
    {
        return FALSE;
    }

    mode_value = get_value_with_size(sizeof(mode_t), mode);

    result_chmod = chmod(path_value, mode_value);

    if(result_chmod < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "chmod(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        FREE(path_value);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();
    (*stack_ptr)++;

    FREE(path_value);

    return TRUE;
}

BOOL System_lchmod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject path;
    CLObject mode;
    char* path_value;
    int result_lchmod;
    mode_t mode_value;

    /// check type ///
    path = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(path, "Path", info)) {
        return FALSE;
    }

    mode = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(mode, "mode_t", info)) {
        return FALSE;
    }

    /// Clover object to C value ///
    if(!create_buffer_from_string_object(path, ALLOC &path_value, info))
    {
        return FALSE;
    }

    mode_value = get_value_with_size(sizeof(mode_t), mode);

    result_lchmod = lchmod(path_value, mode_value);

    if(result_lchmod < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "lchmod(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        FREE(path_value);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();
    (*stack_ptr)++;

    FREE(path_value);

    return TRUE;
}

BOOL System_chown(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject path;
    CLObject owner;
    CLObject group;
    char* path_value;
    uid_t owner_value;
    gid_t group_value;
    int result_chown;

    /// check type ///
    path = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(path, "Path", info)) {
        return FALSE;
    }

    owner = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(owner, "uid_t", info)) {
        return FALSE;
    }

    group = (lvar+2)->mObjectValue.mValue;

    if(!check_type_with_class_name(group, "gid_t", info)) {
        return FALSE;
    }

    /// Clover object to C value ///
    if(!create_buffer_from_string_object(path, ALLOC &path_value, info))
    {
        return FALSE;
    }

    owner_value = get_value_with_size(sizeof(uid_t), owner);
    group_value = get_value_with_size(sizeof(gid_t), group);

    result_chown = chown(path_value, owner_value, group_value);

    if(result_chown < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "chown(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        FREE(path_value);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();
    (*stack_ptr)++;

    FREE(path_value);

    return TRUE;
}

BOOL System_lchown(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject path;
    CLObject owner;
    CLObject group;
    char* path_value;
    uid_t owner_value;
    gid_t group_value;
    int result_lchown;

    /// check type ///
    path = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(path, "Path", info)) {
        return FALSE;
    }

    owner = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(owner, "uid_t", info)) {
        return FALSE;
    }

    group = (lvar+2)->mObjectValue.mValue;

    if(!check_type_with_class_name(group, "gid_t", info)) {
        return FALSE;
    }

    /// Clover object to C value ///
    if(!create_buffer_from_string_object(path, ALLOC &path_value, info))
    {
        return FALSE;
    }

    owner_value = get_value_with_size(sizeof(uid_t), owner);
    group_value = get_value_with_size(sizeof(gid_t), group);

    result_lchown = lchown(path_value, owner_value, group_value);

    if(result_lchown < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "lchown(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        FREE(path_value);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();
    (*stack_ptr)++;

    FREE(path_value);

    return TRUE;
}

BOOL System_getuid(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    uid_t result_getuid;

    result_getuid = getuid();

    (*stack_ptr)->mObjectValue.mValue = create_number_object_from_size(sizeof(uid_t), result_getuid, "uid_t", info);
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_getgid(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    gid_t result_getgid;

    result_getgid = getgid();

    (*stack_ptr)->mObjectValue.mValue = create_number_object_from_size(sizeof(gid_t), result_getgid, "gid_t", info);
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_unlink(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    int result_unlink;
    CLObject path;
    char* path_value;

    /// check type ///
    path = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(path, "Path", info)) {
        return FALSE;
    }

    /// Clover object to C value ///
    if(!create_buffer_from_string_object(path, ALLOC &path_value, info))
    {
        return FALSE;
    }

    result_unlink = unlink(path_value);

    if(result_unlink < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "unlink(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        FREE(path_value);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();
    (*stack_ptr)++;

    FREE(path_value);

    return TRUE;
}

BOOL System_access(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    int result_access;
    CLObject path;
    char* path_value;
    CLObject mode;
    int mode_value;
    CLObject tuple;
    CLObject type_object;
    CLObject result_value1;
    CLObject result_value2;

    /// check type ///
    path = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(path, "Path", info)) {
        return FALSE;
    }

    mode = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(mode, "AccessMode", info)) {
        return FALSE;
    }

    /// Clover object to C value ///
    if(!create_buffer_from_string_object(path, ALLOC &path_value, info))
    {
        return FALSE;
    }

    mode_value = CLINT(mode)->mValue;

    /// go ///
    result_access = access(path_value, mode_value);

    if(result_access < 0) {
        char buf[512];
        
        snprintf(buf, 512, "access(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);

        if(!create_string_object_from_ascii_string(&result_value2, buf, gStringTypeObject, info))
        {
            FREE(path_value);
            return FALSE;
        }
    }
    else {
        result_value2 = create_string_object(L"", 0, gStringTypeObject, info);
    }

    push_object(result_value2, info);

    type_object = create_type_object_with_class_name("Tuple$2");
    push_object(type_object, info);

    CLTYPEOBJECT(type_object)->mGenericsTypesNum = 2;
    CLTYPEOBJECT(type_object)->mGenericsTypes[0] = gIntTypeObject;
    CLTYPEOBJECT(type_object)->mGenericsTypes[1] = gStringTypeObject;

    if(!create_user_object(type_object, &tuple, vm_type, NULL, 0, info)) 
    {
        pop_object_except_top(info);  // result_value2
        pop_object_except_top(info);  // type_object
        FREE(path_value);
        entry_exception_object_with_class_name(info, "Exception", "can't create user object\n");
        return FALSE;
    }

    CLUSEROBJECT(tuple)->mFields[1].mObjectValue.mValue = result_value2;

    pop_object(info);   // result_value2
    pop_object(info);   // type_object
    push_object(tuple, info);

    result_value1 = create_int_object(result_access);

    CLUSEROBJECT(tuple)->mFields[0].mObjectValue.mValue = result_value1;

    pop_object(info); // tuple

    (*stack_ptr)->mObjectValue.mValue = tuple;
    (*stack_ptr)++;

    FREE(path_value);

    return TRUE;
}

BOOL System_utime(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    int result_utime;
    CLObject path;
    CLObject times;
    CLObject actime;
    CLObject modtime;
    char* path_value;
    time_t actime_value;
    time_t modtime_value;
    struct utimbuf utime_buf_value;
    BOOL times_is_null;
    CLObject type_object;

    /// check type ///
    path = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(path, "Path", info)) {
        return FALSE;
    }

    times = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name_and_nullable(times, "utimbuf", info)) {
        return FALSE;
    }

    type_object = CLOBJECT_HEADER(times)->mType;

    times_is_null = CLTYPEOBJECT(type_object)->mClass == gNullClass;

    /// Clover object to C value ///
    if(!create_buffer_from_string_object(path, ALLOC &path_value, info))
    {
        return FALSE;
    }

    if(!times_is_null) {
        actime = CLUSEROBJECT(times)->mFields[0].mObjectValue.mValue;
        modtime = CLUSEROBJECT(times)->mFields[1].mObjectValue.mValue;

        actime_value = get_value_with_size(sizeof(time_t), actime);
        modtime_value = get_value_with_size(sizeof(time_t), modtime);

        utime_buf_value.actime = actime_value;
        utime_buf_value.modtime = modtime_value;
    }

    /// go ///
    if(times_is_null) {
        result_utime = utime(path_value, NULL);
    }
    else {
        result_utime = utime(path_value, &utime_buf_value);
    }

    if(result_utime < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "utime(2) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        FREE(path_value);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();
    (*stack_ptr)++;

    FREE(path_value);

    return TRUE;
}

BOOL System_mktime(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    time_t result_mktime;
    CLObject time;
    struct tm time_value;
    CLObject object;
    CLObject result;

    /// check type ///
    time = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(time, "tm", info)) {
        return FALSE;
    }

    /// Clover object to C value ///
    object = CLUSEROBJECT(time)->mFields[0].mObjectValue.mValue;
    time_value.tm_sec = CLINT(object)->mValue;

    object = CLUSEROBJECT(time)->mFields[1].mObjectValue.mValue;
    time_value.tm_min = CLINT(object)->mValue;

    object = CLUSEROBJECT(time)->mFields[2].mObjectValue.mValue;
    time_value.tm_hour = CLINT(object)->mValue;

    object = CLUSEROBJECT(time)->mFields[3].mObjectValue.mValue;
    time_value.tm_mday = CLINT(object)->mValue;

    object = CLUSEROBJECT(time)->mFields[4].mObjectValue.mValue;
    time_value.tm_mon = CLINT(object)->mValue;

    object = CLUSEROBJECT(time)->mFields[5].mObjectValue.mValue;
    time_value.tm_year = CLINT(object)->mValue;

    object = CLUSEROBJECT(time)->mFields[6].mObjectValue.mValue;
    time_value.tm_wday = CLINT(object)->mValue;

    object = CLUSEROBJECT(time)->mFields[7].mObjectValue.mValue;
    time_value.tm_yday = CLINT(object)->mValue;

    object = CLUSEROBJECT(time)->mFields[8].mObjectValue.mValue;
    time_value.tm_isdst = CLBOOL(object)->mValue;

    /// go ///
    result_mktime = mktime(&time_value);

    if(result_mktime < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "mktime(3) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        return FALSE;
    }

    /// result ///
    result = create_number_object_from_size(sizeof(time_t), result_mktime, "time_t", info);

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    return TRUE;
}

BOOL System_fnmatch(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    int result_fnmatch;
    CLObject pattern;
    char* pattern_value;
    CLObject path;
    char* path_value;
    CLObject flags;
    int flags_value;
    CLObject result;

    /// check type ///
    pattern = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(pattern, "String", info)) {
        return FALSE;
    }

    path = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(path, "Path", info)) {
        return FALSE;
    }

    flags = (lvar+2)->mObjectValue.mValue;

    if(!check_type_with_class_name(flags, "FnmatchFlags", info)) {
        return FALSE;
    }

    /// Clover object to C value ///
    if(!create_buffer_from_string_object(pattern, ALLOC &pattern_value, info))
    {
        return FALSE;
    }

    if(!create_buffer_from_string_object(path, ALLOC &path_value, info))
    {
        FREE(pattern_value);
        return FALSE;
    }

    flags_value = CLINT(flags)->mValue;

    /// go ///
    result_fnmatch = fnmatch(pattern_value, path_value, flags_value);

    if(result_fnmatch != 0 && result_fnmatch != FNM_NOMATCH) {
        entry_exception_object_with_class_name(info, "SystemException", "fnmatch(3) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        return FALSE;
    }

    /// result ///
    result = create_bool_object(result_fnmatch == 0);

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    FREE(pattern_value);
    FREE(path_value);

    return TRUE;
}

BOOL System_system(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject command;
    char* command_value;
    int system_result;

    /// params ///
    command = lvar->mObjectValue.mValue;

    if(!check_type(command, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    /// Clover Object to C data ///
    if(!create_buffer_from_string_object(command, ALLOC &command_value, info))
    {
        return FALSE;
    }

    /// go ///
    system_result = system(command_value);

    if(system_result < 0) {
        entry_exception_object_with_class_name(info, "SystemException", "system(3) is failed. The error is %s. The errno is %d.", strerror(errno), errno);
        FREE(command_value);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(system_result);
    (*stack_ptr)++;

    FREE(command_value);

    return TRUE;
}
