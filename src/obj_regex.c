#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLRegex);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_regex_object(CLObject type_object)
{
    CLObject obj;
    unsigned int size;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    return obj;
}

// enc: ONIG_ENCODING_UTF8, ONIG_ENCODING_EUC_JP, ONIG_ENCODING_SJIS, ONIG_ENCODING_ASCII
BOOL create_regex_object(CLObject* self, CLObject type_object, OnigUChar* regex_str, BOOL ignore_case, BOOL multiline, OnigEncoding enc, sVMInfo* info)
{
    OnigErrorInfo err_info;
    OnigOptionType option;
    int result;
    regex_t* regex;

    *self = alloc_regex_object(type_object);

    option = ONIG_OPTION_DEFAULT;
    if(ignore_case) {
        option |= ONIG_OPTION_IGNORECASE;
    }
    else if(multiline) {
        option |= ONIG_OPTION_MULTILINE;
    }

    result = onig_new(&regex
        , (const OnigUChar*) regex_str
        , (const OnigUChar*) regex + strlen(regex_str)
        , option
        , enc
        , ONIG_SYNTAX_DEFAULT
        , &err_info);

    CLREGEX(*self)->mRegex = regex;

    if(result == ONIG_NORMAL) {
        return TRUE;
    }
    else {
        entry_exception_object(info, gExInvalidRegexClass, "invalid regex");
        return FALSE;
    }
}

static CLObject create_regex_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;
    
    self = alloc_regex_object(type_object);

    CLREGEX(self)->mRegex = NULL;
    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

static void free_regex_object(CLObject obj)
{
    if(CLREGEX(obj)->mRegex != NULL) onig_free(CLREGEX(obj)->mRegex);
}

static void mark_regex_object(CLObject object, unsigned char* mark_flg)
{
    CLObject object2 = CLREGEX(object)->mEncoding;
    mark_object(object2, mark_flg);
}

void initialize_hidden_class_method_of_regex(sCLClass* klass)
{
    klass->mFreeFun = free_regex_object;
    klass->mShowFun = NULL;
    klass->mMarkFun = mark_regex_object;
    klass->mCreateFun = create_regex_object_for_new;
}

BOOL Regex_source(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
/*
    CLObject self;
    sCLClass* klass;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    chars = CLSTRING_DATA(self)->mChars;

    (*stack_ptr)->mObjectValue.mValue = create_int_object(wcslen(chars));
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
    return TRUE;
*/
}

BOOL Regex_ignoreCase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    return TRUE;
}

BOOL Regex_multiLine(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    return TRUE;
}

BOOL Regex_encode(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    return TRUE;
}

BOOL Regex_compile(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    return TRUE;
}

