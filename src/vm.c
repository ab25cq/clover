#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>

sVMInfo* gHeadVMInfo;

void atexit_fun()
{
    sVMInfo* it;
    
#ifdef VM_DEBUG
    it = gHeadVMInfo;
    while(it) {
        fclose(it->debug_log);
        it = it->next_info;
    }
#endif
}

void push_vminfo(sVMInfo* info)
{
    info->next_info = gHeadVMInfo;
    gHeadVMInfo = info;
}

static void pop_vminfo(sVMInfo* info)
{
    sVMInfo* it;
    sVMInfo* it_before;

    it = gHeadVMInfo;
    it_before = NULL;
    while(it) {
        if(it == info) {
            if(it_before == NULL) {
                gHeadVMInfo = info->next_info;
                break;
            }
            else {
                it_before->next_info = it->next_info;
                break;
            }
        }
        it_before = it;
        it = it->next_info;
    }
}

BOOL cl_init(int heap_size, int handle_size)
{
    thread_init();

    heap_init(heap_size, handle_size);

    class_init();
    atexit(atexit_fun);

    return TRUE;
}

void cl_final()
{
    sVMInfo* it;

    heap_final();
    class_final();
    thread_final();
}

void push_object(CLObject object, sVMInfo* info)
{
    info->stack_ptr->mObjectValue = object;
    info->stack_ptr++;
}

// remove the object from stack
CLObject pop_object(sVMInfo* info)
{
    info->stack_ptr--;
    return info->stack_ptr->mObjectValue;
}

void vm_error(char* msg, ...)
{
    char msg2[1024];

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    fprintf(stderr, "%s", msg2);
}

void entry_exception_object(sVMInfo* info, sCLClass* klass, char* msg, ...)
{
    CLObject ovalue, ovalue2;
    wchar_t* wcs;
    int size;
    char msg2[1024];

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    vm_mutex_lock();

    (void)create_user_object(klass, &ovalue);

    wcs = MALLOC(sizeof(wchar_t)*(strlen(msg2)+1));
    (void)mbstowcs(wcs, msg2, strlen(msg2)+1);
    size = wcslen(wcs);

    ovalue2 = create_string_object(gStringType.mClass, wcs, size);

    CLUSEROBJECT(ovalue)->mFields[0].mObjectValue = ovalue2;

    FREE(wcs);

    info->stack_ptr->mObjectValue = ovalue;
    info->stack_ptr++;

    vm_mutex_unlock();
}

static void output_exception_message(sVMInfo* info)
{
    CLObject exception;
    CLObject message;
    char* mbs;

    exception = (info->stack_ptr-1)->mObjectValue;

    if(!is_valid_object(exception)) {
        fprintf(stderr, "invalid exception object\n");
        return;
    }
    message = CLUSEROBJECT(exception)->mFields[0].mObjectValue;

    fwprintf(stderr, L"%ls\n", CLSTRING(message)->mChars);
}

static unsigned char visible_control_character(unsigned char c)
{
    if(c < ' ') {
        return '^';
    }
    else {
        return c;
    }
}

#ifdef VM_DEBUG
static int num_vm;

void start_vm_log(sVMInfo* info)
{
    char file_name[PATH_MAX];

    snprintf(file_name, PATH_MAX, "vm%d.log", num_vm++);
    info->debug_log = fopen(file_name, "w");
}

void vm_log(sVMInfo* info, char* msg, ...)
{
    char msg2[1024];

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    if(info->debug_log) {
        fprintf(info->debug_log, "%s", msg2);
        fflush(info->debug_log);
    }
}

void show_stack(sVMInfo* info, MVALUE* top_of_stack, MVALUE* var)
{
    int i;

    vm_log(info, "stack_ptr %d top_of_stack %d var %d\n", (int)(info->stack_ptr - info->stack), (int)(top_of_stack - info->stack), (int)(var - info->stack));

    for(i=0; i<50; i++) {
        if(info->stack + i == var) {
            if(info->stack + i == info->stack_ptr) {
                vm_log(info, "->v-- stack[%d] value %d\n", i, info->stack[i].mIntValue);
            }
            else {
                vm_log(info, "  v-- stack[%d] value %d\n", i, info->stack[i].mIntValue);
            }
        }
        else if(info->stack + i == top_of_stack) {
            if(info->stack + i == info->stack_ptr) {
                vm_log(info, "->--- stack[%d] value %d\n", i, info->stack[i].mIntValue);
            }
            else {
                vm_log(info, "  --- stack[%d] value %d\n", i, info->stack[i].mIntValue);
            }
        }
        else if(info->stack + i == info->stack_ptr) {
            vm_log(info, "->    stack[%d] value %d\n", i, info->stack[i].mIntValue);
        }
        else {
            vm_log(info, "      stack[%d] value %d\n", i, info->stack[i].mIntValue);
        }
    }
}
#endif

static BOOL get_node_type_from_bytecode(int** pc, sConst* constant, sCLNodeType* type, sCLNodeType* generics_type, sVMInfo* info)
{
    unsigned int ivalue1, ivalue2;
    char* p;
    char* real_class_name;
    char cvalue1;
    int i;
    int size;

    ivalue1 = **pc;                                                      // real class name param offset
    (*pc)++;

    real_class_name = CONS_str(constant, ivalue1);

    type->mClass = cl_get_class_with_generics(real_class_name, generics_type);

    if(type->mClass == NULL) {
        entry_exception_object(info, gExClassNotFoundType.mClass, "can't get a class named %s\n", real_class_name);
        return FALSE;
    }

    type->mGenericsTypesNum = **pc;
    (*pc)++;

    for(i=0; i<type->mGenericsTypesNum; i++) {
        ivalue2 = **pc;                     // real class name offset
        (*pc)++;

        real_class_name = CONS_str(constant, ivalue2);
        type->mGenericsTypes[i] = cl_get_class_with_generics(real_class_name, generics_type);

        if(type->mGenericsTypes[i] == NULL) {
            entry_exception_object(info, gExClassNotFoundType.mClass, "can't get a class named %s\n", real_class_name);
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL excute_block(CLObject block, sCLNodeType* type_, BOOL result_existance, sVMInfo* info);
static BOOL excute_method(sCLMethod* method, sCLClass* klass, sConst* constant, BOOL result_existance, sCLNodeType* type_, int num_params, sVMInfo* info);

static BOOL cl_vm(sByteCode* code, sConst* constant, MVALUE* var, sCLNodeType* type_, sVMInfo* info)
{
    int ivalue1, ivalue2, ivalue3, ivalue4, ivalue5, ivalue6, ivalue7, ivalue8, ivalue9, ivalue10, ivalue11, ivalue12;
    char cvalue1;
    unsigned char bvalue1, bvalue2, bvalue3, bvalue4, bvalue5;
    float fvalue1;
    CLObject ovalue1, ovalue2, ovalue3;
    MVALUE* mvalue1;
    MVALUE* stack_ptr2;

    sCLClass* klass1, *klass2, *klass3;
    sCLMethod* method;
    sCLField* field;
    wchar_t* str;
    char* real_class_name;
    sCLNodeType params[CL_METHOD_PARAM_MAX];
    sCLNodeType params2[CL_METHOD_PARAM_MAX];
    sCLNodeType type2;
    MVALUE objects[CL_ARRAY_ELEMENTS_MAX];
    int num_vars;
    int i, j;
    sCLNodeType generics_type;
    MVALUE* top_of_stack;
    int* pc;

    BOOL result;
    int return_count;

    pc = code->mCode;
    top_of_stack = info->stack_ptr;
    num_vars = info->stack_ptr - var;

    while(pc - code->mCode < code->mLen) {
VMLOG(info, "pc - code->mCode %d\n", pc - code->mCode);
        switch(*pc) {
            case OP_LDCINT:
VMLOG(info, "OP_LDCINT\n");
                pc++;

                ivalue1 = *pc;                      // constant pool offset
                pc++;

                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_LDC int value %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_LDCFLOAT:
VMLOG(info, "OP_LDCFLOAT\n");
                pc++;

                ivalue1 = *pc;          // constant pool offset
                pc++;

                info->stack_ptr->mFloatValue = *(float*)(constant->mConst + ivalue1);
VMLOG(info, "OP_LDC float value %f\n", info->stack_ptr->mFloatValue);
                info->stack_ptr++;
                break;

            case OP_LDCWSTR: {
VMLOG(info, "OP_LDCWSTR\n");
                int size;
                wchar_t* wcs;

                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;                  // offset
                pc++;

                wcs = (wchar_t*)(constant->mConst + ivalue1);
VMLOG(info, "ivalue1 %d\n", ivalue1);
VMLOG(info, "wcs %ls\n", wcs);
                size = wcslen(wcs);

                info->stack_ptr->mObjectValue = create_string_object(gStringType.mClass, wcs, size);
VMLOG(info, "OP_LDC string object %ld\n", info->stack_ptr->mObjectValue);
                info->stack_ptr++;

                vm_mutex_unlock();
                }
                break;

            case OP_LDCSTR: {
VMLOG(info, "OP_LDCSTR\n");
                int size;
                unsigned char* mbs;

                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;                  // offset
                pc++;

                mbs = (unsigned char*)(constant->mConst + ivalue1);
                size = strlen(mbs);

                info->stack_ptr->mObjectValue = create_bytes_object(gBytesType.mClass, mbs, size);
                info->stack_ptr++;

                vm_mutex_unlock();
                }
                break;

            case OP_LDCLASSNAME: {
                sCLNodeType class_name;

                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;          // offset
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                class_name.mClass = cl_get_class_with_generics(real_class_name, type_);

                ivalue2 = *pc;          // num gneric types
                pc++;

                class_name.mGenericsTypesNum = ivalue2;

                for(i=0; i<ivalue2; i++) {
                    ivalue3 = *pc;
                    pc++;

                    real_class_name = CONS_str(constant, ivalue3);
                    class_name.mGenericsTypes[i] = cl_get_class_with_generics(real_class_name, type_);
                }

                info->stack_ptr->mObjectValue = create_class_name_object(class_name);
                info->stack_ptr++;
                vm_mutex_unlock();
                }
                break;

            case OP_ALOAD:
            case OP_ILOAD:
            case OP_FLOAD:
            case OP_OLOAD:
VMLOG(info, "OP_ILOAD\n");
                pc++;

                ivalue1 = *pc;
                pc++;
VMLOG(info, "OP_LOAD %d\n", ivalue1);

                *info->stack_ptr = var[ivalue1];
                info->stack_ptr++;
                break;

            case OP_ASTORE:
            case OP_ISTORE:
            case OP_FSTORE:
            case OP_OSTORE:
VMLOG(info, "OP_ISTORE\n");
                pc++;

                ivalue1 = *pc;
                pc++;

VMLOG(info, "OP_STORE %d\n", ivalue1);
                var[ivalue1] = *(info->stack_ptr-1);
                break;

            case OP_LDFIELD:
VMLOG(info, "OP_LDFIELD\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;                                      // field index
                pc++;

                ovalue1 = (info->stack_ptr-1)->mObjectValue;
                info->stack_ptr--;

VMLOG(info, "LD_FIELD object %d field num %d\n", (int)ovalue1, ivalue1);

                *info->stack_ptr = CLUSEROBJECT(ovalue1)->mFields[ivalue1];
VMLOG(info, "LDFIELD ovalue1 %d ivalue1 %d value (%d)\n", ovalue1, ivalue1, CLUSEROBJECT(ovalue1)->mFields[ivalue1]);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SRFIELD:
VMLOG(info, "OP_SRFIELD\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;                              // field index
                pc++;

                ovalue1 = (info->stack_ptr-2)->mObjectValue;    // target object
                mvalue1 = info->stack_ptr-1;                    // right value

                CLUSEROBJECT(ovalue1)->mFields[ivalue1] = *mvalue1;
VMLOG(info, "SRFIELD object %d field num %d value %d\n", (int)ovalue1, ivalue1, mvalue1->mIntValue);
                info->stack_ptr-=2;

                *info->stack_ptr = *mvalue1;
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LD_STATIC_FIELD: 
VMLOG(info, "OP_LD_STATIC_FIELD\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;                                                  // class name
                pc++;

                ivalue2 = *pc;                                                  // field index
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    entry_exception_object(info, gExClassNotFoundType.mClass, "can't get a class named %s\n", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                field = klass1->mFields + ivalue2;
                *info->stack_ptr = field->uValue.mStaticField;
VMLOG(info, "LD_STATIC_FIELD %d\n", field->uValue.mStaticField.mIntValue);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SR_STATIC_FIELD:
VMLOG(info, "OP_SR_STATIC_FIELD\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;                                                  // class name
                pc++;

                ivalue2 = *pc;                                                  // field index
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    entry_exception_object(info, gExClassNotFoundType.mClass, "can't get a class named %s\n", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                field = klass1->mFields + ivalue2;

                field->uValue.mStaticField = *(info->stack_ptr-1);
VMLOG(info, "OP_SR_STATIC_FIELD value %d\n", (info->stack_ptr-1)->mIntValue);
                vm_mutex_unlock();
                break;

            case OP_NEW_STRING:
VMLOG(info, "OP_NEW_STRING\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;                              // real class name
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    entry_exception_object(info, gExClassNotFoundType.mClass, "can't get a class named %s\n", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                info->stack_ptr->mObjectValue = create_string_object(klass1, L"", 0);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_NEW_ARRAY:
VMLOG(info, "OP_NEW_ARRAY\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    entry_exception_object(info, gExClassNotFoundType.mClass, "can't get a class named %s\n", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue2 = *pc;                              // number of elements
                pc++;

                stack_ptr2 = info->stack_ptr - ivalue2;
                for(i=0; i<ivalue2; i++) {
                    objects[i] = *stack_ptr2++;
                }

                ovalue1 = create_array_object(klass1, objects, ivalue2, info);
VMLOG(info, "new array %d\n", ovalue1);

                info->stack_ptr -= ivalue2;
                info->stack_ptr->mObjectValue = ovalue1;
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_NEW_HASH:
VMLOG(info, "OP_NEW_HASH\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;                                  // class name
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    entry_exception_object(info, gExClassNotFoundType.mClass, "can't get a class named %s\n", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = *pc;                                  // number of elements
                pc++;

                info->stack_ptr->mObjectValue = create_hash_object(klass1, NULL, NULL, 0);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_NEW_BLOCK: {
                char* const_buf;
                int* code_buf;
                int constant2_len;
                int code2_len;

VMLOG(info, "OP_NEW_BLOCK\n");
                vm_mutex_lock();

                pc++;

                ivalue1 = *pc;                          // block type class
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    entry_exception_object(info, gExClassNotFoundType.mClass, "can't get a class named %s\n", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue2 = *pc;                      // max stack
                pc++;

                ivalue3 = *pc;                      // num locals
                pc++;

                ivalue4 = *pc;                      // num params
                pc++;

VMLOG(info, "OP_NEW_BLOCK max stack %d num locals %d num params %d\n", ivalue2, ivalue3, ivalue4);

                constant2_len = *pc;                // constant pool len
                pc++;
                
                ivalue6 = *pc;                      // constant pool offset
                pc++;

                const_buf = CONS_str(constant, ivalue6);

                code2_len = *pc;                    // byte code len
                pc++;

                ivalue7 = *pc;                      // byte code offset
                pc++;

                code_buf = (int*)CONS_str(constant, ivalue7);

                ovalue1 = create_block(klass1, const_buf, constant2_len, code_buf, code2_len, ivalue2, ivalue3, ivalue4, var, num_vars);

                info->stack_ptr->mObjectValue = ovalue1;
                info->stack_ptr++;
                vm_mutex_unlock();
                }
                break;

            case OP_NEW_SPECIAL_CLASS_OBJECT:
VMLOG(info, "OP_NEW_SPECIAL_CLASS_OBJECT\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;                                  // class name
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    entry_exception_object(info, gExClassNotFoundType.mClass, "can't get a class named %s\n", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                if(klass1->mCreateFun == NULL) {
                    entry_exception_object(info, gExceptionType.mClass, "can't create object of this special class(%s) because of no creating object function\n", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                info->stack_ptr->mObjectValue = klass1->mCreateFun(klass1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_NEW_OBJECT:
VMLOG(info, "NEW_OBJECT\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;                      // class name
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    entry_exception_object(info, gExClassNotFoundType.mClass, "can't get a class named %s\n", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                if(!create_user_object(klass1, &ovalue1)) {
                    vm_mutex_unlock();
                    return FALSE;
                }

                info->stack_ptr->mObjectValue = ovalue1;
                info->stack_ptr++;
VMLOG(info, "NEW_OBJECT2\n");
                vm_mutex_unlock();
                break;

            case OP_INVOKE_METHOD:
VMLOG(info, "OP_INVOKE_METHOD\n");
                pc++;

                /// type data ///
                memset(&generics_type, 0, sizeof(generics_type));

                cvalue1 = (char)*pc;      // generics type num
                pc++;

                generics_type.mGenericsTypesNum = cvalue1;

                for(i=0; i<cvalue1; i++) {
                    ivalue1 = *pc;                              // generics type offset
                    pc++;

                    real_class_name = CONS_str(constant, ivalue1);    // real class name of a param
                    generics_type.mGenericsTypes[i] = cl_get_class_with_generics(real_class_name, type_);

                    if(generics_type.mGenericsTypes[i] == NULL) {
                        entry_exception_object(info, gExClassNotFoundType.mClass, "can't get a class named %s\n", real_class_name);
                        return FALSE;
                    }
                }

                /// method data ///
                ivalue1 = *pc;                                                      // method name offset
                pc++;

                ivalue2 = *pc;                                                      // method num params
                pc++;
                
                memset(params, 0, sizeof(params));

                for(i=0; i<ivalue2; i++) {
                    if(!get_node_type_from_bytecode(&pc, constant, &params[i], &generics_type, info)) {
                        return FALSE;
                    }
                }

                memset(params2, 0, sizeof(params2));

                ivalue11 = *pc;                         // existance of block
                pc++;

                ivalue3 = *pc;                           // method block num params
                pc++; 

                for(i=0; i<ivalue3; i++) {          // block params
                    if(!get_node_type_from_bytecode(&pc, constant, &params2[i], &generics_type, info)) {
                        return FALSE;
                    }
                }

                ivalue4 = *pc;                          // the existance of block result
                pc++;

                if(ivalue4) {
                    memset(&type2, 0, sizeof(type2));   // block result type
                    if(!get_node_type_from_bytecode(&pc, constant, &type2, &generics_type, info)) {
                        return FALSE;
                    }
                }
                else {
                    memset(&type2, 0, sizeof(type2));   // block result type
                }

                ivalue5 = *pc;  // existance of result
                pc++;

                ivalue6 = *pc;  // super
                pc++;

                ivalue7 = *pc;   // class method
                pc++;

                ivalue8 = *pc;                           // method num block
                pc++;

                ivalue12 = *pc;                          // num params
                pc++;

ASSERT(ivalue11 == 1 && type2.mClass != NULL || ivalue11 == 0);

                ivalue9 = *pc;   // object kind
                pc++;

                if(ivalue9 == INVOKE_METHOD_KIND_OBJECT) {
                    ovalue1 = (info->stack_ptr-ivalue2-ivalue8-1)->mObjectValue;   // get self

                    if(ivalue6) { // super
                        vm_mutex_lock();
                        klass1 = CLOBJECT_HEADER(ovalue1)->mClass;
                        vm_mutex_unlock();

                        if(klass1 == NULL) {
                            entry_exception_object(info, gExceptionType.mClass, "can't get a class from object #%lu\n", ovalue1);
                            return FALSE;
                        }

                        klass1 = get_super(klass1);

                        if(klass1 == NULL) {
                            entry_exception_object(info, gExceptionType.mClass, "can't get a super class from object #%lu\n", ovalue1);
                            return FALSE;
                        }
                    }
                    else {
                        vm_mutex_lock();
                        klass1 = CLOBJECT_HEADER(ovalue1)->mClass;
                        vm_mutex_unlock();

                        if(klass1 == NULL) {
                            entry_exception_object(info, gExceptionType.mClass, "can't get a class from object #%lu\n", ovalue1);
                            return FALSE;
                        }
                    }
                }
                else {
                    ivalue10 = *pc;                  // class name
                    pc++;

                    real_class_name = CONS_str(constant, ivalue10);
                    klass1 = cl_get_class_with_generics(real_class_name, &generics_type);

                    if(klass1 == NULL) {
                        entry_exception_object(info, gExClassNotFoundType.mClass, "can't get a class named %s\n", real_class_name);
                        return FALSE;
                    }
                }

                method = get_virtual_method_with_params(klass1, CONS_str(constant, ivalue1), params, ivalue2, &klass2, ivalue7, &generics_type, ivalue11, ivalue3, params2, &type2);
                if(method == NULL) {
                    entry_exception_object(info, gExceptionType.mClass, "can't get a method named %s.%s\n", REAL_CLASS_NAME(klass1), CONS_str(constant, ivalue1));
                    return FALSE;
                }
VMLOG(info, "OP_INVOKE_METHOD(%s.%s) starts\n", REAL_CLASS_NAME(klass2), METHOD_NAME2(klass2, method));
VMLOG(info, "pc - code->mCode %d code->mCode %p\n", pc - code->mCode, code->mCode);

                if(!excute_method(method, klass2, &klass2->mConstPool, ivalue5, &generics_type, ivalue12, info)) 
                {
                    return FALSE;
                }

VMLOG(info, "OP_INVOKE_METHOD(%s.%s) end\n", REAL_CLASS_NAME(klass2), METHOD_NAME2(klass2, method));
VMLOG(info, "pc - code->mCode %d code->mCode %p\n", pc - code->mCode, code->mCode);
                break;

            case OP_INVOKE_INHERIT:
VMLOG(info, "OP_INVOKE_INHERIT\n");
                pc++;

                ivalue1 = *pc;                  // real class name offset
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class_with_generics(real_class_name, type_);

                if(klass1 == NULL) {
                    entry_exception_object(info, gExClassNotFoundType.mClass, "can't get a class named %s\n", real_class_name);
                    return FALSE;
                }

                ivalue2 = *pc;                  // method index
                pc++;

                ivalue3 = *pc;                  // existance of result
                pc++;

                ivalue4 = *pc;                  // num params
                pc++;

                method = klass1->mMethods + ivalue2;
VMLOG(info, "klass1 %s\n", REAL_CLASS_NAME(klass1));
VMLOG(info, "method name (%s)\n", METHOD_NAME(klass1, ivalue2));
                if(!excute_method(method, klass1, &klass1->mConstPool, ivalue3, NULL, ivalue4, info))
                {
                    return FALSE;
                }
                break;

            case OP_INVOKE_BLOCK:
VMLOG(info, "OP_INVOKE_BLOCK\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;          // bloc index
                pc++;

                ivalue2 = *pc;         // result existance
                pc++;

                ovalue1 = var[ivalue1].mObjectValue;

                if(!excute_block(ovalue1, type_, ivalue2, info)) 
                {
                    vm_mutex_unlock();
                    return FALSE;
                }
                vm_mutex_unlock();
                break;

            case OP_RETURN:
VMLOG(info, "OP_RETURN\n");
                pc++;

                return TRUE;

            case OP_THROW:
VMLOG(info, "OP_THROW\n");
                pc++;

                return FALSE;

            case OP_TRY:
VMLOG(info, "OP_TRY\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;
                pc++;

                ivalue2 = *pc;
                pc++;

                ivalue3 = *pc;
                pc++;

                ivalue4 = *pc;                  // the result existance of finally block
                pc++;

                ovalue1 = var[ivalue1].mObjectValue;  // try block object
                ovalue2 = var[ivalue2].mObjectValue;  // catch block object
                if(ivalue3 == -1) {
                    ovalue3 = 0;                          // finally block object
                }
                else {
                    ovalue3 = var[ivalue3].mObjectValue; // finally block object
                }

                /// try block ///
                result = excute_block(ovalue1, type_, FALSE, info);
VMLOG(info, "try was finished\n");
                if(result) {
                    if(ovalue3) {
                        /// finally ///
                        if(!excute_block(ovalue3, type_, ivalue4, info)) {
                            vm_mutex_unlock();
                            return FALSE;
                        }
                    }
                }
                else {
                    /// catch ///
                    result = excute_block(ovalue2, type_, FALSE, info);

                    if(result) {
                        /// finally ///
                        if(ovalue3) {
                            if(!excute_block(ovalue3, type_, ivalue4, info)) {
                                vm_mutex_unlock();
                                return FALSE;
                            }
                        }
                    }
                    else {
                        vm_mutex_unlock();
                        return FALSE;
                    }
                }

                vm_mutex_unlock();
                break;

            case OP_IADD:
VMLOG(info, "OP_IADD\n");
                pc++;

                ivalue1 = (info->stack_ptr-2)->mIntValue + (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_IADD %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BADD:
VMLOG(info, "OP_BADD\n");
                pc++;

                bvalue1 = (info->stack_ptr-2)->mByteValue + (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_FADD:
VMLOG(info, "OP_FADD\n");
                pc++;

                fvalue1 = (info->stack_ptr-2)->mFloatValue + (info->stack_ptr-1)->mFloatValue;
                info->stack_ptr-=2;
                info->stack_ptr->mFloatValue = fvalue1;
VMLOG(info, "OP_FADD %d\n", info->stack_ptr->mFloatValue);
                info->stack_ptr++;
                break;

            case OP_SADD:
VMLOG(info, "OP_SADD\n");
                vm_mutex_lock();
                pc++;

                ovalue1 = (info->stack_ptr-2)->mObjectValue;
                ovalue2 = (info->stack_ptr-1)->mObjectValue;

                info->stack_ptr-=2;

                ivalue1 = CLSTRING(ovalue1)->mLen;  // string length of ovalue1
                ivalue2 = CLSTRING(ovalue2)->mLen;  // string length of ovalue2

                str = MALLOC(sizeof(wchar_t)*(ivalue1 + ivalue2 + 1));

                wcscpy(str, CLSTRING(ovalue1)->mChars);
                wcscat(str, CLSTRING(ovalue2)->mChars);

                klass1 = CLOBJECT_HEADER(ovalue1)->mClass;

                ovalue3 = create_string_object(klass1, str, ivalue1 + ivalue2);

                info->stack_ptr->mObjectValue = ovalue3;
VMLOG(info, "OP_SADD %ld\n", ovalue3);
                info->stack_ptr++;

                FREE(str);
                vm_mutex_unlock();
                break;

            case OP_ISUB:
VMLOG(info, "OP_ISUB\n");
                pc++;
    
                ivalue1 = (info->stack_ptr-2)->mIntValue - (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_ISUB %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BSUB:
VMLOG(info, "OP_BSUB\n");
                pc++;
    
                bvalue1 = (info->stack_ptr-2)->mByteValue - (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_FSUB:
VMLOG(info, "OP_FSUB\n");
                pc++;

                fvalue1 = (info->stack_ptr-2)->mFloatValue - (info->stack_ptr-1)->mFloatValue;
                info->stack_ptr-=2;
                info->stack_ptr->mFloatValue = fvalue1;
VMLOG(info, "OP_FSUB %d\n", info->stack_ptr->mFloatValue);
                info->stack_ptr++;
                break;

            case OP_IMULT:
VMLOG(info, "OP_IMULT\n");
                pc++;

                ivalue1 = (info->stack_ptr-2)->mIntValue * (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_IMULT %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BMULT:
VMLOG(info, "OP_BMULT\n");
                pc++;

                bvalue1 = (info->stack_ptr-2)->mByteValue * (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_FMULT:
VMLOG(info, "OP_FMULT\n");
                pc++;

                fvalue1 = (info->stack_ptr-2)->mFloatValue * (info->stack_ptr-1)->mFloatValue;
                info->stack_ptr-=2;
                info->stack_ptr->mFloatValue = fvalue1;
VMLOG(info, "OP_FMULT %d\n", info->stack_ptr->mFloatValue);
                info->stack_ptr++;
                break;

            case OP_IDIV:
VMLOG(info, "OP_IDIV\n");
                pc++;

                if((info->stack_ptr-1)->mIntValue == 0) {
                    entry_exception_object(info, gExceptionType.mClass, "division by zero");
                    return FALSE;
                }

                ivalue1 = (info->stack_ptr-2)->mIntValue / (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_IDIV %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BDIV:
VMLOG(info, "OP_BDIV\n");
                pc++;

                if((info->stack_ptr-1)->mByteValue == 0) {
                    entry_exception_object(info, gExceptionType.mClass, "division by zero");
                    return FALSE;
                }

                bvalue1 = (info->stack_ptr-2)->mByteValue / (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_FDIV:
VMLOG(info, "OP_FDIV\n");
                pc++;

                if((info->stack_ptr-1)->mFloatValue == 0.0) {
                    entry_exception_object(info, gExceptionType.mClass, "division by zero");
                    return FALSE;
                }

                fvalue1 = (info->stack_ptr-2)->mFloatValue / (info->stack_ptr-1)->mFloatValue;
                info->stack_ptr-=2;
                info->stack_ptr->mFloatValue = fvalue1;
VMLOG(info, "OP_FDIV %d\n", info->stack_ptr->mFloatValue);
                info->stack_ptr++;
                break;

            case OP_IMOD:
VMLOG(info, "OP_IMOD\n");
                pc++;

                if((info->stack_ptr-2)->mIntValue == 0) {
                    entry_exception_object(info, gExceptionType.mClass, "remainder by zero");
                    return FALSE;
                }

                ivalue1 = (info->stack_ptr-2)->mIntValue % (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_IMOD %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BMOD:
VMLOG(info, "OP_BMOD\n");
                pc++;

                if((info->stack_ptr-2)->mByteValue == 0) {
                    entry_exception_object(info, gExceptionType.mClass, "remainder by zero");
                    return FALSE;
                }

                bvalue1 = (info->stack_ptr-2)->mByteValue % (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_ILSHIFT:
VMLOG(info, "OP_ILSHIFT\n");
                pc++;

                if((info->stack_ptr-2)->mIntValue == 0) {
                    entry_exception_object(info, gExceptionType.mClass, "division by zero");
                    return FALSE;
                }

                ivalue1 = (info->stack_ptr-2)->mIntValue << (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_ILSHIFT %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BLSHIFT:
VMLOG(info, "OP_BLSHIFT\n");
                pc++;

                if((info->stack_ptr-2)->mByteValue == 0) {
                    entry_exception_object(info, gExceptionType.mClass, "division by zero");
                    return FALSE;
                }

                bvalue1 = (info->stack_ptr-2)->mByteValue << (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_IRSHIFT:
VMLOG(info, "OP_IRSHIFT\n");
                pc++;
                if((info->stack_ptr-2)->mIntValue == 0) {
                    entry_exception_object(info, gExceptionType.mClass, "division by zero");
                    return FALSE;
                }

                ivalue1 = (info->stack_ptr-2)->mIntValue >> (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_IRSHIFT %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BRSHIFT:
VMLOG(info, "OP_BRSHIFT\n");
                pc++;
                if((info->stack_ptr-2)->mByteValue == 0) {
                    entry_exception_object(info, gExceptionType.mClass, "division by zero");
                    return FALSE;
                }

                bvalue1 = (info->stack_ptr-2)->mByteValue >> (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_IGTR:
VMLOG(info, "OP_IGTR\n");
                pc++;
                ivalue1 = (info->stack_ptr-2)->mIntValue > (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_IGTR %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BGTR:
VMLOG(info, "OP_BGTR\n");
                pc++;
                bvalue1 = (info->stack_ptr-2)->mByteValue > (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_FGTR:
VMLOG(info, "OP_FGTR\n");
                pc++;

                ivalue1 = (info->stack_ptr-2)->mFloatValue > (info->stack_ptr-1)->mFloatValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_FGTR %f\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_IGTR_EQ:
                pc++;
                ivalue1 = (info->stack_ptr-2)->mIntValue >= (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_IGTR_EQ %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BGTR_EQ:
                pc++;
                bvalue1 = (info->stack_ptr-2)->mByteValue >= (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_FGTR_EQ:
                pc++;

                ivalue1 = (info->stack_ptr-2)->mFloatValue >= (info->stack_ptr-1)->mFloatValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_FGTR_EQ %f\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_ILESS:
                pc++;
                ivalue1 = (info->stack_ptr-2)->mIntValue < (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_ILESS %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BLESS:
                pc++;
                bvalue1 = (info->stack_ptr-2)->mByteValue < (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_FLESS:
                pc++;

                ivalue1 = (info->stack_ptr-2)->mFloatValue < (info->stack_ptr-1)->mFloatValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_FLESS %f\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_ILESS_EQ:
                pc++;
                ivalue1 = (info->stack_ptr-2)->mIntValue <= (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_ILESS_EQ %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BLESS_EQ:
                pc++;
                bvalue1 = (info->stack_ptr-2)->mByteValue <= (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_FLESS_EQ:
                pc++;

                ivalue1 = (info->stack_ptr-2)->mFloatValue <= (info->stack_ptr-1)->mFloatValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_FLESS_EQ %f\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_IEQ:
                pc++;
                ivalue1 = (info->stack_ptr-2)->mIntValue == (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_IEQ %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BEQ:
                pc++;
                bvalue1 = (info->stack_ptr-2)->mByteValue == (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_FEQ:
                pc++;

                ivalue1 = (info->stack_ptr-2)->mFloatValue == (info->stack_ptr-1)->mFloatValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_FEQ %f\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_SEQ:
                vm_mutex_lock();
                pc++;

                ovalue1 = (info->stack_ptr-2)->mObjectValue;
                ovalue2 = (info->stack_ptr-1)->mObjectValue;
                
                ivalue1 = (wcscmp(CLSTRING(ovalue1)->mChars, CLSTRING(ovalue2)->mChars) == 0);
                
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_SEQ %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_INOTEQ:
                pc++;
                ivalue1 = (info->stack_ptr-2)->mIntValue != (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_INOTEQ %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BNOTEQ:
                pc++;
                bvalue1 = (info->stack_ptr-2)->mByteValue != (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_FNOTEQ:
                pc++;

                ivalue1 = (info->stack_ptr-2)->mFloatValue != (info->stack_ptr-1)->mFloatValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_FNOTEQ %f\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_SNOTEQ:
                vm_mutex_lock();
                pc++;

                ovalue1 = (info->stack_ptr-2)->mObjectValue;
                ovalue2 = (info->stack_ptr-1)->mObjectValue;
                
                ivalue1 = (wcscmp(CLSTRING(ovalue1)->mChars, CLSTRING(ovalue2)->mChars) != 0);
                
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_SNOTEQ %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IAND:
                pc++;
                ivalue1 = (info->stack_ptr-2)->mIntValue & (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_IAND %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BAND:
                pc++;
                bvalue1 = (info->stack_ptr-2)->mByteValue & (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_IXOR:
                pc++;
                ivalue1 = (info->stack_ptr-2)->mIntValue ^ (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_IXOR %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BXOR:
                pc++;
                bvalue1 = (info->stack_ptr-2)->mByteValue ^ (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_IOR:
                pc++;
                ivalue1 = (info->stack_ptr-2)->mIntValue | (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_IOR %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_BOR:
                pc++;
                bvalue1 = (info->stack_ptr-2)->mByteValue | (info->stack_ptr-1)->mByteValue;
                info->stack_ptr-=2;
                info->stack_ptr->mByteValue = bvalue1;
                info->stack_ptr++;
                break;

            case OP_IOROR:
                pc++;
                ivalue1 = (info->stack_ptr-2)->mIntValue || (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_IOROR %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_IANDAND:
                pc++;
                ivalue1 = (info->stack_ptr-2)->mIntValue && (info->stack_ptr-1)->mIntValue;
                info->stack_ptr-=2;
                info->stack_ptr->mIntValue = ivalue1;
VMLOG(info, "OP_IANDAND %d\n", info->stack_ptr->mIntValue);
                info->stack_ptr++;
                break;

            case OP_FOROR:
                pc++;
                fvalue1 = (info->stack_ptr-2)->mFloatValue || (info->stack_ptr-1)->mFloatValue;
                info->stack_ptr-=2;
                info->stack_ptr->mFloatValue = fvalue1;
VMLOG(info, "OP_FOROR %f\n", info->stack_ptr->mFloatValue);
                info->stack_ptr++;
                break;

            case OP_FANDAND:
                pc++;
                fvalue1 = (info->stack_ptr-2)->mFloatValue && (info->stack_ptr-1)->mFloatValue;
                info->stack_ptr-=2;
                info->stack_ptr->mFloatValue = fvalue1;
VMLOG(info, "OP_FANDAND %d\n", info->stack_ptr->mFloatValue);
                info->stack_ptr++;
                break;

            case OP_POP:
VMLOG(info, "OP_POP\n");
                info->stack_ptr--;
                pc++;
                break;

            case OP_POP_N:
                pc++;

                ivalue1 = *pc;
                pc++;
VMLOG(info, "OP_POP %d\n", ivalue1);

                info->stack_ptr -= ivalue1;
                break;

            case OP_LOGICAL_DENIAL:
                pc++;

                ivalue1 = (info->stack_ptr-1)->mIntValue;
                ivalue1 = !ivalue1;

                (info->stack_ptr-1)->mIntValue = ivalue1;
VMLOG(info, "OP_LOGICAL_DENIAL %d\n", ivalue1);
                break;

            case OP_COMPLEMENT:
                pc++;

                ivalue1 = (info->stack_ptr-1)->mIntValue;
                ivalue1 = ~ivalue1;

                (info->stack_ptr-1)->mIntValue = ivalue1;
VMLOG(info, "OP_COMPLEMENT %d\n", ivalue1);
                break;

            case OP_BCOMPLEMENT:
                pc++;

                bvalue1 = (info->stack_ptr-1)->mByteValue;
                bvalue1 = ~bvalue1;

                (info->stack_ptr-1)->mByteValue = bvalue1;
                break;

            case OP_DEC_VALUE:
VMLOG(info, "OP_DEC_VALUE\n");
                pc++;

                ivalue1 = *pc;
                pc++;

                (info->stack_ptr-1)->mIntValue -= ivalue1;
                break;

            case OP_INC_VALUE:
VMLOG(info, "OP_INC_VALUE\n");
                pc++;

                ivalue1 = *pc;
                pc++;

                (info->stack_ptr-1)->mIntValue += ivalue1;
VMLOG(info, "OP_INC_VALUE %d\n", ivalue1);
                break;

            case OP_IF: {
VMLOG(info, "OP_IF\n");
                pc++;

                ivalue2 = *pc;
                pc++;

                /// get result of conditional ///
                ivalue1 = (info->stack_ptr-1)->mIntValue;
                info->stack_ptr--;

                if(ivalue1) {
                    pc += ivalue2;
                }
                }
                break;

            case OP_NOTIF: {
VMLOG(info, "OP_NOTIF\n");
                pc++;

                ivalue2 = *pc;
                pc++;

                /// get result of conditional ///
                ivalue1 = (info->stack_ptr-1)->mIntValue;
                info->stack_ptr--;

                if(!ivalue1) {
                    pc += ivalue2;
                }
                }
                break;

            case OP_GOTO:
VMLOG(info, "OP_GOTO\n");
                pc++;

                ivalue1 = *pc;
                pc++;

                pc = code->mCode + ivalue1;
                break;

            default:
                cl_print("invalid op code(%d). unexpected error at cl_vm\n", *pc);
                exit(1);
        }
SHOW_STACK(info, top_of_stack, var);
SHOW_HEAP(info);
    }

VMLOG(info, "vm finished pc - code->mCode --> %d code->mLen %d\n", pc - code->mCode, code->mLen);

    return TRUE;
}

/// result (TRUE): success (FALSE): threw exception
BOOL cl_main(sByteCode* code, sConst* constant, int lv_num, int max_stack, int stack_size)
{
    MVALUE* lvar;
    BOOL result;
    sVMInfo info;

    vm_mutex_lock();

    info.stack = CALLOC(1, sizeof(MVALUE)*stack_size);
    info.stack_size = stack_size;
    info.stack_ptr = info.stack;
    lvar = info.stack;
    info.stack_ptr += lv_num;

START_VMLOG(&info);
VMLOG(&info, "cl_main lv_num %d max_stack %d\n", lv_num, max_stack);

    push_vminfo(&info);

    if(info.stack_ptr + max_stack > info.stack + info.stack_size) {
        entry_exception_object(&info, gExceptionType.mClass, "overflow stack size\n");
        output_exception_message(&info);
        FREE(info.stack);
        pop_vminfo(&info);
        vm_mutex_unlock();
        return FALSE;
    }

VMLOG(&info, "cl_main lv_num2 %d\n", lv_num);

    vm_mutex_unlock();

    result = cl_vm(code, constant, lvar, NULL, &info);

    vm_mutex_lock();

    if(result == FALSE) {
        output_exception_message(&info); // show exception message
    }

    FREE(info.stack);

    pop_vminfo(&info);

    vm_mutex_unlock();

    return result;
}

BOOL field_initializer(MVALUE* result, sByteCode* code, sConst* constant, int lv_num, int max_stack)
{
    MVALUE* lvar;
    BOOL vm_result;
    sVMInfo info;
    MVALUE mvalue[CL_FILED_INITIALIZAR_STACK_SIZE];

    vm_mutex_lock();

    info.stack = mvalue;
    memset(&mvalue, 0, sizeof(MVALUE)*CL_FILED_INITIALIZAR_STACK_SIZE);
    info.stack_size = CL_FILED_INITIALIZAR_STACK_SIZE;
    info.stack_ptr = info.stack;

START_VMLOG(&info);
VMLOG(&info, "field_initializer\n");

    push_vminfo(&info);

    lvar = info.stack_ptr;
    info.stack_ptr += lv_num;

    if(info.stack_ptr + max_stack > info.stack + info.stack_size) {
        entry_exception_object(&info, gExceptionType.mClass, "overflow stack size\n");
        output_exception_message(&info);
        pop_vminfo(&info);
        vm_mutex_unlock();
        return FALSE;
    }

    vm_result = cl_vm(code, constant, lvar, NULL, &info);
    *result = *(info.stack_ptr-1);

    info.stack_ptr = lvar;

    if(!vm_result) {
        output_exception_message(&info); // show exception message
    }

    pop_vminfo(&info);

    vm_mutex_unlock();

    return vm_result;
}

static BOOL excute_method(sCLMethod* method, sCLClass* klass, sConst* constant, BOOL result_existance, sCLNodeType* type_, int num_params, sVMInfo* info)
{
    int real_param_num;
    BOOL result;
    BOOL native_result;
    BOOL external_result;
    int return_count;

    real_param_num = method->mNumParams + (method->mFlags & CL_CLASS_METHOD ? 0:1) + method->mNumBlockType;

    if(method->mFlags & CL_NATIVE_METHOD) {
        MVALUE* lvar;
        BOOL synchronized;

        lvar = info->stack_ptr - real_param_num;

        if(info->stack_ptr + method->mMaxStack > info->stack + info->stack_size) {
            entry_exception_object(info, gExceptionType.mClass, "overflow stack size\n");
            return FALSE;
        }

        synchronized = method->mFlags & CL_SYNCHRONIZED_METHOD;

        if(synchronized) vm_mutex_lock();
        native_result = method->uCode.mNativeMethod(&info->stack_ptr, lvar, info);
        if(synchronized) vm_mutex_unlock();

        if(native_result) {
            if(result_existance) {
                MVALUE* mvalue;

                mvalue = info->stack_ptr-1;
                info->stack_ptr = lvar;
                *info->stack_ptr = *mvalue;
                info->stack_ptr++;
            }
            else {
                info->stack_ptr = lvar;
            }

            return TRUE;
        }
        else {
            MVALUE* mvalue;

            mvalue = info->stack_ptr-1;
            info->stack_ptr = lvar;
            *info->stack_ptr = *mvalue;
            info->stack_ptr++;

            return FALSE;
        }
    }
    else {
        MVALUE* lvar;
        BOOL synchronized;

        lvar = info->stack_ptr - real_param_num;
        if(method->mNumLocals - real_param_num > 0) {
            info->stack_ptr += (method->mNumLocals - real_param_num);     // forwarded stack pointer for local variable
        }

        if(info->stack_ptr + method->mMaxStack > info->stack + info->stack_size) {
            entry_exception_object(info, gExceptionType.mClass, "overflow stack size\n");
            return FALSE;
        }

        synchronized = method->mFlags & CL_SYNCHRONIZED_METHOD;

        if(synchronized) vm_mutex_lock();
        result = cl_vm(&method->uCode.mByteCodes, constant, lvar, type_, info);
        if(synchronized) vm_mutex_unlock();

        if(!result) {
            MVALUE* mvalue;

            mvalue = info->stack_ptr-1;
            info->stack_ptr = lvar;
            *info->stack_ptr = *mvalue;
            info->stack_ptr++;

            return FALSE;
        }
        else if(result_existance) 
        {
            MVALUE* mvalue;

            mvalue = info->stack_ptr-1;
            info->stack_ptr = lvar;
            *info->stack_ptr = *mvalue;
            info->stack_ptr++;
        }
        else {
            info->stack_ptr = lvar;
        }

        return result;
    }
}

BOOL cl_excute_method(sCLMethod* method, sCLClass* klass, sConst* constant, BOOL result_existance, sCLNodeType* type_, int num_params, sVMInfo* info)
{
    return excute_method(method, klass, constant, result_existance, type_, num_params, info);
}

static BOOL excute_block(CLObject block, sCLNodeType* type_, BOOL result_existance, sVMInfo* info)
{
    int real_param_num;
    MVALUE* lvar;
    BOOL result;

    real_param_num = CLBLOCK(block)->mNumParams;

    lvar = info->stack_ptr - real_param_num;

    /// copy caller local vars to current local vars ///
    memmove(lvar + CLBLOCK(block)->mNumParentVar, lvar, sizeof(MVALUE)*real_param_num);
    memmove(lvar, CLBLOCK(block)->mParentLocalVar, sizeof(MVALUE)*CLBLOCK(block)->mNumParentVar);

    info->stack_ptr += CLBLOCK(block)->mNumParentVar;

    if(CLBLOCK(block)->mNumLocals - real_param_num > 0) {
        info->stack_ptr += (CLBLOCK(block)->mNumLocals - real_param_num);     // forwarded stack pointer for local variable
    }

    if(info->stack_ptr + CLBLOCK(block)->mMaxStack > info->stack + info->stack_size) {
        entry_exception_object(info, gExceptionType.mClass, "overflow stack size\n");
        return FALSE;
    }

    result = cl_vm(CLBLOCK(block)->mCode, CLBLOCK(block)->mConstant, lvar, type_, info);

    /// restore caller local vars, and restore base of all block stack  ///
    memmove(CLBLOCK(block)->mParentLocalVar, lvar, sizeof(MVALUE)*CLBLOCK(block)->mNumParentVar);

    if(!result) {
        MVALUE* mvalue;

        mvalue = info->stack_ptr-1;
        info->stack_ptr = lvar;
        *info->stack_ptr = *mvalue;
        info->stack_ptr++;
    }
    else if(result_existance) {
        MVALUE* mvalue;

        mvalue = info->stack_ptr-1;
        info->stack_ptr = lvar;
        *info->stack_ptr = *mvalue;
        info->stack_ptr++;
    }
    else {
        info->stack_ptr = lvar;
    }

    return result;
}

BOOL cl_excute_block(CLObject block, sCLNodeType* type_, BOOL result_existance, BOOL static_method_block, sVMInfo* info)
{
    return excute_block(block, type_, result_existance, info);
}

static BOOL excute_block_with_new_stack(MVALUE* result, CLObject block, sCLNodeType* type_, BOOL result_existance, sVMInfo* new_info)
{
    BOOL vm_result;
    MVALUE* lvar;
    sByteCode* code;
    sConst* constant;

    lvar = new_info->stack;

    if(new_info->stack_ptr + CLBLOCK(block)->mMaxStack > new_info->stack + new_info->stack_size) {
        entry_exception_object(new_info, gExceptionType.mClass, "overflow stack size\n");
        output_exception_message(new_info); // show exception message
        FREE(new_info->stack);
        pop_vminfo(new_info);
        vm_mutex_unlock();
        return FALSE;
    }

    code = CLBLOCK(block)->mCode;
    constant = CLBLOCK(block)->mConstant;

    vm_mutex_unlock();

START_VMLOG(new_info);

    vm_result = cl_vm(code, constant, lvar, type_, new_info);

    vm_mutex_lock();

    if(!vm_result) {
        *result = *(new_info->stack_ptr-1);
        output_exception_message(new_info); // show exception message
    }
    else if(result_existance) {
        *result = *(new_info->stack_ptr-1);
    }

    FREE(new_info->stack);

    pop_vminfo(new_info);

    vm_mutex_unlock();

    return vm_result;
}

BOOL cl_excute_block_with_new_stack(MVALUE* result, CLObject block, sCLNodeType* type_, BOOL result_existance, sVMInfo* new_info)
{
    return excute_block_with_new_stack(result, block, type_, result_existance, new_info);
}

