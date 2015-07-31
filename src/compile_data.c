#include "clover.h"
#include "common.h"

static sClassCompileData gCompileData[CLASS_HASH_SIZE];

sClassCompileData* get_compile_data(sCLClass* klass, int parametor_num)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    unsigned int hash;
    sClassCompileData* class_compile_data;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(klass), CLASS_NAME(klass), parametor_num);

    hash = get_hash(real_class_name) % CLASS_HASH_SIZE;

    class_compile_data = gCompileData + hash;
    while(1) {
        if(class_compile_data->mRealClassName[0] == 0) {
            return NULL;
        }
        else if(strcmp(class_compile_data->mRealClassName, real_class_name) == 0) {
            return class_compile_data;
        }
        else {
            class_compile_data++;

            if(class_compile_data == gCompileData + CLASS_HASH_SIZE) {
                class_compile_data = gCompileData;
            }
            else if(class_compile_data == gCompileData + hash) {
                return NULL;
            }
        }
    }
}

BOOL add_compile_data(sCLClass* klass, char num_definition, unsigned char num_method, enum eCompileType compile_type, int parametor_num)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    unsigned int hash;
    sClassCompileData* class_compile_data;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, NAMESPACE_NAME(klass), CLASS_NAME(klass), parametor_num);

    hash = get_hash(real_class_name) % CLASS_HASH_SIZE;

    class_compile_data = gCompileData + hash;
    while(class_compile_data->mRealClassName[0] != 0) {
        class_compile_data++;

        if(class_compile_data == gCompileData + CLASS_HASH_SIZE) {
            class_compile_data = gCompileData;
        }
        else if(class_compile_data == gCompileData + hash) {
            return FALSE;
        }
    }

    xstrncpy(class_compile_data->mRealClassName, real_class_name, CL_REAL_CLASS_NAME_MAX);
    class_compile_data->mNumDefinition = num_definition;
    class_compile_data->mNumMethod = num_method;
    class_compile_data->mNumMethodOnLoaded = num_method;
    class_compile_data->mCompileType = compile_type;

    return TRUE;
}

void clear_compile_data()
{
    int i;

    for(i=0; i<CLASS_HASH_SIZE; i++) {
        sClassCompileData* class_compile_data = gCompileData + i;

        if(class_compile_data->mRealClassName[0] != 0)
        {
            class_compile_data->mNumDefinition = 0;
            if(class_compile_data->mCompileType == kCompileTypeLoad) {
                class_compile_data->mNumMethod = class_compile_data->mNumMethodOnLoaded;
            }
            else {
                class_compile_data->mNumMethod = 0;
            }
        }
    }
}

