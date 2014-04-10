#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

static BOOL eval_statment(char** p, char* sname, int* sline, sVarTable* lv_table)
{
    sByteCode code;
    sConst constant;
    int max_stack;
    int err_num;
    char current_namespace[CL_NAMESPACE_NAME_MAX + 1];

    sByteCode_init(&code);
    sConst_init(&constant);

    max_stack = 0;
    err_num = 0;
    *current_namespace = 0;

    if(!parse_statment(p, sname, sline, &code, &constant, &err_num, &max_stack, current_namespace, lv_table)) {
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }
    if(err_num > 0) {
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }
    if(!cl_main(&code, &constant, lv_table->mVarNum + lv_table->mBlockVarNum, max_stack)) {
        sByteCode_free(&code);
        sConst_free(&constant);
        return FALSE;
    }

    sByteCode_free(&code);
    sConst_free(&constant);

    return TRUE;
}

BOOL cl_eval(char* cmdline, char* sname, int* sline)
{
    sBuf source, source2;
    char* p;

    sBuf_init(&source);
    sBuf_append(&source, cmdline, strlen(cmdline));

    /// delete comment ///
    sBuf_init(&source2);

    if(!delete_comment(&source, &source2)) {
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }

    p = source2.mBuf;
    *sline = 1;

    while(*p) {
        if(!eval_statment(&p, sname, sline, &gGVTable)) {
            FREE(source.mBuf);
            FREE(source2.mBuf);
            return FALSE;
        }
    }

    FREE(source.mBuf);
    FREE(source2.mBuf);

    return TRUE;
}

BOOL cl_eval_file(char* file_name)
{
    sBuf source, source2;
    int sline;
    int f;
    char* p;
    char* sname;

    sByteCode code;
    sConst constant;
    int max_stack;
    int err_num;
    char current_namespace[CL_NAMESPACE_NAME_MAX + 1];

    f = open(file_name, O_RDONLY);

    if(f < 0) {
        compile_error("can't open %s\n", file_name);
        return FALSE;
    }

    sBuf_init(&source);

    while(1) {
        char buf2[WORDSIZ];
        int size;

        size = read(f, buf2, WORDSIZ);

        if(size < 0 || size == 0) {
            break;
        }

        sBuf_append(&source, buf2, size);
    }

    close(f);

    /// delete comment ///
    sBuf_init(&source2);

    if(!delete_comment(&source, &source2)) {
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }

    sByteCode_init(&code);
    sConst_init(&constant);

    p = source2.mBuf;
    sname = file_name;
    max_stack = 0;
    err_num = 0;
    *current_namespace = 0;
    sline = 1;

    if(!parse_statments(&p, sname, &sline, &code, &constant, &err_num, &max_stack, current_namespace, &gGVTable)) {
        sByteCode_free(&code);
        sConst_free(&constant);
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }
    if(err_num > 0) {
        sByteCode_free(&code);
        sConst_free(&constant);
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }
    if(!cl_main(&code, &constant, gGVTable.mVarNum + gGVTable.mBlockVarNum, max_stack)) {
        sByteCode_free(&code);
        sConst_free(&constant);
        FREE(source.mBuf);
        FREE(source2.mBuf);
        return FALSE;
    }

    sByteCode_free(&code);
    sConst_free(&constant);
    FREE(source.mBuf);
    FREE(source2.mBuf);

    return TRUE;
}

sBuf* gCLPrintBuffer;

int cl_print(char* msg, ...)
{
    char* msg2;
    int n;

    va_list args;
    va_start(args, msg);
    n = vasprintf(ALLOC &msg2, msg, args);
    va_end(args);

    if(gCLPrintBuffer) {                            // this is hook of all clover output
        sBuf_append(gCLPrintBuffer, msg2, n);
    }
    else {
#ifdef VM_DEBUG
        vm_debug("%s", msg2);
#else
        printf("%s", msg2);
#endif
    }

    free(msg2);

    return n;
}

// result: (FALSE) not found or failed in type checking (TRUE:) success
BOOL cl_get_class_field(sCLClass* klass, char* field_name, sCLClass* field_class, MVALUE* result)
{
    sCLField* field;
    CLObject object;

    field = get_field(klass, field_name, TRUE);

    if(field == NULL || (field->mFlags & CL_STATIC_FIELD) == 0) {
        return FALSE;
    }

    if(field_class->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) {
        *result = field->uValue.mStaticField;
    }
    else {
        object = get_object_from_mvalue(field->uValue.mStaticField);

        /// type checking ///
        if(object && substition_posibility_of_class(field_class, CLOBJECT_HEADER(object)->mClass)) {
            (*result).mObjectValue = object;
        }
        else {
            return FALSE;
        }
    }

    return TRUE;
}

// result: (FALSE) not found or failed in type checking (TRUE:) success
BOOL cl_get_array_element(CLObject array, int index, sCLClass* element_class, MVALUE* result)
{
    if(index < 0 || index >= CLARRAY(array)->mLen) {
        return FALSE;
    }

    if(element_class->mFlags & CLASS_FLAGS_IMMEDIATE_VALUE_CLASS) {
        *result = CLARRAY_ITEMS(array, index);
    }
    else {
        CLObject object;

        object = get_object_from_mvalue(CLARRAY_ITEMS(array, index));

        /// type checking ///
        if(object && substition_posibility_of_class(element_class, CLOBJECT_HEADER(object)->mClass)) {
            (*result).mObjectValue = object;
        }
        else {
            return FALSE;
        }
    }

    return TRUE;
}
