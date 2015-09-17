#include "clover.h"
#include "common.h"
#include <sys/stat.h>

void initialize_hidden_class_method_of_file_access(sCLClass* klass)
{
    int values[15] = {
        S_ISUID, S_ISGID, S_ISVTX, S_IRWXU, S_IRUSR, S_IWUSR, S_IXUSR, S_IRWXG, S_IRGRP, S_IWGRP, S_IXGRP, S_IRWXO, S_IROTH, S_IWOTH, S_IXOTH
    };

    entry_native_enum_fields(klass, 15, values);
}
