#include "clover.h"
#include "common.h"
#include <limits.h>

BOOL Clover_load(MVALUE** stack_ptr, MVALUE* lvar)
{
    MVALUE* file = lvar;
    int size;
    char* str;

    size = (CLSTRING(file->mObjectValue)->mLen + 1) * MB_LEN_MAX;
    str = MALLOC(size);
    wcstombs(str, CLSTRING(file->mObjectValue)->mChars, size);

    if(!load_class_from_classpath(str)) {
cl_print("can't load this class(%s)\n", str);
cl_puts("throw exception");
return FALSE;
    }

    FREE(str);

    return TRUE;
}

BOOL Clover_print(MVALUE** stack_ptr, MVALUE* lvar)
{
    MVALUE* string;
    int size;
    char* str;

    string = lvar;

    if(string->mIntValue == 0) {
        /// exception ///
cl_print("Null Pointer Exception on Clover.print()");
cl_puts("throw exception");
return FALSE;
    }

    size = (CLSTRING(string->mObjectValue)->mLen + 1) * MB_LEN_MAX;
    str = MALLOC(size);
    wcstombs(str, CLSTRING(string->mObjectValue)->mChars, size);

    cl_print("%s\n", str);

    FREE(str);

    return TRUE;
}

BOOL Clover_compile(MVALUE** stack_ptr, MVALUE* lvar)
{

    return TRUE;
}

BOOL Clover_gc(MVALUE** stack_ptr, MVALUE* lvar)
{
puts("running gc...");
    cl_gc();

    return TRUE;
}

BOOL Clover_show_classes(MVALUE** stack_ptr, MVALUE* lvar)
{
    show_class_list();

    return TRUE;
}
