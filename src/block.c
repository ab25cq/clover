#include "clover.h"
#include "common.h"

static unsigned int object_size()
{
    unsigned int size;

    size = sizeof(sCLBlock);

    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_block_object(sCLClass* klass)
{
    CLObject obj;
    unsigned int size;

    size = object_size();

    obj = alloc_heap_mem(size, klass);
    CLBLOCK(obj)->mConstant = CALLOC(1, sizeof(sConst));
    CLBLOCK(obj)->mCode = CALLOC(1, sizeof(sByteCode));

    return obj;
}

static void get_block_from_bytecodes(unsigned char** pc, sConst* constant, sByteCode* code)
{
    int ivalue1;

    /// constant ///
    ivalue1 = *(unsigned int*)(*pc);
    (*pc) += sizeof(int);

    sConst_init(constant);
    sConst_append(constant, *pc, ivalue1);
    (*pc) += ivalue1;

    /// code ///
    ivalue1 = *(unsigned int*)(*pc);
    (*pc) += sizeof(int);

    sByteCode_init(code);
    sByteCode_append(code, *pc, ivalue1);
    (*pc) += ivalue1;
}

CLObject create_block(sCLClass* klass, unsigned char** pc, int max_stack, int num_locals, int num_params, MVALUE* var, int num_vars)
{
    CLObject obj;
    
    obj = alloc_block_object(klass);

    get_block_from_bytecodes(pc, CLBLOCK(obj)->mConstant, CLBLOCK(obj)->mCode);

    CLBLOCK(obj)->mMaxStack = max_stack;
    CLBLOCK(obj)->mNumLocals = num_locals;
    CLBLOCK(obj)->mNumParams = num_params;
    CLBLOCK(obj)->mLocalVar = var;
    CLBLOCK(obj)->mNumVars = num_vars;

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
}
