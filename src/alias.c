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

static struct sAliasItem gAliases[CL_ALIAS_MAX];

struct sEntriedClassItem {
    char mName[CL_CLASS_NAME_MAX];
};

static struct sEntriedClassItem gEntriedClass[CLASS_HASH_SIZE];

static BOOL set_flag_to_entried_class(char* real_class_name)
{
    int hash;
    struct sEntriedClassItem* item;

    hash = get_hash(real_class_name) % CLASS_HASH_SIZE;

    item = gEntriedClass + hash;

    while(1) { 
        if(item->mName[0] == 0) {
            xstrncpy(item->mName, real_class_name, CL_CLASS_NAME_MAX);
            break;
        }
        else {
            item ++ ;

            if(item == gEntriedClass + CLASS_HASH_SIZE) {
                item = gEntriedClass;
            }
            else if(item == gEntriedClass + hash) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL is_this_class_entried(char* real_class_name)
{
    int hash;
    struct sEntriedClassItem* item;

    hash = get_hash(real_class_name) % CLASS_HASH_SIZE;

    item = gEntriedClass + hash;
    
    while(1) {
        if(item->mName[0] == 0) {
            return FALSE;
        }
        else {
            if(strcmp(item->mName, real_class_name) == 0) {
                break;
            }
            else {
                item ++ ;

                if(item == gEntriedClass + CLASS_HASH_SIZE) {
                    item = gEntriedClass;
                }
                else if(item == gEntriedClass + hash) {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

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

            if(method->mFlags & CL_CLASS_METHOD) {
                method->mFlags |= CL_ALIAS_METHOD;

                if(!add_alias_to_alias_table(klass, method)) {
                    return FALSE;
                }
                found = TRUE;
            }
            else {
                return FALSE;
            }
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

        if(method->mFlags & CL_CLASS_METHOD) {
            method->mFlags |= CL_ALIAS_METHOD;

            if(!add_alias_to_alias_table(klass, method)) {
                return FALSE;
            }
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

        if(method->mFlags & CL_CLASS_METHOD 
            && method->mFlags & CL_ALIAS_METHOD)
        {
            if(!add_alias_to_alias_table(klass, method)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL entry_alias_of_methods_and_fields_of_class(sCLClass* klass);
static BOOL entry_alias_of_super_class(sCLClass* klass);
static BOOL entry_alias_of_dependece(sCLClass* klass);

// result: (TRUE) success (FALSE) error
BOOL entry_alias_of_class(sCLClass* klass)
{
    int i;

    if(is_this_class_entried(REAL_CLASS_NAME(klass))) {
        return TRUE;
    }

    if(!set_flag_to_entried_class(REAL_CLASS_NAME(klass))) {
        return FALSE;
    }

    if(!entry_alias_of_class_to_alias_table(klass)) {
        return FALSE;
    }

    if(!entry_alias_of_methods_and_fields_of_class(klass)) {
        return FALSE;
    }

    if(!entry_alias_of_super_class(klass)) {
        return FALSE;
    }

    if(!entry_alias_of_dependece(klass)) {
        return FALSE;
    }
    
    return TRUE;
}

static BOOL entry_alias_of_methods_and_fields_of_class(sCLClass* klass)
{
    int i;

    for(i=0; i<klass->mNumFields; i++) {
        sCLField* field = klass->mFields + i;
        sCLClass* klass2;
        
        klass2 = cl_get_class(CONS_str(&klass->mConstPool, field->mType.mClassNameOffset));
        
        ASSERT(klass2 != NULL);

        if(!entry_alias_of_class(klass2)) {
            return FALSE;
        }
    }

    for(i=0; i<klass->mNumMethods; i++) {
        sCLMethod* method = klass->mMethods + i;
        sCLClass* klass2;
        int j;

        /// result type ///
        klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mResultType.mClassNameOffset));
        
        ASSERT(klass2 != NULL);

        if(!entry_alias_of_class(klass2)) {
            return FALSE;
        }

        /// param types ///
        for(j=0; j<method->mNumParams; j++) {
            klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mParamTypes[j].mClassNameOffset)); 

            ASSERT(klass2 != NULL);

            if(!entry_alias_of_class(klass2)) {
                return FALSE;
            }
        }

        /// block type ///
        if(method->mNumBlockType == 1) {
            klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mBlockType.mResultType.mClassNameOffset));

            ASSERT(klass2 != NULL);

            if(!entry_alias_of_class(klass2)) {
                return FALSE;
            }

            for(j=0; j<method->mBlockType.mNumParams; j++) {
                klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mBlockType.mParamTypes[j].mClassNameOffset));

                ASSERT(klass2 != NULL);

                if(!entry_alias_of_class(klass2)) {
                    return FALSE;
                }
            }
        }

        /// exception ///
        for(j=0; j<method->mNumException; j++) {
            klass2 = cl_get_class(CONS_str(&klass->mConstPool, method->mExceptionClassNameOffset[j]));

            ASSERT(klass2 != NULL);

            if(!entry_alias_of_class(klass2)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

static BOOL entry_alias_of_super_class(sCLClass* klass)
{
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        sCLClass* super_class;
        
        super_class = cl_get_class(CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset));

        ASSERT(super_class != NULL);

        if(!entry_alias_of_class(super_class)) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL entry_alias_of_dependece(sCLClass* klass)
{
    int i;
    for(i=0; i<klass->mNumDependences; i++) {
        sCLClass* depend_class;
        
        depend_class = cl_get_class(CONS_str(&klass->mConstPool, klass->mDependencesOffset[i]));

        ASSERT(depend_class != NULL);

        if(!entry_alias_of_class(depend_class)) {
            return FALSE;
        }
    }

    return TRUE;
}
