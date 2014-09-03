#include "clover.h"
#include "common.h"

sVarTable* gHeadVTable;

void init_vtable()
{
    gHeadVTable = NULL;
}

void final_vtable()
{
    sVarTable* it;

    it = gHeadVTable;
    while(it) {
        sVarTable* next;

        next = it->mNext;
        FREE(it);

        it = next;
    }
}

sVarTable* init_var_table()
{
    sVarTable* result;

    result = CALLOC(1, sizeof(sVarTable));

    result->mNext = gHeadVTable;
    gHeadVTable = result;

    return result;
}

//////////////////////////////////////////////////
// local variable and global variable table
//////////////////////////////////////////////////
// result: (true) success (false) overflow the table or a variable which has the same name exists
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
            p->mType = type_;

            p->mBlockLevel = table->mBlockLevel;

            return TRUE;
        }
        else {
            if(strcmp(p->mName, name) == 0) {
                if(p->mBlockLevel < table->mBlockLevel) {
                    xstrncpy(p->mName, name, CL_METHOD_NAME_MAX);
                    p->mIndex = table->mVarNum++;
                    p->mType = type_;
                    p->mBlockLevel = table->mBlockLevel;

                    return TRUE;
                }
                else {
                    return FALSE;
                }
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
}

// result: (true) success (false) overflow the table or not found the variable
BOOL erase_variable_to_table(sVarTable* table, char* name)
{
    int hash_value;
    sVar* p;

    hash_value = get_hash(name) % CL_LOCAL_VARIABLE_MAX;
    p = table->mLocalVariables + hash_value;

    while(1) {
        if(p->mName[0] == 0) {
            return FALSE;
        }
        else {
            if(strcmp(p->mName, name) == 0) {
                memset(p, 0, sizeof(sVar));
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
}

// result: (null) not found (sVar*) found
static sVar* get_variable_from_this_table_only(sVarTable* table, char* name)
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

// result: (null) not found (sVar*) found
sVar* get_variable_from_table(sVarTable* table, char* name)
{
    sVarTable* it;
    sVar* var;

    it = table;

    while(it) {
        var = get_variable_from_this_table_only(it, name);

        if(var) return var;

        it = it->mParent;
    }

    return NULL;
}

static int get_sum_of_parent_var_num(sVarTable* it)
{
    int sum_of_parent_var_num;

    sum_of_parent_var_num = 0;

    it = it->mParent;

    while(it) {
        sum_of_parent_var_num += it->mVarNum;
        it = it->mParent;
    }

    return sum_of_parent_var_num;
}

sVar* get_variable_from_this_table_only_with_index(sVarTable* table, int index)
{
    sVar* p;

    for(p = table->mLocalVariables; 
        p < table->mLocalVariables + CL_LOCAL_VARIABLE_MAX;
        p++) 
    {
        if(p->mName[0] != 0 && p->mIndex == index) {
            return p;
        }
    }

    return NULL;
}

// result: (null) not found (sVar*) found
sVar* get_variable_from_table_by_var_index(sVarTable* table, int index)
{
    sVarTable* it;
    sVar* var;

    it = table;

    while(it) {
        int sum_of_parent_var_num;

        sum_of_parent_var_num = get_sum_of_parent_var_num(it);

        var = get_variable_from_this_table_only_with_index(it, index-sum_of_parent_var_num);

        if(var) return var;

        it = it->mParent;
    }

    return NULL;
}

// result: (-1) not found (non -1) found
int get_variable_index_from_table(sVarTable* table, char* name)
{
    sVarTable* it;
    int sum_of_parent_var_num;
    sVar* var;

    it = table;

    while(it) {
        var = get_variable_from_this_table_only(it, name);

        if(var) {
            int sum_of_parent_var_num;

            it = it->mParent;
            sum_of_parent_var_num = 0;

            while(it) {
                sum_of_parent_var_num += it->mVarNum;
                it = it->mParent;
            }

            return var->mIndex + sum_of_parent_var_num;
        }
        it = it->mParent;
    }

    return -1;
}

BOOL does_this_var_exist(sVarTable* table, char* name)
{
    return get_variable_from_table(table, name) != NULL;
}

void show_var_table(sVarTable* var_table)
{
    int i;

    printf("--- vtable list ---\n");

    for(i=0; i<CL_LOCAL_VARIABLE_MAX; i++) {
        sVar* var = &var_table->mLocalVariables[i];
        if(var->mName[0] != 0) {
            if(var->mType->mClass) {
                printf("var %s index %d class %s\n", var->mName, var->mIndex, REAL_CLASS_NAME(var->mType->mClass));
            }
            else {
                printf("var %s index %d class NULL\n", var->mName, var->mIndex);
            }
        }
    }
    printf("-----\n");
}

void show_var_table_with_parent(sVarTable* var_table)
{
    sVarTable* it;

    it = var_table;

    while(it) {
        show_var_table(it);
        it = it->mParent;
    }
}

sVarTable* init_block_vtable(sVarTable* lv_table)
{
    sVarTable* new_table;

    ASSERT(lv_table != NULL);

    new_table = init_var_table();
    new_table->mBlockLevel = lv_table->mBlockLevel + 1;
    new_table->mParent = lv_table;

    return new_table;
}

void entry_vtable_to_node_block(unsigned int block, sVarTable* new_table, sVarTable* lv_table)
{
    int lv_num_of_this_block;

    /// get local var number of this block ///
    lv_num_of_this_block = new_table->mVarNum + new_table->mMaxBlockVarNum;
    if(lv_num_of_this_block > lv_table->mMaxBlockVarNum) {
       lv_table->mMaxBlockVarNum = lv_num_of_this_block;
    }

    gNodeBlocks[block].mLVTable = new_table;
}

