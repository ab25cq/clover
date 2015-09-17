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
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "failed to mbstowcs on converting string");
        FREE(*result);
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

CLObject create_string_object_with_class_name(wchar_t* str, int len, char* class_name, sVMInfo* info)
{
    CLObject result;
    CLObject type_object;

    type_object = create_type_object_with_class_name(class_name);

    push_object(type_object, info);

    result = create_string_object(str, len, type_object, info);

    pop_object(info);

    return result;
}

BOOL create_string_object_from_ascii_string(CLObject* result, char* str, CLObject type_object, sVMInfo* info)
{
    int wlen;
    wchar_t* wstr;

    wlen = strlen(str)+1;
    wstr = MALLOC(sizeof(wchar_t)*wlen);

    if((int)mbstowcs(wstr, str, wlen) < 0) {
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "error mbstowcs on converting string");
        FREE(wstr);
        return FALSE;
    }

    *result = create_string_object(wstr, wlen, type_object, info);

    FREE(wstr);

    return TRUE;
}

BOOL create_string_object_from_ascii_string_with_class_name(CLObject* result, char* str, char* class_name, sVMInfo* info)
{
    CLObject type_object;

    type_object = create_type_object_with_class_name(class_name);

    push_object(type_object, info);

    if(!create_string_object_from_ascii_string(result, str, type_object, info)) {
        pop_object_except_top(info);
        return FALSE;
    }

    pop_object(info);

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

    obj_size = object_size();

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

    len = tail-head;
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

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gStringClass = klass;
        gStringTypeObject = create_type_object(gStringClass);
    }
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

    if(index >= 0 && index < CLSTRING(self)->mLen) {
        chars = CLSTRING_DATA(self)->mChars;

        (*stack_ptr)->mObjectValue.mValue = create_char_object(chars[index]);
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
    if(!check_type(ovalue2, gCharTypeObject, info)) {
        vm_mutex_unlock();
        return FALSE;
    }
    character = CLCHAR(ovalue2)->mValue;

    if(index < 0) index += CLSTRING(self)->mLen;

    if(index < 0 || index >= CLSTRING(self)->mLen) {
        entry_exception_object_with_class_name(info, "RangeException", "rage exception");
        vm_mutex_unlock();
        return FALSE;
    }

    chars = CLSTRING_DATA(self)->mChars;
    chars[index] = character;

    (*stack_ptr)->mObjectValue.mValue = create_char_object(character);
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
        entry_exception_object_with_class_name(info, "ConvertingStringCodeException", "failed to mbstowcs on converting string");
        FREE(buf);
        vm_mutex_unlock();
        return FALSE;
    }
    new_obj = create_bytes_object(buf, strlen(buf), gBytesTypeObject, info);

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

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
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

BOOL String_toDouble(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    wchar_t* chars;
    double dvalue;
    wchar_t* endptr;

    self = lvar->mObjectValue.mValue;

    if(!check_type(self, gStringTypeObject, info)) {
        return FALSE;
    }

    chars = CLSTRING_DATA(self)->mChars;

    dvalue = wcstod(chars, &endptr);

    (*stack_ptr)->mObjectValue.mValue = create_double_object(dvalue);
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
        CLObject head_object;
        CLObject tail_object;

        begin_value = um_pointer2index(kUtf8, str, str + region->beg[i]);
        end_value = um_pointer2index(kUtf8, str, str + region->end[i]);

        head_object = create_int_object(begin_value);
        push_object(head_object, info);

        tail_object = create_int_object(end_value);
        push_object(tail_object, info);

        element = create_range_object(gRangeTypeObject, head_object, tail_object);

        pop_object(info);
        pop_object(info);

        add_to_array(group_strings, element, info);
    }

    return TRUE;
}

static BOOL correct_offset(CLObject offset, int* offset_value, CLObject self, sVMInfo* info)
{
    if(!check_type_with_nullable(offset, gIntTypeObject, info)) {
        return FALSE;
    }

    if(substitution_posibility_of_type_object(CLOBJECT_HEADER(offset)->mType, gNullTypeObject, FALSE))
    {
        *offset_value = CLSTRING(self)->mLen;
    }
    else {
        *offset_value = CLINT(offset)->mValue;

        if(*offset_value < 0) {
            *offset_value += CLSTRING(self)->mLen;
        }
    }

    if(*offset_value < 0) { 
        *offset_value = 0; 
    }
    if(*offset_value > CLSTRING(self)->mLen) { 
        *offset_value = CLSTRING(self)->mLen; 
    }

    return TRUE;
}

BOOL String_match(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, regex, offset, count, block;
    int count2;
    int offset_value;

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

    if(!check_type_with_nullable(offset, gIntTypeObject, info)) {
        return FALSE;
    }
    if(!correct_offset(offset, &offset_value, self, info)) {
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
        int region_beg_before;
        int region_end_before;

        if(!string_object_to_str(ALLOC &str, self)) {
            return FALSE;
        }

        region_beg_before = -1;
        region_end_before = -1;
        p = um_index2pointer(kUtf8, str, offset_value);

        while(1) {
            OnigRegion* region;
            regex_t* regex2;
            int r;
            wchar_t* chars;

            regex2 = CLONIGURUMAREGEX(regex)->mRegex;

            while(1) {
                region = onig_region_new();

                r = onig_search(regex2
                    , (OnigUChar*)str
                    , (OnigUChar*)str + strlen(str)
                    , (OnigUChar*)p
                    , (OnigUChar*)p + strlen(p)
                    , region, ONIG_OPTION_NONE);

                if(r == ONIG_MISMATCH) {
                    break;
                }

                if(region->beg[0] == region_beg_before 
                    && region->end[0] == region_end_before
                    && region->beg[0] == region->end[0]) 
                {
                    p = str + region_end_before + 1;
                    onig_region_free(region, 1);
                }
                else {
                    break;
                }
            }

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
                    int begin_value;
                    int end_value;
                    CLObject block_result;
                    CLObject break_existance;

                    count2 = 0;

                    begin_value = um_pointer2index(kUtf8, str, str + region->beg[0]);
                    begin = create_int_object(begin_value);

                    push_object(begin, info);

                    end_value = um_pointer2index(kUtf8, str, str + region->end[0]);

                    end = create_int_object(end_value);

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

                    if(!cl_excute_block(block, result_existance, info, vm_type)) 
                    {
                        onig_region_free(region, 1);
                        FREE(str);
                        return FALSE;
                    }
                
                    block_result = (info->stack_ptr -1)->mObjectValue.mValue;

                    if(!check_type_with_class_name(block_result, "bool", info)) {
                        onig_region_free(region, 1);
                        FREE(str);
                        return FALSE;
                    }

                    if(CLBOOL(block_result)->mValue) {
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

            region_beg_before = region->beg[0];
            region_end_before = region->end[0];

            onig_region_free(region, 1);
        }

        FREE(str);
    }
    else {
        entry_exception_object_with_class_name(info, "Exception", "Clover does not support this regex object(%s).", REAL_CLASS_NAME(CLTYPEOBJECT(CLOBJECT_HEADER(regex)->mType)->mClass));
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

BOOL String_matchReverse(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self, regex, offset, count, block;
    int count2;
    int offset_value;

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

    if(!check_type_with_nullable(offset, gIntTypeObject, info)) {
        return FALSE;
    }
    if(!correct_offset(offset, &offset_value, self, info)) {
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

        p = um_index2pointer(kUtf8, str, offset_value);

        while(1) {
            OnigRegion* region;
            regex_t* regex2;
            int r;
            wchar_t* chars;

            region = onig_region_new();

            regex2 = CLONIGURUMAREGEX(regex)->mRegex;

            r = onig_search(regex2
                    , (OnigUChar*)str
                    , (OnigUChar*)str + strlen(str)
                    , (OnigUChar*)p
                    , (OnigUChar*)str
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
                    int begin_value;
                    int end_value;
                    CLObject block_result;
                    CLObject break_existance;

                    count2 = 0;

                    begin_value = um_pointer2index(kUtf8, str, str + region->beg[0]);
                    begin = create_int_object(begin_value);

                    push_object(begin, info);

                    end_value = um_pointer2index(kUtf8, str, str + region->end[0]);

                    end = create_int_object(end_value);

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

                    if(!cl_excute_block(block, result_existance, info, vm_type)) 
                    {
                        onig_region_free(region, 1);
                        FREE(str);
                        return FALSE;
                    }
                
                    block_result = (info->stack_ptr -1)->mObjectValue.mValue;

                    if(!check_type_with_class_name(block_result, "bool", info)) {
                        onig_region_free(region, 1);
                        FREE(str);
                        return FALSE;
                    }

                    if(CLBOOL(block_result)->mValue) {
                        onig_region_free(region, 1);
                        break;
                    }
                }

                p = str + region->beg[0] -1;

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

    (*stack_ptr)->mObjectValue.mValue = create_null_object();  // push result
    (*stack_ptr)++;

    return TRUE;
}

