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
    CLBLOCK(obj)->mConstant = MCALLOC(1, sizeof(sConst));
    CLBLOCK(obj)->mCode = MCALLOC(1, sizeof(sByteCode));

    return obj;
}

CLObject create_block(char* constant, int const_len, int* code, int code_len, int max_stack, int num_locals, int num_params, MVALUE* parent_var, int num_parent_vars, CLObject result_type, CLObject* params, BOOL breakable, BOOL caller_existance)
{
    CLObject obj;
    int j;

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
    CLBLOCK(obj)->mNumParentVar = num_parent_vars;

    CLBLOCK(obj)->mBreakable = breakable;
    CLBLOCK(obj)->mCallerExistance = caller_existance;

    CLBLOCK(obj)->mResultType = result_type;
    for(j=0; j<num_params; j++) {
        CLBLOCK(obj)->mParams[j] = params[j];
    }

    return obj;
}

static void free_block_object(CLObject self)
{
    MFREE(CLBLOCK(self)->mConstant->mConst);
    MFREE(CLBLOCK(self)->mCode->mCode);
    MFREE(CLBLOCK(self)->mConstant);
    MFREE(CLBLOCK(self)->mCode);
}

static void mark_block_object(CLObject object, unsigned char* mark_flg)
{
    int i;

    CLObject object2 = CLBLOCK(object)->mResultType;

    mark_object(object2, mark_flg);
    
    for(i=0; i<CLBLOCK(object)->mNumParams; i++) {
        CLObject object3 = CLBLOCK(object)->mParams[i];

        mark_object(object3, mark_flg);
    }
}

void initialize_hidden_class_method_of_block(sCLClass* klass)
{
    klass->mFreeFun = free_block_object;
    klass->mShowFun = NULL;
    klass->mMarkFun = mark_block_object;
    klass->mCreateFun = NULL;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gBlockClass = klass;
        gBlockTypeObject = create_type_object(gBlockClass);
    }
}

BOOL Block_resultType(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLClass* klass2;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Block", info)) {
        return FALSE;
    }

    (*stack_ptr)->mObjectValue.mValue = CLBLOCK(self)->mResultType;
    (*stack_ptr)++;

    return TRUE;
}

BOOL Block_parametors(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    CLObject self;
    sCLClass* klass2;
    CLObject array;
    int i;

    self = lvar->mObjectValue.mValue;

    if(!check_type_with_class_name(self, "Block", info)) {
        return FALSE;
    }

    array = create_array_object_with_element_class_name("Type", NULL, 0, info);

    push_object(array, info);

    for(i=0; i<CLBLOCK(self)->mNumParams; i++) {
        CLObject element;

        element = CLBLOCK(self)->mParams[i];

        add_to_array(array, element, info);
    }

    pop_object(info);

    (*stack_ptr)->mObjectValue.mValue = array;
    (*stack_ptr)++;

    return TRUE;
}
