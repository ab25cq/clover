#ifndef COMMON_H
#define COMMON_H

#include <stdarg.h>

//////////////////////////////////////////////////
// heap.c
//////////////////////////////////////////////////
#define OBJECT_HEADER_NUM 2

#define CLEXISTENCE(obj) (object_to_ptr(obj))[0].mIntValue
#define CLPEXISTENCE(data) ((MVALUE*)data)[0].mIntValue

#define CLCLASS(obj) (object_to_ptr(obj))[1].mClassRef
#define CLPCLASS(data) ((MVALUE*)data)[1].mClassRef

#define CLFIELD(obj, n) (object_to_ptr(obj))[2 + n]

void heap_init(int heap_size, int size_hadles);
void heap_final();
MVALUE* object_to_ptr(CLObject obj);
CLObject alloc_object(uint size);
void cl_gc();
void show_heap();
CLObject create_object(sCLClass* klass);
CLObject alloc_heap_mem(uint size);
MVALUE* object_to_ptr(CLObject obj);
void mark_object(CLObject obj, uchar* mark_flg);

//////////////////////////////////////////////////
// klass.c
//////////////////////////////////////////////////
extern sCLNodeType gIntType;      // foudamental classes
extern sCLNodeType gFloatType;
extern sCLNodeType gVoidType;

extern sCLNodeType gStringType;
extern sCLNodeType gHashType;
extern sCLNodeType gArrayType;

extern sCLNodeType gAnonymousType[CL_GENERICS_CLASS_PARAM_MAX];;

extern sCLClass* gCloverClass;
sCLClass* alloc_class(char* namespace, char* class_name, BOOL private_, BOOL open_, char** generics_types, int generics_types_num);
BOOL add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL native_, char* name, sCLNodeType* result_type, sCLNodeType* class_params, uint num_params, BOOL constructor);
BOOL check_super_class_offsets(sCLClass* klass);
void class_init(BOOL load_foundamental_class);
void class_final();
uint get_hash(char* name);
void show_class(sCLClass* klass);
void show_all_classes();
BOOL save_class(sCLClass* klass);
void save_all_modified_class();
sCLClass* load_class_from_classpath(char* file_name);
ALLOC uchar* native_load_class(char* file_name);
void show_constants(sConst* constant);
void alloc_bytecode(sCLMethod* method);
void create_real_class_name(char* result, int result_size, char* namespace, char* class_name);
void increase_class_version(sCLClass* klass);

// result (TRUE) --> success (FALSE) --> overflow methods number or method parametor number
BOOL add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL native_, char* name, sCLNodeType* result_type, sCLNodeType* class_params, uint num_params, BOOL constructor);

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
int get_method_index(sCLClass* klass, char* method_name);

// result: (-1) --> not found (non -1) --> method index
int get_method_index_from_method_pointer(sCLClass* klass, sCLMethod* method);

// result: (-1) --> not found (non -1) --> method index
// if type_ is NULL, don't solve generics type
int get_method_index_with_type_params(sCLClass* klass, char* method_name, sCLNodeType* class_params, uint num_params, BOOL search_for_class_method, sCLNodeType* type_);

// result: (-1) --> not found (non -1) --> method index
int get_method_index_from_the_parametor_point(sCLClass* klass, char* method_name, int method_index, BOOL search_for_class_method);

// result: (-1) --> not found (non -1) --> method index
// if type_ is NULL, don't solve generics type
int get_method_index_with_type_params_from_the_parametor_point(sCLClass* klass, char* method_name, sCLNodeType* class_params, uint num_params, int method_index, BOOL search_for_class_method, sCLNodeType* type_);

// result should be found
void get_method_param_types(sCLClass* klass, sCLMethod* method, int param_num, sCLNodeType* result);

// result: (FALSE) can't solve a generics type (TRUE) success
// if type_ is NULL, don't solve generics type
BOOL get_method_result_type(sCLClass* klass, sCLMethod* method, sCLNodeType* result, sCLNodeType* type_);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class
// if type_ is NULL, don't solve generics type
sCLMethod* get_virtual_method_with_params(sCLClass* klass, char* method_name, sCLNodeType* class_params, uint num_params, sCLClass** founded_class, BOOL search_for_class_method, sCLNodeType* type_);

// result: (NULL) --> not found (non NULL) --> method
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params(sCLClass* klass, char* method_name, sCLNodeType* class_params, uint num_params, BOOL search_for_class_method, sCLNodeType* type_);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** foud_class) was setted on the method owner class.
sCLMethod* get_method_on_super_classes(sCLClass* klass, char* method_name, sCLClass** found_class);

// result: (TRUE) found (FALSE) not found
BOOL search_for_super_class(sCLClass* klass, sCLClass* searched_class);

// return method parametor number
int get_method_num_params(sCLMethod* method);

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class.
// if type_ is NULL, don't solve generics type
sCLMethod* get_method_with_type_params_on_super_classes(sCLClass* klass, char* method_name, sCLNodeType* class_params, uint num_params, sCLClass** founded_class, BOOL search_for_class_method, sCLNodeType* type_);

//////////////////////////////////////////////////
// parser.c
//////////////////////////////////////////////////
typedef struct {
    char mName[CL_METHOD_NAME_MAX];
    int mIndex;
    sCLNodeType mType;
} sVar;

typedef struct {
    sVar mLocalVariables[CL_LOCAL_VARIABLE_MAX];  // open address hash
    int mVarNum;
} sVarTable;

extern sVarTable gGVTable;       // global variable table

// result: (true) success (false) overflow the table
BOOL add_variable_to_table(sVarTable* table, char* name, sCLNodeType* type_);

// result: (null) not found (sVar*) found
sVar* get_variable_from_table(sVarTable* table, char* name);

void sBuf_init(sBuf* self);
void sBuf_append_char(sBuf* self, char c);
void sBuf_append(sBuf* self, void* str, size_t size);

void sConst_init(sConst* self);
void sConst_free(sConst* self);
void sConst_append_str(sConst* constant, char* str);
void sConst_append(sConst* self, void* data, uint size);
void sConst_append_wstr(sConst* constant, char* str);
void sConst_append_int(sConst* constant, int n);

void sByteCode_init(sByteCode* self);
void sByteCode_free(sByteCode* self);
void sByteCode_append(sByteCode* self, void* code, uint size);

void parser_init(BOOL load_foundamental_class);
void parser_final();

BOOL parse_word(char* buf, int buf_size, char** p, char* sname, int* sline, int* err_num);
void skip_spaces_and_lf(char** p, int* sline);
void skip_spaces(char** p);
void parser_err_msg(char* msg, char* sname, int sline);
void parser_err_msg_format(char* sname, int sline, char* msg, ...);
BOOL expect_next_character(char* characters, int* err_num, char** p, char* sname, int* sline);
// characters is null-terminated
void expect_next_character_with_one_forward(char* characters, int* err_num, char** p, char* sname, int* sline);

BOOL node_expression(uint* node, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, char* current_namespace, sCLClass* klass);

BOOL parse_generics_types_name(char** p, char* sname, int* sline, int* err_num, char* generics_types_num, sCLClass** generics_types, char* current_namespace, sCLClass* klass);

BOOL parse_namespace_and_class(sCLClass** result, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass);
    // result: (FALSE) there is an error (TRUE) success
    // result class is setted on first parametor
BOOL parse_namespace_and_class_and_generics_type(sCLNodeType* type, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass) ;
    // result: (FALSE) there is an error (TRUE) success
    // result type is setted on first parametor
int get_generics_type_num(sCLClass* klass, char* type_name);


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

enum eOperand { 
    kOpAdd, kOpSub, kOpMult, kOpDiv, kOpMod, kOpPlusPlus2, kOpMinusMinus2
};

typedef struct sNodeTreeStruct {
    uchar mNodeType;
    sCLNodeType mType;

    union {
        enum eOperand mOperand;
        int mValue;
        char* mStringValue;
        char* mVarName;
    };

    uint mLeft;     // node index
    uint mRight;
    uint mMiddle;
} sNodeTree;

extern sNodeTree* gNodes; // All nodes at here. Index is node number. sNodeTree_create* functions return a node number.

BOOL compile_method(sCLMethod* method, sCLClass* klass, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, BOOL constructor, char* current_namespace);
// left_type is stored calss. right_type is class of value.
BOOL type_checking(sCLNodeType* left_type, sCLNodeType* right_type);
BOOL type_checking_with_class(sCLClass* left_type, sCLClass* right_type);
BOOL type_identity(sCLNodeType* type1, sCLNodeType* type2);

// Below functions return a node number. It is an index of gNodes.
uint sNodeTree_create_operand(enum eOperand operand, uint left, uint right, uint middle);
uint sNodeTree_create_value(int value, uint left, uint right, uint middle);
uint sNodeTree_create_string_value(MANAGED char* value, uint left, uint right, uint middle);
uint sNodeTree_create_array(uint left, uint right, uint middle);
uint sNodeTree_create_var(char* var_name, sCLNodeType* klass, uint left, uint right, uint middle);
uint sNodeTree_create_define_var(char* var_name, sCLNodeType* klass, uint left, uint right, uint middle);
uint sNodeTree_create_return(sCLNodeType* klass, uint left, uint right, uint middle);
uint sNodeTree_create_class_method_call(char* var_name, sCLNodeType* klass, uint left, uint right, uint middle);
uint sNodeTree_create_class_field(char* var_name, sCLNodeType* klass, uint left, uint right, uint middle);
uint sNodeTree_create_param(uint left, uint right, uint middle);
uint sNodeTree_create_new_expression(sCLNodeType* klass, uint left, uint right, uint middle);
uint sNodeTree_create_fields(char* name, uint left, uint right, uint middle);
uint sNodeTree_create_method_call(char* var_name, uint left, uint right, uint middle);
uint sNodeTree_create_super(uint left, uint right, uint middle);
uint sNodeTree_create_inherit(uint left, uint right, uint middle);

//////////////////////////////////////////////////
// vm.c
//////////////////////////////////////////////////
extern MVALUE* gCLStack;
extern int gCLStackSize;
extern MVALUE* gCLStackPtr;

#define INVOKE_METHOD_KIND_CLASS 0
#define INVOKE_METHOD_KIND_OBJECT 1

//////////////////////////////////////////////////
// string.c
//////////////////////////////////////////////////
#define STRING_HEADER_NUM 3

#define CLSTRING_LEN(obj) (object_to_ptr(obj))[2].mIntValue
#define CLSTRING_START(obj) (wchar_t*)(object_to_ptr(obj) + 3)

CLObject alloc_string_object(uint len);
uint string_size(CLObject string);
CLObject create_string_object(wchar_t* str, uint len);

//////////////////////////////////////////////////
// array.c
//////////////////////////////////////////////////
#define ARRAY_HEADER_NUM 4

#define CLARRAY_LEN(obj) (object_to_ptr(obj))[2].mIntValue
#define CLARRAY_SIZE(obj) (object_to_ptr(obj))[3].mIntValue
#define CLARRAY_START(obj) (MVALUE*)(object_to_ptr(obj) + 4)
#define CLARRAY_ITEM(obj, n) (object_to_ptr(obj))[4 + n]

CLObject alloc_array_object(uint array_size);
uint array_size(CLObject array);
CLObject create_array_object(MVALUE elements[], uint elements_len);
void mark_array_object(CLObject object, uchar* mark_flg);

//////////////////////////////////////////////////
// xfunc.c
//////////////////////////////////////////////////
char* xstrncpy(char* des, char* src, int size);
char* xstrncat(char* des, char* str, int size);

#endif
