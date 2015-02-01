#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLBlock);

    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_block_object()
{
    CLObject obj;
    unsigned int size;
    CLObject type_object;

    size = object_size();

    type_object = gBlockTypeObject;

    obj = alloc_heap_mem(size, type_object);
    CLBLOCK(obj)->mConstant = CALLOC(1, sizeof(sConst));
    CLBLOCK(obj)->mCode = CALLOC(1, sizeof(sByteCode));

    return obj;
}

CLObject create_block(char* constant, int const_len, int* code, int code_len, int max_stack, int num_locals, int num_params, MVALUE* parent_var, int num_parent_vars, int max_block_var_num)
{
    CLObject obj;

    obj = alloc_block_object();

    /// constant ///
    sConst_init(CLBLOCK(obj)->mConstant);
    append_buf_to_constant_pool(CLBLOCK(obj)->mConstant, constant, const_len, FALSE);

    /// code ///
    sByteCode_init(CLBLOCK(obj)->mCode);
    append_buf_to_bytecodes(CLBLOCK(obj)->mCode, code, code_len, FALSE);

    CLBLOCK(obj)->mMaxStack = max_stack;
    CLBLOCK(obj)->mNumLocals = num_locals;
    CLBLOCK(obj)->mNumParams = num_params;
    CLBLOCK(obj)->mParentLocalVar = parent_var;
    CLBLOCK(obj)->mNumParentVar = num_parent_vars - max_block_var_num;

    return obj;
}

static void free_block_object(CLObject self)
{
    FREE(CLBLOCK(self)->mConstant->mConst);
    FREE(CLBLOCK(self)->mCode->mCode);
    FREE(CLBLOCK(self)->mConstant);
    FREE(CLBLOCK(self)->mCode);
}

void initialize_hidden_class_method_of_block(sCLClass* klass)
{
    klass->mFreeFun = free_block_object;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = NULL;
}
