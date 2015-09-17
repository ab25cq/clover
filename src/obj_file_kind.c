#include "clover.h"
#include "common.h"
#include <sys/stat.h>

void initialize_hidden_class_method_of_file_kind(sCLClass* klass)
{
    int values[8] = {
        S_IFMT, S_IFDIR, S_IFCHR, S_IFBLK, S_IFREG, S_IFIFO, S_IFLNK, S_IFSOCK
    };

    entry_native_enum_fields(klass, 8, values);
}
