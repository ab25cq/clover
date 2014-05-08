#ifndef COMMON_H
#define COMMON_H

#include <stdarg.h>

#define GETINT64(b) (long)((unsigned long)((b)[0])<<56 |  (unsigned long)((b)[1])<<48 | (unsigned long)((b)[2])<<40 | (unsigned long)((b)[3])<<32 | (unsigned long)((b)[4])<<24 | (unsingned long)((b)[5])<<16 | (unsigned long)((b)[6])<<8 | (unsigned long)((b)[7]) )
#define GETINT(b) (int)((unsigned int)((b)[0])<<24 | (unsingned int)((b)[1])<<16 | (unsigned int)((b)[2])<<8 | (unsigned int)((b)[3]))
#define GETSHORT(b) (short)(((b)[0]<<8)|(b)[1])

//#define VM_DEBUG

//////////////////////////////////////////////////
// heap.c
//////////////////////////////////////////////////
void heap_init(int heap_size, int size_hadles);
void heap_final();
void* object_to_ptr(CLObject obj);
CLObject alloc_object(int size);
void cl_gc();
CLObject alloc_heap_mem(int size, sCLClass* klass);
void mark_object(CLObject obj, unsigned char* mark_flg);
// result --> (0: not found) (non 0: found)
CLObject get_object_from_mvalue(MVALUE mvalue);
BOOL is_valid_object(CLObject obj);

#ifdef VM_DEBUG
void show_heap();
#endif

//////////////////////////////////////////////////
// klass.c
//////////////////////////////////////////////////
extern sCLClass* gClassHashList[CLASS_HASH_SIZE];

extern sCLNodeType gIntType;      // foudamental classes
extern sCLNodeType gFloatType;
extern sCLNodeType gVoidType;
extern sCLNodeType gBoolType;

extern sCLNodeType gNullType;
extern sCLNodeType gObjectType;
extern sCLNodeType gStringType;
extern sCLNodeType gHashType;
extern sCLNodeType gArrayType;

extern sCLNodeType gBlockType;
extern sCLNodeType gExceptionType;;

extern sCLNodeType gExNullPointerType;
extern sCLNodeType gExRangeType;

extern sCLNodeType gClassNameType;

extern sCLNodeType gAnonymousType[CL_GENERICS_CLASS_PARAM_MAX];;

extern sCLClass* gCloverClass;
sCLClass* alloc_class(char* namespace, char* class_name, BOOL private_, BOOL open_, char* generics_types[CL_CLASS_TYPE_VARIABLE_MAX], int generics_types_num);

void class_init();
void class_final();
sCLClass* read_class_from_file(int fd);
void remove_class_from_class_table(char* namespace, char* class_name);
void free_class(sCLClass* klass);

// result : (TRUE) found (FALSE) not found
// set class file path on class_file arguments
BOOL search_for_class_file_from_class_name(char* class_file, unsigned int class_file_size, char* real_class_name);

void add_class_to_class_table(char* namespace, char* class_name, sCLClass* klass);
void initialize_hidden_class_method_and_flags(char* namespace, char* class_name, sCLClass* klass);
BOOL read_from_file(int f, void* buf, size_t size);
unsigned int get_hash(char* name);
void show_class(sCLClass* klass);
void show_class_list();
BOOL save_class(sCLClass* klass);
void save_all_modified_class();
sCLClass* load_class_from_classpath(char* class_name, BOOL resolve_dependences);
sCLClass* load_class_with_namespace_from_classpath(char* namespace, char* class_name, BOOL resolve_dependences);
ALLOC char* native_load_class(char* file_name);
void alloc_bytecode_of_method(sCLMethod* method);
void create_real_class_name(char* result, int result_size, char* namespace, char* class_name);
void increase_class_version(sCLClass* klass);
void show_all_method(sCLClass* klass, char* method_name);
void show_method(sCLClass* klass, sCLMethod* method);
void show_type(sCLClass* klass, sCLType* type);
void show_node_type(sCLNodeType* type);
BOOL is_valid_class_pointer(void* class_pointer);
void mark_class_fields(unsigned char* mark_flg);
int get_static_fields_num(sCLClass* klass);
int get_static_fields_num_including_super_class(sCLClass* klass);
BOOL substition_posibility(sCLNodeType* left_type, sCLNodeType* right_type);
BOOL substition_posibility_of_class(sCLClass* left_type, sCLClass* right_type);
BOOL operand_posibility(sCLNodeType* left_type, sCLNodeType* right_type);
BOOL run_fields_initializar(CLObject object, sCLClass* klass);
BOOL run_class_fields_initializar(sCLClass* klass);
BOOL is_parent_immediate_value_class(sCLClass* klass);
BOOL is_parent_special_class(sCLClass* klass);

// result: (TRUE) success (FALSE) overflow exception number
BOOL add_exception_class(sCLClass* klass, sCLMethod* method, sCLClass* exception_class);
BOOL is_method_exception_class(sCLClass* klass, sCLMethod* method, sCLClass* exception_class);

// result: (null) --> file not found (char* pointer) --> success
ALLOC char* load_file(char* file_name, int* file_size);

// result TRUE: (success) FALSE: (can't get external program. $PATH is NULL or something)
BOOL import_external_program(sCLClass* klass);

// result (TRUE) --> success (FALSE) --> overflow methods number or method parametor number
// last parametor returns the method which is added
BOOL add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL native_, BOOL external, char* name, sCLNodeType* result_type, sCLNodeType* class_params, int num_params, BOOL constructor, sCLMethod** method);

void add_block_type_to_method(sCLClass* klass, sCLMethod* method, char* block_name, sCLNodeType* bt_result_type, sCLNodeType bt_class_params[], int bt_num_params);

// result (TRUE) --> success (FLASE) --> overflow super class number 
BOOL add_super_class(sCLClass* klass, sCLClass* super_klass);

void add_dependence_class(sCLClass* klass, sCLClass* dependence_class);

// result (TRUE) --> success (FALSE) --> overflow number fields
// initializar_code should be allocated and is managed inside this function after called
BOOL add_field(sCLClass* klass, BOOL static_, BOOL private_, char* name, sCLNodeType* type_);

// result (TRUE) --> success (FALSE) --> can't find a field which is indicated by an argument
BOOL add_field_initializar(sCLClass* klass, BOOL static_, char* name, MANAGED sByteCode initializar_code, sVarTable* lv_table, int max_stack);

// result: (NULL) not found (sCLClass*) found
sCLClass* get_super(sCLClass* klass);

// result: (NULL) --> not found (non NULL) --> field
sCLField* get_field(sCLClass* klass, char* field_name, BOOL class_field);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
sCLMethod* get_method_on_super_classes(sCLClass* klass, char* method_name, sCLClass** founded_class);

// result: (NULL) --> not found (non NULL) --> field
// also return the class in which is found the the field 
sCLField* get_field_including_super_classes(sCLClass* klass, char* field_name, sCLClass** founded_class, BOOL class_field);

// result: (-1) --> not found (non -1) --> field index
int get_field_index(sCLClass* klass, char* field_name, BOOL class_field);

// result: (-1) --> not found (non -1) --> field index
// also return the class which is found the index to found_class parametor
int get_field_index_including_super_classes(sCLClass* klass, char* field_name, BOOL class_field);

// result: (-1) --> not found (non -1) --> field index
int get_field_index_including_super_classes_without_class_field(sCLClass* klass, char* field_name);

// result is seted on this parametors(sCLNodeType* result)
// if the field is not found, result->mClass is setted on NULL
void get_field_type(sCLClass* klass, sCLField* field, sCLNodeType* result, sCLNodeType* type_);

// return field number
int get_field_num_including_super_classes(sCLClass* klass);

// return field number
int get_field_num_including_super_classes_without_class_field(sCLClass* klass);

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
sCLMethod* get_method_with_type_params(sCLClass* klass, char* method_name, sCLNodeType* class_params, int num_params, BOOL search_for_class_method, sCLNodeType* type_, int start_point, int block_num, int block_num_params, sCLNodeType* block_param_type, sCLNodeType* block_type);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** foud_class) was setted on the method owner class.
sCLMethod* get_method_on_super_classes(sCLClass* klass, char* method_name, sCLClass** found_class);

// result: (TRUE) found (FALSE) not found
BOOL search_for_super_class(sCLClass* klass, sCLClass* searched_class);

// return method parametor number
int get_method_num_params(sCLMethod* method);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params_on_super_classes(sCLClass* klass, char* method_name, sCLNodeType* class_params, int num_params, sCLClass** founded_class, BOOL search_for_class_method, sCLNodeType* type_, int block_num, int block_num_params, sCLNodeType* block_param_type, sCLNodeType* block_type);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class
// if type_ is NULL, don't solve generics type
sCLMethod* get_virtual_method_with_params(sCLClass* klass, char* method_name, sCLNodeType* class_params, int num_params, sCLClass** founded_class, BOOL search_for_class_method, sCLNodeType* type_, int block_num, int block_num_params, sCLNodeType* block_param_type, sCLNodeType* block_type);

// result: (FALSE) fail (TRUE) success
// result of parametors is setted on the result
// if type_ is NULL, it is not solved with generics type.
BOOL cl_type_to_node_type(sCLNodeType* result, sCLType* cl_type, sCLNodeType* type_, sCLClass* klass);

//////////////////////////////////////////////////
// parser.c
//////////////////////////////////////////////////
BOOL parse_word(char* buf, int buf_size, char** p, char* sname, int* sline, int* err_num, BOOL print_out_err_msg);
void skip_spaces_and_lf(char** p, int* sline);
void skip_spaces(char** p);
void parser_err_msg(char* msg, char* sname, int sline);
void parser_err_msg_format(char* sname, int sline, char* msg, ...);
BOOL expect_next_character(char* characters, int* err_num, char** p, char* sname, int* sline);
// characters is null-terminated
void expect_next_character_with_one_forward(char* characters, int* err_num, char** p, char* sname, int* sline);

BOOL node_expression(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table);
BOOL node_expression_without_comma(unsigned int* node, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table);

BOOL parse_generics_types_name(char** p, char* sname, int* sline, int* err_num, char* generics_types_num, sCLClass** generics_types, char* current_namespace, sCLClass* klass);

BOOL parse_namespace_and_class(sCLClass** result, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass);
    // result: (FALSE) there is an error (TRUE) success
    // result class is setted on first parametor
BOOL parse_namespace_and_class_and_generics_type(sCLNodeType* type, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass) ;
    // result: (FALSE) there is an error (TRUE) success
    // result type is setted on first parametor
int get_generics_type_num(sCLClass* klass, char* type_name);

BOOL delete_comment(sBuf* source, sBuf* source2);

void compile_error(char* msg, ...);
BOOL parse_params(sCLNodeType* class_params, int* num_params, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sVarTable* lv_table, char close_character, int sline_top);

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
#define NODE_TYPE_REVERT 32
#define NODE_TYPE_BLOCK 33
#define NODE_TYPE_CHARACTER_VALUE 34
#define NODE_TYPE_THROW 35
#define NODE_TYPE_TRY 36
#define NODE_TYPE_CLASS_NAME 37
#define NODE_TYPE_MAX 38

enum eOperand { 
    kOpAdd, kOpSub, kOpMult, kOpDiv, kOpMod, kOpPlusPlus2, kOpMinusMinus2, kOpIndexing, kOpPlusPlus, kOpMinusMinus, kOpComplement, kOpLogicalDenial, kOpLeftShift, kOpRightShift, kOpComparisonGreater, kOpComparisonLesser, kOpComparisonGreaterEqual, kOpComparisonLesserEqual, kOpComparisonEqual, kOpComparisonNotEqual, kOpAnd, kOpXor, kOpOr, kOpOrOr, kOpAndAnd, kOpConditional, kOpComma
};

enum eNodeSubstitutionType {
    kNSNone, kNSPlus, kNSMinus, kNSMult, kNSDiv, kNSMod, kNSLShift, kNSRShift, kNSAnd, kNSXor, kNSOr
};

struct sNodeTreeStruct {
    char mNodeType;
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

        struct {
            unsigned int mTryBlock;
            unsigned int mCatchBlock;
            unsigned int mFinallyBlock;
            sCLClass* mExceptionClass;
            char mExceptionVariableName[CL_VARIABLE_NAME_MAX+1];
        } sTryBlock;

        unsigned int mWhileBlock;                            // node block id

        unsigned int mDoBlock;                               // node block id

        unsigned int mForBlock;                              // node block id

        unsigned int mBlock;                                 // node block id

        enum eOperand mOperand;
        int mValue;
        char* mStringValue;
        float mFValue;
        char mCharacterValue;

        sCLClass* mClass;
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
    int mSizeNodes;
    int mLenNodes;

    sCLNodeType mBlockType;
    sCLNodeType mClassParams[CL_METHOD_PARAM_MAX];
    int mNumParams;

    sVarTable mLVTable;
    int mMaxStack;
    int mNumLocals;
};

typedef struct sNodeBlockStruct sNodeBlock;

extern sNodeBlock* gNodeBlocks; // All node blocks at here. Index is node block number. alloc_node_block() returns a node block number

extern sNodeTree* gNodes; // All nodes at here. Index is node number. sNodeTree_create* functions return a node number.

BOOL compile_method(sCLMethod* method, sCLNodeType* klass, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, BOOL constructor, char* current_namespace);
// left_type is stored type. right_type is value type.
BOOL type_identity(sCLNodeType* type1, sCLNodeType* type2);
BOOL compile_field_initializar(sByteCode* initializar, sCLNodeType* initializar_code_type, sCLNodeType* klass, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sVarTable* lv_table, int* max_stack);

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
unsigned int sNodeTree_create_new_expression(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle, unsigned int block);
unsigned int sNodeTree_create_fields(char* name, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_method_call(char* var_name, unsigned int left, unsigned int right, unsigned int middle, unsigned int block);
unsigned int sNodeTree_create_super(unsigned int left, unsigned int right, unsigned int middle, unsigned int block);
unsigned int sNodeTree_create_inherit(unsigned int left, unsigned int right, unsigned int middle, unsigned int block);
unsigned int sNodeTree_create_if(unsigned int if_conditional, unsigned int if_block, unsigned int else_block, unsigned int* else_if_conditional, unsigned int* else_if_block, int else_if_num, sCLNodeType* type_);
unsigned int sNodeTree_create_try(unsigned int try_block, unsigned int catch_block, unsigned int finally_block, sCLClass* exception_class, char* exception_variable_name);
unsigned int sNodeTree_create_while(unsigned int conditional, unsigned int block, sCLNodeType* type_);
unsigned int sNodeTree_create_for(unsigned int conditional, unsigned int conditional2, unsigned int conditional3, unsigned int block, sCLNodeType* type_);
unsigned int sNodeTree_create_do(unsigned int conditional, unsigned int block, sCLNodeType* type_);
unsigned int sNodeTree_create_block(sCLNodeType* type_, unsigned int block);
unsigned int sNodeTree_create_revert(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_character_value(char c);
unsigned int sNodeTree_create_throw(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_class_name(sCLNodeType* type);

BOOL parse_statment(char** p, char* sname, int* sline, sByteCode* code, sConst* constant, int* err_num, int* max_stack, char* current_namespace, sVarTable* var_table);
BOOL parse_statments(char** p, char* sname, int* sline, sByteCode* code, sConst* constant, int* err_num, int* max_stack, char* current_namespace, sVarTable* var_table);
BOOL skip_field_initializar(char** p, char* sname, int* sline, char* current_namespace, sCLNodeType* klass, sVarTable* lv_table);
BOOL parse_block(unsigned int* block_id, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType block_type, sCLMethod* method, sVarTable* lv_table);
BOOL parse_block_object(unsigned int* block_id, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType block_type,  BOOL staic_method, sCLMethod* method, sVarTable* lv_table, int sline_top, BOOL inhibit_param);

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
void entry_exception_object(sCLClass* klass, char* msg, ...);
BOOL field_initializar(MVALUE* result, sByteCode* code, sConst* constant, int lv_num, int max_stack);
void sigttou_block(int block);
void output_exception_message();

////////////////////////////////////////////////////////////
// alias.c
////////////////////////////////////////////////////////////

// result: (TRUE) success (FALSE) faield
BOOL load_fundamental_classes_on_compile_time();

// result: (NULL) not found (sCLMethod*) found
sCLMethod* get_method_from_alias_table(char* name, sCLClass** klass);

// result: (TRUE) success (FALSE) not found the method or overflow alias table
BOOL set_alias_flag_to_method(sCLClass* klass, char* method_name);

// result: (TRUE) success (FALSE) overflow alias table
BOOL set_alias_flag_to_all_methods(sCLClass* klass);

// result: (sCLClass*) success (NULL) not found file
sCLClass* load_class_with_namespace_on_compile_time(char* namespace, char* class_name, BOOL resolve_dependences);

//////////////////////////////////////////////////
// xfunc.c
//////////////////////////////////////////////////
char* xstrncpy(char* des, char* src, int size);
char* xstrncat(char* des, char* str, int size);
int xgetmaxx();
int xgetmaxy();

//////////////////////////////////////////////////
// clover.c
//////////////////////////////////////////////////
BOOL Clover_show_classes(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Clover_gc(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Clover_compile(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Clover_print(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Clover_output_to_s(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Clover_sleep(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Clover_exit(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Clover_getenv(MVALUE** stack_ptr, MVALUE* lvar);

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
// user_object.c
//////////////////////////////////////////////////
// result (TRUE): success (FALSE): threw exception
BOOL create_user_object(sCLClass* klass, CLObject* obj);
void initialize_hidden_class_method_of_user_object(sCLClass* klass);

BOOL Object_show_class(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Object_class_name(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Object_instanceof(MVALUE** stack_ptr, MVALUE* lvar);
BOOL Object_is_child(MVALUE** stack_ptr, MVALUE* lvar);

//////////////////////////////////////////////////
// string.c
//////////////////////////////////////////////////
CLObject create_string_object(sCLClass* klass, wchar_t* str, int len);
void initialize_hidden_class_method_of_string(sCLClass* klass);

BOOL String_String(MVALUE** stack_ptr, MVALUE* lvar);
BOOL String_length(MVALUE** stack_ptr, MVALUE* lvar);
BOOL String_append(MVALUE** stack_ptr, MVALUE* lvar);
BOOL String_char(MVALUE** stack_ptr, MVALUE* lvar);

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
CLObject create_hash_object(sCLClass* klass, MVALUE keys[], MVALUE elements[], int elements_len);
void initialize_hidden_class_method_of_hash(sCLClass* klass);

//////////////////////////////////////////////////
// hash.c
//////////////////////////////////////////////////
void initialize_hidden_class_method_of_block(sCLClass* klass);
CLObject create_block(sCLClass* klass, char* constant, int const_len, int* code, int code_len, int max_stack, int num_locals, int num_params, MVALUE* parent_var, int num_parent_vars);

//////////////////////////////////////////////////
// interface.c
//////////////////////////////////////////////////
extern sBuf* gCLPrintBuffer;   // this is hook of all clover output. see cl_print().

//////////////////////////////////////////////////
// buffer.c
//////////////////////////////////////////////////
void show_buffer(char* buf, int len);

void sBuf_init(sBuf* self);
void sBuf_append(sBuf* self, void* str, size_t size);
void sBuf_append_char(sBuf* self, char c);
void sBuf_show(sBuf* self);

void sByteCode_init(sByteCode* self);
void sByteCode_free(sByteCode* self);
void append_opecode_to_bytecodes(sByteCode* self, int value);
void append_int_value_to_bytecodes(sByteCode* self, int value);
void append_str_to_bytecodes(sByteCode* code, sConst* constant, char* str);
void append_constant_pool_to_bytecodes(sByteCode* code, sConst* constant, sConst* constatnt2);
void append_code_to_bytecodes(sByteCode* code, sConst* constant, sByteCode* code2);
void append_buf_to_bytecodes(sByteCode* self, int* code, int len);

void sConst_init(sConst* self);
void sConst_free(sConst* self);
int append_int_value_to_constant_pool(sConst* constant, int n);
int append_float_value_to_constant_pool(sConst* constant, float n);
int append_str_to_constant_pool(sConst* constant, char* str);
int append_wstr_to_constant_pool(sConst* constant, char* str);
void append_buf_to_constant_pool(sConst* self, char* src, int src_len);

//////////////////////////////////////////////////
// vtable.c
//////////////////////////////////////////////////
void init_var_table(sVarTable* table);

// result: (true) success (false) overflow the table or a variable which has the same name exists
BOOL add_variable_to_table(sVarTable* table, char* name, sCLNodeType* type_);

// result: (true) success (false) overflow the table
BOOL append_var_table(sVarTable* var_table, sVarTable* var_table2);

// result: (null) not found (sVar*) found
sVar* get_variable_from_table(sVarTable* table, char* name);

void copy_var_table(sVarTable* src, sVarTable* dest);

void inc_var_table(sVarTable* var_table, int value);

sVar* get_variable_from_table_by_var_index(sVarTable* table, int index);

void show_var_table(sVarTable* var_table);

void init_block_vtable(sVarTable* new_table, sVarTable* lv_table);

void entry_block_vtable_to_node_block(sVarTable* new_table, sVarTable* lv_table, unsigned int block);

void entry_method_block_vtable_to_node_block(sVarTable* new_table, sVarTable* lv_table, unsigned int block, int local_var_num_before);

////////////////////////////////////////////////////////////
// system.c
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// class_name.c
////////////////////////////////////////////////////////////
CLObject create_class_name_object(sCLNodeType type_);
void initialize_hidden_class_method_of_class_name(sCLClass* klass);

BOOL ClassName_to_s(MVALUE** stack_ptr, MVALUE* lvar);

////////////////////////////////////////////////////////////
// compiler.c
////////////////////////////////////////////////////////////

int num_loaded_class();
char* get_loaded_class(int index);
void add_loaded_class_to_table(char* namespace, char* class_name);

#endif

