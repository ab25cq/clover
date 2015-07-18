#include "clover.h"
#include "common.h"
#include <fcntl.h>

void initialize_hidden_class_method_of_file_mode(sCLClass* klass)
{
    int values[12] = {
        O_APPEND, O_ASYNC, O_CLOEXEC, O_CREAT, O_DIRECTORY, O_DSYNC, O_EXCL, O_NOCTTY, O_NOFOLLOW, O_NONBLOCK, O_SYNC, O_TRUNC
    };

    entry_native_enum_fields(klass, 12, values);
}
