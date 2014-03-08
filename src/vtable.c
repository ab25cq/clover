#include "clover.h"
#include "common.h"

//////////////////////////////////////////////////
// local variable and global variable table
//////////////////////////////////////////////////
// result: (true) success (false) overflow the table
BOOL add_variable_to_table(sVarTable* table, char* name, sCLNodeType* type_)
{
    int hash_value;
    sVar* p;

    hash_value = get_hash(name) % CL_LOCAL_VARIABLE_MAX;
    p = table->mLocalVariables + hash_value;

    while(1) {
        if(p->mName[0] == 0) {
            xstrncpy(p->mName, name, CL_METHOD_NAME_MAX);
            p->mIndex = table->mVarNum++;
            if(type_ == NULL) {
                memset(&p->mType, 0, sizeof(p->mType));
            }
            else {
                p->mType = *type_;
            }

            return TRUE;
        }
        else {
            p++;

            if(p == table->mLocalVariables + CL_LOCAL_VARIABLE_MAX) {
                p = table->mLocalVariables;
            }
            else if(p == table->mLocalVariables + hash_value) {
                return FALSE;
            }
        }
    }
}

// result: (null) not found (sVar*) found
sVar* get_variable_from_table(sVarTable* table, char* name)
{
    int hash_value;
    sVar* p;

    hash_value = get_hash(name) % CL_LOCAL_VARIABLE_MAX;

    p = table->mLocalVariables + hash_value;

    while(1) {
        if(p->mName[0] == 0) {
            return NULL;
        }
        else if(strcmp((char*)p->mName, name) == 0) {
            return p;
        }

        p++;

        if(p == table->mLocalVariables + CL_LOCAL_VARIABLE_MAX) {
            p = table->mLocalVariables;
        }
        else if(p == table->mLocalVariables + hash_value) {
            return NULL;
        }
    }
}

sVar* get_variable_from_table_by_var_index(sVarTable* table, int index)
{
    int i;

    for(i=0; i<CL_LOCAL_VARIABLE_MAX; i++) {
        sVar* var = &table->mLocalVariables[i];

        if(var->mName[0] != 0) {
            if(var->mIndex == index) {
                return var;
            }
        }
    }

    return NULL;
}

void copy_var_table(sVarTable* dest, sVarTable* src)
{
    int i;

    for(i=0; i<CL_LOCAL_VARIABLE_MAX; i++) {
        memcpy(&dest->mLocalVariables[i], &src->mLocalVariables[i], sizeof(sVar));
    }

    dest->mVarNum = src->mVarNum;
}

// result: (true) success (false) overflow the table
BOOL append_var_table(sVarTable* var_table, sVarTable* var_table2)
{
    int i;
    int j;
    int count;

    count = 0;
    for(j=0; j<CL_LOCAL_VARIABLE_MAX; j++) {
        for(i=0; i<CL_LOCAL_VARIABLE_MAX; i++) {
            sVar* var = &var_table2->mLocalVariables[i];

            if(var->mName[0] != 0) {
                if(var->mIndex == j) {
                    if(!add_variable_to_table(var_table, var->mName, &var->mType)) {
                        return FALSE;
                    }
                    count++;
                    if(count == var_table2->mVarNum) {
                        return TRUE;
                    }
                    else {
                        break;
                    }
                }
            }
        }
    }

    return TRUE;
}

void show_var_table(sVarTable* var_table)
{
    int i;
    for(i=0; i<CL_LOCAL_VARIABLE_MAX; i++) {
        sVar* var = &var_table->mLocalVariables[i];
        if(var->mName[0] != 0) {
            printf("var %s %d\n", var->mName, var->mIndex);
        }
    }
}

void inc_var_table(sVarTable* var_table, int value)
{
    var_table->mVarNum += value;
}

