#ifndef COMMON_H
#define COMMON_H

#include <stdarg.h>

extern sByteCode* gCode;
//#define VM_DEBUG

//////////////////////////////////////////////////
// heap.c
//////////////////////////////////////////////////
void heap_init(int heap_size, int size_hadles);
void heap_final();
void* object_to_ptr(CLObject obj);
CLObject alloc_object(unsigned int size);
void cl_gc();
CLObject create_object(sCLClass* klass);
CLObject alloc_heap_mem(unsigned int size, sCLClass* klass);
void mark_object(CLObject obj, unsigned char* mark_flg);

#ifdef VM_DEBUG
void show_heap();
#endif

//////////////////////////////////////////////////
// klass.c
//////////////////////////////////////////////////
extern sCLNodeType gIntType;      // foudamental classes
extern sCLNodeType gFloatType;
extern sCLNodeType gVoidType;

extern sCLNodeType gNullType;
extern sCLNodeType gObjectType;
extern sCLNodeType gStringType;
extern sCLNodeType gHashType;
extern sCLNodeType gArrayType;

extern sCLNodeType gBoolType;

extern sCLNodeType gBlockType;

extern sCLNodeType gAnonymousType[CL_GENERICS_CLASS_PARAM_MAX];;

extern sCLClass* gCloverClass;
sCLClass* alloc_class(char* namespace, char* class_name, BOOL private_, BOOL open_, char** generics_types, int generics_types_num);
BOOL add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL native_, char* name, sCLNodeType* result_type, sCLNodeType* class_params, unsigned int num_params, BOOL constructor);

BOOL check_super_class_offsets(sCLClass* klass);
void class_init(BOOL load_foundamental_class);
void class_final();
unsigned int get_hash(char* name);
void show_class(sCLClass* klass);
void show_class_list();
BOOL save_class(sCLClass* klass);
void save_all_modified_class();
sCLClass* load_class_from_classpath(char* file_name);
ALLOC unsigned char* native_load_class(char* file_name);
void show_constants(sConst* constant);
void alloc_bytecode_of_method(sCLMethod* method);
void create_real_class_name(char* result, int result_size, char* namespace, char* class_name);
void increase_class_version(sCLClass* klass);
void show_all_method(sCLClass* klass, char* method_name);
void show_method(sCLClass* klass, sCLMethod* method);
void show_type(sCLClass* klass, sCLType* type);
void show_node_type(sCLNodeType* type);
BOOL is_valid_class_pointer(void* class_pointer);

// result: (null) --> file not found (char* pointer) --> success
ALLOC char* load_file(char* file_name);

// result (TRUE) --> success (FALSE) --> overflow methods number or method parametor number
BOOL add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL native_, char* name, sCLNodeType* result_type, sCLNodeType* class_params, unsigned int num_params, BOOL constructor);

void add_block_type_to_method(sCLClass* klass, sCLMethod* method, char* block_name, sCLNodeType* bt_result_type, sCLNodeType bt_class_params[], unsigned int bt_num_params);

// result (TRUE) --> success (FLASE) --> overflow super class number 
BOOL add_super_class(sCLClass* klass, sCLClass* super_klass);

// result (TRUE) --> success (FALSE) --> overflow number fields
BOOL add_field(sCLClass* klass, BOOL static_, BOOL private_, char* name, sCLNodeType* type_);

// result: (NULL) not found (sCLClass*) found
sCLClass* get_super(sCLClass* klass);

// result: (NULL) --> not found (non NULL) --> field
sCLField* get_field(sCLClass* klass, char* field_name);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
sCLMethod* get_method_on_super_classes(sCLClass* klass, char* method_name, sCLClass** founded_class);

// result: (NULL) --> not found (non NULL) --> field
// also return the class in which is found the the field 
sCLField* get_field_including_super_classes(sCLClass* klass, char* field_name, sCLClass** founded_class);

// result: (-1) --> not found (non -1) --> field index
int get_field_index(sCLClass* klass, char* field_name);

// result: (-1) --> not found (non -1) --> field index
// also return the class which is found the index to found_class parametor
int get_field_index_including_super_classes(sCLClass* klass, char* field_name);

// result is seted on this parametors(sCLNodeType* result)
// if the field is not found, result->mClass is setted on NULL
void get_field_type(sCLClass* klass, sCLField* field, sCLNodeType* result, sCLNodeType* type_);

// return field number
int get_field_num_including_super_classes(sCLClass* klass);

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method(sCLClass* klass, char* method_name);

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method_from_index(sCLClass* klass, int method_index);

// result: (-1) --> not found (non -1) --> method index
int get_method_index(sCLClass* klass, sCLMethod* method);

// result: (-1) --> not found (non -1) --> method index
int get_method_index_from_the_parametor_point(sCLClass* klass, char* method_name, int method_index, BOOL search_for_class_method);

// result should be found
void get_param_type_of_method(sCLClass* klass, sCLMethod* method, int param_num, sCLNodeType* result);

// result: (FALSE) can't solve a generics type (TRUE) success
// if type_ is NULL, don't solve generics type
BOOL get_result_type_of_method(sCLClass* klass, sCLMethod* method, sCLNodeType* result, sCLNodeType* type_);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class
// if type_ is NULL, don't solve generics type
sCLMethod* get_virtual_method_with_params(sCLClass* klass, char* method_name, sCLNodeType* class_params, unsigned int num_params, sCLClass** founded_class, BOOL search_for_class_method, sCLNodeType* type_);

// result: (NULL) --> not found (non NULL) --> method
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params(sCLClass* klass, char* method_name, sCLNodeType* class_params, unsigned int num_params, BOOL search_for_class_method, sCLNodeType* type_, int start_point);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** foud_class) was setted on the method owner class.
sCLMethod* get_method_on_super_classes(sCLClass* klass, char* method_name, sCLClass** found_class);

// result: (TRUE) found (FALSE) not found
BOOL search_for_super_class(sCLClass* klass, sCLClass* searched_class);

// return method parametor number
int get_method_num_params(sCLMethod* method);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params_on_super_classes(sCLClass* klass, char* method_name, sCLNodeType* class_params, unsigned int num_params, sCLClass** founded_class, BOOL search_for_class_method, sCLNodeType* type_);

// result: (FALSE) fail (TRUE) success
// result of parametors is setted on the result
// if type_ is NULL, it is not solved with generics type.
BOOL cl_type_to_node_type(sCLNodeType* result, sCLType* cl_type, sCLNodeType* type_, sCLClass* klass);

//////////////////////////////////////////////////
// parser.c
//////////////////////////////////////////////////
extern sVarTable gGVTable;       // global variable table

void sBuf_init(sBuf* self);
void sBuf_append_char(sBuf* self, char c);
void sBuf_append(sBuf* self, void* str, size_t size);

void sConst_init(sConst* self);
void sConst_free(sConst* self);
void sConst_append_str(sConst* constant, char* str);
void sConst_append(sConst* self, void* data, unsigned int size);
void sConst_append_wstr(sConst* constant, char* str);
void sConst_append_int(sConst* constant, int n);
void sConst_append_float(sConst* constant, float n);

void sByteCode_init(sByteCode* self);
void sByteCode_free(sByteCode* self);
void sByteCode_append(sByteCode* self, void* code, unsigned int size);

void parser_init(BOOL load_foundamental_class);
void parser_final();

BOOL parse_word(char* buf, int buf_size, char** p, char* sname, int* sline, int* err_num, BOOL print_out_err_msg);
void skip_spaces_and_lf(char** p, int* sline);
void skip_spaces(char** p);
void parser_err_msg(char* msg, char* sname, int sline);
void parser_err_msg_format(char* sname, int sline, char* msg, ...);
BOOL expect_next_character(char* characters, int* err_num, char** p, char* sname, int* sline);
// characters is null-terminated
void expect_next_character_with_one_forward(char* characters, int* err_num, char** p, char* sname, int* sline);

BOOL node_expression(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass);
BOOL node_expression_without_comma(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass);

BOOL parse_generics_types_name(char** p, char* sname, int* sline, int* err_num, char* generics_types_num, sCLClass** generics_types, char* current_namespace, sCLClass* klass);

BOOL parse_namespace_and_class(sCLClass** result, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass);
    // result: (FALSE) there is an error (TRUE) success
    // result class is setted on first parametor
BOOL parse_namespace_and_class_and_generics_type(sCLNodeType* type, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass) ;
    // result: (FALSE) there is an error (TRUE) success
    // result type is setted on first parametor
int get_generics_type_num(sCLClass* klass, char* type_name);

BOOL delete_comment(sBuf* source, sBuf* source2);

// result: (true) success (false) overflow the table
BOOL add_variable_to_table(sVarTable* table, char* name, sCLNodeType* type_);

// result: (true) success (false) overflow the table
BOOL append_var_table(sVarTable* var_table, sVarTable* var_table2);

// result: (null) not found (sVar*) found
sVar* get_variable_from_table(sVarTable* table, char* name);

void copy_var_table(sVarTable* src, sVarTable* dest);
void inc_var_table(sVarTable* var_table, int value);

void compile_error(char* msg, ...);
BOOL parse_params(sCLNodeType* class_params, unsigned int* num_params, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sVarTable* lv_table, char close_character);

//////////////////////////////////////////////////
// node.c
//////////////////////////////////////////////////
#define NODE_TYPE_OPERAND 1
#define NODE_TYPE_VALUE 2
#define NODE_TYPE_STRING_VALUE 3
#define NODE_TYPE_VARIABLE_NAME 4
#define NODE_TYPE_ARRAY_VALUE 5
#define NODE_TYPE_DEFINE_VARIABLE_NAME 7
#define NODE_TYPE_FIELD 8
#define NODE_TYPE_CLASS_FIELD 9
#define NODE_TYPE_STORE_VARIABLE_NAME 10
#define NODE_TYPE_DEFINE_AND_STORE_VARIABLE_NAME 11
#define NODE_TYPE_STORE_FIELD 12
#define NODE_TYPE_STORE_CLASS_FIELD 13
#define NODE_TYPE_CLASS_METHOD_CALL 14
#define NODE_TYPE_PARAM 15
#define NODE_TYPE_RETURN 16
#define NODE_TYPE_NEW 17
#define NODE_TYPE_METHOD_CALL 18
#define NODE_TYPE_SUPER 19
#define NODE_TYPE_INHERIT 20
#define NODE_TYPE_NULL 21
#define NODE_TYPE_TRUE 22
#define NODE_TYPE_FALSE 23
#define NODE_TYPE_FVALUE 24
#define NODE_TYPE_IF 25
#define NODE_TYPE_WHILE 26
#define NODE_TYPE_BREAK 27
#define NODE_TYPE_DO 28
#define NODE_TYPE_FOR 29
#define NODE_TYPE_CONTINUE 30
#define NODE_TYPE_BLOCK_CALL 31
#define NODE_TYPE_MAX 32

enum eOperand { 
    kOpAdd, kOpSub, kOpMult, kOpDiv, kOpMod, kOpPlusPlus2, kOpMinusMinus2, kOpIndexing, kOpPlusPlus, kOpMinusMinus, kOpComplement, kOpLogicalDenial, kOpLeftShift, kOpRightShift, kOpComparisonGreater, kOpComparisonLesser, kOpComparisonGreaterEqual, kOpComparisonLesserEqual, kOpComparisonEqual, kOpComparisonNotEqual, kOpAnd, kOpXor, kOpOr, kOpOrOr, kOpAndAnd, kOpConditional, kOpComma
};

enum eNodeSubstitutionType {
    kNSNone, kNSPlus, kNSMinus, kNSMult, kNSDiv, kNSMod, kNSLShift, kNSRShift, kNSAnd, kNSXor, kNSOr
};

struct sNodeTreeStruct {
    unsigned char mNodeType;
    sCLNodeType mType;

    union {
        struct {
            char* mVarName;
            enum eNodeSubstitutionType mNodeSubstitutionType;
        } sVarName;

        struct {
            char* mVarName;
            unsigned int mBlock;
        } sMethod;

        struct {
            unsigned int mIfBlock;                           // node block id
            unsigned int mElseIfConditional[CL_ELSE_IF_MAX]; // node id
            unsigned int mElseIfBlock[CL_ELSE_IF_MAX];       // node block id
            unsigned int mElseIfBlockNum;
            unsigned int mElseBlock;                         // node block id
        } sIfBlock;

        unsigned int mWhileBlock;                            // node block id

        unsigned int mDoBlock;                               // node block id

        unsigned int mForBlock;                              // node block id

        enum eOperand mOperand;
        int mValue;
        char* mStringValue;
        float mFValue;
    } uValue;

    unsigned int mLeft;     // node index
    unsigned int mRight;
    unsigned int mMiddle;
};

typedef struct sNodeTreeStruct sNodeTree;

struct sNodeStruct {
    unsigned int mNode;
    char* mSName;
    int mSLine;
};

typedef struct sNodeStruct sNode;

struct sNodeBlockStruct {
    sNode* mNodes;
    unsigned int mSizeNodes;
    unsigned int mLenNodes;

    sCLNodeType mBlockType;
    sCLNodeType mClassParams[CL_METHOD_PARAM_MAX];
    unsigned int mNumParams;
    sVarTable mLVTable;
    int mMaxStack;
    int mNumLocals;
};

typedef struct sNodeBlockStruct sNodeBlock;

extern sNodeBlock* gNodeBlocks; // All node blocks at here. Index is node block number. alloc_node_block() returns a node block number

extern sNodeTree* gNodes; // All nodes at here. Index is node number. sNodeTree_create* functions return a node number.

BOOL compile_method(sCLMethod* method, sCLClass* klass, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, BOOL constructor, char* current_namespace);
// left_type is stored type. right_type is value type.
BOOL substition_posibility(sCLNodeType* left_type, sCLNodeType* right_type);
BOOL type_identity(sCLNodeType* type1, sCLNodeType* type2);

// Below functions return a node number. It is an index of gNodes.
unsigned int sNodeTree_create_operand(enum eOperand operand, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_value(int value, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_fvalue(float fvalue, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_string_value(MANAGED char* value, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_array(unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_var(char* var_name, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_call_block(char* var_name, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_define_var(char* var_name, sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_return(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_break(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_continue();
unsigned int sNodeTree_create_null();
unsigned int sNodeTree_create_true();
unsigned int sNodeTree_create_false();
unsigned int sNodeTree_create_class_method_call(char* var_name, sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle, unsigned int block);
unsigned int sNodeTree_create_class_field(char* var_name, sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_param(unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_new_expression(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_fields(char* name, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_method_call(char* var_name, unsigned int left, unsigned int right, unsigned int middle, unsigned int block);
unsigned int sNodeTree_create_super(unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_inherit(unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_if(unsigned int if_conditional, unsigned int if_block, unsigned int else_block, unsigned int* else_if_conditional, unsigned int* else_if_block, int else_if_num, sCLNodeType* type_);
unsigned int sNodeTree_create_while(unsigned int conditional, unsigned int block, sCLNodeType* type_);
unsigned int sNodeTree_create_for(unsigned int conditional, unsigned int conditional2, unsigned int conditional3, unsigned int block, sCLNodeType* type_);
unsigned int sNodeTree_create_do(unsigned int conditional, unsigned int block, sCLNodeType* type_);

BOOL parse_block(unsigned int* block_id, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLNodeType block_type, BOOL enable_param);
BOOL parse_statment(char** p, char* sname, int* sline, sByteCode* code, sConst* constant, int* err_num, int* max_stack, char* current_namespace, sVarTable* var_table);

//////////////////////////////////////////////////
// vm.c
//////////////////////////////////////////////////
extern MVALUE* gCLStack;
extern int gCLStackSize;
extern MVALUE* gCLStackPtr;

#define INVOKE_METHOD_KIND_CLASS 0
#define INVOKE_METHOD_KIND_OBJECT 1

void push_object(CLObject object);  // get under shelter the object
CLObject pop_object();              // remove the object from stack

#ifdef VM_DEBUG
void vm_debug(char* msg, ...);
#endif

void vm_error(char* msg, ...);

//////////////////////////////////////////////////
// xfunc.c
//////////////////////////////////////////////////
char* xstrncpy(char* des, char* src, int size);
char* xstrncat(char* des, char* str, int size);

//////////////////////////////////////////////////
// clover.c
//////////////////////////////////////////////////
BOOL Clover_show_classes(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Clover_gc(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Clover_compile(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Clover_load(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Clover_print(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Clover_output_to_s(MVALUE** stack_ptr, MVALUE* lvar);

//////////////////////////////////////////////////
// int.c
//////////////////////////////////////////////////
BOOL int_to_s(MVALUE** stack_ptr, MVALUE* lvar);
void initialize_hidden_class_method_of_immediate_value(sCLClass* klass);

//////////////////////////////////////////////////
// float.c
//////////////////////////////////////////////////
BOOL float_floor(MVALUE** stack_ptr, MVALUE* lvar);
BOOL float_to_s(MVALUE** stack_ptr, MVALUE* lvar);

//////////////////////////////////////////////////
// bool.c
//////////////////////////////////////////////////
BOOL bool_to_s(MVALUE** stack_ptr, MVALUE* lvar);

//////////////////////////////////////////////////
// object.c
//////////////////////////////////////////////////
CLObject create_object(sCLClass* klass);
void initialize_hidden_class_method_of_user_object(sCLClass* klass);

BOOL Object_show_class(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Object_class_name(MVALUE** stack_ptr, MVALUE* lvar);

//////////////////////////////////////////////////
// string.c
//////////////////////////////////////////////////
CLObject create_string_object(sCLClass* klass, wchar_t* str, unsigned int len);
void initialize_hidden_class_method_of_string(sCLClass* klass);

BOOL String_String(MVALUE** stack_ptr, MVALUE* lvar);
BOOL String_length(MVALUE** stack_ptr, MVALUE* lvar);
BOOL String_append(MVALUE** stack_ptr, MVALUE* lvar);

//////////////////////////////////////////////////
// array.c
//////////////////////////////////////////////////
CLObject create_array_object(sCLClass* klass, MVALUE elements[], int num_elements);
void initialize_hidden_class_method_of_array(sCLClass* klass);

BOOL Array_Array(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Array_items(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Array_length(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Array_add(MVALUE** stack_ptr, MVALUE* lvar);

//////////////////////////////////////////////////
// hash.c
//////////////////////////////////////////////////
CLObject create_hash_object(sCLClass* klass, MVALUE keys[], MVALUE elements[], unsigned int elements_len);
void initialize_hidden_class_method_of_hash(sCLClass* klass);

//////////////////////////////////////////////////
// hash.c
//////////////////////////////////////////////////
void initialize_hidden_class_method_of_block(sCLClass* klass);
CLObject create_block(sCLClass* klass, unsigned char** pc, int max_stack, int num_locals, int num_params, MVALUE* var, int num_vars);

//////////////////////////////////////////////////
// interface.c
//////////////////////////////////////////////////
extern sBuf* gCLPrintBuffer;   // this is hook of all clover output. see cl_print().

#endif
