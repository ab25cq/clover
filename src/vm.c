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
    free_cl_types();
    heap_final();
    class_final();
    thread_final();
}

void push_object(CLObject object, sVMInfo* info)
{
    info->stack_ptr->mObjectValue.mValue = object;
    info->stack_ptr++;
}

// remove the object from stack
CLObject pop_object(sVMInfo* info)
{
    info->stack_ptr--;
    return info->stack_ptr->mObjectValue.mValue;
}

void remove_object2(CLObject obj, sVMInfo* info)
{
    MVALUE* stack_ptr;

    stack_ptr = info->stack_ptr;
    while(stack_ptr >= info->stack) {
        if(stack_ptr->mObjectValue.mValue == obj) {
            memcpy(stack_ptr, stack_ptr + 1, sizeof(MVALUE)*(info->stack_ptr - info->stack));
            info->stack_ptr--;
            break;
        }

        stack_ptr--;
    }
}

void remove_object(sVMInfo* info, int number)
{
    info->stack_ptr-=number;
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
    CLObject type1;

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    vm_mutex_lock();

    type1 = create_type_object(klass);

    (void)create_user_object(type1, &ovalue, 0, info);

    info->stack_ptr->mObjectValue.mValue = ovalue;
    info->stack_ptr++;

    wcs = MALLOC(sizeof(wchar_t)*(strlen(msg2)+1));
    (void)mbstowcs(wcs, msg2, strlen(msg2)+1);
    size = wcslen(wcs);

    ovalue2 = create_string_object(wcs, size, gStringTypeObject, info);

    CLUSEROBJECT(ovalue)->mFields[0].mObjectValue.mValue = ovalue2;

    FREE(wcs);

    vm_mutex_unlock();
}

static void output_exception_message(sVMInfo* info)
{
    CLObject exception;
    CLObject message;
    char* mbs;
    CLObject type_object;

SHOW_STACK2(info);
SHOW_HEAP(info);

    exception = (info->stack_ptr-1)->mObjectValue.mValue;

    if(!check_type(exception, gExceptionTypeObject, info)) {
        fprintf(stderr, "invalid exception object\n");
        return;
    }

    message = CLUSEROBJECT(exception)->mFields[0].mObjectValue.mValue;
    if(!check_type(message, gStringTypeObject, info)) {
        fprintf(stderr, "invalid exception message object\n");
        return;
    }

    mbs = CALLOC(1, MB_LEN_MAX*(CLSTRING(message)->mLen + 1));

    (void)wcstombs(mbs, CLSTRING_DATA(message)->mChars, CLSTRING(message)->mLen + 1);

    type_object = CLOBJECT_HEADER(exception)->mType;

    fprintf(stderr, "%s: %s\n", CLASS_NAME(CLTYPEOBJECT(type_object)->mClass), mbs);

    FREE(mbs);
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

static void show_type_object2(CLObject type_object, sVMInfo* info)
{
    int i;

    if(type_object == 0) return;

    if(CLTYPEOBJECT(type_object)->mGenericsTypesNum > 0) {
        vm_log(info, "%s<", REAL_CLASS_NAME(CLTYPEOBJECT(type_object)->mClass));
    }
    else {
        vm_log(info, "%s", REAL_CLASS_NAME(CLTYPEOBJECT(type_object)->mClass));
    }

    for(i=0; i<CLTYPEOBJECT(type_object)->mGenericsTypesNum; i++) {
        CLObject type_object2;

        type_object2 = CLTYPEOBJECT(type_object)->mGenericsTypes[i];
        show_type_object2(type_object2, info);

        if(i != CLTYPEOBJECT(type_object)->mGenericsTypesNum-1) {
            vm_log(info, ",");
        }
    }

    if(CLTYPEOBJECT(type_object)->mGenericsTypesNum > 0) {
        vm_log(info, ">");
    }
}

void show_object_value(sVMInfo* info, CLObject obj)
{
    CLObject type_object;

    if(obj == 0) {
        vm_log(info, "value %d\n", obj);
    }
    else {
        type_object = CLOBJECT_HEADER(obj)->mType;

        if(type_object) {
            if(CLTYPEOBJECT(type_object)->mClass == gNullClass) {
                vm_log(info, "obj %d data null");
            }
            else if(substitution_posibility_of_type_object_without_generics(gArrayTypeObject, type_object))
            {
                int j;
                vm_log(info, "obj %d data (len %d) (", obj, CLARRAY(obj)->mLen);

                for(j=0; j<10 && j<CLARRAY(obj)->mLen; j++) {
                    vm_log(info, "[%d] %d ", j, CLARRAY_ITEMS2(obj, j));
                }

                vm_log(info, ")");
            }
            else if(substitution_posibility_of_type_object_without_generics(gStringTypeObject, type_object))
            {
                vm_log(info, "obj %d data %d (%ls)", obj, CLSTRING(obj)->mData, CLSTRING_DATA(obj)->mChars);
            }
            else if(substitution_posibility_of_type_object_without_generics(gIntTypeObject, type_object))
            {
                vm_log(info, "obj %d data %d", obj, CLINT(obj)->mValue);
            }
            else if(substitution_posibility_of_type_object_without_generics(gByteTypeObject, type_object))
            {
                vm_log(info, "obj %d data %d", obj, CLBYTE(obj)->mValue);
            }
            else if(substitution_posibility_of_type_object_without_generics(gFloatTypeObject, type_object))
            {
                vm_log(info, "obj %d data %f", obj, CLFLOAT(obj)->mValue);
            }
            else {
                vm_log(info, "obj %d", obj);
            }

            vm_log(info, " ");

            vm_log(info, "type (obj %d) ", type_object);

            show_type_object2(type_object, info);
            vm_log(info, "\n");
        }
        else {
            vm_log(info, "value %d ", obj);
            vm_log(info, "type (obj %d) \n", type_object);
        }
    }
}

void show_stack(sVMInfo* info, MVALUE* top_of_stack, MVALUE* var)
{
    int i;

    vm_mutex_lock();

    if(top_of_stack && var ) {
        vm_log(info, "stack_ptr %d top_of_stack %d var %d\n", (int)(info->stack_ptr - info->stack), (int)(top_of_stack - info->stack), (int)(var - info->stack));

        if(info->stack_ptr < info->stack) 
        {
            vm_log(info, "invalid stack ptr");
            exit(2);
        }

        if(info->stack_ptr > info->stack) {
            for(i=0; i<50; i++) {
                if(info->stack + i == info->stack_ptr) {
                    break;
                }
                if(info->stack + i == var) {
                    if(info->stack + i == info->stack_ptr) {
                        vm_log(info, "->v-- stack[%d] ", i);
                    }
                    else {
                        vm_log(info, "  v-- stack[%d] ", i);
                    }
                    show_object_value(info, (info->stack + i)->mObjectValue.mValue);
                }
                else if(info->stack + i == top_of_stack) {
                    if(info->stack + i == info->stack_ptr) {
                        vm_log(info, "->--- stack[%d] ", i);
                    }
                    else {
                        vm_log(info, "  --- stack[%d] ", i);
                    }
                    show_object_value(info, (info->stack + i)->mObjectValue.mValue);
                }
                else if(info->stack + i == info->stack_ptr) {
                    vm_log(info, "->    stack[%d] ", i);
                    show_object_value(info, (info->stack + i)->mObjectValue.mValue);
                }
                else {
                    vm_log(info, "      stack[%d] ", i);
                    show_object_value(info, (info->stack + i)->mObjectValue.mValue);
                }
            }
        }
        else {
            vm_log(info, "stack is empty\n");
        }
    }
    else {
        for(i=0; i<50; i++) {
            if(info->stack + i == info->stack_ptr) {
                vm_log(info, "->    stack[%d] ", i);
                show_object_value(info, (info->stack + i)->mObjectValue.mValue);
            }
            else {
                vm_log(info, "      stack[%d] ",i);
                show_object_value(info, (info->stack + i)->mObjectValue.mValue);
            }
        }
    }

    vm_mutex_unlock();
}
#endif

static void get_class_name_from_bytecodes(int** pc, sConst* constant, char** type)
{
    int ivalue1;

    ivalue1 = **pc;
    (*pc)++;

    *type = CONS_str(constant, ivalue1);
}

static sCLClass* get_class_info_from_bytecode(int** pc, sConst* constant)
{
    char* real_class_name;

    get_class_name_from_bytecodes(pc, constant, &real_class_name);

    return cl_get_class(real_class_name);
}

static BOOL excute_block(CLObject block, BOOL result_existance, sVMInfo* info, CLObject vm_type);
static BOOL excute_method(sCLMethod* method, sCLClass* klass, sConst* constant, int result_type, sVMInfo* info, CLObject vm_type);
static BOOL param_initializer(sCLClass* klass, sCLMethod* method, int param_num, sVMInfo* info, CLObject vm_type);

CLObject get_type_from_mvalue(MVALUE* mvalue, sVMInfo* info)
{
    CLObject result;

    vm_mutex_lock();
    if(mvalue->mObjectValue.mValue == 0) {
        result = 0;
    }
    else {
        result = CLOBJECT_HEADER(mvalue->mObjectValue.mValue)->mType;
    }
    vm_mutex_unlock();

    return result;
}

static BOOL get_two_int_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        return FALSE;
    }

    if(!check_type(*ovalue1, gIntTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gIntTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_two_float_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        return FALSE;
    }

    if(!check_type(*ovalue1, gFloatTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gFloatTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_two_byte_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        return FALSE;
    }

    if(!check_type(*ovalue1, gByteTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gByteTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_two_string_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        return FALSE;
    }

    if(!check_type(*ovalue1, gStringTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gStringTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_two_bytes_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        return FALSE;
    }

    if(!check_type(*ovalue1, gBytesTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gBytesTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_two_bool_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0 || *ovalue2 == 0) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        return FALSE;
    }

    if(!check_type(*ovalue1, gBoolTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gBoolTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}


static BOOL get_string_and_int_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(ovalue1 == 0 || ovalue2 == 0) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        return FALSE;
    }

    if(!check_type(*ovalue1, gStringTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gIntTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_bytes_and_int_object_from_stack(CLObject* ovalue1, CLObject* ovalue2, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;
    *ovalue2 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(ovalue1 == 0 || ovalue2 == 0) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        return FALSE;
    }

    if(!check_type(*ovalue1, gBytesTypeObject, info)) {
        return FALSE;
    }
    if(!check_type(*ovalue2, gIntTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_one_bool_object_from_stack(CLObject* ovalue1, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        return FALSE;
    }

    if(!check_type(*ovalue1, gBoolTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_one_int_object_from_stack(CLObject* ovalue1, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        return FALSE;
    }

    if(!check_type(*ovalue1, gIntTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL get_one_byte_object_from_stack(CLObject* ovalue1, sVMInfo* info)
{
    *ovalue1 = (info->stack_ptr-1)->mObjectValue.mValue;

    if(*ovalue1 == 0) {
        entry_exception_object(info, gExNullPointerClass, "Null pointer exception");
        return FALSE;
    }

    if(!check_type(*ovalue1, gByteTypeObject, info)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL cl_vm(sByteCode* code, sConst* constant, MVALUE* var, sVMInfo* info, CLObject vm_type)
{
    int ivalue1, ivalue2, ivalue3, ivalue4, ivalue5, ivalue6, ivalue7, ivalue8, ivalue9, ivalue10, ivalue11, ivalue12, ivalue13, ivalue14;
    char cvalue1;
    unsigned char bvalue1, bvalue2, bvalue3, bvalue4, bvalue5;
    float fvalue1, fvalue2, fvalue3;
    CLObject ovalue1, ovalue2, ovalue3;
    MVALUE* mvalue1;
    MVALUE* stack_ptr2;
    CLObject catch_blocks[CL_CATCH_BLOCK_NUMBER_MAX];
    CLObject catch_block_type[CL_CATCH_BLOCK_NUMBER_MAX];

    sCLClass* klass1, *klass2, *klass3;
    sCLMethod* method;
    sCLField* field;
    wchar_t* str;
    char* str2;
    char* real_class_name;
    char* params[CL_METHOD_PARAM_MAX];
    char* params2[CL_METHOD_PARAM_MAX];
    char* string_type1;
    MVALUE objects[CL_ARRAY_ELEMENTS_MAX];
    int num_vars;
    int i, j;
    MVALUE* top_of_stack;
    int* pc;
    CLObject type1, type2, type3;
    fCreateFun create_fun;

    BOOL result;
    int return_count;

    pc = code->mCode;
    top_of_stack = info->stack_ptr;
    num_vars = info->stack_ptr - var;

VMLOG(info, "VM starts\n");

    while(pc - code->mCode < code->mLen) {
VMLOG(info, "pc - code->mCode %d\n", pc - code->mCode);
        switch(*pc) {
            case OP_LDCINT:
VMLOG(info, "OP_LDCINT\n");
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                vm_mutex_lock();
                info->stack_ptr->mObjectValue.mValue = create_int_object(ivalue1);

VMLOG(info, "%d(%d) is created\n", ivalue1, info->stack_ptr->mObjectValue.mValue);

                vm_mutex_unlock();
                info->stack_ptr++;
                break;

            case OP_LDCBOOL:
VMLOG(info, "OP_LDCBOOL\n");
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                vm_mutex_lock();
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);

VMLOG(info, "%d(%d) is created\n", ivalue1, info->stack_ptr->mObjectValue.mValue);

                vm_mutex_unlock();
                info->stack_ptr++;
                break;

            case OP_LDCNULL:
VMLOG(info, "OP_LDCNULL\n");
                pc++;

                vm_mutex_lock();
                info->stack_ptr->mObjectValue.mValue = create_null_object();

VMLOG(info, "null(%d) is created\n", info->stack_ptr->mObjectValue.mValue);

                vm_mutex_unlock();
                info->stack_ptr++;
                break;

            case OP_LDCFLOAT:
VMLOG(info, "OP_LDCFLOAT\n");
                pc++;

                ivalue1 = *pc;          // constant pool offset
                pc++;

                fvalue1 = *(float*)(constant->mConst + ivalue1);

                vm_mutex_lock();
                info->stack_ptr->mObjectValue.mValue = create_float_object(fvalue1);
                vm_mutex_unlock();

VMLOG(info, "%f(%d) is created\n", fvalue1, info->stack_ptr->mObjectValue.mValue);
                info->stack_ptr++;
                break;

            case OP_LDCBYTE:
VMLOG(info, "OP_LDCBYTE\n");
                pc++;

                ivalue1 = *pc;       // constant pool value
                pc++;

                vm_mutex_lock();
                info->stack_ptr->mObjectValue.mValue = create_byte_object(ivalue1);

VMLOG(info, "%d(%d) is created\n", ivalue1, info->stack_ptr->mObjectValue.mValue);

                vm_mutex_unlock();
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
                size = wcslen(wcs);

                info->stack_ptr->mObjectValue.mValue = create_string_object(wcs, size, gStringTypeObject, info);
VMLOG(info, "%ls(%d) is created\n", wcs, info->stack_ptr->mObjectValue.mValue);
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
                size = strlen((const char*)mbs);

                info->stack_ptr->mObjectValue.mValue = create_bytes_object(mbs, size, gBytesTypeObject, info);
VMLOG(info, "%s(%d) is created\n", mbs, info->stack_ptr->mObjectValue.mValue);
                info->stack_ptr++;

                vm_mutex_unlock();
                }
                break;

            case OP_LDTYPE: {
VMLOG(info, "OP_LDTYPE\n");
                CLObject obj;

                pc++;

                vm_mutex_lock();

                obj = create_type_object_from_bytecodes(&pc, code, constant, info);

                if(obj == 0) {
                    vm_mutex_unlock();
                    return FALSE;
                }

                info->stack_ptr->mObjectValue.mValue = obj;
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

                ivalue1 = *pc;                // field index
                pc++;

                ovalue1 = (info->stack_ptr-1)->mObjectValue.mValue;
                info->stack_ptr--;

VMLOG(info, "LD_FIELD object %d field num %d\n", (int)ovalue1, ivalue1);

                *info->stack_ptr = CLUSEROBJECT(ovalue1)->mFields[ivalue1];
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SRFIELD:
VMLOG(info, "OP_SRFIELD\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;                              // field index
                pc++;

                ovalue1 = (info->stack_ptr-2)->mObjectValue.mValue;    // target object
                mvalue1 = info->stack_ptr-1;                    // right value

                CLUSEROBJECT(ovalue1)->mFields[ivalue1] = *mvalue1;
VMLOG(info, "SRFIELD object %d field num %d value %d\n", (int)ovalue1, ivalue1, mvalue1->mObjectValue.mValue);
                info->stack_ptr-=2;

                *info->stack_ptr = *mvalue1;
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_LD_STATIC_FIELD: 
VMLOG(info, "OP_LD_STATIC_FIELD\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;             // class name
                pc++;

                ivalue2 = *pc;             // field index
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class(real_class_name);

                if(klass1 == NULL) {
                    entry_exception_object(info, gExClassNotFoundClass, "can't get a class named %s\n", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                field = klass1->mFields + ivalue2;
                *info->stack_ptr = field->uValue.mStaticField;
VMLOG(info, "LD_STATIC_FIELD %d\n", field->uValue.mStaticField.mObjectValue.mValue);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SR_STATIC_FIELD:
VMLOG(info, "OP_SR_STATIC_FIELD\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;                    // class name
                pc++;

                ivalue2 = *pc;                    // field index
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class(real_class_name);

                if(klass1 == NULL) {
                    entry_exception_object(info, gExClassNotFoundClass, "can't get a class named %s\n", real_class_name);
                    vm_mutex_unlock();
                    return FALSE;
                }

                field = klass1->mFields + ivalue2;

                field->uValue.mStaticField = *(info->stack_ptr-1);
VMLOG(info, "OP_SR_STATIC_FIELD value %d\n", (info->stack_ptr-1)->mObjectValue.mValue);
                vm_mutex_unlock();
                break;

            case OP_NEW_ARRAY:
VMLOG(info, "OP_NEW_ARRAY\n");
                vm_mutex_lock();
                pc++;

                type1 = create_type_object_from_bytecodes(&pc, code, constant, info);
                push_object(type1, info);

                if(type1 == 0) {
                    entry_exception_object(info, gExClassNotFoundClass, "can't get a type data");
                    pop_object(info);
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue2 = *pc;                              // number of elements
                pc++;

                stack_ptr2 = info->stack_ptr - ivalue2 -1;
                for(i=0; i<ivalue2; i++) {
                    objects[i] = *stack_ptr2++;
                }

                ovalue1 = create_array_object(type1, objects, ivalue2, info);
VMLOG(info, "new array %d\n", ovalue1);
                pop_object(info);

                info->stack_ptr -= ivalue2;
                info->stack_ptr->mObjectValue.mValue = ovalue1;
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_NEW_HASH:
VMLOG(info, "OP_NEW_HASH\n");
                vm_mutex_lock();
                pc++;

                type1 = create_type_object_from_bytecodes(&pc, code, constant, info);
                push_object(type1, info);

                if(type1 == 0) {
                    entry_exception_object(info, gExClassNotFoundClass, "can't get a type data");
                    pop_object(info);
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = *pc;                                  // number of elements
                pc++;

                ovalue1 = create_hash_object(type1, NULL, NULL, 0);

                pop_object(info);

                info->stack_ptr->mObjectValue.mValue = ovalue1;
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

                ivalue2 = *pc;                      // max stack
                pc++;

                ivalue3 = *pc;                      // num locals
                pc++;

                ivalue4 = *pc;                      // num params
                pc++;

                ivalue8 = *pc;                      // max_block_var_num
                pc++;

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

                ovalue1 = create_block(const_buf, constant2_len, code_buf, code2_len, ivalue2, ivalue3, ivalue4, var, num_vars, ivalue8);

                info->stack_ptr->mObjectValue.mValue = ovalue1;
                info->stack_ptr++;
                vm_mutex_unlock();
                }
                break;

            case OP_NEW_SPECIAL_CLASS_OBJECT:
VMLOG(info, "OP_NEW_SPECIAL_CLASS_OBJECT\n");
                vm_mutex_lock();
                pc++;

                type1 = create_type_object_from_bytecodes(&pc, code, constant, info);
                push_object(type1, info);

                if(type1 == 0) {
                    entry_exception_object(info, gExClassNotFoundClass, "can't get a type data");
                    pop_object(info);
                    vm_mutex_unlock();
                    return FALSE;
                }

                klass1 = CLTYPEOBJECT(type1)->mClass;

                create_fun = klass1->mCreateFun;

                if(create_fun == NULL) {
                    entry_exception_object(info, gExceptionClass, "can't create object of this special class(%s) because of no creating object function\n", real_class_name);
                    pop_object(info);
                    vm_mutex_unlock();
                    return FALSE;
                }

                ovalue1 = create_fun(type1, info);

                pop_object(info);

                info->stack_ptr->mObjectValue.mValue = ovalue1;
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_NEW_OBJECT:
VMLOG(info, "NEW_OBJECT\n");
                vm_mutex_lock();
                pc++;

                type1 = create_type_object_from_bytecodes(&pc, code, constant, info);
                push_object(type1, info);

                if(type1 == 0) {
                    entry_exception_object(info, gExClassNotFoundClass, "can't get a type data");
                    pop_object(info);
                    vm_mutex_unlock();
                    return FALSE;
                }

                if(!solve_generics_types_of_type_object(type1, ALLOC &type2, vm_type, info))
                {
                    vm_mutex_unlock();
                    pop_object(info);
                    return FALSE;
                }

                pop_object(info);

                klass1 = CLTYPEOBJECT(type2)->mClass;

                if(!create_user_object(type2, &ovalue1, vm_type, info)) {
                    entry_exception_object(info, gExceptionClass, "can't create user object\n");
                    vm_mutex_unlock();
                    return FALSE;
                }
                info->stack_ptr->mObjectValue.mValue = ovalue1;
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_INVOKE_METHOD:
VMLOG(info, "OP_INVOKE_METHOD\n");
                pc++;

                /// method data ///
                ivalue1 = *pc;                  // real class name offset
                pc++;

                real_class_name = CONS_str(constant, ivalue1);
                klass1 = cl_get_class(real_class_name);

                if(klass1 == NULL) {
                    entry_exception_object(info, gExClassNotFoundClass, "can't get a class named %s\n", real_class_name);
                    return FALSE;
                }

                ivalue2 = *pc;                  // method index
                pc++;

                ivalue3 = *pc;                  // existance of result
                pc++;

                ivalue4 = *pc;                  // num params
                pc++;

                ivalue5 = *pc;                  // num used param initializer
                pc++;

                ivalue6 = *pc;                  // method num block type
                pc++;

                ivalue9 = *pc;                  // object kind
                pc++;

                if(ivalue9 == INVOKE_METHOD_KIND_CLASS) {
VMLOG(info, "INVOKE_METHOD_KIND_CLASS\n");
                    vm_mutex_lock();
                    type1 = create_type_object_from_bytecodes(&pc, code, constant, info);
                    push_object(type1, info);

                    if(type1 == 0) {
                        entry_exception_object(info, gExClassNotFoundClass, "can't get a type data");
                        pop_object(info);
                        vm_mutex_unlock();
                        return FALSE;
                    }

                    if(!solve_generics_types_of_type_object(type1, ALLOC &type2, vm_type, info))
                    {
                        vm_mutex_unlock();
                        pop_object(info);
                        return FALSE;
                    }

                    pop_object(info);

                    vm_mutex_unlock();

                    if(type2 == 0) {
                        entry_exception_object(info, gExClassNotFoundClass, "can't get a type data");
                        return FALSE;
                    }
                }
                else {
VMLOG(info, "INVOKE_METHOD_KIND_OBJECT\n");
                    mvalue1 = info->stack_ptr-ivalue4-ivalue6+ivalue5-1;  // get self

                    type2 = get_type_from_mvalue(mvalue1, info);
                }

                info->vm_type = type2;

                if(ivalue2 >= 0 && ivalue2 < klass1->mNumMethods) {
                    method = klass1->mMethods + ivalue2;
                }
                else {
                    entry_exception_object(info, gExMethodMissingClass, "can't get a method which has this index of %d from %s\n", ivalue2, REAL_CLASS_NAME(klass1));
                    return FALSE;
                }

VMLOG(info, "klass1 %s\n", REAL_CLASS_NAME(klass1));
VMLOG(info, "method name (%s)\n", METHOD_NAME(klass1, ivalue2));

                /// call param initializers ///
                for(i=method->mNumParams-ivalue5; i<method->mNumParams; i++) {
                    if(!param_initializer(klass1, method, i, info, vm_type)) {
                        return FALSE;
                    }
                }

                if(!excute_method(method, klass1, &klass1->mConstPool, ivalue3, info, type2))
                {
                    return FALSE;
                }
                break;

            case OP_INVOKE_VIRTUAL_METHOD: {
VMLOG(info, "OP_INVOKE_VIRTUAL_METHOD\n");
                pc++;

                /// method data ///
                ivalue1 = *pc;           // method name offset
                pc++;

                ivalue2 = *pc;           // method num params
                pc++;
                
                memset(params, 0, sizeof(params));

                for(i=0; i<ivalue2; i++) {  // method param data
                    get_class_name_from_bytecodes(&pc, constant, &params[i]);
                }

                ivalue11 = *pc;                         // existance of block
                pc++;

                ivalue3 = *pc;                           // method block num params
                pc++; 

                memset(params2, 0, sizeof(params2));

                for(i=0; i<ivalue3; i++) {          // block params
                    get_class_name_from_bytecodes(&pc, constant, &params2[i]);
                }

                ivalue4 = *pc;        // the existance of block result
                pc++;

                /// block result type ///
                if(ivalue4) {
                    get_class_name_from_bytecodes(&pc, constant, &string_type1);
                }
                else {
                    string_type1 = NULL;
                }

                ivalue5 = *pc;  // existance of result
                pc++;

                ivalue6 = *pc;  // super
                pc++;

                ivalue7 = *pc;   // class method
                pc++;

                ivalue8 = *pc;   // method num block
                pc++;

                ivalue12 = *pc;  // num params
                pc++;

                ivalue13 = *pc;  // num used param initializer
                pc++;
ASSERT(ivalue11 == 1 && string_type1 != NULL || ivalue11 == 0);

                ivalue9 = *pc;   // object kind
                pc++;

                if(ivalue9 == INVOKE_METHOD_KIND_OBJECT) {
VMLOG(info, "INVOKE_METHOD_KIND_OBJECT\n");
                    mvalue1 = (info->stack_ptr-ivalue2-ivalue8+ivalue13-1);

                    if(ivalue6) { // super
                        vm_mutex_lock();
                        type2 = CLOBJECT_HEADER(mvalue1->mObjectValue.mValue)->mType;
                        type2 = get_super_from_type_object(type2, info);
                        vm_mutex_unlock();

                        if(type2 == 0) {
                            entry_exception_object(info, gExceptionClass, "can't get a super class from object #%lu\n", ovalue1);
                            return FALSE;
                        }
                    }
                    else {
                        type2 = get_type_from_mvalue(mvalue1, info);
                    }
                }
                else {
VMLOG(info, "INVOKE_METHOD_KIND_CLASS\n");
                    vm_mutex_lock();

                    type1 = create_type_object_from_bytecodes(&pc, code, constant, info);

                    push_object(type1, info);

                    if(type1 == 0) {
                        pop_object(info);
                        entry_exception_object(info, gExClassNotFoundClass, "can't get a type data");
                        vm_mutex_unlock();
                        return FALSE;
                    }

                    if(!solve_generics_types_of_type_object(type1, ALLOC &type2, vm_type, info))
                    {
                        pop_object(info);
                        vm_mutex_unlock();
                        return FALSE;
                    }

                    pop_object(info);

                    vm_mutex_unlock();

                    if(type2 == 0) {
                        entry_exception_object(info, gExClassNotFoundClass, "can't get a type data");
                        return FALSE;
                    }

                    bvalue1 = 0;
                }

                type3 = info->vm_type;

                push_object(type3, info);

                info->vm_type = type2;

                /// searching for method ///
                vm_mutex_lock();
                method = get_virtual_method_with_params(type2, CONS_str(constant, ivalue1), params, ivalue2, &klass2, ivalue7, ivalue11, ivalue3, params2, string_type1, info, type3);
                vm_mutex_unlock();
                pop_object(info);

                if(method == NULL) {
                    entry_exception_object(info, gExMethodMissingClass, "can't get a method named %s.%s\n", REAL_CLASS_NAME(CLTYPEOBJECT(type2)->mClass), CONS_str(constant, ivalue1));
                    return FALSE;
                }

                /// call param initializers ///
                for(i=method->mNumParams-ivalue13; i<method->mNumParams; i++) {
                    if(!param_initializer(klass2, method, i, info, type2)) {
                        return FALSE;
                    }
                }
VMLOG(info, "klass2 %s\n", REAL_CLASS_NAME(klass2));
VMLOG(info, "method name (%s)\n", METHOD_NAME2(klass2, method));

                /// do call method ///
                if(!excute_method(method, klass2, &klass2->mConstPool, ivalue5, info, type2)) 
                {
                    return FALSE;
                }

                }
                break;

            case OP_INVOKE_BLOCK:
VMLOG(info, "OP_INVOKE_BLOCK\n");
                vm_mutex_lock();
                pc++;

                ivalue1 = *pc;          // block index
                pc++;

                ivalue2 = *pc;         // result existance
                pc++;

                ovalue1 = var[ivalue1].mObjectValue.mValue;

                if(!excute_block(ovalue1, ivalue2, info, vm_type)) 
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

                ivalue1 = *pc;                  // catch block number
                pc++;

                ivalue2 = *pc;                  // finally block existance
                pc++;

                ivalue3 = *pc;                  // finally block result existance
                pc++;

                mvalue1 = info->stack_ptr - ivalue2 -ivalue1*2 -1;  // try object pointer

                /// get try block object ///
                ovalue1 = mvalue1->mObjectValue.mValue;

                /// get catch block object and the type ///
                for(j=0; j<ivalue1; j++) {
                    catch_blocks[j] = (mvalue1 + 1 + j*2)->mObjectValue.mValue;
                    catch_block_type[j] = (mvalue1 + 1 +j*2 + 1)->mObjectValue.mValue;
                }

                /// get finally block object ///
                if(ivalue2) { 
                    ovalue3 = (info->stack_ptr -1)->mObjectValue.mValue;
                }
                else {
                    ovalue3 = 0;
                }

                /// try block ///
VMLOG(info, "TRY BLOCK START\n");
                result = excute_block(ovalue1, FALSE, info, vm_type);
VMLOG(info, "TRY BLOCK FINISHED. result is %d\n", result);
                if(result) {
                    if(ovalue3 && check_type(ovalue3, gBlockTypeObject, info)) {
VMLOG(info, "FINALLY BLOCK START\n");
                        /// finally ///
                        if(!excute_block(ovalue3, ivalue3, info, vm_type)) {
                            ovalue1 = (info->stack_ptr -1)->mObjectValue.mValue;

                            info->stack_ptr = mvalue1;

                            info->stack_ptr->mObjectValue.mValue = ovalue1;
                            info->stack_ptr++;
                            vm_mutex_unlock();
                            return FALSE;
                        }

                        if(ivalue3) {
                            ovalue1 = (info->stack_ptr -1)->mObjectValue.mValue;

                            info->stack_ptr = mvalue1;

                            info->stack_ptr->mObjectValue.mValue = ovalue1;
                            info->stack_ptr++;
                            goto OP_TRY_END;
                        }
                    }
                }
                else {
                    ovalue2 = (info->stack_ptr - 1)->mObjectValue.mValue; // exception object

                    /// catch ///
                    for(j=0; j<ivalue1; j++) {
                        if(check_type(ovalue2, catch_block_type[j], info)) {
VMLOG(info, "CATCH BLOCK STARTS %d\n", j);
                            result = excute_block(catch_blocks[j], FALSE, info, vm_type);
VMLOG(info, "CATCH BLOCK ENDS . result is %d\n", result);

                            if(result) {
                                /// finally ///
                                if(ovalue3 && check_type(ovalue3, gBlockTypeObject, info)) 
                                {
                                    /// finally ///
VMLOG(info, "FINALLY BLOCK STARTS.\n");
                                    if(!excute_block(ovalue3, ivalue3, info, vm_type)) {
                                        ovalue1 = (info->stack_ptr -1)->mObjectValue.mValue;

                                        info->stack_ptr = mvalue1;

                                        info->stack_ptr->mObjectValue.mValue = ovalue1;
                                        info->stack_ptr++;
                                        vm_mutex_unlock();
                                        return FALSE;
                                    }
VMLOG(info, "FINALLY BLOCK FINISHED.\n");
                                }

                                if(ivalue3) {
                                    ovalue1 = (info->stack_ptr -1)->mObjectValue.mValue;

                                    info->stack_ptr = mvalue1;

                                    info->stack_ptr->mObjectValue.mValue = ovalue1;
                                    info->stack_ptr++;
                                    goto OP_TRY_END;
                                }
                            }
                            else {
                                ovalue1 = (info->stack_ptr -1)->mObjectValue.mValue;

                                info->stack_ptr = mvalue1;

                                info->stack_ptr->mObjectValue.mValue = ovalue1;
                                info->stack_ptr++;
                                vm_mutex_unlock();
                                return FALSE;
                            }
                            break;
                        }
                    }

                    /// can't catch the exception ///
                    if(j == ivalue1) {
                        info->stack_ptr = mvalue1;
                        entry_exception_object(info, gExceptionClass, "can't catch the exception");
                        vm_mutex_unlock();
                        return FALSE;
                    }
                }

                info->stack_ptr = mvalue1;

OP_TRY_END:
                vm_mutex_unlock();
                break;

            case OP_IADD:
VMLOG(info, "OP_IADD\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue + CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BADD:
VMLOG(info, "OP_BADD\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue + CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object(bvalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FADD:
VMLOG(info, "OP_FADD\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                fvalue1 = CLFLOAT(ovalue1)->mValue + CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_float_object(fvalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SADD:
VMLOG(info, "OP_SADD\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_string_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLSTRING(ovalue1)->mLen;  // string length of ovalue1
                ivalue2 = CLSTRING(ovalue2)->mLen;  // string length of ovalue2

                str = MALLOC(sizeof(wchar_t)*(ivalue1 + ivalue2 + 1));

                wcscpy(str, CLSTRING_DATA(ovalue1)->mChars);
                wcscat(str, CLSTRING_DATA(ovalue2)->mChars);
VMLOG(info, "%ls + %ls\n", CLSTRING_DATA(ovalue1)->mChars, CLSTRING_DATA(ovalue2)->mChars);

                ovalue3 = create_string_object(str, ivalue1 + ivalue2, gStringTypeObject, info);

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = ovalue3;
                info->stack_ptr++;

                FREE(str);
                vm_mutex_unlock();
                break;

            case OP_BSADD:
                vm_mutex_lock();
                pc++;

                if(!get_two_bytes_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBYTES(ovalue1)->mLen;  // string length of ovalue1
                ivalue2 = CLBYTES(ovalue2)->mLen;  // string length of ovalue2

                str2 = CALLOC(1, sizeof(char)*(ivalue1 + ivalue2 + 1));

                xstrncpy(str2, CLBYTES_DATA(ovalue1)->mChars, ivalue1 + ivalue2 + 1);

                xstrncat(str2, CLBYTES_DATA(ovalue2)->mChars, ivalue1 + ivalue2 + 1);

                ovalue3 = create_bytes_object((unsigned char*)str2, ivalue1 + ivalue2, gBytesTypeObject, info);

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = ovalue3;
                info->stack_ptr++;

                FREE(str2);
                vm_mutex_unlock();
                break;

            case OP_ISUB:
VMLOG(info, "OP_ISUB\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }
    
                ivalue1 = CLINT(ovalue1)->mValue - CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BSUB:
VMLOG(info, "OP_BSUB\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue - CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object(bvalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FSUB:
VMLOG(info, "OP_FSUB\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                fvalue1 = CLFLOAT(ovalue1)->mValue - CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_float_object(fvalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IMULT:
VMLOG(info, "OP_IMULT\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }
    
                ivalue1 = CLINT(ovalue1)->mValue * CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SMULT:
VMLOG(info, "OP_SMULT\n");
                pc++;

                vm_mutex_lock();
                if(!get_string_and_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue2)->mValue;
                ovalue3 = create_string_object_by_multiply(ovalue1, ivalue1, info);

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = ovalue3;
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BSMULT:
VMLOG(info, "OP_BSMULT\n");
                pc++;

                vm_mutex_lock();

                if(!get_bytes_and_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue2)->mValue;
                ovalue3 = create_bytes_object_by_multiply(ovalue1, ivalue1, info);

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = ovalue3;
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BMULT:
VMLOG(info, "OP_BMULT\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue * CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object(bvalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FMULT:
VMLOG(info, "OP_FMULT\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                fvalue1 = CLFLOAT(ovalue1)->mValue * CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_float_object(fvalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IDIV:
VMLOG(info, "OP_IDIV\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue;
                ivalue2 = CLINT(ovalue2)->mValue;

                if(ivalue2 == 0) {
                    entry_exception_object(info, gExDivisionByZeroClass, "division by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = ivalue1 / ivalue2;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BDIV:
VMLOG(info, "OP_BDIV\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue;
                bvalue2 = CLBYTE(ovalue2)->mValue;

                if(bvalue2 == 0) {
                    entry_exception_object(info, gExDivisionByZeroClass, "division by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = bvalue1 / bvalue2;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object(bvalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FDIV:
VMLOG(info, "OP_FDIV\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                fvalue1 = CLFLOAT(ovalue1)->mValue;
                fvalue2 = CLFLOAT(ovalue2)->mValue;

                if(fvalue2 == 0.0f) {
                    entry_exception_object(info, gExDivisionByZeroClass, "division by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                fvalue1 = fvalue1 / fvalue2;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_float_object(fvalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IMOD:
VMLOG(info, "OP_IMOD\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue;
                ivalue2 = CLINT(ovalue2)->mValue;

                if(ivalue2 == 0) {
                    entry_exception_object(info, gExDivisionByZeroClass, "remainder by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = ivalue1 % ivalue2;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BMOD:
VMLOG(info, "OP_BMOD\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue;
                bvalue2 = CLBYTE(ovalue2)->mValue;

                if(bvalue2 == 0) {
                    entry_exception_object(info, gExDivisionByZeroClass, "remainder by zero");
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = bvalue1 % bvalue2;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object(bvalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_ILSHIFT:
VMLOG(info, "OP_ILSHIFT\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue << CLINT(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BLSHIFT:
VMLOG(info, "OP_BLSHIFT\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue << CLBYTE(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object(bvalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IRSHIFT:
VMLOG(info, "OP_IRSHIFT\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue >> CLINT(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BRSHIFT:
VMLOG(info, "OP_BRSHIFT\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue >> CLBYTE(ovalue2)->mValue;

                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object(bvalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IGTR:
VMLOG(info, "OP_IGTR\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue > CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BGTR:
VMLOG(info, "OP_BGTR\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBYTE(ovalue1)->mValue > CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FGTR:
VMLOG(info, "OP_FGTR\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLFLOAT(ovalue1)->mValue > CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IGTR_EQ:
VMLOG(info, "OP_IGTR_EQ\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue >= CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BGTR_EQ:
VMLOG(info, "OP_BGTR_EQ\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBYTE(ovalue1)->mValue >= CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FGTR_EQ:
VMLOG(info, "OP_FGTR\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLFLOAT(ovalue1)->mValue >= CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_ILESS:
VMLOG(info, "OP_ILESS\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue < CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BLESS:
VMLOG(info, "OP_BLESS\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBYTE(ovalue1)->mValue < CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FLESS:
VMLOG(info, "OP_FLESS\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLFLOAT(ovalue1)->mValue < CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_ILESS_EQ:
VMLOG(info, "OP_ILESS_EQ\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue <= CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BLESS_EQ:
VMLOG(info, "OP_BLESS_EQ\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBYTE(ovalue1)->mValue <= CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FLESS_EQ:
VMLOG(info, "OP_FLESS\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLFLOAT(ovalue1)->mValue <= CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IEQ:
VMLOG(info, "OP_IEQ\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue == CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BEQ:
VMLOG(info, "OP_BEQ\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBYTE(ovalue1)->mValue == CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FEQ:
VMLOG(info, "OP_FEQ\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLFLOAT(ovalue1)->mValue == CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BLEQ:
VMLOG(info, "OP_BLEQ\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_bool_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBOOL(ovalue1)->mValue == CLBOOL(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SEQ:
                pc++;

                vm_mutex_lock();

                if(!get_two_string_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = (wcscmp(CLSTRING_DATA(ovalue1)->mChars, CLSTRING_DATA(ovalue2)->mChars) == 0);
                
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_BSEQ:
                pc++;

                vm_mutex_lock();

                if(!get_two_bytes_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = (strcmp(CLBYTES_DATA(ovalue1)->mChars, (const char*)CLBYTES_DATA(ovalue2)->mChars) == 0);
                
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_INOTEQ:
VMLOG(info, "OP_INOTEQ\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue != CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BNOTEQ:
VMLOG(info, "OP_BNOTEQ\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBYTE(ovalue1)->mValue != CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_FNOTEQ:
VMLOG(info, "OP_FNOTEQ\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_float_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLFLOAT(ovalue1)->mValue != CLFLOAT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BLNOTEQ:
VMLOG(info, "OP_BLNOTEQ\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_bool_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBOOL(ovalue1)->mValue != CLBOOL(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_SNOTEQ:
                pc++;

                vm_mutex_lock();

                if(!get_two_string_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = (wcscmp(CLSTRING_DATA(ovalue1)->mChars, CLSTRING_DATA(ovalue2)->mChars) != 0);
                
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_BSNOTEQ:
                pc++;

                vm_mutex_lock();

                if(!get_two_bytes_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = (strcmp((const char*)CLBYTES_DATA(ovalue1)->mChars, (const char*)CLBYTES_DATA(ovalue2)->mChars) != 0);
                
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_IAND:
VMLOG(info, "OP_IAND\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue & CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BAND:
VMLOG(info, "OP_BAND\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue & CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object(bvalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IXOR:
VMLOG(info, "OP_IXOR\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue ^ CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BXOR:
VMLOG(info, "OP_BXOR\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue ^ CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object(bvalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IOR:
VMLOG(info, "OP_IOR\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_int_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue | CLINT(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_int_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BOR:
VMLOG(info, "OP_BAOR\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_byte_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue | CLBYTE(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_byte_object(bvalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BLOROR:
VMLOG(info, "OP_BLOROR\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_bool_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBOOL(ovalue1)->mValue || CLBOOL(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_BLANDAND:
VMLOG(info, "OP_BLANDAND\n");
                pc++;

                vm_mutex_lock();

                if(!get_two_bool_object_from_stack(&ovalue1, &ovalue2, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLBOOL(ovalue1)->mValue && CLBOOL(ovalue2)->mValue;
                info->stack_ptr-=2;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
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
VMLOG(info, "OP_POP_N %d\n", ivalue1);

                info->stack_ptr -= ivalue1;
                break;

            case OP_POP_N_WITHOUT_TOP:
                pc++;

                ivalue1 = *pc;
                pc++;
VMLOG(info, "OP_POP_N_WITHOUT_TOP %d\n", ivalue1);
                
                ovalue1 = (info->stack_ptr-1)->mObjectValue.mValue;

                info->stack_ptr -= ivalue1;

                info->stack_ptr->mObjectValue.mValue = ovalue1;
                info->stack_ptr++;
                break;

            case OP_LOGICAL_DENIAL:
VMLOG(info, "OP_LOGICAL_DENIAL %d\n", ivalue1);
                pc++;

                vm_mutex_lock();

                if(!get_one_bool_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = !CLBOOL(ovalue1)->mValue;

                info->stack_ptr--;
                info->stack_ptr->mObjectValue.mValue = create_bool_object(ivalue1);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_COMPLEMENT:
VMLOG(info, "OP_COMPLEMENT\n");
                pc++;

                vm_mutex_lock();

                if(!get_one_int_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = CLINT(ovalue1)->mValue;
                ivalue1 = ~ivalue1;

                (info->stack_ptr-1)->mObjectValue.mValue = create_int_object(ivalue1);
                vm_mutex_unlock();
                break;

            case OP_BCOMPLEMENT:
VMLOG(info, "OP_BCOMPLEMENT\n");
                pc++;

                vm_mutex_lock();

                if(!get_one_byte_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                bvalue1 = CLBYTE(ovalue1)->mValue;
                bvalue1 = ~bvalue1;

                (info->stack_ptr-1)->mObjectValue.mValue = create_byte_object(bvalue1);
                vm_mutex_unlock();
                break;


            case OP_DEC_VALUE:
VMLOG(info, "OP_DEC_VALUE\n");
                pc++;

                vm_mutex_lock();
                if(!get_one_int_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = *pc;
                pc++;

                ivalue2 = CLINT(ovalue1)->mValue - ivalue1;

                info->stack_ptr--;
                info->stack_ptr->mObjectValue.mValue = create_int_object(ivalue2);
                info->stack_ptr++;

                vm_mutex_unlock();
                break;

            case OP_INC_VALUE:
VMLOG(info, "OP_INC_VALUE\n");
                pc++;

                vm_mutex_lock();
                if(!get_one_int_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                ivalue1 = *pc;
                pc++;

                ivalue2 = CLINT(ovalue1)->mValue + ivalue1;

                info->stack_ptr--;
                info->stack_ptr->mObjectValue.mValue = create_int_object(ivalue2);
                info->stack_ptr++;
                vm_mutex_unlock();
                break;

            case OP_IF: {
VMLOG(info, "OP_IF\n");
                pc++;

                vm_mutex_lock();

                if(!get_one_bool_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                /// get result of conditional ///
                ivalue1 = CLBOOL(ovalue1)->mValue;
                info->stack_ptr--;

                ivalue2 = *pc;
                pc++;

                if(ivalue1) {
                    pc += ivalue2;
                }
                vm_mutex_unlock();
                }
                break;

            case OP_NOTIF: {
VMLOG(info, "OP_NOTIF\n");
                pc++;

                vm_mutex_lock();

                if(!get_one_bool_object_from_stack(&ovalue1, info))
                {
                    vm_mutex_unlock();
                    return FALSE;
                }

                /// get result of conditional ///
                ivalue1 = CLBOOL(ovalue1)->mValue;
                info->stack_ptr--;

                ivalue2 = *pc;
                pc++;

                if(!ivalue1) {
                    pc += ivalue2;
                }

                vm_mutex_unlock();
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

VMLOG(info, "VM finished pc - code->mCode --> %d code->mLen %d\n", pc - code->mCode, code->mLen);

    return TRUE;
}

/// result (TRUE): success (FALSE): threw exception
BOOL cl_main(sByteCode* code, sConst* constant, int lv_num, int max_stack, int stack_size)
{
    MVALUE* lvar;
    BOOL result;
    sVMInfo info;

    vm_mutex_lock();

    memset(&info, 0, sizeof(info));

    info.stack = CALLOC(1, sizeof(MVALUE)*stack_size);
    info.stack_size = stack_size;
    info.stack_ptr = info.stack;
    lvar = info.stack;
    info.stack_ptr += lv_num;

START_VMLOG(&info);
VMLOG(&info, "cl_main lv_num %d max_stack %d\n", lv_num, max_stack);

    push_vminfo(&info);

    if(info.stack_ptr + max_stack > info.stack + info.stack_size) {
        entry_exception_object(&info, gExOverflowStackSizeClass, "overflow stack size\n");
        output_exception_message(&info);
        FREE(info.stack);
        pop_vminfo(&info);
        vm_mutex_unlock();
        return FALSE;
    }

VMLOG(&info, "cl_main lv_num2 %d\n", lv_num);

    vm_mutex_unlock();

    result = cl_vm(code, constant, lvar, &info, 0);

#ifdef VM_DEBUG
    if(info.stack_ptr != info.stack + lv_num) {
        fprintf(stderr, "invalid stack ptr. An error occurs on cl_main\n");
        exit(2);
    }
#endif

    vm_mutex_lock();

    if(result == FALSE) {
        output_exception_message(&info); // show exception message
    }

    FREE(info.stack);

    pop_vminfo(&info);

    vm_mutex_unlock();

    return result;
}

// called by create_user_object
BOOL field_initializer(MVALUE* result, sByteCode* code, sConst* constant, int lv_num, int max_stack, CLObject vm_type)
{
    MVALUE* lvar;
    BOOL vm_result;
    sVMInfo info;
    MVALUE mvalue[CL_FIELD_INITIALIZER_STACK_SIZE];

    vm_mutex_lock();

    memset(&info, 0, sizeof(info));

    info.stack = mvalue;
    memset(&mvalue, 0, sizeof(MVALUE)*CL_FIELD_INITIALIZER_STACK_SIZE);
    info.stack_size = CL_FIELD_INITIALIZER_STACK_SIZE;
    info.stack_ptr = info.stack;

START_VMLOG(&info);
VMLOG(&info, "field_initializer\n");

    push_vminfo(&info);

    lvar = info.stack_ptr;
    info.stack_ptr += lv_num;

    if(info.stack_ptr + max_stack > info.stack + info.stack_size) {
        entry_exception_object(&info, gExOverflowStackSizeClass, "overflow stack size\n");
        output_exception_message(&info);
        pop_vminfo(&info);
        vm_mutex_unlock();
        return FALSE;
    }

    vm_result = cl_vm(code, constant, lvar, &info, vm_type);
    *result = *(info.stack_ptr-1);

#ifdef VM_DEBUG
    if(info.stack_ptr != lvar + lv_num && info.stack_ptr != lvar + lv_num + 1) {
        fprintf(stderr, "invalid stack ptr. An error occurs on field_initializer\n");
        fprintf(stderr, "stack pointer is %d\n", info.stack_ptr - lvar - lv_num -1);
        exit(2);
    }
#endif

    if(!vm_result) {
        output_exception_message(&info); // show exception message
    }

    info.stack_ptr = lvar;

    pop_vminfo(&info);

    vm_mutex_unlock();

    return vm_result;
}

static BOOL param_initializer(sCLClass* klass, sCLMethod* method, int param_num, sVMInfo* info, CLObject vm_type)
{
    MVALUE* lvar;
    BOOL vm_result;
    sVMInfo new_info;
    MVALUE mvalue[CL_PARAM_INITIALIZER_STACK_SIZE];
    sCLParamInitializer* param_initializer;

    sByteCode* code;
    sConst* constant;
    int lv_num;
    int max_stack;

    ASSERT(param_num >= 0 && param_num < method->mNumParams); // checked on compile time

    param_initializer = method->mParamInitializers + param_num;

    code = &param_initializer->mInitializer;
    constant = &klass->mConstPool;
    lv_num = param_initializer->mLVNum;
    max_stack = param_initializer->mMaxStack;

    vm_mutex_lock();

    memset(&new_info, 0, sizeof(new_info));

    new_info.stack = mvalue;
    memset(&mvalue, 0, sizeof(MVALUE)*CL_PARAM_INITIALIZER_STACK_SIZE);
    new_info.stack_size = CL_PARAM_INITIALIZER_STACK_SIZE;
    new_info.stack_ptr = new_info.stack;

START_VMLOG(&new_info);
VMLOG(&new_info, "field_initializer\n");

    push_vminfo(&new_info);

    lvar = new_info.stack_ptr;
    new_info.stack_ptr += lv_num;

    if(new_info.stack_ptr + max_stack > new_info.stack + new_info.stack_size) {
        entry_exception_object(&new_info, gExOverflowStackSizeClass, "overflow stack size\n");
        output_exception_message(&new_info);
        pop_vminfo(&new_info);
        vm_mutex_unlock();
        return FALSE;
    }

    vm_result = cl_vm(code, constant, lvar, &new_info, vm_type);

    *(info->stack_ptr) = *(new_info.stack_ptr-1);
    info->stack_ptr++;

#ifdef VM_DEBUG
    if(new_info.stack_ptr != lvar + lv_num + 1) {
        fprintf(stderr, "invalid stack ptr. An error occurs on param_initializer\n");
        exit(2);
    }
#endif

    if(!vm_result) {
        output_exception_message(&new_info); // show exception message
    }

    pop_vminfo(&new_info);

    vm_mutex_unlock();

    return vm_result;
}

// result_type 0: nothing 1: result exists 2: dynamic typing class
static BOOL excute_method(sCLMethod* method, sCLClass* klass, sConst* constant, int result_type, sVMInfo* info, CLObject vm_type)
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
            entry_exception_object(info, gExOverflowStackSizeClass, "overflow stack size\n");
            return FALSE;
        }

        synchronized = method->mFlags & CL_SYNCHRONIZED_METHOD;

        if(synchronized) vm_mutex_lock();

        if(method->uCode.mNativeMethod == NULL) {
            entry_exception_object(info, gExMethodMissingClass, "can't get a native method named %s.%s\n", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
            return FALSE;
        }

        native_result = method->uCode.mNativeMethod(&info->stack_ptr, lvar, info);
        if(synchronized) vm_mutex_unlock();

        if(native_result) {
            if(result_type == 1) {
                MVALUE* mvalue;

                mvalue = info->stack_ptr-1;
                info->stack_ptr = lvar;
                *info->stack_ptr = *mvalue;
                info->stack_ptr++;
            }
            else if(result_type == 2) { // dynamic type
                if(strcmp(CONS_str(constant, method->mResultType.mClassNameOffset), "void") == 0)
                {
                    info->stack_ptr = lvar;
                    (*info->stack_ptr).mObjectValue.mValue = 0;               // uninitialized object
                    info->stack_ptr++;
                }
                else {
                    MVALUE* mvalue;

                    mvalue = info->stack_ptr-1;
                    info->stack_ptr = lvar;
                    *info->stack_ptr = *mvalue;
                    info->stack_ptr++;
                }
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
        MVALUE* stack_top;

        lvar = info->stack_ptr - real_param_num;
        if(method->mNumLocals - real_param_num > 0) {
            info->stack_ptr += (method->mNumLocals - real_param_num);     // forwarded stack pointer for local variable
        }

        if(info->stack_ptr + method->mMaxStack > info->stack + info->stack_size) {
            entry_exception_object(info, gExOverflowStackSizeClass, "overflow stack size\n");
            return FALSE;
        }

        synchronized = method->mFlags & CL_SYNCHRONIZED_METHOD;

        if(synchronized) vm_mutex_lock();

        stack_top = info->stack_ptr;

        result = cl_vm(&method->uCode.mByteCodes, constant, lvar, info, vm_type);
        if(synchronized) vm_mutex_unlock();

        if(!result) {
            MVALUE* mvalue;

#ifdef VM_DEBUG
            if(info->stack_ptr != stack_top + 1) {
                fprintf(stderr, "invalid stack ptr. An error occurs on excute_method1\n");
                exit(2);
            }
#endif

            mvalue = info->stack_ptr-1;
            info->stack_ptr = lvar;
            *info->stack_ptr = *mvalue;
            info->stack_ptr++;

            return FALSE;
        }
        else if(result_type == 1) 
        {
            MVALUE* mvalue;

#ifdef VM_DEBUG
            if(info->stack_ptr != stack_top + 1) {
                fprintf(stderr, "invalid stack ptr. An error occurs on excute_method2\n");
                exit(2);
            }
#endif

            mvalue = info->stack_ptr-1;
            info->stack_ptr = lvar;
            *info->stack_ptr = *mvalue;
            info->stack_ptr++;
        }
        else if(result_type == 2) {  // dynamic type
#ifdef VM_DEBUG
            if(info->stack_ptr != stack_top && info->stack_ptr != stack_top + 1) {
                fprintf(stderr, "invalid stack ptr. An error occurs on excute_method3\n");
                fprintf(stderr, "stack pointer is %d\n", info->stack_ptr - stack_top -1);
                exit(2);
            }
#endif

            if(strcmp(CONS_str(constant, method->mResultType.mClassNameOffset), "void") == 0)
            {
                info->stack_ptr = lvar;
                (*info->stack_ptr).mObjectValue.mValue = 0;               // uninitialized object
                info->stack_ptr++;
            }
            else {
                MVALUE* mvalue;

                mvalue = info->stack_ptr-1;
                info->stack_ptr = lvar;
                *info->stack_ptr = *mvalue;
                info->stack_ptr++;
            }
        }
        else {
#ifdef VM_DEBUG
            if(info->stack_ptr != stack_top) {
                fprintf(stderr, "invalid stack ptr. An error occurs on excute_method4\n");
                fprintf(stderr, "info->stack_ptr - stack_top is %d\n", info->stack_ptr - stack_top);
                exit(2);
            }
#endif
            info->stack_ptr = lvar;
        }

        return result;
    }
}

static BOOL excute_block(CLObject block, BOOL result_existance, sVMInfo* info, CLObject vm_type)
{
    int real_param_num;
    MVALUE* lvar;
    BOOL result;
    MVALUE* stack_top;

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
        entry_exception_object(info, gExOverflowStackSizeClass, "overflow stack size\n");
        return FALSE;
    }

    stack_top = info->stack_ptr;
    result = cl_vm(CLBLOCK(block)->mCode, CLBLOCK(block)->mConstant, lvar, info, vm_type);

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

#ifdef VM_DEBUG
        if(info->stack_ptr != stack_top + 1) {
            fprintf(stderr, "invalid stack ptr. An error occurs on excute_block2\n");
            fprintf(stderr, "stack pointer is %d\n", info->stack_ptr - stack_top -1);
            exit(2);
        }
#endif

        mvalue = info->stack_ptr-1;
        info->stack_ptr = lvar;
        *info->stack_ptr = *mvalue;
        info->stack_ptr++;
    }
    else {

#ifdef VM_DEBUG
        if(info->stack_ptr != stack_top) {
            fprintf(stderr, "invalid stack ptr. An error occurs on excute_block3\n");
            fprintf(stderr, "stack pointer is %d\n", info->stack_ptr - stack_top);
            exit(2);
        }
#endif
        info->stack_ptr = lvar;
    }

    return result;
}

BOOL cl_excute_block(CLObject block, BOOL result_existance, BOOL static_method_block, sVMInfo* info, CLObject vm_type)
{
    return excute_block(block, result_existance, info, vm_type);
}

static BOOL excute_block_with_new_stack(MVALUE* result, CLObject block, BOOL result_existance, sVMInfo* new_info, CLObject vm_type)
{
    BOOL vm_result;
    MVALUE* lvar;
    sByteCode* code;
    sConst* constant;
    MVALUE* stack_top;

    lvar = new_info->stack;

    if(new_info->stack_ptr + CLBLOCK(block)->mMaxStack > new_info->stack + new_info->stack_size) {
        entry_exception_object(new_info, gExOverflowStackSizeClass, "overflow stack size\n");
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

    stack_top = new_info->stack_ptr;

    vm_result = cl_vm(code, constant, lvar, new_info, vm_type);

    vm_mutex_lock();

    if(!vm_result) {
#ifdef VM_DEBUG
        if(new_info->stack_ptr != stack_top + 1) {
            fprintf(stderr, "invalid stack ptr. An error occurs on excute_block_with_new_stack\n");
            exit(2);
        }
#endif

        *result = *(new_info->stack_ptr-1);
        output_exception_message(new_info); // show exception message
    }
    else if(result_existance) {
#ifdef VM_DEBUG
        if(new_info->stack_ptr != stack_top + 1) {
            fprintf(stderr, "invalid stack ptr. An error occurs on excute_block_with_new_stack\n");
            exit(2);
        }
#endif
        *result = *(new_info->stack_ptr-1);
    }
    else {
#ifdef VM_DEBUG
        if(new_info->stack_ptr != stack_top) {
            fprintf(stderr, "invalid stack ptr. An error occurs on excute_block_with_new_stack\n");
            exit(2);
        }
#endif
    }

    FREE(new_info->stack);

    pop_vminfo(new_info);

    vm_mutex_unlock();

    return vm_result;
}

BOOL cl_excute_block_with_new_stack(MVALUE* result, CLObject block, BOOL result_existance, sVMInfo* new_info, CLObject vm_type)
{
    return excute_block_with_new_stack(result, block, result_existance, new_info, vm_type);
}

