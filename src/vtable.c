#include "clover.h"
#include "common.h"

void init_var_table(sVarTable* table)
{
    memset(table, 0, sizeof(sVarTable));
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
            if(type_ == NULL) {
                memset(&p->mType, 0, sizeof(p->mType));
            }
            else {
                p->mType = *type_;
            }

            p->mBlockLevel = table->mBlockLevel;

            return TRUE;
        }
        else {
            if(strcmp(p->mName, name) == 0) {
                if(p->mBlockLevel < table->mBlockLevel) {
                    xstrncpy(p->mName, name, CL_METHOD_NAME_MAX);
                    p->mIndex = table->mVarNum++;
                    if(type_ == NULL) {
                        memset(&p->mType, 0, sizeof(p->mType));
                    }
                    else {
                        p->mType = *type_;
                    }

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

    printf("--- vtable list ---\n");

    for(i=0; i<CL_LOCAL_VARIABLE_MAX; i++) {
        sVar* var = &var_table->mLocalVariables[i];
        if(var->mName[0] != 0) {
            printf("var %s %d %s\n", var->mName, var->mIndex, REAL_CLASS_NAME(var->mType.mClass));
        }
    }
    printf("-----\n");
}

void inc_var_table(sVarTable* var_table, int value)
{
    var_table->mVarNum += value;
}

void init_block_vtable(sVarTable* new_table, sVarTable* lv_table)
{
    ASSERT(lv_table != NULL);
    memset(new_table, 0, sizeof(sVarTable));
    copy_var_table(new_table, lv_table);
    
    new_table->mBlockLevel = lv_table->mBlockLevel + 1;
}

void entry_block_vtable_to_node_block(sVarTable* new_table, sVarTable* lv_table, unsigned int block)
{
    int lv_num_of_this_block;

    /// get local var number of this block ///
    lv_num_of_this_block = new_table->mVarNum - lv_table->mVarNum + new_table->mBlockVarNum;
    if(lv_num_of_this_block > lv_table->mBlockVarNum) {
       lv_table->mBlockVarNum = lv_num_of_this_block;
    }

    /// copy table ///
    copy_var_table(&gNodeBlocks[block].mLVTable, new_table);

    gNodeBlocks[block].mLVTable.mBlockLevel = new_table->mBlockLevel;
}

void entry_method_block_vtable_to_node_block(sVarTable* new_table, sVarTable* lv_table, unsigned int block, int local_var_num_before)
{
    /// copy table ///
    copy_var_table(&gNodeBlocks[block].mLVTable, new_table);

    gNodeBlocks[block].mNumLocals = new_table->mVarNum - local_var_num_before;
    gNodeBlocks[block].mLVTable.mBlockLevel = new_table->mBlockLevel;
}
