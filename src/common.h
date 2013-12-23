#ifndef COMMON_H
#define COMMON_H

#define CLOVER_PARAM_MAX 16

//////////////////////////////////////////////////
// heap.c
//////////////////////////////////////////////////
#define OBJECT_HEADER_NUM 2
#define ARRAY_HEADER_NUM 4

#define CLEXISTENCE(obj) (object_to_ptr(obj))[0].mIntValue
#define CLCLASS(obj) (object_to_ptr(obj))[1].mClassRef
#define CLFIELD(obj, n) (object_to_ptr(obj))[2 + n]
#define CLATYPE(obj) (object_to_ptr(obj))[2].mIntValue
#define CLALEN(obj) (object_to_ptr(obj))[3].mIntValue
#define CLASTART(obj) (void*)(object_to_ptr(obj) + 4)

#define CLPEXISTENCE(data) ((MVALUE*)data)[0].mIntValue
#define CLPCLASS(data) ((MVALUE*)data)[1].mClassRef
#define CLPATYPE(data) ((MVALUE*)data)[2].mIntValue
#define CLPALEN(data) ((MVALUE*)data)[3].mIntValue
#define CLPASTART(data) (void*)(((MVALUE*)data) + 4)

void heap_init(int heap_size, int size_hadles);
void heap_final();
MVALUE* object_to_ptr(CLObject obj);
CLObject alloc_object(uint size);
CLObject create_string_object(wchar_t* str, uint len);
void cl_gc();
void show_heap();
uint object_size(sCLClass* klass);
CLObject create_object(sCLClass* klass);

//////////////////////////////////////////////////
// klass.c
//////////////////////////////////////////////////
extern sCLClass* gIntClass;      // foudamental classes
extern sCLClass* gStringClass;
extern sCLClass* gFloatClass;
extern sCLClass* gVoidClass;
extern sCLClass* gCloverClass;

sCLClass* alloc_class(char* class_name);        // result must be not NULL
void class_init(BOOL load_foundamental_class);
void class_final();
uint get_hash(char* name);
void show_class(sCLClass* klass);
void show_all_classes();
BOOL save_class(sCLClass* klass, char* file_name);
sCLClass* load_class(char* file_name);
BOOL load_object_file(char* file_name);
BOOL save_object_file(char* file_name);
void* alloc_class_part(uint size);
ALLOC uchar* native_load_class(char* file_name);
void show_constants(sConst* constant);
void alloc_bytecode(sCLMethod* method);

// result (TRUE) --> success (FALSE) --> overflow number methods or method parametor number
BOOL add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL native_, char* name, sCLClass* result_type, sCLClass* class_params[], uint num_params, BOOL constructor);

// result (TRUE) --> success (FALSE) --> overflow number fields
BOOL add_field(sCLClass* klass, BOOL static_, BOOL private_, char* name, sCLClass* type_);

// result: (NULL) --> not found (non NULL) --> field
sCLField* get_field(sCLClass* klass, char* field_name);

// result: (-1) --> not found (non -1) --> field index
int get_field_index(sCLClass* klass, char* field_name);

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method(sCLClass* klass, char* method_name);

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method_with_params(sCLClass* klass, char* method_name, sCLClass** class_params, uint num_params);

// result: (-1) --> not found (non -1) --> method index
int get_method_index(sCLClass* klass, char* method_name);

// result: (-1) --> not found (non -1) --> method index
int get_method_index_with_params(sCLClass* klass, char* method_name, sCLClass** class_params, uint num_params);

// result: (-1) --> not found (non -1) --> index
int get_method_num_params(sCLClass* klass, int method_index);

// result (NULL) --> not found (pointer of sCLClass) --> found
sCLClass* get_method_param_types(sCLClass* klass, int method_index, int param_num);

// result: (NULL) --> not found (sCLClass pointer) --> found
sCLClass* get_method_result_type(sCLClass* klass, int method_index);

// expected result_size == CL_METHOD_NAME_REAL_MAX
void make_real_method_name(char* result, uint result_size, char* name, sCLClass* class_params[], uint num_params);

//////////////////////////////////////////////////
// parser.c
//////////////////////////////////////////////////
typedef struct {
    char mName[CL_METHOD_NAME_MAX];
    uint mIndex;
    sCLClass* mClass;
} sVar;

typedef struct {
    sVar mLocalVariables[CL_LOCAL_VARIABLE_MAX];  // open address hash
    uint mVarNum;
} sVarTable;

// result: (true) success (false) overflow the table
BOOL add_variable_to_table(sVarTable* table, char* name, sCLClass* klass);

// result: (null) not found (sVar*) found
sVar* get_variable_from_table(sVarTable* table, char* name);

void parser_init(BOOL load_foundamental_class);
void parser_final();

BOOL parse_word(char* buf, int buf_size, char** p, char* sname, int* sline, int* err_num);
void skip_spaces_and_lf(char** p, int* sline);
void parser_err_msg(char* msg, char* sname, int sline);
BOOL expect_next_character(char* characters, int* err_num, char** p, char* sname, int* sline);

void sConst_init(sConst* self);
void sConst_free(sConst* self);
void sConst_append_str(sConst* constant, char* str);
void sConst_append(sConst* self, void* data, uint size);
void sConst_append_wstr(sConst* constant, char* str);

void sByteCode_init(sByteCode* self);
void sByteCode_free(sByteCode* self);

void sByteCode_append(sByteCode* self, void* code, uint size);

BOOL compile_method(sCLMethod* method, sCLClass* klass, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table, BOOL constructor);

/// resizable buffer
typedef struct {
    char* mBuf;
    uint mSize;
    uint mLen;
} sBuf;

void sBuf_init(sBuf* self);
void sBuf_append_char(sBuf* self, char c);
void sBuf_append(sBuf* self, void* str, size_t size);

extern sVarTable gGVTable;       // global variable table

//////////////////////////////////////////////////
// vm.c
//////////////////////////////////////////////////
MVALUE* object_to_ptr(CLObject obj);

extern MVALUE* gCLStack;
extern uint gCLStackSize;
extern MVALUE* gCLStackPtr;

//////////////////////////////////////////////////
// xfunc.c
//////////////////////////////////////////////////
char* xstrncpy(char* des, char* src, int size);
char* xstrncat(char* des, char* str, int size);

#define INVOKE_METHOD_OBJECT_TYPE_VOID 0
#define INVOKE_METHOD_OBJECT_TYPE_STRING 1
#define INVOKE_METHOD_OBJECT_TYPE_INT 2
#define INVOKE_METHOD_OBJECT_TYPE_FLOAT 3
#define INVOKE_METHOD_OBJECT_TYPE_OBJECT 4

#endif
