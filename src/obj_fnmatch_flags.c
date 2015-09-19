#include "clover.h"
#include "common.h"
#define _GNU_SOURCE
#include <fnmatch.h>

void initialize_hidden_class_method_of_fnmatch_flags(sCLClass* klass)
{
    int values[5] = {
        FNM_NOESCAPE, FNM_PATHNAME, FNM_PERIOD, FNM_LEADING_DIR, FNM_CASEFOLD
    };

    entry_native_enum_fields(klass, 5, values);
}

