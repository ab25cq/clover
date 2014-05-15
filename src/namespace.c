#include "clover.h"
#include "common.h"

BOOL append_namespace_to_curernt_namespace(char* current_namespace, char* namespace)
{
    int len;

    if(current_namespace[0] == 0) {
        len = strlen(namespace) + 1;

        if(len >= CL_NAMESPACE_NAME_MAX) {
            return FALSE;
        }

        xstrncpy(current_namespace, namespace, CL_NAMESPACE_NAME_MAX);
    }
    else {
        len = strlen(current_namespace) + strlen(namespace) + 1 + 2;

        if(len >= CL_NAMESPACE_NAME_MAX) {
            return FALSE;
        }

        xstrncat(current_namespace, "::", CL_NAMESPACE_NAME_MAX);
        xstrncat(current_namespace, namespace, CL_NAMESPACE_NAME_MAX);
    }

    return TRUE;
}

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
// don't search for default namespace
sCLClass* cl_get_class_with_argument_namespace_only(char* namespace, char* class_name)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    return cl_get_class(real_class_name);
}
