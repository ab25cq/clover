#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

struct sAliasItem {
    char mName[CL_METHOD_NAME_MAX];
    sCLMethod* mMethod;
    sCLClass* mClass;
};

struct sAliasItem gAliases[CL_ALIAS_MAX];

static int get_alias_hash(char* alias_name)
{
    int n;
    char* p;

    n = 0;
    p = alias_name;
    while(*p) {
        n += *p++;
    }

    return n % CL_ALIAS_MAX;
}

// result: (TRUE) success (FALSE) overflow
static BOOL add_alias_to_alias_table(sCLClass* klass, sCLMethod* method)
{
    int hash;
    char* name;
    struct sAliasItem* item;

    name = METHOD_NAME2(klass, method);

    hash = get_alias_hash(name);

    item = gAliases + hash;
    while(1) {
        if(item->mName[0] == 0) {  // found the empty space
            xstrncpy(item->mName, name, CL_METHOD_NAME_MAX);
            item->mMethod = method;
            item->mClass = klass;
            break;
        }
        else {
            if(strcmp(item->mName, name) == 0) {
                xstrncpy(item->mName, name, CL_METHOD_NAME_MAX);
                item->mMethod = method;
                item->mClass = klass;
                break;
            }

            item++;

            if(item == gAliases + CL_ALIAS_MAX) {
                item = gAliases;
            }
            else if(item == gAliases + hash) {      // can't found the empty space. overflow
                return FALSE;
            }
        }
    }

    return TRUE;
}

// result: (NULL) not found (sCLMethod*) found
sCLMethod* get_method_from_alias_table(char* name, sCLClass** klass)
{
    int hash;
    struct sAliasItem* item;

    *klass = NULL;

    hash = get_alias_hash(name);

    item = gAliases + hash;

    while(1) {
        if(item->mName[0] == 0) {
            return NULL;
        }
        else if(strcmp(item->mName, name) == 0) {
            *klass = item->mClass;
            return item->mMethod;
        }
        else {
            item++;

            if(item == gAliases + CL_ALIAS_MAX) {
                item = gAliases;
            }
            else if(item == gAliases + hash) {
                return NULL;
            }
        }
    }

    return NULL;
}

// result: (TRUE) success (FALSE) not found the method or overflow alias table
BOOL set_alias_flag_to_method(sCLClass* klass, char* method_name)
{
    BOOL found;
    int i;

    found = FALSE;
    for(i=0; i<klass->mNumMethods; i++) {
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            sCLMethod* method;

            method = klass->mMethods + i;

            method->mFlags |=  CL_ALIAS_METHOD;

            if(!add_alias_to_alias_table(klass, method)) {
                return FALSE;
            }
            found = TRUE;
            break;
        }
    }

    if(!found) {
        return FALSE;
    }

    return TRUE;
}

// result: (TRUE) success (FALSE) overflow alias table
BOOL set_alias_flag_to_all_methods(sCLClass* klass)
{
    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        sCLMethod* method;

        method = klass->mMethods + i;

        method->mFlags |=  CL_ALIAS_METHOD;

        if(!add_alias_to_alias_table(klass, method)) {
            return FALSE;
        }
    }

    return TRUE;
}

// result: (TRUE) success (FALSE) overflow alias table
static BOOL entry_alias_of_class_to_alias_table(sCLClass* klass)
{
    int i;
    for(i=0; i<klass->mNumMethods; i++) {
        sCLMethod* method;

        method = klass->mMethods + i;

        if((method->mFlags & CL_CLASS_METHOD) && (method->mFlags & CL_ALIAS_METHOD)) {
            if(!add_alias_to_alias_table(klass, method)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static sCLClass* load_class_from_classpath_on_compile_time(char* real_class_name, BOOL resolve_dependences);

static BOOL check_method_and_field_types_offset_on_compile_time(sCLClass* klass)
{
    int i;

    for(i=0; i<klass->mNumFields; i++) {
        sCLField* field = klass->mFields + i;
        sCLClass* klass2;
        
        klass2 = cl_get_class(CONS_str(&klass->mConstPool, field->mType.mClassNameOffset));

        if(klass2 == NULL) {
            if(load_class_from_classpath_on_compile_time(CONS_str(&klass->mConstPool, field->mType.mClassNameOffset), TRUE) == NULL)
            {
                vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, field->mType.mClassNameOffset));
                return FALSE;
            }
        }
    }

    for(i=0; i<klass->mNumMethods; i++) {
        sCLMethod* method = klass->mMethods + i;
        sCLClass* klass2;
        int j;

        /// result type ///
        klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mResultType.mClassNameOffset));

        if(klass2 == NULL) {
            if(load_class_from_classpath_on_compile_time(CONS_str(&klass->mConstPool, method->mResultType.mClassNameOffset), TRUE) == NULL)
            {
                vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, method->mResultType.mClassNameOffset));
                return FALSE;
            }
        }

        /// param types ///
        for(j=0; j<method->mNumParams; j++) {
            klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mParamTypes[j].mClassNameOffset)); 

            if(klass2 == NULL) {
                if(load_class_from_classpath_on_compile_time(CONS_str(&klass->mConstPool, method->mParamTypes[j].mClassNameOffset), TRUE) == NULL)
                {
                    vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, method->mParamTypes[j].mClassNameOffset));
                    return FALSE;
                }
            }
        }

        /// block type ///
        if(method->mNumBlockType == 1) {
            klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mBlockType.mResultType.mClassNameOffset));

            if(klass2 == NULL) {
                if(load_class_from_classpath_on_compile_time(CONS_str(&klass->mConstPool, method->mBlockType.mResultType.mClassNameOffset), TRUE) == NULL)
                {
                    vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, method->mBlockType.mResultType.mClassNameOffset));
                    return FALSE;
                }
            }

            for(j=0; j<method->mBlockType.mNumParams; j++) {
                klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mBlockType.mParamTypes[j].mClassNameOffset));

                if(klass2 == NULL) {
                    if(load_class_from_classpath_on_compile_time(CONS_str(&klass->mConstPool, method->mBlockType.mParamTypes[j].mClassNameOffset), TRUE) == NULL)
                    {
                        vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, method->mBlockType.mParamTypes[j].mClassNameOffset));
                        return FALSE;
                    }
                }
            }
        }

        /// exception ///
        for(j=0; j<method->mNumException; j++) {
            klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mExceptionClassNameOffset[j]));

            if(klass2 == NULL) {
                if(load_class_from_classpath_on_compile_time(CONS_str(&klass->mConstPool, method->mExceptionClassNameOffset[j]), TRUE) == NULL)
                {
                    vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, method->mExceptionClassNameOffset[j]));
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

static BOOL check_super_class_offsets_on_compile_time(sCLClass* klass)
{
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        sCLClass* super_class;
        
        super_class = cl_get_class(CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]));

        if(super_class == NULL) {
            if(load_class_from_classpath_on_compile_time(CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]), TRUE) == NULL)
            {
                vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, klass->mSuperClassesOffset[i]));
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL check_dependece_offsets_on_compile_time(sCLClass* klass)
{
    int i;
    for(i=0; i<klass->mNumDependences; i++) {
        sCLClass* dependence_class;
        
        dependence_class = cl_get_class(CONS_str(&klass->mConstPool, klass->mDepedencesOffset[i]));
        if(dependence_class == NULL) {
            if(load_class_from_classpath_on_compile_time(CONS_str(&klass->mConstPool, klass->mDepedencesOffset[i]), TRUE) == NULL)
            {
                vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, klass->mDepedencesOffset[i]));
                return FALSE;
            }
        }
    }

    return TRUE;
}

// result: (NULL) --> file not found (sCLClass*) loaded class
static sCLClass* load_class_on_compile_time(char* file_name, BOOL resolve_dependences) 
{
    sCLClass* klass;
    sCLClass* klass2;
    int fd;
    char c;
    int size;

    fd = open(file_name, O_RDONLY);

    if(fd < 0) {
        return NULL;
    }

    /// check magic number. Is this Clover object file? ///
    if(!read_from_file(fd, &c, 1) || c != 12) { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 17) { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 33) { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 79) { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'C') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'L') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'O') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'V') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'E') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'R') { close(fd); return NULL; }

    klass = read_class_from_file(fd);
    close(fd);

    if(klass == NULL) {
        return NULL;
    }

    /// immediate class is special ///
    initialize_hidden_class_method_and_flags(NAMESPACE_NAME(klass), CLASS_NAME(klass), klass);

    /// if there is a class which is entried to class table already, return the class
    klass2 = cl_get_class_with_namespace(NAMESPACE_NAME(klass), CLASS_NAME(klass));
    
    if(klass2) {
        free_class(klass);
        return klass2;
    }

    //// add class to class table ///
    add_class_to_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass), klass);

    if(resolve_dependences) {
        if(!check_super_class_offsets_on_compile_time(klass)){
            vm_error("can't load class %s because of the super classes\n", REAL_CLASS_NAME(klass));
            remove_class_from_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass));
            free_class(klass);
            return NULL;
        }

        if(!check_method_and_field_types_offset_on_compile_time(klass)) {
            vm_error("can't load class %s because of dependences\n", REAL_CLASS_NAME(klass));
            remove_class_from_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass));
            free_class(klass);
            return NULL;
        }

        if(!check_dependece_offsets_on_compile_time(klass)) {
            vm_error("can't load class %s because of dependences\n", REAL_CLASS_NAME(klass));
            remove_class_from_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass));
            free_class(klass);
            return NULL;
        }
    }

    /// call class field initializar ///
    if(!run_class_fields_initializar(klass)) {
        output_exception_message();
        /// show exception ///
        remove_class_from_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass));
        free_class(klass);
        return NULL;
    }

    return klass;
}

// result: (NULL) --> file not found (sCLClass*) loaded class
static sCLClass* load_class_from_classpath_on_compile_time(char* real_class_name, BOOL resolve_dependences)
{
    sCLClass* result;
    char class_file[PATH_MAX];

    if(!search_for_class_file_from_class_name(class_file, PATH_MAX, real_class_name)) {
        return NULL;
    }

    result = load_class_on_compile_time(class_file, resolve_dependences);

    if(result) {
        if(!entry_alias_of_class_to_alias_table(result)) {
            return NULL;
        }
    }

    return result;
}

// result: (NULL) --> file not found (sCLClass*) loaded class
sCLClass* load_class_with_namespace_on_compile_time(char* namespace, char* class_name, BOOL resolve_dependences)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    sCLClass* result;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name);

    return load_class_from_classpath_on_compile_time(real_class_name, resolve_dependences);
}

// result: (TRUE) success (FALSE) faield
BOOL load_fundamental_classes_on_compile_time()
{
    sCLClass* system;
    sCLClass* thread;
    sCLClass* clover;

    clover = load_class_from_classpath_on_compile_time("Clover", TRUE);
    gVoidType.mClass = load_class_from_classpath_on_compile_time("void", TRUE);
    gIntType.mClass = load_class_from_classpath_on_compile_time("int", TRUE);
    gFloatType.mClass = load_class_from_classpath_on_compile_time("float", TRUE);
    gBoolType.mClass = load_class_from_classpath_on_compile_time("bool", TRUE);

    gObjectType.mClass = load_class_from_classpath_on_compile_time("Object", TRUE);

    gArrayType.mClass = load_class_from_classpath_on_compile_time("Array", TRUE);
    gStringType.mClass = load_class_from_classpath_on_compile_time("String", TRUE);
    gHashType.mClass = load_class_from_classpath_on_compile_time("Hash", TRUE);

    gBlockType.mClass = load_class_from_classpath_on_compile_time("Block", TRUE);
    gExceptionType.mClass = load_class_from_classpath_on_compile_time("Exception", TRUE);

    gExNullPointerType.mClass = load_class_from_classpath_on_compile_time("NullPointerException", TRUE);
    gExRangeType.mClass = load_class_from_classpath_on_compile_time("RangeException", TRUE);

    gClassNameType.mClass = load_class_from_classpath_on_compile_time("ClassName", TRUE);

    system = load_class_from_classpath_on_compile_time("System", TRUE);
    //thread = load_class_from_classpath_on_compile_time("Thread", TRUE);

    if(gVoidType.mClass == NULL || gIntType.mClass == NULL || gFloatType.mClass == NULL || gBoolType.mClass == NULL || gObjectType.mClass == NULL || gStringType.mClass == NULL || gBlockType.mClass == NULL || gArrayType.mClass == NULL || gHashType.mClass == NULL || gExceptionType.mClass == NULL || gClassNameType.mClass == NULL || system == NULL || clover == NULL)
    {
        return FALSE;
    }

    return TRUE;
}
