#ifndef COMMON_H
#define COMMON_H

#include <stdarg.h>

#define GETINT64(b) (long)((unsigned long)((b)[0])<<56 |  (unsigned long)((b)[1])<<48 | (unsigned long)((b)[2])<<40 | (unsigned long)((b)[3])<<32 | (unsigned long)((b)[4])<<24 | (unsigned long)((b)[5])<<16 | (unsigned long)((b)[6])<<8 | (unsigned long)((b)[7]) )
#define GETINT(b) (int)((unsigned int)((b)[0])<<24 | (unsigned int)((b)[1])<<16 | (unsigned int)((b)[2])<<8 | (unsigned int)((b)[3]))
#define GETSHORT(b) (short)(((b)[0]<<8)|(b)[1])

//////////////////////////////////////////////////
// heap.c
//////////////////////////////////////////////////
void heap_init(int heap_size, int size_hadles);
void heap_final();
void* object_to_ptr(CLObject obj);
CLObject alloc_object(int size);
void cl_gc();
CLObject alloc_heap_mem(int size, CLObject type_object);
void mark_object(CLObject obj, unsigned char* mark_flg);
// result --> (0: not found) (non 0: found)
CLObject get_object_from_mvalue(MVALUE mvalue);
BOOL is_valid_object(CLObject obj);

#ifdef VM_DEBUG
void show_heap(sVMInfo* info);
#endif

//////////////////////////////////////////////////
// klass.c
//////////////////////////////////////////////////
sCLMethod* get_clone_method(sCLClass* klass);

BOOL search_for_implemeted_interface(sCLClass* klass, sCLClass* interface);

BOOL read_generics_param_types(int fd, sCLGenericsParamTypes* generics_param_types);

extern sCLClass* gClassHashList[CLASS_HASH_SIZE];

extern sCLNodeType* gIntType;      // foudamental classes
extern sCLNodeType* gByteType;
extern sCLNodeType* gFloatType;
extern sCLNodeType* gVoidType;
extern sCLNodeType* gBoolType;
extern sCLNodeType* gNullType;

extern sCLNodeType* gObjectType;
extern sCLNodeType* gStringType;
extern sCLNodeType* gBytesType;
extern sCLNodeType* gArrayType;
extern sCLNodeType* gRangeType;
extern sCLNodeType* gHashType;
extern sCLNodeType* gBlockType;
extern sCLNodeType* gExceptionType;
extern sCLNodeType* gThreadType;
extern sCLNodeType* gTypeType;

extern sCLNodeType* gGParamTypes[CL_GENERICS_CLASS_PARAM_MAX];
extern sCLNodeType* gAnonymousType;

extern sCLClass* gVoidClass;
extern sCLClass* gIntClass;
extern sCLClass* gNullClass;
extern sCLClass* gByteClass;
extern sCLClass* gIntClass;
extern sCLClass* gByteClass;
extern sCLClass* gFloatClass;
extern sCLClass* gBoolClass;
extern sCLClass* gObjectClass;
extern sCLClass* gArrayClass;
extern sCLClass* gBytesClass;
extern sCLClass* gHashClass;
extern sCLClass* gRangeClass;
extern sCLClass* gBlockClass;
extern sCLClass* gTypeClass;
extern sCLClass* gStringClass;
extern sCLClass* gThreadClass;
extern sCLClass* gExceptionClass;
extern sCLClass* gExNullPointerClass;
extern sCLClass* gExRangeClass;
extern sCLClass* gExConvertingStringCodeClass;
extern sCLClass* gExClassNotFoundClass;
extern sCLClass* gExIOClass;
extern sCLClass* gExCantSolveGenericsTypeClass;
extern sCLClass* gExTypeError;
extern sCLClass* gExMethodMissingClass;
extern sCLClass* gExOverflowClass;
extern sCLClass* gExOverflowStackSizeClass;
extern sCLClass* gExDivisionByZeroClass;
extern sCLClass* gGParamClass[CL_GENERICS_CLASS_PARAM_MAX];
extern sCLClass* gAnonymousClass;

extern CLObject gTypeObject;
extern CLObject gIntTypeObject;
extern CLObject gStringTypeObject;
extern CLObject gArrayTypeObject;
extern CLObject gRangeTypeObject;
extern CLObject gNullTypeObject;
extern CLObject gFloatTypeObject;
extern CLObject gBoolTypeObject;
extern CLObject gByteTypeObject;
extern CLObject gBytesTypeObject;
extern CLObject gBlockTypeObject;
extern CLObject gExceptionTypeObject;

extern sCLClass* gCloverClass;

void class_init();
void class_final();

unsigned int get_hash(char* name);
void show_class_list();
sCLClass* load_class_from_classpath(char* real_class_name, BOOL solve_dependences);
sCLClass* load_class_with_namespace_from_classpath(char* namespace, char* class_name, BOOL solve_dependences);
sCLClass* load_class_with_namespace_on_compile_time(char* namespace, char* class_name, BOOL solve_dependences);
ALLOC char* native_load_class(char* file_name);
void alloc_bytecode_of_method(sCLMethod* method);
void create_real_class_name(char* result, int result_size, char* namespace, char* class_name);
BOOL is_valid_class_pointer(void* class_pointer);
void mark_class_fields(unsigned char* mark_flg);
int get_static_fields_num(sCLClass* klass);
int get_static_fields_num(sCLClass* klass);

void create_real_method_name(char* real_method_name, int real_method_name_size, char* method_name, int num_params);

// result: (null) --> file not found (char* pointer) --> success
ALLOC char* load_file(char* file_name, int* file_size);

// result: (NULL) not found (sCLClass*) found
sCLClass* get_super(sCLClass* klass);

// result: (NULL) --> not found (non NULL) --> field
sCLField* get_field(sCLClass* klass, char* field_name, BOOL class_field);

// return field number
int get_field_num_including_super_classes(sCLClass* klass);

// return field number
int get_field_num_including_super_classes_without_class_field(sCLClass* klass);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** foud_class) was setted on the method owner class.
sCLMethod* get_method_on_super_classes(sCLClass* klass, char* method_name, sCLClass** found_class);

// result: (TRUE) found (FALSE) not found
BOOL search_for_super_class(sCLClass* klass, sCLClass* searched_class);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class
sCLMethod* get_virtual_method_with_params(CLObject type_object, char* method_name, CLObject* class_params, int num_params, sCLClass** founded_class, BOOL search_for_class_method, int block_num, int block_num_params, CLObject* block_param_type, CLObject block_type,sVMInfo* info);

// result is setted on (sCLClass** result_class)
// result (TRUE) success on solving or not solving (FALSE) error on solving the generic type
BOOL run_fields_initializer(CLObject object, sCLClass* klass, CLObject vm_type);

// result should be not NULL
sCLClass* alloc_class(char* namespace, char* class_name, BOOL private_, BOOL abstract_, BOOL interface, BOOL dynamic_typing_, BOOL final_, BOOL struct_);

//////////////////////////////////////////////////
// klass_ctime.c
//////////////////////////////////////////////////
BOOL load_fundamental_classes_on_compile_time();
void initialize_hidden_class_method_and_flags(sCLClass* klass);

sCLClass* alloc_class_on_compile_time(char* namespace, char* class_name, BOOL private_, BOOL abstract_, BOOL interface, BOOL dynamic_typing_, BOOL final_, BOOL struct_);

// result (TRUE) --> success (FLASE) --> overflow super class number 
BOOL add_super_class(sCLClass* klass, sCLNodeType* super_klass);

// result (TRUE) --> success (FLASE) --> overflow implemented interface number
BOOL add_implemented_interface(sCLClass* klass, sCLNodeType* interface);

BOOL check_implemented_interface_between_super_classes(sCLNodeType* klass, sCLNodeType* interface, sCLMethod* method);

// result (TRUE) --> implemeted all methods (FALSE) --> there are not implemented methods
BOOL check_implemented_interface(sCLNodeType* klass, sCLNodeType* interface);

BOOL check_the_same_parametor_of_two_methods(sCLNodeType* klass1, sCLMethod* method1, sCLNodeType* klass2, sCLMethod* method2);

// result (TRUE) --> implemeted this interface (FALSE) --> not implemented this interface
BOOL check_implemented_interface2(sCLClass* klass, sCLNodeType* interface);

// result (TRUE) --> implemeted all methods (FALSE) --> there are not implemented methods
BOOL check_implemented_abstract_methods(sCLNodeType* klass);

void add_dependence_class(sCLClass* klass, sCLClass* dependence_class);

BOOL is_parent_special_class(sCLClass* klass);
BOOL is_generics_param_class(sCLClass* klass);
BOOL is_parent_class(sCLClass* klass1, sCLClass* klass2) ;

int get_generics_param_number(sCLClass* klass);
//int get_generics_param_number_of_method_scope(sCLClass* klass);
sCLGenericsParamTypes* get_generics_param_types(sCLClass* node_type, sCLClass* caller_class, sCLMethod* caller_method);

// result (TRUE) --> success (FALSE) --> overflow number fields
// initializer_code should be allocated and is managed inside this function after called
BOOL add_field(sCLClass* klass, BOOL static_, BOOL private_, BOOL protected, char* name, sCLNodeType* node_type);

// result (TRUE) --> success (FALSE) --> can't find a field which is indicated by an argument
BOOL add_field_initializer(sCLClass* klass, BOOL static_, char* name, MANAGED sByteCode initializer_code, sVarTable* lv_table, int max_stack);

// result is seted on this parametors(unsigned int* result)
// if the field is not found, result->mClass is setted on NULL
BOOL get_field_type(sCLClass* klass, sCLField* field, ALLOC sCLNodeType** result, sCLNodeType* type_);

// result: (-1) --> not found (non -1) --> field index
int get_field_index_including_super_classes_without_class_field(sCLClass* klass, char* field_name);

// result: (-1) --> not found (non -1) --> field index
int get_field_index(sCLClass* klass, char* field_name, BOOL class_field);

// result: (NULL) --> not found (non NULL) --> field
// also return the class in which is found the the field 
sCLField* get_field_including_super_classes(sCLNodeType* klass, char* field_name, sCLNodeType** founded_class, BOOL class_field, sCLNodeType** field_type, sCLNodeType* type_);

// result: (-1) --> not found (non -1) --> field index
// also return the class which is found the index to found_class parametor
int get_field_index_including_super_classes(sCLClass* klass, char* field_name, BOOL class_field);

// result: (NULL) --> not found (non NULL) --> method
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params(sCLNodeType* klass, char* method_name, sCLNodeType** class_params, int num_params, BOOL search_for_class_method, sCLNodeType* type_, int start_point, int block_num, int block_num_params, sCLNodeType** block_param_type, sCLNodeType* block_type, ALLOC sCLNodeType** result_type);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params_on_super_classes(sCLNodeType* klass, char* method_name, sCLNodeType** class_params, int num_params, sCLNodeType** founded_class, BOOL search_for_class_method, sCLNodeType* type_, int block_num, int block_num_params, sCLNodeType** block_param_type, sCLNodeType* block_type, ALLOC sCLNodeType** result_type);

// result: (NULL) --> not found (non NULL) --> method
// if type_ is NULL, don't solve generics type

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params_and_param_initializer(sCLNodeType* klass, char* method_name, sCLNodeType** class_params, int num_params, BOOL search_for_class_method, sCLNodeType* type_, int start_point, int block_num, int block_num_params, sCLNodeType** block_param_type, sCLNodeType* block_type, int* used_param_num_with_initializer, sCLNodeType** result_type);

// no solve generics type
ALLOC sCLNodeType* get_result_type_of_method(sCLNodeType* klass, sCLMethod* method);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params_and_param_initializer_on_super_classes(sCLNodeType* klass, char* method_name, sCLNodeType** class_params, int num_params, sCLNodeType** founded_class, BOOL search_for_class_method, sCLNodeType* type_, int block_num, int block_num_params, sCLNodeType** block_param_type, sCLNodeType* block_type, int* used_param_num_with_initializer, sCLNodeType** result_type);

// result (TRUE) --> success (FALSE) --> overflow methods number
// last parametor returns the method which is added
BOOL create_method(sCLClass* klass, sCLMethod** method);

void add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL protected_, BOOL native_, BOOL synchronized_, BOOL virtual_, BOOL abstract_, BOOL generics_newable, char* name, sCLNodeType* result_type, BOOL constructor, sCLMethod* method);

// result (TRUE) --> success (FALSE) --> overflow parametor number
BOOL add_param_to_method(sCLClass* klass, sCLNodeType** class_params, MANAGED sByteCode* code_params, int* max_stack_params, int* lv_num_params, int num_params, sCLMethod* method, int block_num, char* block_name, sCLNodeType* bt_result_type, sCLNodeType** bt_class_params, int bt_num_params, char* name);

BOOL add_generics_param_type_to_method(sCLClass* klass, sCLMethod* method, char* name, sCLNodeType* extends_type, char num_implements_types, sCLNodeType* implements_types[CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX]);

// result: (TRUE) success (FALSE) overflow exception number
BOOL add_exception_class(sCLClass* klass, sCLMethod* method, sCLClass* exception_class);
BOOL is_method_exception_class(sCLClass* klass, sCLMethod* method, sCLClass* exception_class);

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method(sCLClass* klass, char* method_name);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
sCLMethod* get_method_on_super_classes(sCLClass* klass, char* method_name, sCLClass** founded_class);

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method_from_index(sCLClass* klass, int method_index);

// result: (-1) --> not found (non -1) --> method index
int get_method_index(sCLClass* klass, sCLMethod* method);

// result: (-1) --> not found (non -1) --> method index
int get_method_index_from_the_parametor_point(sCLClass* klass, char* method_name, int method_index, BOOL search_for_class_method);

// return method parametor number
int get_method_num_params(sCLMethod* method);

void save_all_modified_classes();

void increase_class_version(sCLClass* klass);

void show_class(sCLClass* klass);

void show_node_type(sCLNodeType* node_type);

void show_type(sCLClass* klass, sCLType* type);

void show_method(sCLClass* klass, sCLMethod* method);

void show_all_method(sCLClass* klass, char* method_name);

// result (TRUE): success (FALSE):overflow table
BOOL add_generics_param_type_name(sCLClass* klass, char* name);

// result (TRUE): success (FALSE):not found the param type name
BOOL add_generics_param_type(sCLClass* klass, char* name, sCLNodeType* extends_type, char num_implements_types, sCLNodeType* implements_types[CL_GENERICS_CLASS_PARAM_IMPLEMENTS_MAX]);


//////////////////////////////////////////////////
// parser.c
//////////////////////////////////////////////////
struct sParserInfoStruct {
    char** p;
    char* sname;
    int* sline;
    int* err_num;
    char* current_namespace;
    sCLNodeType* klass;
    sCLMethod* method;
};

typedef struct sParserInfoStruct sParserInfo;

BOOL parse_word(char* buf, int buf_size, char** p, char* sname, int* sline, int* err_num, BOOL print_out_err_msg);
void skip_spaces_and_lf(char** p, int* sline);
void skip_spaces(char** p);
void parser_err_msg(char* msg, char* sname, int sline);
void parser_err_msg_format(char* sname, int sline, char* msg, ...);
BOOL expect_next_character(char* characters, int* err_num, char** p, char* sname, int* sline);
// characters is null-terminated
void expect_next_character_with_one_forward(char* characters, int* err_num, char** p, char* sname, int* sline);

BOOL node_expression(unsigned int* node, sParserInfo* info, sVarTable* lv_table);
BOOL node_expression_without_comma(unsigned int* node, sParserInfo* info, sVarTable* lv_table);

BOOL parse_generics_types_name(char** p, char* sname, int* sline, int* err_num, char* generics_types_num, sCLNodeType** generics_types, char* current_namespace, sCLClass* klass, sCLMethod* method, BOOL skip);

BOOL parse_namespace_and_class(sCLClass** result, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLMethod* method, BOOL skip, BOOL* star);
    // result: (FALSE) there is an error (TRUE) success
    // result class is setted on first parametor
BOOL parse_namespace_and_class_and_generics_type(ALLOC sCLNodeType** type, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLMethod* method, BOOL skip);
    // result: (FALSE) there is an error (TRUE) success
    // result type is setted on first parametor
int get_generics_type_num(sCLClass* klass, char* type_name);

BOOL delete_comment(sBuf* source, sBuf* source2);

void compile_error(char* msg, ...);
BOOL parse_params(sCLNodeType** class_params, int* num_params, int size_params, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLMethod* method, sVarTable* lv_table, char close_character, int sline_top);
BOOL parse_params_with_initializer(sCLNodeType** class_params, sByteCode* code_params, int* max_stack_params, int* lv_num_params, int* num_params, int size_params, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, char close_character, int sline_top);

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
#define NODE_TYPE_BYTES_VALUE 38
#define NODE_TYPE_RANGE_VALUE 39
#define NODE_TYPE_MAX 40

enum eOperand { 
    kOpAdd, kOpSub, kOpMult, kOpDiv, kOpMod, kOpPlusPlus2, kOpMinusMinus2, kOpIndexing, kOpSubstitutionIndexing, kOpPlusPlus, kOpMinusMinus, kOpComplement, kOpLogicalDenial, kOpLeftShift, kOpRightShift, kOpComparisonGreater, kOpComparisonLesser, kOpComparisonGreaterEqual, kOpComparisonLesserEqual, kOpComparisonEqual, kOpComparisonNotEqual, kOpAnd, kOpXor, kOpOr, kOpOrOr, kOpAndAnd, kOpConditional, kOpComma
};

enum eNodeSubstitutionType {
    kNSNone, kNSPlus, kNSMinus, kNSMult, kNSDiv, kNSMod, kNSLShift, kNSRShift, kNSAnd, kNSXor, kNSOr
};

struct sNodeTreeStruct {
    char mNodeType;
    sCLNodeType* mType;

    union {
        struct {
            char* mVarName;
            enum eNodeSubstitutionType mNodeSubstitutionType;
            BOOL mQuote;
        } sVarName;

        struct {
            char* mVarName;
            unsigned int mBlock;
            unsigned int mBlockNode;
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
            unsigned int mCatchBlocks[CL_CATCH_BLOCK_NUMBER_MAX];
            int mCatchBlockNumber;
            unsigned int mFinallyBlock;
            sCLNodeType* mExceptionType[CL_CATCH_BLOCK_NUMBER_MAX];
            char mExceptionVariableName[CL_CATCH_BLOCK_NUMBER_MAX][CL_VARIABLE_NAME_MAX+1];
        } sTryBlock;

        struct {
            enum eOperand mOperand;
            BOOL mQuote;
        } sOperand;

        unsigned int mWhileBlock;                            // node block id

        unsigned int mDoBlock;                               // node block id

        unsigned int mForBlock;                              // node block id

        unsigned int mBlock;                                 // node block id

        int mValue;
        char* mStringValue;
        float mFValue;
        wchar_t mCharacterValue;

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

    sCLNodeType* mBlockType;

    sVarTable* mLVTable;

    sCLNodeType* mClassParams[CL_METHOD_PARAM_MAX];
    int mNumParams;

    int mMaxStack;
    int mNumLocals;
};

typedef struct sNodeBlockStruct sNodeBlock;

enum eBlockKind { kBKNone, kBKWhileDoForBlock, kBKMethodBlock, kBKTryBlock };

struct sCompileInfoStruct {
    sByteCode* code;
    sConst* constant;
    char* sname;
    int* sline;
    int* err_num;
    sVarTable* lv_table;
    int* stack_num;
    int* max_stack;

    sCLNodeType* caller_class;
    sCLMethod* caller_method;
    sCLNodeType* real_caller_class;
    sCLMethod* real_caller_method;

    BOOL* exist_return;
    BOOL* exist_break;

    struct {
        unsigned int* break_labels;
        int* break_labels_len;
        unsigned int* continue_labels;
        int* continue_labels_len;
    } sLoopInfo;

    struct {
        enum eBlockKind block_kind;
        BOOL in_try_block;
        sNodeBlock* method_block;
        sCLNodeType* while_type;
    } sBlockInfo;

    BOOL no_output_to_bytecodes;

    struct {
        sCLClass* class_of_calling_method;
        sCLMethod* calling_method;
        BOOL calling_block;
        BOOL calling_array_value;
    } sParamInfo;
};

typedef struct sCompileInfoStruct sCompileInfo;

extern sNodeBlock* gNodeBlocks; // All node blocks at here. Index is node block number. alloc_node_block() returns a node block number

extern sNodeTree* gNodes; // All nodes at here. Index is node number. sNodeTree_create* functions return a node number.

BOOL compile_method(sCLMethod* method, sCLNodeType* klass, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, BOOL constructor, char* current_namespace);
BOOL compile_field_initializer(sByteCode* initializer, ALLOC sCLNodeType** initializer_code_type, sCLNodeType* klass, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sVarTable* lv_table, int* max_stack);
BOOL compile_param_initializer(ALLOC sByteCode* initializer, sCLNodeType** initializer_code_type, int* max_stack, int* lv_var_num, sCLNodeType* klass, char** p, char* sname, int* sline, int* err_num, char* current_namespace);


BOOL compile_statments(char** p, char* sname, int* sline, sByteCode* code, sConst* constant, int* err_num, int* max_stack, char* current_namespace, sVarTable* var_table);
BOOL skip_field_initializer(char** p, char* sname, int* sline, char* current_namespace, sCLNodeType* klass, sVarTable* lv_table);
BOOL parse_block_object(unsigned int* block_id, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType* block_type, sCLMethod* method, sVarTable* lv_table, int sline_top, int num_params, sCLNodeType** class_params);

unsigned int alloc_node_block(sCLNodeType* block_type);
void append_node_to_node_block(unsigned int node_block_id, sNode* node);

BOOL compile_node(unsigned int node, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info);

//////////////////////////////////////////////////
// node_tree.c
//////////////////////////////////////////////////

void init_nodes();
void free_nodes();

// Below functions return a node number. It is an index of gNodes.
unsigned int sNodeTree_create_operand(enum eOperand operand, unsigned int left, unsigned int right, unsigned int middle, BOOL quote);
unsigned int sNodeTree_create_value(int value, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_fvalue(float fvalue, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_string_value(MANAGED char* value, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_bytes_value(MANAGED char* value, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_array(unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_var(char* var_name, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_null();
unsigned int sNodeTree_create_call_block(char* var_name, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_define_var(char* var_name, sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_return(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_break(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_continue();
unsigned int sNodeTree_create_true();
unsigned int sNodeTree_create_false();
unsigned int sNodeTree_create_class_method_call(char* var_name, sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle, unsigned int block_object, unsigned int block_node);
unsigned int sNodeTree_create_class_field(char* var_name, sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_param(unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_new_expression(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle, unsigned int block_object, unsigned int block_node);
unsigned int sNodeTree_create_fields(char* name, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_method_call(char* var_name, unsigned int left, unsigned int right, unsigned int middle, unsigned int block_object, unsigned int block_node);
unsigned int sNodeTree_create_super(unsigned int left, unsigned int right, unsigned int middle, unsigned int block_object, unsigned int block_node);
unsigned int sNodeTree_create_inherit(unsigned int left, unsigned int right, unsigned int middle, unsigned int block_object, unsigned int block_node);
unsigned int sNodeTree_create_if(unsigned int if_conditional, unsigned int if_block, unsigned int else_block, unsigned int* else_if_conditional, unsigned int* else_if_block, int else_if_num, sCLNodeType* type_);
unsigned int sNodeTree_create_while(unsigned int conditional, unsigned int block, sCLNodeType* type_);
unsigned int sNodeTree_create_try(unsigned int try_block, unsigned int* catch_blocks, int catch_block_number, unsigned int finally_block, sCLNodeType** exception_type, char exception_variable_name[CL_CATCH_BLOCK_NUMBER_MAX][CL_VARIABLE_NAME_MAX+1]);
unsigned int sNodeTree_create_for(unsigned int conditional, unsigned int conditional2, unsigned int conditional3, unsigned int block, sCLNodeType* type_);
unsigned int sNodeTree_create_do(unsigned int conditional, unsigned int block, sCLNodeType* type_);
unsigned int sNodeTree_create_block(sCLNodeType* type_, unsigned int block);
unsigned int sNodeTree_create_character_value(wchar_t c);
unsigned int sNodeTree_create_throw(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_class_name(sCLNodeType* type);
unsigned int sNodeTree_create_range(unsigned int head, unsigned int tail);

//////////////////////////////////////////////////
// compile.c
//////////////////////////////////////////////////
void correct_stack_pointer_n(int* stack_num, int n, char* sname, int* sline, sByteCode* code, int* err_num, BOOL no_output_to_bytecodes);
void correct_stack_pointer(int* stack_num, char* sname, int* sline, sByteCode* code, int* err_num, BOOL no_output_to_bytecodes);

BOOL compile_block(sNodeBlock* block, sCLNodeType** type_, sCompileInfo* info);
BOOL compile_loop_block(sNodeBlock* block, sCLNodeType** type_, sCompileInfo* info);
BOOL compile_block_object(sNodeBlock* block, sConst* constant, sByteCode* code, sCLNodeType** type_, sCompileInfo* info, sCLNodeType* caller_class, sCLMethod* caller_method, enum eBlockKind block_kind);
BOOL parse_block(unsigned int* block_id, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType* block_type, sCLMethod* method, sVarTable* lv_table);

//////////////////////////////////////////////////
// vm.c
//////////////////////////////////////////////////
#define INVOKE_METHOD_KIND_CLASS 0
#define INVOKE_METHOD_KIND_OBJECT 1

BOOL check_type(CLObject ovalue1, CLObject type_object, sVMInfo* info);
BOOL check_type_without_generics(CLObject ovalue1, CLObject type_object, sVMInfo* info);
BOOL cl_excute_block(CLObject block, BOOL result_existance, BOOL static_method_block, sVMInfo* info, CLObject vm_type);

BOOL cl_excute_method(sCLMethod* method, sCLClass* klass);

void push_object(CLObject object, sVMInfo* info);
// remove the object from stack
CLObject pop_object(sVMInfo* info);
void remove_object(sVMInfo* info, int number);
void push_vminfo(sVMInfo* info);
void show_object_value(sVMInfo* info, CLObject obj);

#ifdef VM_DEBUG

#define START_VMLOG(o) start_vm_log(o)
#define VMLOG(o, ...) vm_log(o, __VA_ARGS__)

#define SHOW_STACK(o, o2, o3) show_stack(o, o2, o3)
#define SHOW_STACK2(o) show_stack(o, NULL, NULL)
#define SHOW_HEAP(o) show_heap(o)

void vm_log(sVMInfo* info, char* msg, ...);
void start_vm_log(sVMInfo* info);
void show_stack(sVMInfo* info, MVALUE* top_of_stack, MVALUE* var);

#else

#define VMLOG(o, ...)
#define START_VMLOG(o)
#define SHOW_STACK(o, o2, o3)
#define SHOW_STACK2(o)
#define SHOW_HEAP(o) 

#endif

void vm_error(char* msg, ...);
void entry_exception_object(sVMInfo* info, sCLClass* klass, char* msg, ...);
BOOL field_initializer(MVALUE* result, sByteCode* code, sConst* constant, int lv_num, int max_stack, CLObject vm_type);
void sigttou_block(int block);

extern sVMInfo* gHeadVMInfo;

void atexit_fun();

////////////////////////////////////////////////////////////
// alias.c
////////////////////////////////////////////////////////////

// result: (TRUE) success (FALSE) error
BOOL entry_alias_of_class(sCLClass* klass);

// result: (NULL) not found (sCLMethod*) found
sCLMethod* get_method_from_alias_table(char* name, sCLClass** klass);

// result: (TRUE) success (FALSE) not found the method or overflow alias table
BOOL set_alias_flag_to_method(sCLClass* klass, char* method_name);

// result: (TRUE) success (FALSE) overflow alias table
BOOL set_alias_flag_to_all_methods(sCLClass* klass);

//////////////////////////////////////////////////
// xfunc.c
//////////////////////////////////////////////////
char* xstrncpy(char* des, char* src, int size);
char* xstrncat(char* des, char* str, int size);
int xgetmaxx();
int xgetmaxy();
void* xxrealloc(void* old_data, size_t old_data_size, size_t size);

//////////////////////////////////////////////////
// obj_clover.c
//////////////////////////////////////////////////
BOOL Clover_showClasses(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Clover_gc(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Clover_print(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Clover_outputToString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

//////////////////////////////////////////////////
// obj_null.c
//////////////////////////////////////////////////
CLObject create_null_object();
void initialize_hidden_class_method_of_immediate_null(sCLClass* klass);

//////////////////////////////////////////////////
// obj_int.c
//////////////////////////////////////////////////
CLObject create_int_object(int value);

BOOL int_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL int_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL int_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL int_toByte(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL int_toFloat(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
void initialize_hidden_class_method_of_int(sCLClass* klass);
BOOL int_upcase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL int_downcase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

//////////////////////////////////////////////////
// obj_byte.c
//////////////////////////////////////////////////
CLObject create_byte_object(unsigned char value);

BOOL byte_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL byte_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL byte_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL byte_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL byte_upcase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL byte_downcase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

//////////////////////////////////////////////////
// obj_void.c
//////////////////////////////////////////////////
void initialize_hidden_class_method_of_void(sCLClass* klass);

//////////////////////////////////////////////////
// obj_anonymous.c
//////////////////////////////////////////////////
void initialize_hidden_class_method_of_anonymous(sCLClass* klass);

//////////////////////////////////////////////////
// obj_float.c
//////////////////////////////////////////////////
CLObject create_float_object(float value);

BOOL float_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL float_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL float_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL float_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

//////////////////////////////////////////////////
// obj_bool.c
//////////////////////////////////////////////////
CLObject create_bool_object(BOOL value);

BOOL bool_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL bool_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

//////////////////////////////////////////////////
// obj_user_object.c
//////////////////////////////////////////////////
// result (TRUE): success (FALSE): threw exception
BOOL create_user_object(CLObject type_object, CLObject* obj, CLObject vm_type, sVMInfo* info);
void initialize_hidden_class_method_of_user_object(sCLClass* klass);

BOOL Object_type(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Object_setType(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Object_ID(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Object_isUninitialized(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

//////////////////////////////////////////////////
// obj_string.c
//////////////////////////////////////////////////
CLObject create_string_object(wchar_t* str, int len, CLObject type_object, sVMInfo* info);
CLObject create_string_object_by_multiply(CLObject string, int number, sVMInfo* info);
void initialize_hidden_class_method_of_string(sCLClass* klass);

BOOL String_String(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL String_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL String_char(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL String_replace(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL String_toBytes(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL String_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL String_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL String_cmp(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

//////////////////////////////////////////////////
// obj_bytes.c
//////////////////////////////////////////////////
CLObject create_bytes_object(unsigned char* str, int len, CLObject type_object, sVMInfo* info);
CLObject create_bytes_object_by_multiply(CLObject string, int number, sVMInfo* info);

void initialize_hidden_class_method_of_bytes(sCLClass* klass);

BOOL Bytes_Bytes(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Bytes_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Bytes_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Bytes_replace(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Bytes_char(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Bytes_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Bytes_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Bytes_cmp(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

//////////////////////////////////////////////////
// obj_array.c
//////////////////////////////////////////////////
CLObject create_array_object(CLObject type_object, MVALUE elements[], int num_elements, sVMInfo* info);
void initialize_hidden_class_method_of_array(sCLClass* klass);

BOOL Array_Array(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Array_items(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Array_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Array_add(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Array_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Array_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Array_setItem(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

//////////////////////////////////////////////////
// obj_hash.c
//////////////////////////////////////////////////
CLObject create_hash_object(CLObject type_object, MVALUE keys[], MVALUE elements[], int elements_len);
void initialize_hidden_class_method_of_hash(sCLClass* klass);

BOOL Hash_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Hash_getValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

//////////////////////////////////////////////////
// hash.c
//////////////////////////////////////////////////
void initialize_hidden_class_method_of_block(sCLClass* klass);
CLObject create_block(char* constant, int const_len, int* code, int code_len, int max_stack, int num_locals, int num_params, MVALUE* parent_var, int num_parent_vars, int max_block_var_num, CLObject result_type, CLObject* params);

//////////////////////////////////////////////////
// interface.c
//////////////////////////////////////////////////
extern sBuf* gCLPrintBuffer;   // this is hook of all clover output. see cl_print().

//////////////////////////////////////////////////
// buffer.c
//////////////////////////////////////////////////
void show_buffer(char* buf, int len);
void show_constants(sConst* constant);

void sBuf_init(sBuf* self);
void sBuf_append(sBuf* self, void* str, size_t size);
void sBuf_append_char(sBuf* self, char c);
void sBuf_show(sBuf* self);

void sByteCode_init(sByteCode* self);
void sByteCode_free(sByteCode* self);
void append_opecode_to_bytecodes(sByteCode* self, int value, BOOL no_output_to_bytecodes);
void append_int_value_to_bytecodes(sByteCode* self, int value, BOOL no_output_to_bytecodes);
void append_str_to_bytecodes(sByteCode* code, sConst* constant, char* str, BOOL no_output_to_bytecodes);
void append_constant_pool_to_bytecodes(sByteCode* code, sConst* constant, sConst* constant2, BOOL no_output_to_bytecodes);
void append_code_to_bytecodes(sByteCode* code, sConst* constant, sByteCode* code2, BOOL no_output_to_bytecodes);
void append_buf_to_bytecodes(sByteCode* self, int* code, int len, BOOL no_output_to_bytecodes);
void append_generics_type_to_bytecode(sByteCode* self, sConst* constant, sCLNodeType* type_, BOOL no_output_to_bytecodes);

void sConst_init(sConst* self);
void sConst_free(sConst* self);
int append_int_value_to_constant_pool(sConst* constant, int n, BOOL no_output_to_bytecodes);
int append_float_value_to_constant_pool(sConst* constant, float n, BOOL no_output_to_bytecodes);
int append_str_to_constant_pool(sConst* constant, char* str, BOOL no_output_to_bytecodes);
int append_wstr_to_constant_pool(sConst* constant, char* str, BOOL no_output_to_bytecodes);
void append_buf_to_constant_pool(sConst* self, char* src, int src_len, BOOL no_output_to_bytecodes);

//////////////////////////////////////////////////
// vtable.c
//////////////////////////////////////////////////
void init_vtable();
void final_vtable();

sVarTable* init_var_table();

// result: (true) success (false) overflow the table or a variable which has the same name exists
BOOL add_variable_to_table(sVarTable* table, char* name, sCLNodeType* type_);

// result: (true) success (false) overflow the table or not found the variable
BOOL erase_variable_to_table(sVarTable* table, char* name);

BOOL does_this_var_exist(sVarTable* table, char* name);

void show_var_table(sVarTable* var_table);
void show_var_table_with_parent(sVarTable* var_table);

sVarTable* init_block_vtable(sVarTable* lv_table);
sVarTable* init_method_block_vtable(sVarTable* lv_table);

void entry_vtable_to_node_block(unsigned int block, sVarTable* new_table, sVarTable* lv_table);

void entry_method_block_vtable_to_node_block(sVarTable* new_table, unsigned int block);

// result: (-1) not found (non -1) found
int get_variable_index_from_table(sVarTable* table, char* name);

// result: (null) not found (sVar*) found
sVar* get_variable_from_table(sVarTable* table, char* name);

// result: (null) not found (sVar*) found
sVar* get_variable_from_table_by_var_index(sVarTable* table, int index);

////////////////////////////////////////////////////////////
// obj_system.c
////////////////////////////////////////////////////////////
BOOL System_sleep(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL System_exit(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL System_getenv(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL System_srand(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL System_rand(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL System_time(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

////////////////////////////////////////////////////////////
// obj_thread.c
////////////////////////////////////////////////////////////
void initialize_hidden_class_method_of_thread(sCLClass* klass);

BOOL Thread_Thread(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Thread_join(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

void thread_init();
void thread_final();

void vm_mutex_lock();
void vm_mutex_unlock();
void mutex_lock();
void mutex_unlock();
void start_vm_mutex_signal();
void start_vm_mutex_wait();

////////////////////////////////////////////////////////////
// compiler.c
////////////////////////////////////////////////////////////
enum eCompileType { kCompileTypeInclude, kCompileTypeLoad, kCompileTypeFile };

BOOL add_compile_data(sCLClass* klass, char num_definition, unsigned char num_method, enum eCompileType compile_type);

int num_loaded_class();
char* get_loaded_class(int index);
void add_loaded_class_to_table(char* namespace, char* class_name);

////////////////////////////////////////////////////////////
// namespace.c
////////////////////////////////////////////////////////////
BOOL append_namespace_to_curernt_namespace(char* current_namespace, char* namespace);

//////////////////////////////////////////////////
// obj_range.c
//////////////////////////////////////////////////

CLObject create_range_object(CLObject type_object, int head, int tail);
void initialize_hidden_class_method_of_range(sCLClass* klass);

BOOL Range_tail(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Range_head(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

////////////////////////////////////////////////////////////
// obj_mutex.c
////////////////////////////////////////////////////////////
void initialize_hidden_class_method_of_mutex(sCLClass* klass);

BOOL Mutex_run(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Mutex_Mutex(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

////////////////////////////////////////////////////////////
// obj_file.c
////////////////////////////////////////////////////////////
void initialize_hidden_class_method_of_file(sCLClass* klass);

BOOL File_write(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

////////////////////////////////////////////////////////////
// obj_regular_file.c
////////////////////////////////////////////////////////////
void initialize_hidden_class_method_of_regular_file(sCLClass* klass);

BOOL RegularFile_RegularFile(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

BOOL RegularFile_RegularFile(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

////////////////////////////////////////////////////////////
// obj_type_object.c
////////////////////////////////////////////////////////////
CLObject create_type_object_from_bytecodes(int** pc, sByteCode* code, sConst* constant, sVMInfo* info);
CLObject create_type_object_from_cl_type(sCLClass* klass, sCLType* cl_type, sVMInfo* info);

CLObject create_type_object_from_other_type_object(CLObject type_object, sVMInfo* info);
CLObject create_type_object(sCLClass* klass);
CLObject get_type_object_from_cl_type(sCLType* cl_type, sCLClass* klass, sVMInfo* info);
BOOL solve_generics_types_of_type_object(CLObject type_object, ALLOC CLObject* solved_type_object, CLObject type_, sVMInfo* info);
// result (0): can't create type object (non 0): success
CLObject get_super_from_type_object(CLObject type_object, sVMInfo* info);
void initialize_hidden_class_method_of_type(sCLClass* klass);

void write_type_name_to_buffer(char* buf, int size, CLObject type_object);

BOOL substitution_posibility_of_type_object(CLObject left_type, CLObject right_type);
BOOL substitution_posibility_of_type_object_without_generics(CLObject left_type, CLObject right_type);

BOOL Type_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Type_equals(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Type_class(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Type_genericsParam(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Type_parentClass(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Type_genericsParamNumber(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);
BOOL Type_parentClassNumber(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info);

void show_type_object(CLObject type_object);

////////////////////////////////////////////////////////////
// type.c
////////////////////////////////////////////////////////////
sCLType* allocate_cl_type();
void free_cl_types();

ALLOC sCLType* clone_cl_type(sCLType* cl_type2, sCLClass* klass, sCLClass* klass2);
void clone_cl_type2(sCLType* self, sCLType* cl_type2, sCLClass* klass, sCLClass* klass2);
ALLOC sCLType* create_cl_type_from_node_type(sCLNodeType* node_type, sCLClass* klass);
BOOL substitution_posibility_of_class(sCLClass* left_type, sCLClass* right_type);
BOOL check_valid_generics_type(sCLNodeType* type, char* sname, int* sline, int* err_num, sCLClass* caller_class, sCLMethod* method);
BOOL check_valid_star_type(sCLClass* klass);
BOOL type_identity_of_cl_type(sCLClass* klass1, sCLType* type1, sCLClass* klass2, sCLType* type2);
void create_cl_type_from_node_type2(sCLType* cl_type, sCLNodeType* node_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// node_type.c
////////////////////////////////////////////////////////////
void init_node_types();
void free_node_types();

sCLNodeType* alloc_node_type();

ALLOC sCLNodeType* create_node_type_from_cl_type(sCLType* cl_type, sCLClass* klass);
ALLOC sCLNodeType* clone_node_type(sCLNodeType* node_type);
BOOL substitution_posibility(sCLNodeType* left_type, sCLNodeType* right_type);
// left_type is stored type. right_type is value type.
BOOL substitution_posibility_with_solving_generics(sCLNodeType* left_type, sCLNodeType* right_type, sCLClass* caller_class, sCLMethod* caller_method);
BOOL operand_posibility(sCLNodeType* left_type, sCLNodeType* right_type);

// left_type is stored type. right_type is value type.
BOOL type_identity(sCLNodeType* type1, sCLNodeType* type2);

// left_type is stored type. right_type is value type.
BOOL type_identity_without_star(sCLNodeType* type1, sCLNodeType* type2);

BOOL solve_generics_types_for_node_type(sCLNodeType* node_type, ALLOC sCLNodeType** result, sCLNodeType* type_);

BOOL get_type_patterns_from_generics_param_type(sCLClass* klass, sCLGenericsParamTypes* generics_param_types, sCLNodeType** extends_type, sCLNodeType** implements_types, int* num_implements_types);

////////////////////////////////////////////////////////////
// module.c
////////////////////////////////////////////////////////////
void module_init();
sCLModule* create_module(char* namespace, char* name);
void append_character_to_module(sCLModule* self, char c);
char* get_module(char* namespace, char* name);
void module_final();
void this_module_is_modified(sCLModule* self);

// result (TRUE): success (FALSE): failed to write module to the file
void save_all_modified_modules();

#endif
