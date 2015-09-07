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

//////////////////////////////////////////////////
// klass.c
//////////////////////////////////////////////////
BOOL is_dynamic_typing_class(sCLClass* klass);
BOOL search_for_implemeted_interface(sCLClass* klass, sCLClass* interface);
BOOL is_generics_param_class(sCLClass* klass);

BOOL read_generics_param_types(int fd, sCLGenericsParamTypes* generics_param_types);

extern sCLClass* gClassHashList[CLASS_HASH_SIZE];

extern sCLNodeType* gIntType;      // foudamental classes
extern sCLNodeType* gByteType;
extern sCLNodeType* gShortType;
extern sCLNodeType* gUIntType;
extern sCLNodeType* gLongType;
extern sCLNodeType* gCharType;
extern sCLNodeType* gDoubleType;
extern sCLNodeType* gFloatType;
extern sCLNodeType* gVoidType;
extern sCLNodeType* gBoolType;
extern sCLNodeType* gPointerType;
extern sCLNodeType* gNullType;

extern sCLNodeType* gObjectType;
extern sCLNodeType* gStringType;
extern sCLNodeType* gBytesType;
extern sCLNodeType* gArrayType;
extern sCLNodeType* gTupleType[CL_GENERICS_CLASS_PARAM_MAX+1];
extern sCLNodeType* gRangeType;
extern sCLNodeType* gHashType;
extern sCLNodeType* gBlockType;
extern sCLNodeType* gExceptionType;
extern sCLNodeType* gThreadType;
extern sCLNodeType* gTypeType;
extern sCLNodeType* gRegexType;

extern sCLNodeType* gGParamTypes[CL_GENERICS_CLASS_PARAM_MAX];
extern sCLNodeType* gAnonymousType;

extern sCLClass* gVoidClass;
extern sCLClass* gIntClass;
extern sCLClass* gNullClass;
extern sCLClass* gByteClass;
extern sCLClass* gShortClass;
extern sCLClass* gUIntClass;
extern sCLClass* gLongClass;
extern sCLClass* gCharClass;
extern sCLClass* gIntClass;
extern sCLClass* gByteClass;
extern sCLClass* gFloatClass;
extern sCLClass* gDoubleClass;
extern sCLClass* gBoolClass;
extern sCLClass* gPointerClass;
extern sCLClass* gObjectClass;
extern sCLClass* gArrayClass;
extern sCLClass* gTupleClass[CL_GENERICS_CLASS_PARAM_MAX+1];
extern sCLClass* gBytesClass;
extern sCLClass* gHashClass;
extern sCLClass* gRangeClass;
extern sCLClass* gBlockClass;
extern sCLClass* gTypeClass;
extern sCLClass* gStringClass;
extern sCLClass* gThreadClass;
extern sCLClass* gOnigurumaRegexClass;
extern sCLClass* gGParamClass[CL_GENERICS_CLASS_PARAM_MAX];
extern sCLClass* gAnonymousClass;
extern sCLClass* gExceptionClass;

extern CLObject gTypeObject;
extern CLObject gIntTypeObject;
extern CLObject gShortTypeObject;
extern CLObject gUIntTypeObject;
extern CLObject gLongTypeObject;
extern CLObject gCharTypeObject;
extern CLObject gStringTypeObject;
extern CLObject gArrayTypeObject;
extern CLObject gHashTypeObject;
extern CLObject gRangeTypeObject;
extern CLObject gNullTypeObject;
extern CLObject gFloatTypeObject;
extern CLObject gDoubleTypeObject;
extern CLObject gBoolTypeObject;
extern CLObject gPointerTypeObject;
extern CLObject gByteTypeObject;
extern CLObject gBytesTypeObject;
extern CLObject gBlockTypeObject;
extern CLObject gOnigurumaRegexTypeObject;

extern sCLClass* gCloverClass;

void class_init();
void class_final();

unsigned int get_hash(char* name);
void show_class_list(sVMInfo* info);
void show_class_list_on_compile_time();
sCLClass* load_class_from_classpath(char* real_class_name, BOOL solve_dependences, int mixin_version);
sCLClass* load_class_with_namespace_from_classpath(char* namespace, char* class_name, BOOL solve_dependences);
sCLClass* load_class_with_namespace_on_compile_time(char* namespace, char* class_name, BOOL solve_dependences, int parametor_num, int mixin_version);
ALLOC char* native_load_class(char* file_name);
void alloc_bytecode_of_method(sCLMethod* method);
void create_real_class_name(char* result, int result_size, char* namespace, char* class_name, int parametor_num);
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
sCLClass* alloc_class(char* namespace, char* class_name, BOOL private_, BOOL abstract_, BOOL interface, BOOL dynamic_typing_, BOOL final_, BOOL native_, BOOL struct_, BOOL enum_, int parametor_num);

BOOL run_all_loaded_class_fields_initializer();
BOOL run_all_loaded_class_initialize_method();

// result should be freed
ALLOC char** get_class_names();

void cl_unload_all_classes();

//////////////////////////////////////////////////
// klass_ctime.c
//////////////////////////////////////////////////
sCLMethod* get_clone_method(sCLClass* klass);

BOOL load_fundamental_classes_on_compile_time();

sCLClass* alloc_class_on_compile_time(char* namespace, char* class_name, BOOL private_, BOOL abstract_, BOOL interface, BOOL dynamic_typing_, BOOL final_, BOOL native_, BOOL struct_, BOOL enum_, int parametor_num);

// result (TRUE) --> success (FLASE) --> overflow super class number 
BOOL add_super_class(sCLClass* klass, sCLNodeType* super_klass);

// result (TRUE) --> success (FLASE) --> overflow implemented interface number
BOOL add_implemented_interface(sCLClass* klass, sCLNodeType* interface);

// result (TRUE) --> success (FLASE) --> overflow included module number
BOOL add_included_module(sCLClass* klass, sCLModule* module);

BOOL check_implemented_interface_between_super_classes(sCLNodeType* klass, sCLNodeType* interface, sCLMethod* method);

// result (TRUE) --> implemeted all methods (FALSE) --> there are not implemented methods
BOOL check_implemented_interface(sCLNodeType* klass, sCLNodeType* interface);

// result (TRUE) --> implemeted all methods (FALSE) --> there are not implemented methods
BOOL check_implemented_interface_without_super_class(sCLNodeType* klass, sCLNodeType* interface);

BOOL check_the_same_parametor_of_two_methods(sCLNodeType* klass1, sCLMethod* method1, sCLNodeType* klass2, sCLMethod* method2);

// result (TRUE) --> implemeted this interface (FALSE) --> not implemented this interface
BOOL check_implemented_interface2(sCLClass* klass, sCLNodeType* interface);

// result (TRUE) --> implemeted all methods (FALSE) --> there are not implemented methods
BOOL check_implemented_abstract_methods(sCLNodeType* klass, char** not_implemented_method_name);

void add_dependence_class(sCLClass* klass, sCLClass* dependence_class);

BOOL is_generics_param_type(sCLNodeType* node_type);
BOOL is_parent_class(sCLClass* klass1, sCLClass* klass2) ;

int get_generics_param_number(sCLClass* klass);
//int get_generics_param_number_of_method_scope(sCLClass* klass);
sCLGenericsParamTypes* get_generics_param_types(sCLClass* klass, sCLClass* caller_class, sCLMethod* caller_method);

struct sGenericsParamPatternStruct {
    sCLNodeType* mGenericsTypes[CL_GENERICS_CLASS_PARAM_MAX];
    int mGenericsTypesNum;

    int mNumber;
};

typedef struct sGenericsParamPatternStruct sGenericsParamPattern;

void create_generics_param_type_pattern(ALLOC sGenericsParamPattern** generics_type_array, int* generics_type_array_num, sCLNodeType* caller_class);

// result (TRUE) --> success (FALSE) --> overflow number fields
// initializer_code should be allocated and is managed inside this function after called
BOOL add_field(sCLClass* klass, BOOL static_, BOOL private_, BOOL protected, char* name, sCLNodeType* node_type);

/// set field index on time of compiling code
void set_field_index(sCLClass* klass, char* name, BOOL class_field);

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
sCLMethod* get_method_with_type_params_and_param_initializer(sCLNodeType* klass, char* method_name, sCLNodeType** class_params, int num_params, BOOL search_for_class_method, sCLNodeType* type_, sCLNodeType* generics_solving_type, int start_point, int block_num, int block_num_params, sCLNodeType** block_param_type, sCLNodeType* block_type, int* used_param_num_with_initializer, sCLNodeType** result_type);

// no solve generics type
ALLOC sCLNodeType* get_result_type_of_method(sCLNodeType* klass, sCLMethod* method);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params_and_param_initializer_on_super_classes(sCLNodeType* klass, char* method_name, sCLNodeType** class_params, int num_params, sCLNodeType** founded_class, BOOL search_for_class_method, sCLNodeType* type_, sCLNodeType* generics_solving_type, int block_num, int block_num_params, sCLNodeType** block_param_type, sCLNodeType* block_type, int* used_param_num_with_initializer, sCLNodeType** result_type);

// result (TRUE) --> success (FALSE) --> overflow methods number
// last parametor returns the method which is added
BOOL create_method(sCLClass* klass, sCLMethod** method);

void add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL protected_, BOOL native_, BOOL synchronized_, BOOL virtual_, BOOL abstract_, BOOL generics_newable, char* name, sCLNodeType* result_type, BOOL constructor, sCLMethod* method);

// result (TRUE) --> success (FALSE) --> overflow parametor number
BOOL add_param_to_method(sCLClass* klass, sCLNodeType** class_params, MANAGED sByteCode* code_params, int* max_stack_params, int* lv_num_params, int num_params, sCLMethod* method, int block_num, char* block_name, sCLNodeType* bt_result_type, sCLNodeType** bt_class_params, int bt_num_params, char* name, BOOL variable_arguments);

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

// result should be freed
ALLOC char** get_method_names(sCLClass* klass);

// result and contained elements should be freed
ALLOC ALLOC char** get_method_names_with_arguments(sCLClass* klass);

void clear_method_index_of_compile_time();

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
BOOL expect_next_character(char* characters, int* err_num, char** p, char* sname, int* sline);
// characters is null-terminated
void expect_next_character_with_one_forward(char* characters, int* err_num, char** p, char* sname, int* sline);

BOOL node_expression(unsigned int* node, sParserInfo* info, sVarTable* lv_table);
BOOL node_expression_without_comma(unsigned int* node, sParserInfo* info, sVarTable* lv_table);

BOOL parse_generics_types_name(char** p, char* sname, int* sline, int* err_num, char* generics_types_num, sCLNodeType** generics_types, char* current_namespace, sCLClass* klass, sCLMethod* method, BOOL skip);

BOOL parse_namespace_and_class(sCLClass** result, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLMethod* method, BOOL skip, BOOL* star);
    // result: (FALSE) there is an error (TRUE) success
    // result class is setted on first parametor
BOOL parse_module_name(sCLModule** result, char** p, char* sname, int* sline, int* err_num, char* current_namespace);
    // result: (FALSE) there is an error (TRUE) success
    // result module is setted on first parametor
BOOL parse_namespace_and_class_and_generics_type(ALLOC sCLNodeType** type, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLMethod* method, BOOL skip);
    // result: (FALSE) there is an error (TRUE) success
    // result type is setted on first parametor
int get_generics_type_num(sCLClass* klass, char* type_name);

BOOL delete_comment(sBuf* source, sBuf* source2);

void compile_error(char* msg, ...);
BOOL parse_params(sCLNodeType** class_params, int* num_params, int size_params, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLMethod* method, sVarTable* lv_table, char close_character, int sline_top);
BOOL parse_params_with_initializer(sCLNodeType** class_params, sByteCode* code_params, int* max_stack_params, int* lv_num_params, int* num_params, int size_params, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, char close_character, int sline_top, BOOL* variable_arguments);

extern BOOL gParserOutput;

//////////////////////////////////////////////////
// node.c
//////////////////////////////////////////////////
#define NODE_TYPE_OPERAND 1
#define NODE_TYPE_VALUE 2
#define NODE_TYPE_STRING_VALUE 3
#define NODE_TYPE_VARIABLE_NAME 4
#define NODE_TYPE_ARRAY_VALUE 5
#define NODE_TYPE_DEFINE_VARIABLE_NAME 6
#define NODE_TYPE_FIELD 7
#define NODE_TYPE_CLASS_FIELD 8
#define NODE_TYPE_STORE_VARIABLE_NAME 9
#define NODE_TYPE_DEFINE_AND_STORE_VARIABLE_NAME 10
#define NODE_TYPE_STORE_FIELD 11
#define NODE_TYPE_STORE_CLASS_FIELD 12
#define NODE_TYPE_CLASS_METHOD_CALL 13
#define NODE_TYPE_PARAM 14
#define NODE_TYPE_RETURN 15
#define NODE_TYPE_NEW 16
#define NODE_TYPE_METHOD_CALL 17
#define NODE_TYPE_SUPER 18
#define NODE_TYPE_INHERIT 19
#define NODE_TYPE_NULL 20
#define NODE_TYPE_TRUE 21
#define NODE_TYPE_FALSE 22
#define NODE_TYPE_FVALUE 23
#define NODE_TYPE_IF 24
#define NODE_TYPE_WHILE 25
#define NODE_TYPE_BREAK 26
#define NODE_TYPE_DO 27
#define NODE_TYPE_FOR 28
#define NODE_TYPE_CONTINUE 29
#define NODE_TYPE_BLOCK_CALL 30
#define NODE_TYPE_REVERT 31
#define NODE_TYPE_BLOCK 32
#define NODE_TYPE_CHARACTER_VALUE 33
#define NODE_TYPE_THROW 34
#define NODE_TYPE_TRY 35
#define NODE_TYPE_CLASS_NAME 36
#define NODE_TYPE_BYTES_VALUE 37
#define NODE_TYPE_RANGE_VALUE 38
#define NODE_TYPE_HASH_VALUE 39
#define NODE_TYPE_TUPLE_VALUE 40
#define NODE_TYPE_REGEX_VALUE 41
#define NODE_TYPE_STORE_TUPLE 42
#define NODE_TYPE_BYTE_VALUE 43
#define NODE_TYPE_SHORT_VALUE 44
#define NODE_TYPE_UINT_VALUE 45
#define NODE_TYPE_LONG_VALUE 46
#define NODE_TYPE_DVALUE 47
#define NODE_TYPE_MAX 48

enum eOperand { 
    kOpAdd, kOpSub, kOpMult, kOpDiv, kOpMod, kOpPlusPlus2, kOpMinusMinus2, kOpIndexing, kOpSubstitutionIndexing, kOpPlusPlus, kOpMinusMinus, kOpComplement, kOpLogicalDenial, kOpLeftShift, kOpRightShift, kOpComparisonGreater, kOpComparisonLesser, kOpComparisonGreaterEqual, kOpComparisonLesserEqual, kOpComparisonEqual, kOpComparisonNotEqual, kOpComparisonEqualTilda, kOpAnd, kOpXor, kOpOr, kOpOrOr, kOpAndAnd, kOpConditional, kOpComma
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
            char mRegexString[REGEX_LENGTH_MAX+1];
            BOOL mGlobal;
            BOOL mMultiline;
            BOOL mIgnoreCase;
        } sRegex;

        struct {
            enum eOperand mOperand;
            BOOL mQuote;
        } sOperand;

        struct {
            char* mVarNames[MULTIPLE_ASSIGNMENT_NUM_MAX];
            int mVarNum;
        } sMultipleVar;

        unsigned int mWhileBlock;                            // node block id

        unsigned int mDoBlock;                               // node block id

        unsigned int mForBlock;                              // node block id

        unsigned int mBlock;                                 // node block id

        int mValue;
        unsigned char mByteValue;
        unsigned short mShortValue;
        unsigned int mUIntValue;
        unsigned long mLongValue;
        char* mStringValue;
        float mFValue;
        double mDValue;
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

    BOOL mBreakable;
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

    int mNestOfMethodFromDefinitionPoint;
};

typedef struct sCompileInfoStruct sCompileInfo;


extern sNodeBlock* gNodeBlocks; // All node blocks at here. Index is node block number. alloc_node_block() returns a node block number

extern sNodeTree* gNodes; // All nodes at here. Index is node number. sNodeTree_create* functions return a node number.

void show_node(unsigned int node);
BOOL compile_method(sCLMethod* method, sCLNodeType* klass, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, BOOL constructor, char* current_namespace);
BOOL compile_field_initializer(sByteCode* initializer, ALLOC sCLNodeType** initializer_code_type, sCLNodeType* klass, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sVarTable* lv_table, int* max_stack);
BOOL compile_param_initializer(ALLOC sByteCode* initializer, sCLNodeType** initializer_code_type, int* max_stack, int* lv_var_num, sCLNodeType* klass, char** p, char* sname, int* sline, int* err_num, char* current_namespace);


BOOL compile_statments(char** p, char* sname, int* sline, sByteCode* code, sConst* constant, int* err_num, int* max_stack, char* current_namespace, sVarTable* var_table, BOOL output_result);
BOOL skip_field_initializer(char** p, char* sname, int* sline, char* current_namespace, sCLNodeType* klass, sVarTable* lv_table);
BOOL parse_block_object(unsigned int* block_id, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType* block_type, sCLMethod* method, sVarTable* lv_table, int sline_top, int num_params, sCLNodeType** class_params);

unsigned int alloc_node_block(sCLNodeType* block_type);
void append_node_to_node_block(unsigned int node_block_id, sNode* node);

BOOL compile_node(unsigned int node, sCLNodeType** type_, sCLNodeType** class_params, int* num_params, sCompileInfo* info);
BOOL compile_middle_node(unsigned int node, sCLNodeType** middle_type, sCLNodeType** class_params, int* num_params, sCompileInfo* info);
BOOL compile_right_node(unsigned int node, sCLNodeType** right_type, sCLNodeType** class_params, int* num_params, sCompileInfo* info);
BOOL compile_left_node(unsigned int node, sCLNodeType** left_type, sCLNodeType** class_params, int* num_params, sCompileInfo* info);

//////////////////////////////////////////////////
// node_tree.c
//////////////////////////////////////////////////

void init_nodes();
void free_nodes();

// Below functions return a node number. It is an index of gNodes.
unsigned int sNodeTree_create_operand(enum eOperand operand, unsigned int left, unsigned int right, unsigned int middle, BOOL quote);
unsigned int sNodeTree_create_revert(sCLNodeType* klass, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_value(int value, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_fvalue(float fvalue, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_dvalue(double dvalue, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_string_value(MANAGED char* value, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_long_value(unsigned long value, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_uint_value(unsigned int value, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_short_value(unsigned short value, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_byte_value(unsigned char value, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_bytes_value(MANAGED char* value, unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_array(unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_tuple(unsigned int left, unsigned int right, unsigned int middle);
unsigned int sNodeTree_create_regex(char* regex, BOOL global, BOOL multiline, BOOL ignore_case);
unsigned int sNodeTree_create_hash(unsigned int left, unsigned int right, unsigned int middle);
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
unsigned int sNodeTree_create_regex(char* regex, BOOL global, BOOL multiline, BOOL ignore_case);

//////////////////////////////////////////////////
// compile.c
//////////////////////////////////////////////////
void cl_compiler_final();
void cl_compiler_init();
void correct_stack_pointer_n(int* stack_num, int n, char* sname, int* sline, sByteCode* code, int* err_num, BOOL no_output_to_bytecodes);
void correct_stack_pointer(int* stack_num, char* sname, int* sline, sByteCode* code, int* err_num, BOOL no_output_to_bytecodes);
BOOL get_result_type_of_method_block(sNodeBlock* block, sCompileInfo* info, enum eBlockKind block_kind);

BOOL compile_block(sNodeBlock* block, sCLNodeType** type_, sCompileInfo* info);
BOOL compile_loop_block(sNodeBlock* block, sCLNodeType** type_, sCompileInfo* info);
BOOL compile_block_object(sNodeBlock* block, sConst* constant, sByteCode* code, sCLNodeType** type_, sCompileInfo* info, sCLNodeType* caller_class, sCLMethod* caller_method, enum eBlockKind block_kind);
BOOL parse_block(unsigned int* block_id, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType* block_type, sCLMethod* method, sVarTable* lv_table);
void make_block_result(sCLNodeType** result_type);

//////////////////////////////////////////////////
// vm.c
//////////////////////////////////////////////////
#define INVOKE_METHOD_KIND_CLASS 0
#define INVOKE_METHOD_KIND_OBJECT 1

BOOL check_type(CLObject ovalue1, CLObject type_object, sVMInfo* info);
BOOL check_type_with_nullable(CLObject ovalue1, CLObject type_object, sVMInfo* info);
BOOL check_type_with_class_name(CLObject ovalue1, char* class_name, sVMInfo* info);
BOOL check_type_without_generics(CLObject ovalue1, CLObject type_object, sVMInfo* info);
BOOL check_type_with_dynamic_typing(CLObject ovalue1, CLObject type_object, sVMInfo* info);
BOOL check_type_without_exception(CLObject ovalue1, CLObject type_object, sVMInfo* info);
BOOL check_type_for_array(CLObject obj, char* generics_param_type, sVMInfo* info);
BOOL check_type_without_info(CLObject ovalue1, char* class_name);

BOOL cl_excute_block(CLObject block, BOOL result_existance, sVMInfo* info, CLObject vm_type);

BOOL cl_excute_method(sCLMethod* method, sCLClass* klass, sCLClass* class_of_class_method_call, NULLABLE sVMInfo* info, CLObject* result_value);

void push_object(CLObject object, sVMInfo* info);
// remove the object from stack
CLObject pop_object(sVMInfo* info);
void pop_object_except_top(sVMInfo* info);
void remove_object(sVMInfo* info, int number);
void push_vminfo(sVMInfo* info);
void show_object_value(sVMInfo* info, CLObject obj);

#if defined(VM_DEBUG)

#define START_VMLOG(o) start_vm_log(o)
#define VMLOG(o, ...) vm_log(o, __VA_ARGS__)

#define SHOW_STACK(o, o2, o3) show_stack(o, o2, o3)
#define SHOW_STACK2(o) show_stack(o, NULL, NULL)
#define SHOW_HEAP(o) show_heap(o)

void vm_log(sVMInfo* info, char* msg, ...);
void start_vm_log(sVMInfo* info);
void show_stack(sVMInfo* info, MVALUE* top_of_stack, MVALUE* var);
void show_heap(sVMInfo* info);

#elif defined(VM_DEBUG2)

#define START_VMLOG(o)
#define VMLOG(o, ...) vm_log(o, __VA_ARGS__)

#define SHOW_STACK(o, o2, o3) show_stack(o, o2, o3)
#define SHOW_STACK2(o) show_stack(o, NULL, NULL)
#define SHOW_HEAP(o) show_heap(o)

void vm_log(sVMInfo* info, char* msg, ...);
void show_stack(sVMInfo* info, MVALUE* top_of_stack, MVALUE* var);
void show_heap(sVMInfo* info);

#else

#define VMLOG(o, ...)
#define START_VMLOG(o)
#define SHOW_STACK(o, o2, o3)
#define SHOW_STACK2(o)
#define SHOW_HEAP(o) 

#endif

void vm_error(char* msg, ...);
void entry_exception_object(sVMInfo* info, sCLClass* klass, char* msg, ...);
void entry_exception_object_with_class_name(sVMInfo* info, char* class_name, char* msg, ...);
BOOL field_initializer(MVALUE* result, sByteCode* code, sConst* constant, int lv_num, int max_stack, CLObject vm_type);
void sigttou_block(int block);

extern sVMInfo* gHeadVMInfo;

void atexit_fun();
void output_exception_message(CLObject exception_object);

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
BOOL cl_call_runtime_method();

BOOL Clover_gc(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Clover_printf(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Clover_sprintf(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Clover_showClasses(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Clover_gc(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Clover_print(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Clover_outputToString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

//////////////////////////////////////////////////
// obj_null.c
//////////////////////////////////////////////////
CLObject create_null_object();
void initialize_hidden_class_method_of_immediate_null(sCLClass* klass);

//////////////////////////////////////////////////
// obj_int.c
//////////////////////////////////////////////////
CLObject create_int_object(int value);

BOOL int_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL int_toCharacter(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL int_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL int_toByte(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL int_toFloat(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL int_toDouble(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
void initialize_hidden_class_method_of_int(sCLClass* klass);
BOOL int_upcase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL int_downcase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL int_toLong(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL int_toUInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL int_toShort(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL int_toChar(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

void initialize_hidden_class_method_of_immediate_int(sCLClass* klass);

//////////////////////////////////////////////////
// obj_byte.c
//////////////////////////////////////////////////
void initialize_hidden_class_method_of_immediate_byte(sCLClass* klass);

CLObject create_byte_object(unsigned char value);

BOOL byte_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL byte_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

//////////////////////////////////////////////////
// obj_void.c
//////////////////////////////////////////////////
void initialize_hidden_class_method_of_immediate_void(sCLClass* klass);

//////////////////////////////////////////////////
// obj_anonymous.c
//////////////////////////////////////////////////
void initialize_hidden_class_method_of_anonymous(sCLClass* klass);

//////////////////////////////////////////////////
// obj_float.c
//////////////////////////////////////////////////
CLObject create_float_object(float value);

BOOL float_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL float_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL float_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL float_toDouble(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

void initialize_hidden_class_method_of_immediate_float(sCLClass* klass);

//////////////////////////////////////////////////
// obj_bool.c
//////////////////////////////////////////////////
CLObject create_bool_object(BOOL value);

BOOL bool_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

void initialize_hidden_class_method_of_immediate_bool(sCLClass* klass);

//////////////////////////////////////////////////
// obj_user_object.c
//////////////////////////////////////////////////
// result (TRUE): success (FALSE): threw exception
BOOL create_user_object(CLObject type_object, CLObject* obj, CLObject vm_type, MVALUE* fields, int num_fields, sVMInfo* info);
BOOL create_user_object_with_class_name(char* class_name, CLObject* obj, CLObject vm_type, sVMInfo* info);
void initialize_hidden_class_method_of_user_object(sCLClass* klass);

BOOL Object_type(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Object_setType(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Object_ID(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Object_fields(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Object_numFields(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Object_setField(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

//////////////////////////////////////////////////
// obj_string.c
//////////////////////////////////////////////////
CLObject create_string_object(wchar_t* str, int len, CLObject type_object, sVMInfo* info);
CLObject create_string_object_by_multiply(CLObject string, int number, sVMInfo* info);
BOOL create_string_object_from_ascii_string(CLObject* result, char* str, CLObject type_object, sVMInfo* info);
void initialize_hidden_class_method_of_string(sCLClass* klass);

BOOL create_buffer_from_string_object(CLObject str, ALLOC char** result, sVMInfo* info);

BOOL String_String(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_char(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_replace(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_toBytes(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_cmp(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_toDouble(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_sub(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_sub_with_block(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_sub_with_hash(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_count(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_index(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_match(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL String_matchReverse(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

BOOL string_object_to_str(ALLOC char** result, CLObject string);

//////////////////////////////////////////////////
// obj_bytes.c
//////////////////////////////////////////////////
CLObject create_bytes_object(char* str, int len, CLObject type_object, sVMInfo* info);
CLObject create_bytes_object_by_multiply(CLObject string, int number, sVMInfo* info);

void initialize_hidden_class_method_of_bytes(sCLClass* klass);

BOOL Bytes_Bytes(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Bytes_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Bytes_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Bytes_replace(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Bytes_char(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Bytes_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Bytes_cmp(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Bytes_toPointer(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

void replace_bytes(CLObject bytes, char* buf, int size);

//////////////////////////////////////////////////
// obj_array.c
//////////////////////////////////////////////////
CLObject create_array_object(CLObject type_object, MVALUE elements[], int num_elements, sVMInfo* info);
CLObject create_array_object_with_element_class_name(char* element_class_name, MVALUE elements[], int num_elements, sVMInfo* info);
CLObject create_array_object2(CLObject type_object, CLObject elements[], int num_elements, sVMInfo* info);
void initialize_hidden_class_method_of_array(sCLClass* klass);

BOOL Array_Array(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Array_items(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Array_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Array_add(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Array_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Array_setItem(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
void add_to_array(CLObject self, CLObject item, sVMInfo* info);

//////////////////////////////////////////////////
// obj_hash.c
/////////////////////////////////////////////////
BOOL create_hash_object(CLObject* obj, CLObject type_object, MVALUE keys[], MVALUE elements[], int num_elements, sVMInfo* info);
void initialize_hidden_class_method_of_hash(sCLClass* klass);

BOOL Hash_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Hash_length(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Hash_assoc(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Hash_put(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Hash_each(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Hash_erase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

BOOL add_item_to_hash(CLObject self, CLObject key, CLObject item, sVMInfo* info);

//////////////////////////////////////////////////
// hash.c
//////////////////////////////////////////////////
void initialize_hidden_class_method_of_block(sCLClass* klass);
CLObject create_block(char* constant, int const_len, int* code, int code_len, int max_stack, int num_locals, int num_params, MVALUE* parent_var, int num_parent_vars, int max_block_var_num, CLObject result_type, CLObject* params, BOOL breakable);

//////////////////////////////////////////////////
// interface.c
//////////////////////////////////////////////////
BOOL cl_eval_file(char* file_name);

//////////////////////////////////////////////////
// buffer.c
//////////////////////////////////////////////////
void show_buffer(char* buf, int len);
void show_constants(sConst* constant);

void sBuf_init(sBuf* self);
void sBuf_append(sBuf* self, void* str, size_t size);
void sBuf_append_char(sBuf* self, char c);
void sBuf_append_str(sBuf* self, char* str);
void sBuf_show(sBuf* self);

void sByteCode_init(sByteCode* self);
void sByteCode_free(sByteCode* self);
void append_opecode_to_bytecodes(sByteCode* self, int value, BOOL no_output_to_bytecodes);
void append_int_value_to_bytecodes(sByteCode* self, int value, BOOL no_output_to_bytecodes);
void append_ulong_value_to_bytecodes(sByteCode* self, unsigned long value, BOOL no_output_to_bytecodes);
void append_str_to_bytecodes(sByteCode* code, sConst* constant, char* str, BOOL no_output_to_bytecodes);
void append_constant_pool_to_bytecodes(sByteCode* code, sConst* constant, sConst* constant2, BOOL no_output_to_bytecodes);
void append_code_to_bytecodes(sByteCode* code, sConst* constant, sByteCode* code2, BOOL no_output_to_bytecodes);
void append_buf_to_bytecodes(sByteCode* self, int* code, int len, BOOL no_output_to_bytecodes);
void append_generics_type_to_bytecode(sByteCode* self, sConst* constant, sCLNodeType* type_, BOOL no_output_to_bytecodes);

void sConst_init(sConst* self);
void sConst_free(sConst* self);
int append_int_value_to_constant_pool(sConst* constant, int n, BOOL no_output_to_bytecodes);
int append_float_value_to_constant_pool(sConst* constant, float n, BOOL no_output_to_bytecodes);
int append_double_value_to_constant_pool(sConst* constant, double n, BOOL no_output_to_bytecodes);
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
BOOL System_open(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_write(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_close(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_sleep(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_msleep(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_nanosleep(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_exit(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_getenv(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_srand(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_rand(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_execv(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_execvp(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_wait(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_fork(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_read(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_pipe(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_dup2(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_getpid(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_getppid(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_getpgid(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_setpgid(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_tcsetpgrp(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_stat(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL System_time(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_thread.c
////////////////////////////////////////////////////////////
void initialize_hidden_class_method_of_thread(sCLClass* klass);

BOOL Thread_Thread(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Thread_join(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Thread_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

void thread_init();
void thread_final();

void vm_mutex_lock();
void vm_mutex_unlock();
void mutex_lock();
void mutex_unlock();
void start_vm_mutex_signal();
void start_vm_mutex_wait();
void new_vm_mutex();

////////////////////////////////////////////////////////////
// load_class.c
////////////////////////////////////////////////////////////
void add_loaded_class_to_table(sCLClass* loaded_class);
char* get_loaded_class(int index);
int num_loaded_class();
void load_class_init();
void load_class_final();

////////////////////////////////////////////////////////////
// namespace.c
////////////////////////////////////////////////////////////
BOOL append_namespace_to_curernt_namespace(char* current_namespace, char* namespace);

//////////////////////////////////////////////////
// obj_range.c
//////////////////////////////////////////////////

CLObject create_range_object(CLObject type_object, CLObject head_object, CLObject tail_object);
void initialize_hidden_class_method_of_range(sCLClass* klass);

BOOL Range_tail(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Range_head(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Range_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Range_setTail(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Range_setHead(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_mutex.c
////////////////////////////////////////////////////////////
void initialize_hidden_class_method_of_mutex(sCLClass* klass);

BOOL Mutex_run(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Mutex_Mutex(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Mutex_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_file.c
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// obj_regular_file.c
////////////////////////////////////////////////////////////
void initialize_hidden_class_method_of_regular_file(sCLClass* klass);

BOOL RegularFile_RegularFile(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_type_object.c
////////////////////////////////////////////////////////////
CLObject create_type_object_from_bytecodes(int** pc, sByteCode* code, sConst* constant, sVMInfo* info);

// result: (0) --> class not found(exception) (non 0) --> created object
CLObject create_type_object_with_class_name_and_generics_name(char* class_name, sVMInfo* info);
BOOL include_generics_param_type(CLObject type_object);
CLObject create_type_object_from_cl_type(sCLClass* klass, sCLType* cl_type, sVMInfo* info);

CLObject create_type_object_from_other_type_object(CLObject type_object, sVMInfo* info);
CLObject create_type_object(sCLClass* klass);
CLObject create_type_object_with_class_name(char* class_name);
CLObject get_type_object_from_cl_type(sCLType* cl_type, sCLClass* klass, sVMInfo* info);
BOOL solve_generics_types_of_type_object(CLObject type_object, ALLOC CLObject* solved_type_object, CLObject type_, sVMInfo* info);
// result (0): can't create type object (non 0): success
CLObject get_super_from_type_object(CLObject type_object, sVMInfo* info);
void initialize_hidden_class_method_of_type(sCLClass* klass);

void write_type_name_to_buffer(char* buf, int size, CLObject type_object);

BOOL substitution_posibility_of_type_object(CLObject left_type, CLObject right_type, BOOL dynamic_typing);
BOOL substitution_posibility_of_type_object_with_class_name(char* left_type_object_name, CLObject right_type, BOOL dynamic_typing, sVMInfo* info);
BOOL substitution_posibility_of_type_object_without_generics(CLObject left_type, CLObject right_type, BOOL dynamic_typing);

BOOL Type_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Type_createFromString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Type_equals(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Type_class(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Type_genericsParam(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Type_parentClass(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Type_genericsParamNumber(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Type_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Type_parentClassNumber(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Type_classObject(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Type_substitutionPosibility(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

void show_type_object(CLObject type_object);

////////////////////////////////////////////////////////////
// type.c
////////////////////////////////////////////////////////////
sCLType* allocate_cl_type();
void free_cl_types();

void show_cl_type(sCLType* self, sCLClass* klass, sVMInfo* info);
void show_cl_type_for_errmsg(sCLType* self, sCLClass* klass);

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
BOOL operand_posibility_with_class_name(sCLNodeType* left_type, char* class_name);

// left_type is stored type. right_type is value type.
BOOL type_identity(sCLNodeType* type1, sCLNodeType* type2);

// left_type is stored type. right_type is value type.
BOOL type_identity_without_star(sCLNodeType* type1, sCLNodeType* type2);

BOOL solve_generics_types_for_node_type(sCLNodeType* node_type, ALLOC sCLNodeType** result, sCLNodeType* type_);

BOOL get_type_patterns_from_generics_param_type(sCLClass* klass, sCLGenericsParamTypes* generics_param_types, sCLNodeType** extends_type, sCLNodeType** implements_types, int* num_implements_types);

ALLOC char* node_type_to_buffer(sCLNodeType* node_type);

sCLNodeType* create_node_type_from_class_name(char* class_name);

////////////////////////////////////////////////////////////
// module.c
////////////////////////////////////////////////////////////
void module_init();
void module_final();
sCLModule* create_module(char* namespace, char* name);
void append_character_to_module(sCLModule* self, char c);
sCLModule* get_module(char* namespace, char* name);
sCLModule* get_module_from_real_module_name(char* real_module_name);
char* get_module_body(sCLModule* module);
void module_final();
void this_module_is_modified(sCLModule* self);

// result (TRUE): success (FALSE): failed to write module to the file
void save_all_modified_modules();

void create_cl_type_from_module(sCLType* cl_type, sCLModule* module, sCLClass* klass);
sCLModule* get_module_from_cl_type(sCLClass* klass, sCLType* cl_type);

////////////////////////////////////////////////////////////
// obj_regex.c
////////////////////////////////////////////////////////////
void initialize_hidden_class_method_of_oniguruma_regex(sCLClass* klass);
BOOL create_oniguruma_regex_object(CLObject* self, CLObject type_object, OnigUChar* regex_str, BOOL ignore_case, BOOL multiline, BOOL global, OnigEncoding enc, sVMInfo* info, CLObject vm_type);

BOOL OnigurumaRegex_source(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL OnigurumaRegex_ignoreCase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL OnigurumaRegex_multiLine(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL OnigurumaRegex_encode(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL OnigurumaRegex_compile(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL OnigurumaRegex_global(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL OnigurumaRegex_setEncode(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL OnigurumaRegex_setGlobal(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL OnigurumaRegex_setMultiLine(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL OnigurumaRegex_setIgnoreCase(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL OnigurumaRegex_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_class.c
////////////////////////////////////////////////////////////

CLObject create_class_object(CLObject type_object, CLObject klass);
void initialize_hidden_class_method_of_class_object(sCLClass* klass);

BOOL create_generics_type_object(CLObject* result, sCLGenericsParamTypes* generics_param_type, CLObject vm_type, sVMInfo* info, sCLClass* klass);
BOOL Class_newInstance(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Class_isNativeClass(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Class_isInterface(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Class_isAbstractClass(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Class_isFinalClass(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Class_isStruct(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Class_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Class_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Class_fields(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Class_methods(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Class_superClasses(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Class_implementedInterfaces(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Class_classDependences(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Class_toType(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Class_genericsParametorTypes(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_field.c
////////////////////////////////////////////////////////////
CLObject create_field_object(CLObject type_object, sCLClass* klass, sCLField* field);

void initialize_hidden_class_method_of_field_object(sCLClass* klass);

BOOL Field_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Field_name(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Field_isProtectedField(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Field_isPrivateField(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Field_isStaticField(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Field_fieldType(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Field_get(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Field_set(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_method.c
////////////////////////////////////////////////////////////
void initialize_hidden_class_method_of_method_object(sCLClass* klass);
CLObject create_method_object(CLObject type_object, sCLClass* klass, sCLMethod* method);

BOOL Method_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_isNativeMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_isClassMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_isPrivateMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_isConstructor(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_isSyncronizedMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_isVirtualMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_isAbstractMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_isGenericsNewableConstructor(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL isParamVariableArguments(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type);
BOOL Method_isProtectedMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_isParamVariableArguments(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_name(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_path(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_resultType(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_blockResultType(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_parametors(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_blockParametors(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_exceptions(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Method_invokeMethod(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_enum.c
////////////////////////////////////////////////////////////
BOOL Enum_toHash(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
void entry_native_enum_fields(sCLClass* klass, int num_fields, int values[]);

////////////////////////////////////////////////////////////
// utf_mb_str.c
////////////////////////////////////////////////////////////
enum eUtfMbsKind { kEucjp, kSjis, kUtf8, kUtf8Mac, kByte, kUnknown };

char* um_index2pointer(enum eUtfMbsKind code, char* mbs, int pos);
int um_pointer2index(enum eUtfMbsKind code, char* mbs, char* pointer);
int um_is_none_ascii(enum eUtfMbsKind code, unsigned char c);
int um_strlen(enum eUtfMbsKind code, char* mbs);

////////////////////////////////////////////////////////////
// obj_block.c
////////////////////////////////////////////////////////////
BOOL Block_parametors(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL Block_resultType(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_wait_status.c
////////////////////////////////////////////////////////////
BOOL WaitStatus_signalNumber(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL WaitStatus_signaled(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL WaitStatus_exitStatus(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL WaitStatus_exited(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_file_mode.c
////////////////////////////////////////////////////////////
void initialize_hidden_class_method_of_file_mode(sCLClass* klass);

////////////////////////////////////////////////////////////
// errmsg.c
////////////////////////////////////////////////////////////
void parser_err_msg(char* msg, char* sname, int sline);
void parser_err_msg_format(char* sname, int sline, char* msg, ...);
void parser_err_msg_without_line(char* msg, ...);
void show_node_type_for_errmsg(sCLNodeType* node_type);
void show_type_for_errmsg(sCLClass* klass, sCLType* type);
void show_method_for_errmsg(sCLClass* klass, sCLMethod* method);
void show_all_method_for_errmsg(sCLClass* klass, char* method_name);

////////////////////////////////////////////////////////////
// obj_file_stat.c
////////////////////////////////////////////////////////////
void initialize_hidden_class_method_of_fstat(sCLClass* klass);

CLObject create_fstat_object(sVMInfo* info);

////////////////////////////////////////////////////////////
// preprocessor.c
////////////////////////////////////////////////////////////
BOOL preprocessor(sBuf* source, sBuf* source2);
void preprocessor_init();
void preprocessor_final();

////////////////////////////////////////////////////////////
// obj_short.c
////////////////////////////////////////////////////////////
CLObject create_short_object(unsigned short value);

void initialize_hidden_class_method_of_immediate_short(sCLClass* klass);

BOOL short_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL short_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_long.c
////////////////////////////////////////////////////////////
CLObject create_long_object(unsigned long value);

BOOL long_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL long_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL long_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
void initialize_hidden_class_method_of_immediate_long(sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_uint.c
////////////////////////////////////////////////////////////
void initialize_hidden_class_method_of_immediate_uint(sCLClass* klass);

CLObject create_uint_object(unsigned int value);

BOOL uint_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL uint_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL uint_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_char.c
////////////////////////////////////////////////////////////
void initialize_hidden_class_method_of_immediate_char(sCLClass* klass);

CLObject create_char_object(wchar_t value);

BOOL char_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL char_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL char_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_double.c
////////////////////////////////////////////////////////////
BOOL double_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL double_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL double_toInt(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL double_toFloat(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

CLObject create_double_object(double value);
void initialize_hidden_class_method_of_immediate_double(sCLClass* klass);

////////////////////////////////////////////////////////////
// obj_pointer.c
////////////////////////////////////////////////////////////
void initialize_hidden_class_method_of_pointer(sCLClass* klass);

BOOL pointer_setValue(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
CLObject create_pointer_object(void* value, int size);
BOOL pointer_toString(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL pointer_forward(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);
BOOL pointer_getByte(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

////////////////////////////////////////////////////////////
// c_to_clover.c
////////////////////////////////////////////////////////////
CLObject create_number_object_from_size(size_t size, unsigned long value, char* type_name, sVMInfo* info);
unsigned long get_value_with_size(size_t size, CLObject number);

////////////////////////////////////////////////////////////
// obj_time.c
////////////////////////////////////////////////////////////
BOOL Time_Time(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass);

#endif
