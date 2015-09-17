#include "clover.h"
#include "common.h"
#include <fcntl.h>
#include <unistd.h>

void initialize_hidden_class_method_of_access_mode(sCLClass* klass)
{
    int values[4] = {
        F_OK, R_OK, W_OK, X_OK
    };

    entry_native_enum_fields(klass, 4, values);
}
