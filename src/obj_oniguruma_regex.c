#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLOnigurumaRegex);

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
static BOOL compile_regex(CLObject self, const OnigUChar* regex_str, BOOL ignore_case, BOOL multiline, BOOL global, OnigEncoding enc, sVMInfo* info, CLObject vm_type)
{
    OnigErrorInfo err_info;
    OnigOptionType option;
    int result;
    regex_t* regex;
    CLObject type_object2;
    CLObject encode;
    CLObject encoding;

    option = ONIG_OPTION_DEFAULT;
    if(ignore_case) {
        option |= ONIG_OPTION_IGNORECASE;
    }
    else if(multiline) {
        option |= ONIG_OPTION_MULTILINE;
    }

    result = onig_new(&regex
        , (const OnigUChar*) regex_str
        , (const OnigUChar*) regex_str + strlen(regex_str)
        , option
        , enc
        , ONIG_SYNTAX_DEFAULT
        , &err_info);

    if(result != ONIG_NORMAL) {
        entry_exception_object_with_class_name(info, "Exception", "invalid regex");
        return FALSE;
    }

    CLONIGURUMAREGEX(self)->mRegex = regex;

    CLONIGURUMAREGEX(self)->mSource = STRDUP((char*)regex_str);

    CLONIGURUMAREGEX(self)->mIgnoreCase = ignore_case;
    CLONIGURUMAREGEX(self)->mMultiLine = multiline;
    CLONIGURUMAREGEX(self)->mGlobal = global;

    type_object2 = create_type_object_with_class_name("Encoding");
    push_object(type_object2, info);

    if(enc == ONIG_ENCODING_ASCII) {
        encode = create_int_object(1);
    }
    else if(enc == ONIG_ENCODING_UTF8) {
        encode = create_int_object(2);
    }
    else if(enc == ONIG_ENCODING_EUC_JP) {
        encode = create_int_object(3);
    }
    else if(enc == ONIG_ENCODING_SJIS) {
        encode = create_int_object(4);
    }
    else {
        entry_exception_object_with_class_name(info, "Exception", "invalid encoding");
        pop_object_except_top(info);
        return FALSE;
    }

    push_object(encode, info);

    if(!create_user_object(type_object2, &encoding, vm_type, NULL, 0, info))
    {
        pop_object_except_top(info);
        pop_object_except_top(info);
        return FALSE;
    }
    pop_object(info);
    pop_object(info);

    CLONIGURUMAREGEX(self)->mEncoding = encoding;

    CLUSEROBJECT(encoding)->mFields[0].mObjectValue.mValue = encode;

    if(result == ONIG_NORMAL) {
        return TRUE;
    }
    else {
        entry_exception_object(info, gExInvalidRegexClass, "invalid regex");
        return FALSE;
    }
}

// enc: ONIG_ENCODING_UTF8, ONIG_ENCODING_EUC_JP, ONIG_ENCODING_SJIS, ONIG_ENCODING_ASCII
BOOL create_oniguruma_regex_object(CLObject* self, CLObject type_object, OnigUChar* regex_str, BOOL ignore_case, BOOL multiline, BOOL global, OnigEncoding enc, sVMInfo* info, CLObject vm_type)
{
    BOOL result;

    *self = alloc_regex_object(type_object);

    CLOBJECT_HEADER(*self)->mType = type_object;

    push_object(*self, info);

    result = compile_regex(*self, regex_str, ignore_case, multiline, global, enc, info, vm_type);

    if(!result) {
        pop_object_except_top(info);
    }
    else {
        pop_object(info);
    }

    return result;
}

static CLObject create_regex_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject self;
    
    self = alloc_regex_object(type_object);

    CLONIGURUMAREGEX(self)->mRegex = NULL;
    CLOBJECT_HEADER(self)->mType = type_object;

    return self;
}

static void free_regex_object(CLObject obj)
{
    if(CLONIGURUMAREGEX(obj)->mRegex != NULL) onig_free(CLONIGURUMAREGEX(obj)->mRegex);
    if(CLONIGURUMAREGEX(obj)->mSource != NULL) FREE(CLONIGURUMAREGEX(obj)->mSource);
}

static void mark_regex_object(CLObject object, unsigned char* mark_flg)
{
    CLObject object2 = CLONIGURUMAREGEX(object)->mEncoding;
    mark_object(object2, mark_flg);
}

void initialize_hidden_class_method_of_oniguruma_regex(sCLClass* klass)
{
    klass->mFreeFun = free_regex_object;
    klass->mShowFun = NULL;
    klass->mMarkFun = mark_regex_object;
    klass->mCreateFun = create_regex_object_for_new;
}

BOOL OnigurumaRegex_source(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject source;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "OnigurumaRegex", info)) { 
        return FALSE;
    }

    if(CLONIGURUMAREGEX(self)->mRegex == NULL || CLONIGURUMAREGEX(self)->mSource == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "This regex object is not compiled");
        return FALSE;
    }

    if(!create_string_object_from_ascii_string(&source, CLONIGURUMAREGEX(self)->mSource, gStringTypeObject, info))
    {
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = source;
    (*stack_ptr)++;

    return TRUE;
}

BOOL OnigurumaRegex_ignoreCase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    BOOL ignore_case;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "OnigurumaRegex", info))
    { 
        return FALSE;
    }

    ignore_case = CLONIGURUMAREGEX(self)->mIgnoreCase;

    if(CLONIGURUMAREGEX(self)->mRegex == NULL || CLONIGURUMAREGEX(self)->mSource == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "This regex object is not compiled");
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(ignore_case);
    (*stack_ptr)++;

    return TRUE;
}

BOOL OnigurumaRegex_multiLine(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    BOOL multiline;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "OnigurumaRegex", info))
    { 
        return FALSE;
    }

    multiline = CLONIGURUMAREGEX(self)->mMultiLine;

    if(CLONIGURUMAREGEX(self)->mRegex == NULL || CLONIGURUMAREGEX(self)->mSource == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "This regex object is not compiled");
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(multiline);
    (*stack_ptr)++;

    return TRUE;
}

BOOL OnigurumaRegex_global(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    BOOL global;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "OnigurumaRegex", info))
    { 
        return FALSE;
    }

    global = CLONIGURUMAREGEX(self)->mGlobal;

    if(CLONIGURUMAREGEX(self)->mRegex == NULL || CLONIGURUMAREGEX(self)->mSource == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "This regex object is not compiled");
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(global);
    (*stack_ptr)++;

    return TRUE;
}

BOOL OnigurumaRegex_encode(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject encode;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "OnigurumaRegex", info))
    { 
        return FALSE;
    }

    if(CLONIGURUMAREGEX(self)->mRegex == NULL || CLONIGURUMAREGEX(self)->mSource == NULL) {
        entry_exception_object_with_class_name(info, "NullPointerException", "This regex object is not compiled");
        return FALSE;
    }

    encode = CLONIGURUMAREGEX(self)->mEncoding;

    if(!check_type_with_class_name(encode, "Encoding", info)) {
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = encode;
    (*stack_ptr)++;

    return TRUE;
}

BOOL OnigurumaRegex_compile(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject source;
    CLObject ignore_case;
    CLObject multiline;
    CLObject global;
    CLObject encode;
    OnigUChar* regex_str;
    OnigEncoding enc;
    CLObject encode_value;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "OnigurumaRegex", info))
    { 
        return FALSE;
    }

    source = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(source, "String", info))
    { 
        return FALSE;
    }

    ignore_case = (lvar+2)->mObjectValue.mValue;

    if(!check_type_with_class_name(ignore_case, "bool", info))
    { 
        return FALSE;
    }

    multiline = (lvar+3)->mObjectValue.mValue;

    if(!check_type_with_class_name(multiline, "bool", info))
    { 
        return FALSE;
    }

    global = (lvar+4)->mObjectValue.mValue;

    if(!check_type_with_class_name(global, "bool", info))
    { 
        return FALSE;
    }

    encode = (lvar+5)->mObjectValue.mValue;

    if(!check_type_with_class_name(encode, "Encoding", info))
    { 
        return FALSE;
    }
    
    encode_value = CLINT(CLUSEROBJECT(encode)->mFields[0].mObjectValue.mValue)->mValue;

    switch(encode_value) {
        case 0:
            enc = ONIG_ENCODING_ASCII;
            break;

        case 1:
            enc = ONIG_ENCODING_ASCII;
            break;

        case 2:
            enc = ONIG_ENCODING_UTF8;
            break;

        case 3:
            enc = ONIG_ENCODING_EUC_JP;
            break;

        case 4:
            enc = ONIG_ENCODING_SJIS;
            break;

        default:
            entry_exception_object_with_class_name(info, "Exception", "invalid encoding value");
            return FALSE;
    }

    if(!string_object_to_str(ALLOC (char**)&regex_str, source)) {
        return FALSE;
    }

    if(!compile_regex(self, regex_str, CLBOOL(ignore_case)->mValue, CLBOOL(multiline)->mValue, CLBOOL(global)->mValue, enc, info, vm_type))
    {
        FREE(regex_str);
        return FALSE;
    }

    FREE(regex_str);
    
    return TRUE;
}

