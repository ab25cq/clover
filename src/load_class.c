#include "clover.h"
#include "common.h"

//////////////////////////////////////////////////
// loaded class on compile time
//////////////////////////////////////////////////
static char* gLoadedClassOnCompileTime = NULL;
static int gSizeLoadedClassOnCompileTime;
static int gNumLoadedClassOnCompileTime;

void load_class_init()
{
    gSizeLoadedClassOnCompileTime = 4;
    gLoadedClassOnCompileTime = CALLOC(1, CL_REAL_CLASS_NAME_MAX*gSizeLoadedClassOnCompileTime);
    gNumLoadedClassOnCompileTime = 0;
}

void load_class_final()
{
    if(gLoadedClassOnCompileTime) {
        FREE(gLoadedClassOnCompileTime);
    }
}

static BOOL is_already_added_on_loaded_class_table(char* real_class_name)
{
    char* p = gLoadedClassOnCompileTime;

    while(p < gLoadedClassOnCompileTime + CL_REAL_CLASS_NAME_MAX*gNumLoadedClassOnCompileTime)
    {
        if(strcmp(p, real_class_name) == 0) {
            return TRUE;
        }

        p += CL_REAL_CLASS_NAME_MAX;
    }

    return FALSE;
}

void add_loaded_class_to_table(sCLClass* loaded_class)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];

    xstrncpy(real_class_name, REAL_CLASS_NAME(loaded_class), CL_REAL_CLASS_NAME_MAX);

    if(is_already_added_on_loaded_class_table(real_class_name)) {
        return;
    }

    if(gNumLoadedClassOnCompileTime == gSizeLoadedClassOnCompileTime) {
        int new_size;
        
        new_size = gSizeLoadedClassOnCompileTime * 2;
        gLoadedClassOnCompileTime = xxrealloc(gLoadedClassOnCompileTime, CL_REAL_CLASS_NAME_MAX * gSizeLoadedClassOnCompileTime, CL_REAL_CLASS_NAME_MAX*new_size);
        memset(gLoadedClassOnCompileTime + CL_REAL_CLASS_NAME_MAX*gSizeLoadedClassOnCompileTime, 0, CL_REAL_CLASS_NAME_MAX*(new_size - gSizeLoadedClassOnCompileTime));
        gSizeLoadedClassOnCompileTime = new_size;
    }

    memcpy(gLoadedClassOnCompileTime + CL_REAL_CLASS_NAME_MAX*gNumLoadedClassOnCompileTime, real_class_name, strlen(real_class_name)+1);
    gNumLoadedClassOnCompileTime++;
}

char* get_loaded_class(int index)
{
    return gLoadedClassOnCompileTime + CL_REAL_CLASS_NAME_MAX*index;
}

int num_loaded_class()
{
    return gNumLoadedClassOnCompileTime;
}
