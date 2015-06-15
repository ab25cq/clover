#include "clover.h"
#include "common.h"
#include <limits.h>
#include <wchar.h>

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLString);

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static unsigned int chars_object_size(unsigned int len2)
{
    unsigned int size;

    size = sizeof(sCLStringData) - sizeof(wchar_t)*DUMMY_ARRAY_SIZE + sizeof(wchar_t) * len2;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

BOOL create_buffer_from_string_object(CLObject str, ALLOC char** result, sVMInfo* info)
{
    wchar_t* wstr;
    int len;

    wstr = CLSTRING_DATA(str)->mChars;
    len = CLSTRING(str)->mLen;

    *result = CALLOC(1, MB_LEN_MAX * (len + 1));

    if((int)wcstombs(*result, wstr, MB_LEN_MAX * (len+1)) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "failed to mbstowcs on converting string");
        return FALSE;
    }

    return TRUE;
}

static CLObject alloc_string_object(unsigned int len2, CLObject type_object, sVMInfo* info)
{
    CLObject obj;
    CLObject obj2;
    unsigned int size;
    unsigned int chars_size;

    size = object_size();
    obj = alloc_heap_mem(size, type_object);

    push_object(obj, info);

    chars_size = chars_object_size(len2);
    obj2 = alloc_heap_mem(chars_size, 0);
    CLSTRING(obj)->mData = obj2;

    pop_object(info);

    return obj;
}

CLObject create_string_object(wchar_t* str, int len, CLObject type_object, sVMInfo* info)
{
    CLObject obj;
    wchar_t* chars;
    int i;


    obj = alloc_string_object(len+1, type_object, info);

    chars = CLSTRING_DATA(obj)->mChars;
    for(i=0; i<len; i++) {
        chars[i] = str[i];
    }
    chars[i] = 0;

    CLSTRING(obj)->mLen = len;

    return obj;
}

BOOL create_string_object_from_ascii_string(CLObject* result, char* str, CLObject type_object, sVMInfo* info)
{
    int wlen;
    wchar_t* wstr;

    wlen = strlen(str)+1;
    wstr = MALLOC(sizeof(wchar_t)*wlen);

    if((int)mbstowcs(wstr, str, wlen) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "error mbstowcs on converting string");
        FREE(wstr);
        return FALSE;
    }

    *result = create_string_object(wstr, wlen, type_object, info);

    FREE(wstr);

    return TRUE;
}

static CLObject create_string_object_for_new(CLObject type_object, sVMInfo* info)
{
    CLObject obj;

    obj = create_string_object(L"", 0, type_object, info);
    CLOBJECT_HEADER(obj)->mType = type_object;

    return obj;
}

CLObject create_string_object_by_multiply(CLObject string, int number, sVMInfo* info)
{
    wchar_t* wstr;
    int len;
    int i;
    CLObject result;

    len = CLSTRING(string)->mLen * number;
    wstr = CALLOC(1, sizeof(wchar_t)*(len + 1));
    wstr[0] = 0;
    for(i=0; i<number; i++) {
        wcsncat(wstr, CLSTRING_DATA(string)->mChars, len-wcslen(wstr));
    }

    result = create_string_object(wstr, len, gStringTypeObject, info);

    FREE(wstr);

    return result;
}

void string_append(CLObject string, char* str, int n)
{
}

static void show_string_object(sVMInfo* info, CLObject obj)
{
    unsigned int obj_size;
    int len;
    int size;
    char* str;
    wchar_t* chars;

    obj_size = object_size(obj);

    len = CLSTRING(obj)->mLen;

    size = (len + 1) * MB_LEN_MAX;
    str = MALLOC(size);
    chars = CLSTRING_DATA(obj)->mChars;
    if((int)wcstombs(str, chars, size) >= 0) {
        VMLOG(info, " (len %d) (%s)\n", len, str);
    }

    FREE(str);
}

static void mark_string_object(CLObject object, unsigned char* mark_flg)
{
    int i;
    CLObject obj2;

    obj2 = CLSTRING(object)->mData;

    mark_object(obj2, mark_flg);
}

static CLObject make_substr_from_string_object(CLObject str, int head, int tail, sVMInfo* info)
{
    wchar_t* wstr;
    int i;
    int len;
    CLObject result;

    len = tail-head+1;
    wstr = MALLOC(sizeof(wchar_t)*(len+1));
    for(i=0; i<len; i++) {
        wstr[i] = CLSTRING_DATA(str)->mChars[i+head];
    }
    wstr[i] = 0;

    result = create_string_object(wstr, len, gStringTypeObject, info);

    FREE(wstr);

    return result;
}

void initialize_hidden_class_method_of_string(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = show_string_object;
    klass->mMarkFun = mark_string_object;
    klass->mCreateFun = create_string_object_for_new;
}

BOOL String_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    wchar_t* chars;

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
}

BOOL String_char(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int index;
    CLObject ovalue1;
    wchar_t* chars;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        return FALSE;
    }

    ovalue1 = (lvar+1)->mObjectValue.mValue;

    if(!check_type(ovalue1, gIntTypeObject, info)) {
        return FALSE;
    }

    index = CLINT(ovalue1)->mValue;

    if(index < 0) index += CLSTRING(self)->mLen;

    if(index >= 0 || index < CLSTRING(self)->mLen) {
        chars = CLSTRING_DATA(self)->mChars;

        (*stack_ptr)->mObjectValue.mValue = create_int_object(chars[index]);
        (*stack_ptr)++;
    }
    else {
        (*stack_ptr)->mObjectValue.mValue = create_null_object();
        (*stack_ptr)++;
    }

    return TRUE;
}

BOOL String_replace(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    int index;
    wchar_t character;
    CLObject ovalue1, ovalue2;
    wchar_t* chars;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    ovalue1 = (lvar+1)->mObjectValue.mValue;
    if(!check_type(ovalue1, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    index = CLINT(ovalue1)->mValue;

    ovalue2 = (lvar+2)->mObjectValue.mValue;
    if(!check_type(ovalue2, gIntTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    character = (wchar_t)CLINT(ovalue2)->mValue;

    if(index < 0) index += CLSTRING(self)->mLen;

    if(index < 0 || index >= CLSTRING(self)->mLen) {
        entry_exception_object(info, gExRangeClass, "rage exception");
        vm_mutex_unlock();
        return FALSE;
    }

    chars = CLSTRING_DATA(self)->mChars;
    chars[index] = character;

    (*stack_ptr)->mObjectValue.mValue = create_int_object(character);
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL String_append(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    return TRUE;
}

BOOL String_toBytes(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    wchar_t* wstr;
    int len;
    char* buf;
    int buf_len;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    wstr = CLSTRING_DATA(self)->mChars;
    len = CLSTRING(self)->mLen;

    buf = CALLOC(1, MB_LEN_MAX * (len + 1));

    if((int)wcstombs(buf, wstr, MB_LEN_MAX * (len+1)) < 0) {
        entry_exception_object(info, gExConvertingStringCodeClass, "failed to mbstowcs on converting string");
        FREE(buf);
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_bytes_object((unsigned char*)buf, strlen(buf), gBytesTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    FREE(buf);
    vm_mutex_unlock();

    return TRUE;
}

BOOL String_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, value;
    unsigned int chars_size;
    CLObject obj2;
    CLObject new_obj;
    wchar_t* chars;
    wchar_t* chars2;
    int i;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    value = (lvar+1)->mObjectValue.mValue;

    if(!check_type(value, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    chars_size = chars_object_size(CLSTRING(value)->mLen+1);
    obj2 = alloc_heap_mem(chars_size, 0);
    CLSTRING(self)->mData = obj2;
    CLSTRING(self)->mLen = CLSTRING(value)->mLen;

    chars = CLSTRING_DATA(self)->mChars;
    chars2 = CLSTRING_DATA(value)->mChars;

    for(i=0; i<CLSTRING(value)->mLen; i++) {
        chars[i] = chars2[i];
    }
    chars[i] = 0;

    new_obj = create_string_object(CLSTRING_DATA(value)->mChars, CLSTRING(value)->mLen, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL String_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    CLObject new_obj;

    vm_mutex_lock();

    self = lvar->mObjectValue.mValue; // self

    if(!check_type(self, gStringTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }

    new_obj = create_string_object(CLSTRING_DATA(self)->mChars, CLSTRING(self)->mLen, gStringTypeObject, info);

    (*stack_ptr)->mObjectValue.mValue = new_obj;  // push result
    (*stack_ptr)++;

    vm_mutex_unlock();

    return TRUE;
}

BOOL String_cmp(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, right, ignore_case;
    wchar_t* chars;
    wchar_t* chars2;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        return FALSE;
    }

    chars = CLSTRING_DATA(self)->mChars;

    right = (lvar+1)->mObjectValue.mValue;

    if(!check_type(right, gStringTypeObject, info)) {
        return FALSE;
    }

    chars2 = CLSTRING_DATA(right)->mChars;

    ignore_case = (lvar+2)->mObjectValue.mValue;

    if(!check_type(ignore_case, gBoolTypeObject, info)) {
        return FALSE;
    }

    if(CLBOOL(ignore_case)->mValue) {
        (*stack_ptr)->mObjectValue.mValue = create_int_object(wcscasecmp(chars, chars2));
    }
    else {
        (*stack_ptr)->mObjectValue.mValue = create_int_object(wcscmp(chars, chars2));
    }

    (*stack_ptr)++;

    return TRUE;
}

BOOL string_object_to_str(ALLOC char** result, CLObject string)
{
    int len;
    int size;
    wchar_t* chars;

    len = CLSTRING(string)->mLen;

    size = (len + 1) * MB_LEN_MAX;
    *result = MALLOC(size);
    chars = CLSTRING_DATA(string)->mChars;
    if((int)wcstombs(*result, chars, size) < 0) {
        FREE(*result);
        return FALSE;
    }

    return TRUE;
}

BOOL String_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    wchar_t* chars;
    long l;
    int base;
    wchar_t* stopwcs;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        return FALSE;
    }

    chars = CLSTRING_DATA(self)->mChars;

    base = 10;

    l = wcstol(chars, &stopwcs, base);

    (*stack_ptr)->mObjectValue.mValue = create_int_object((int)l);
    (*stack_ptr)++;

    return TRUE;
}

BOOL String_toCharacterCode(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    wchar_t* chars;
    int value;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        return FALSE;
    }

    chars = CLSTRING_DATA(self)->mChars;

    if(CLSTRING(self)->mLen == 1) {
        value = (int)chars[0];
    }
    else {
        entry_exception_object_with_class_name(info, "Exception", "The length of this string is greater than 1");
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(value);
    (*stack_ptr)++;

    return TRUE;
}

static BOOL make_group_strings_from_region(OnigRegion* region, char* str, CLObject group_strings, sVMInfo* info)
{
    int i;

    for(i=1; i<region->num_regs; i++) {
        int size;
        char* group_string;
        CLObject element;
        CLObject type_object;
        
        size = region->end[i] - region->beg[i];

        group_string = MALLOC(size + 1);

        memcpy(group_string, str + region->beg[i], size);

        group_string[size] = 0;

        if(!create_string_object_from_ascii_string(&element, group_string, gStringTypeObject, info))
        {
            FREE(group_string);
            return FALSE;
        }

        add_to_array(group_strings, element, info);

        FREE(group_string);
    }

    return TRUE;
}

static BOOL make_group_strings_with_range_from_region(OnigRegion* region, CLObject group_strings, char* str, sVMInfo* info)
{
    int i;

    for(i=1; i<region->num_regs; i++) {
        CLObject element;
        int begin_value;
        int end_value;

        begin_value = um_pointer2index(kUtf8, str, str + region->beg[i]);
        end_value = um_pointer2index(kUtf8, str, str + region->end[i]);
        
        element = create_range_object(gRangeTypeObject, begin_value, end_value-1);

        add_to_array(group_strings, element, info);
    }

    return TRUE;
}

BOOL String_match(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, regex, offset, count, block;
    int count2;

    /// type checking ///
    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        return FALSE;
    }

    regex = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(regex, "Regex", info)) {
        return FALSE;
    }

    offset = (lvar+2)->mObjectValue.mValue;

    if(!check_type(offset, gIntTypeObject, info)) {
        return FALSE;
    }

    count = (lvar+3)->mObjectValue.mValue;

    if(!check_type(count, gIntTypeObject, info)) {
        return FALSE;
    }

    block = (lvar+4)->mObjectValue.mValue;

    /// go ///
    count2 = 0;

    /// onigruma regex ///
    if(substitution_posibility_of_type_object(gOnigurumaRegexTypeObject, CLOBJECT_HEADER(regex)->mType, FALSE))
    {
        char* str;
        char* p;

        if(!string_object_to_str(ALLOC &str, self)) {
            return FALSE;
        }

        p = um_index2pointer(kUtf8, str, CLINT(offset)->mValue);

        while(1) {
            OnigRegion* region;
            regex_t* regex2;
            int r;
            wchar_t* chars;

            region = onig_region_new();

            regex2 = CLONIGURUMAREGEX(regex)->mRegex;

            r = onig_search(regex2
                    , str
                    , str + strlen(str)
                    , p
                    , p + strlen(p)
                    , region, ONIG_OPTION_NONE);

            if(r == ONIG_MISMATCH) {
                onig_region_free(region, 1);
                break;
            }
            else {
                count2++;

                if(count2 == CLINT(count)->mValue) {
                    CLObject begin;
                    CLObject end;
                    CLObject group_strings;
                    BOOL result_existance;
                    BOOL static_method_block;
                    int begin_value;
                    int end_value;
                    CLObject block_result;

                    count2 = 0;

                    begin_value = um_pointer2index(kUtf8, str, str + region->beg[0]);
                    begin = create_int_object(begin_value);

                    push_object(begin, info);

                    end_value = um_pointer2index(kUtf8, str, str + region->end[0]);

                    end = create_int_object(end_value-1);

                    push_object(end, info);

                    /// make group strings ///
                    group_strings = create_array_object_with_element_class_name("Range", NULL, 0, info);

                    push_object(group_strings, info);

                    if(!make_group_strings_with_range_from_region(region, group_strings, str, info)) {
                        pop_object_except_top(info);
                        pop_object_except_top(info);
                        pop_object_except_top(info);
                        onig_region_free(region, 1);
                        FREE(str);
                        return FALSE;
                    }

                    result_existance = TRUE;
                    static_method_block = FALSE;

                    if(!cl_excute_block(block, result_existance, static_method_block, info, vm_type)) 
                    {
                        onig_region_free(region, 1);
                        FREE(str);
                        return FALSE;
                    }
                
                    block_result = (info->stack_ptr -1)->mObjectValue.mValue;

                    if(!CLBOOL(block_result)->mValue) {
                        onig_region_free(region, 1);
                        break;
                    }
                }

                p = str + region->end[0];

                if(!CLONIGURUMAREGEX(regex)->mGlobal) {
                    onig_region_free(region, 1);
                    break;
                }
            }

            onig_region_free(region, 1);
        }

        FREE(str);
    }
    else {
        entry_exception_object_with_class_name(info, "Exception", "Clover does not support this regex object(%s).", REAL_CLASS_NAME(CLTYPEOBJECT(CLOBJECT_HEADER(regex)->mType)->mClass));
        return FALSE;
    }

    return TRUE;
}

/*
BOOL String_equal_tilda(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, regex, group_strings;
    CLObject group_string_type;
    CLObject type_object;
    BOOL group_string_type_is_null;
    BOOL result;

    /// type checking ///
    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        return FALSE;
    }

    regex = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(regex, "Regex", info)) {
        return FALSE;
    }

    group_strings = (lvar+2)->mObjectValue.mValue;

    group_string_type_is_null = substitution_posibility_of_type_object(gNullTypeObject, CLOBJECT_HEADER(group_strings)->mType, FALSE);

    if(!group_string_type_is_null) {
        group_string_type = create_type_object_with_class_name("Array$1");
        push_object(group_string_type, info);
        type_object = create_type_object_with_class_name("String");
        CLTYPEOBJECT(group_string_type)->mGenericsTypes[0] = type_object;
        CLTYPEOBJECT(group_string_type)->mGenericsTypesNum = 1;

        if(!check_type(group_strings, group_string_type, info)) 
        {
            pop_object_except_top(info);
            return FALSE;
        }

        pop_object(info);
    }

    /// go ///
    /// onigruma regex ///
    if(substitution_posibility_of_type_object(gOnigurumaRegexTypeObject, CLOBJECT_HEADER(regex)->mType, FALSE))
    {
        OnigRegion* region;
        regex_t* regex2;
        wchar_t* chars;
        char* str;
        int r;
        char* p;

        region = onig_region_new();

        regex2 = CLONIGURUMAREGEX(regex)->mRegex;

        if(!string_object_to_str(ALLOC &str, self)) {
            return FALSE;
        }

        p = str;

        r = onig_search(regex2
                , str
                , str + strlen(str)
                , p
                , p + strlen(p)
                , region, ONIG_OPTION_NONE);

        if(r == ONIG_MISMATCH) {
            result = FALSE;
        }
        else {
            result = TRUE;

            if(!group_string_type_is_null && region->num_regs > 1) {
                if(!make_group_strings_from_region(region, str, group_strings, info)) {
                    onig_region_free(region, 1);
                    FREE(str);
                    return FALSE;
                }
            }
        }

        onig_region_free(region, 1);

        FREE(str);
    }
    else {
        entry_exception_object_with_class_name(info, "Exception", "Clover does not support this regex object(%s).", REAL_CLASS_NAME(CLTYPEOBJECT(CLOBJECT_HEADER(regex)->mType)->mClass));
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_bool_object(result);
    (*stack_ptr)++;

    return TRUE;
}
*/

BOOL String_sub(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, regex, replacement;
    char* replacement_str;
    CLObject result;

    /// type checking ///
    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        return FALSE;
    }

    regex = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(regex, "Regex", info)) {
        return FALSE;
    }

    replacement = (lvar+2)->mObjectValue.mValue;

    if(!check_type(replacement, gStringTypeObject, info)) {
        return FALSE;
    }

    if(!string_object_to_str(ALLOC &replacement_str, replacement)) {
        return FALSE;
    }

    /// go ///
    /// onigruma regex ///
    if(substitution_posibility_of_type_object(gOnigurumaRegexTypeObject, CLOBJECT_HEADER(regex)->mType, FALSE))
    {
        char* str;
        char* p;
        sBuf buf;

        if(!string_object_to_str(ALLOC &str, self)) {
            FREE(replacement_str);
            return FALSE;
        }

        p = str;

        sBuf_init(&buf);

        while(1) {
            OnigRegion* region;
            regex_t* regex2;
            int r;
            wchar_t* chars;

            region = onig_region_new();

            regex2 = CLONIGURUMAREGEX(regex)->mRegex;

            r = onig_search(regex2
                    , str
                    , str + strlen(str)
                    , p
                    , p + strlen(p)
                    , region, ONIG_OPTION_NONE);

            if(r == ONIG_MISMATCH) {
                onig_region_free(region, 1);
                sBuf_append(&buf, p, strlen(p));
                break;
            }
            else {
                int size;

                sBuf_append(&buf, p, str + region->beg[0] - p);

                size = region->end[0] - region->beg[0];

                sBuf_append(&buf, replacement_str, strlen(replacement_str));

                p = str + region->end[0];

                if(!CLONIGURUMAREGEX(regex)->mGlobal) {
                    onig_region_free(region, 1);
                    sBuf_append(&buf, p, strlen(p));
                    break;
                }
            }

            onig_region_free(region, 1);
        }

        if(!create_string_object_from_ascii_string(&result, buf.mBuf, gStringTypeObject, info)) 
        {
            FREE(buf.mBuf);
            FREE(str);
            FREE(replacement_str);

            return FALSE;
        }

        FREE(buf.mBuf);

        FREE(str);
    }
    else {
        entry_exception_object_with_class_name(info, "Exception", "Clover does not support this regex object(%s).", REAL_CLASS_NAME(CLTYPEOBJECT(CLOBJECT_HEADER(regex)->mType)->mClass));
        FREE(replacement_str);
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    FREE(replacement_str);

    return TRUE;
}

BOOL String_sub_with_block(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, regex, block;
    CLObject result;

    /// type checking ///
    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        return FALSE;
    }

    regex = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(regex, "Regex", info)) {
        return FALSE;
    }

    block = (lvar+2)->mObjectValue.mValue;

    /// go ///
    /// onigruma regex ///
    if(substitution_posibility_of_type_object(gOnigurumaRegexTypeObject, CLOBJECT_HEADER(regex)->mType, FALSE))
    {
        char* str;
        char* p;
        sBuf buf;

        if(!string_object_to_str(ALLOC &str, self)) {
            return FALSE;
        }

        p = str;

        sBuf_init(&buf);

        while(1) {
            OnigRegion* region;
            regex_t* regex2;
            int r;
            wchar_t* chars;

            region = onig_region_new();

            regex2 = CLONIGURUMAREGEX(regex)->mRegex;

            r = onig_search(regex2
                    , str
                    , str + strlen(str)
                    , p
                    , p + strlen(p)
                    , region, ONIG_OPTION_NONE);

            if(r == ONIG_MISMATCH) {
                onig_region_free(region, 1);
                sBuf_append(&buf, p, strlen(p));
                break;
            }
            else {
                int size;
                CLObject group_strings;
                CLObject prematch;
                CLObject match;
                CLObject postmatch;
                char* prematch_string;
                char* match_string;
                char* postmatch_string;
                BOOL result_existance;
                BOOL static_method_block;
                CLObject block_result;
                char* block_result_str;

                sBuf_append(&buf, p, str + region->beg[0] - p);

                /// make group strings ///
                group_strings = create_array_object_with_element_class_name("String", NULL, 0, info);

                push_object(group_strings, info);

                if(!make_group_strings_from_region(region, str, group_strings, info)) {
                    pop_object_except_top(info);
                    onig_region_free(region, 1);
                    FREE(str);
                    FREE(buf.mBuf);
                    return FALSE;
                }

                /// make prematch string ///
                size = region->beg[0];

                prematch_string = MALLOC(size + 1);
                memcpy(prematch_string, str, size);
                prematch_string[size] = 0;

                if(!create_string_object_from_ascii_string(&prematch, prematch_string, gStringTypeObject, info)) {
                    FREE(prematch_string);
                    pop_object_except_top(info);
                    onig_region_free(region, 1);
                    FREE(str);
                    FREE(buf.mBuf);
                    return FALSE;
                }

                FREE(prematch_string);

                push_object(prematch, info);

                /// make match string ///
                size = region->end[0] - region->beg[0];

                match_string = MALLOC(size + 1);
                memcpy(match_string, str + region->beg[0], size);
                match_string[size] = 0;

                if(!create_string_object_from_ascii_string(&match, match_string, gStringTypeObject, info)) {
                    FREE(match_string);
                    pop_object_except_top(info);
                    pop_object_except_top(info);
                    onig_region_free(region, 1);
                    FREE(str);
                    FREE(buf.mBuf);
                    return FALSE;
                }

                FREE(match_string);

                push_object(match, info);

                /// make postmatch string ///
                size = strlen(str) - region->end[0];

                postmatch_string = MALLOC(size + 1);
                memcpy(postmatch_string, str + region->end[0], size);
                postmatch_string[size] = 0;

                if(!create_string_object_from_ascii_string(&postmatch, postmatch_string, gStringTypeObject, info)) 
                {
                    FREE(postmatch_string);
                    pop_object_except_top(info);
                    pop_object_except_top(info);
                    pop_object_except_top(info);
                    onig_region_free(region, 1);
                    FREE(str);
                    FREE(buf.mBuf);
                    return FALSE;
                }

                FREE(postmatch_string);

                push_object(postmatch, info);

                result_existance = TRUE;
                static_method_block = FALSE;

                if(!cl_excute_block(block, result_existance, static_method_block, info, vm_type)) 
                {
                    FREE(buf.mBuf);
                    FREE(str);
                    onig_region_free(region, 1);
                    return FALSE;
                }

                block_result = (info->stack_ptr -1)->mObjectValue.mValue;

                if(!string_object_to_str(ALLOC &block_result_str, block_result)) {
                    FREE(buf.mBuf);
                    FREE(str);
                    onig_region_free(region, 1);
                    return FALSE;
                }

                sBuf_append(&buf, block_result_str, strlen(block_result_str));

                FREE(block_result_str);

                p = str + region->end[0];

                if(!CLONIGURUMAREGEX(regex)->mGlobal) {
                    onig_region_free(region, 1);
                    sBuf_append(&buf, p, strlen(p));
                    break;
                }
            }

            onig_region_free(region, 1);
        }

        if(!create_string_object_from_ascii_string(&result, buf.mBuf, gStringTypeObject, info)) 
        {
            FREE(buf.mBuf);
            FREE(str);

            return FALSE;
        }

        FREE(buf.mBuf);

        FREE(str);
    }
    else {
        entry_exception_object_with_class_name(info, "Exception", "Clover does not support this regex object(%s).", REAL_CLASS_NAME(CLTYPEOBJECT(CLOBJECT_HEADER(regex)->mType)->mClass));
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    return TRUE;
}

BOOL String_count(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, regex;
    int count;

    /// type checking ///
    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        return FALSE;
    }

    regex = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(regex, "Regex", info)) {
        return FALSE;
    }

    /// go ///
    count = 0;

    /// onigruma regex ///
    if(substitution_posibility_of_type_object(gOnigurumaRegexTypeObject, CLOBJECT_HEADER(regex)->mType, FALSE))
    {
        char* str;
        char* p;

        if(!string_object_to_str(ALLOC &str, self)) {
            return FALSE;
        }

        p = str;

        while(1) {
            OnigRegion* region;
            regex_t* regex2;
            int r;

            region = onig_region_new();

            regex2 = CLONIGURUMAREGEX(regex)->mRegex;

            r = onig_search(regex2
                    , str
                    , str + strlen(str)
                    , p
                    , p + strlen(p)
                    , region, ONIG_OPTION_NONE);

            if(r == ONIG_MISMATCH) {
                onig_region_free(region, 1);
                break;
            }
            else {
                count ++;
                p = str + region->end[0];

                if(!CLONIGURUMAREGEX(regex)->mGlobal) {
                    onig_region_free(region, 1);
                    break;
                }
            }

            onig_region_free(region, 1);
        }

        FREE(str);
    }
    else {
        entry_exception_object_with_class_name(info, "Exception", "Clover does not support this regex object(%s).", REAL_CLASS_NAME(CLTYPEOBJECT(CLOBJECT_HEADER(regex)->mType)->mClass));
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(count);
    (*stack_ptr)++;

    return TRUE;
}

BOOL String_sub_with_hash(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, regex, hash;
    CLObject result;
    CLObject hash_type_object;

    /// type checking ///
    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        return FALSE;
    }

    regex = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(regex, "Regex", info)) {
        return FALSE;
    }

    hash = (lvar+2)->mObjectValue.mValue;

    hash_type_object = create_type_object_with_class_name_and_generics_name("Hash<String,String>", info);

    push_object(hash_type_object, info);

    if(!check_type(hash, hash_type_object, info)) {
        pop_object_except_top(info);
        return FALSE;
    }

    pop_object(info);

    /// go ///
    /// onigruma regex ///
    if(substitution_posibility_of_type_object(gOnigurumaRegexTypeObject, CLOBJECT_HEADER(regex)->mType, FALSE))
    {
        char* str;
        char* p;
        sBuf buf;

        if(!string_object_to_str(ALLOC &str, self)) {
            return FALSE;
        }

        p = str;

        sBuf_init(&buf);

        while(1) {
            OnigRegion* region;
            regex_t* regex2;
            int r;
            wchar_t* chars;

            region = onig_region_new();

            regex2 = CLONIGURUMAREGEX(regex)->mRegex;

            r = onig_search(regex2
                    , str
                    , str + strlen(str)
                    , p
                    , p + strlen(p)
                    , region, ONIG_OPTION_NONE);

            if(r == ONIG_MISMATCH) {
                onig_region_free(region, 1);
                sBuf_append(&buf, p, strlen(p));
                break;
            }
            else {
                int size;
                CLObject match;
                char* match_string;
                CLObject block_result;
                char* block_result_str;
                int i;
                CLObject hash_data;

                sBuf_append(&buf, p, str + region->beg[0] - p);

                /// make match string ///
                size = region->end[0] - region->beg[0];

                match_string = MALLOC(size + 1);
                memcpy(match_string, str + region->beg[0], size);
                match_string[size] = 0;

                hash_data = CLHASH(hash)->mData;
                for(i=0; i<CLHASH(hash)->mSize; i++) {
                    unsigned int hash_value;

                    hash_value = CLHASH_DATA(hash_data)->mItems[i].mHashValue;

                    if(hash_value) {
                        CLObject key;
                        CLObject item;
                        char* key_string;
                        char* item_string;

                        key = CLHASH_DATA(hash_data)->mItems[i].mKey;
                        item = CLHASH_DATA(hash_data)->mItems[i].mItem;

                        if(!check_type(key, gStringTypeObject, info)) {
                            FREE(match_string);
                            onig_region_free(region, 1);
                            FREE(str);
                            FREE(buf.mBuf);
                            return FALSE;
                        }

                        if(!check_type(item, gStringTypeObject, info)) {
                            FREE(match_string);
                            onig_region_free(region, 1);
                            FREE(str);
                            FREE(buf.mBuf);
                            return FALSE;
                        }

                        if(!string_object_to_str(ALLOC &key_string, key)) {
                            FREE(match_string);
                            onig_region_free(region, 1);
                            FREE(str);
                            FREE(buf.mBuf);
                            return FALSE;
                        }

                        if(!string_object_to_str(ALLOC &item_string, item)) {
                            FREE(key_string);
                            FREE(match_string);
                            onig_region_free(region, 1);
                            FREE(str);
                            FREE(buf.mBuf);
                            return FALSE;
                        }

                        if(strcmp(match_string, key_string) == 0) {
                            sBuf_append(&buf, item_string, strlen(item_string));
                        }

                        FREE(key_string);
                        FREE(item_string);
                    }
                }

                FREE(match_string);

                p = str + region->end[0];

                if(!CLONIGURUMAREGEX(regex)->mGlobal) {
                    onig_region_free(region, 1);
                    sBuf_append(&buf, p, strlen(p));
                    break;
                }
            }

            onig_region_free(region, 1);
        }

        if(!create_string_object_from_ascii_string(&result, buf.mBuf, gStringTypeObject, info)) 
        {
            FREE(buf.mBuf);
            FREE(str);

            return FALSE;
        }

        FREE(buf.mBuf);

        FREE(str);
    }
    else {
        entry_exception_object_with_class_name(info, "Exception", "Clover does not support this regex object(%s).", REAL_CLASS_NAME(CLTYPEOBJECT(CLOBJECT_HEADER(regex)->mType)->mClass));
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = result;
    (*stack_ptr)++;

    return TRUE;
}

BOOL String_index(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, regex, offset, count2;
    int count;
    int index;

    /// type checking ///
    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        return FALSE;
    }

    regex = (lvar+1)->mObjectValue.mValue;

    if(!check_type_with_class_name(regex, "Regex", info)) {
        return FALSE;
    }

    offset = (lvar+2)->mObjectValue.mValue;

    if(!check_type(offset, gIntTypeObject, info)) {
        return FALSE;
    }

    count2 = (lvar+3)->mObjectValue.mValue;

    if(!check_type(count2, gIntTypeObject, info)) {
        return FALSE;
    }

    /// go ///
    index = -1;
    count = 0;

    /// onigruma regex ///
    if(substitution_posibility_of_type_object(gOnigurumaRegexTypeObject, CLOBJECT_HEADER(regex)->mType, FALSE))
    {
        char* str;
        char* p;
        CLObject substr;

        substr = make_substr_from_string_object(self, CLINT(offset)->mValue, CLSTRING(self)->mLen-1, info);

        push_object(substr, info);

        if(!string_object_to_str(ALLOC &str, substr)) {
            pop_object_except_top(info);
            return FALSE;
        }

        pop_object(info);

        p = str;

        while(1) {
            OnigRegion* region;
            regex_t* regex2;
            int r;

            region = onig_region_new();

            regex2 = CLONIGURUMAREGEX(regex)->mRegex;

            r = onig_search(regex2
                    , str
                    , str + strlen(str)
                    , p
                    , p + strlen(p)
                    , region, ONIG_OPTION_NONE);

            if(r == ONIG_MISMATCH) {
                onig_region_free(region, 1);
                break;
            }
            else {
                count ++;

                if(count == CLINT(count2)->mValue) {
                    int len;
                    char* str2;
                    int wlen;
                    wchar_t* wstr;

                    len = region->beg[0];

                    str2 = MALLOC(len+1);
                    memcpy(str2, str, len);
                    str2[len] = 0;

                    wlen = len+1;
                    wstr = MALLOC(sizeof(wchar_t)*wlen);

                    if((int)mbstowcs(wstr, str2, wlen) < 0) {
                        entry_exception_object(info, gExConvertingStringCodeClass, "error mbstowcs on converting string");
                        FREE(wstr);
                        FREE(str2);
                        onig_region_free(region, 1);
                        FREE(str);
                        return FALSE;
                    }

                    index = wcslen(wstr) + CLINT(offset)->mValue;

                    FREE(wstr);
                    FREE(str2);
                    break;
                }

                p = str + region->end[0];

                if(!CLONIGURUMAREGEX(regex)->mGlobal) {
                    onig_region_free(region, 1);
                    break;
                }
            }

            onig_region_free(region, 1);
        }

        FREE(str);
    }
    else {
        entry_exception_object_with_class_name(info, "Exception", "Clover does not support this regex object(%s).", REAL_CLASS_NAME(CLTYPEOBJECT(CLOBJECT_HEADER(regex)->mType)->mClass));
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_int_object(index);
    (*stack_ptr)++;

    return TRUE;
}

