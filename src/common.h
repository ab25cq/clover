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

void heap_init(int heap_size, int num_handles);
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

sCLClass* alloc_class(uchar* class_name);        // result must be not NULL
void class_init(BOOL load_foundamental_class);
uint get_hash(uchar* name);
void show_class(sCLClass* klass);
void* alloc_class_part(uint size);
ALLOC uchar* native_load_class(uchar* file_name);
void show_constants(sConst* constant);
void alloc_bytecode(sCLMethod* method);

/// result: true --> success, false --> failed to write
BOOL save_class(sCLClass* klass, uchar* file_name);

// result: (null) --> file not found (class pointer) --> success
sCLClass* load_class(uchar* file_name);

// result (TRUE) --> success (FALSE) --> overflow number methods or method parametor number
BOOL add_method(sCLClass* klass, BOOL static_, BOOL private_, BOOL native_, uchar* name, sCLClass* result_type, sCLClass* class_params[], uint num_params);

// result (TRUE) --> success (FALSE) --> overflow number fields
BOOL add_field(sCLClass* klass, BOOL static_, BOOL private_, uchar* name, sCLClass* type_);

// result: (NULL) --> not found (non NULL) --> field
sCLField* get_field(sCLClass* klass, uchar* field_name);

// result: (-1) --> not found (non -1) --> field index
int get_field_index(sCLClass* klass, uchar* field_name);

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method(sCLClass* klass, uchar* method_name);

// result: (NULL) --> not found (non NULL) --> method
sCLMethod* get_method_with_params(sCLClass* klass, uchar* method_name, sCLClass** class_params, uint num_params);

// result: (-1) --> not found (non -1) --> method index
int get_method_index(sCLClass* klass, uchar* method_name);

// result: (-1) --> not found (non -1) --> method index
int get_method_index_with_params(sCLClass* klass, uchar* method_name, sCLClass** class_params, uint num_params);

// result: (-1) --> not found (non -1) --> index
int get_method_num_params(sCLClass* klass, uint method_index);

// result (NULL) --> not found (pointer of sCLClass) --> found
sCLClass* get_method_param_types(sCLClass* klass, uint method_index, uint param_num);

// result: (NULL) --> not found (sCLClass pointer) --> found
sCLClass* get_method_result_type(sCLClass* klass, uint method_index);

// expected result_size == CL_METHOD_NAME_REAL_MAX
void make_real_method_name(char* result, uint result_size, char* name, sCLClass* class_params[], uint num_params);

//////////////////////////////////////////////////
// parser.c
//////////////////////////////////////////////////
typedef struct {
    uchar* mName[CL_METHOD_NAME_MAX];
    uint mIndex;
    sCLClass* mClass;
} sVar;

typedef struct {
    sVar mLocalVariables[CL_LOCAL_VARIABLE_MAX];  // open address hash
    uint mVarNum;
} sVarTable;

// result: (true) success (false) overflow the table
BOOL add_variable_to_table(sVarTable* table, uchar* name, sCLClass* klass);

// result: (null) not found (sVar*) found
sVar* get_variable_from_table(sVarTable* table, uchar* name);

void parser_init(BOOL load_foundamental_class);
void parser_final();
void skip_spaces_and_lf(char** p, uint* sline);
void parser_err_msg(char* msg, char* sname, int sline);
BOOL expect_next_character(uchar* characters, int* err_num, char** p, char* sname, int* sline);

void sConst_init(sConst* self);
void sConst_free(sConst* self);
void sConst_append_str(sConst* constant, uchar* str);
void sConst_append(sConst* self, void* data, uint size);
void sConst_append_wstr(sConst* constant, uchar* str);

void sByteCode_init(sByteCode* self);
void sByteCode_free(sByteCode* self);
BOOL compile_method(sCLMethod* method, sCLClass* klass, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table);

void sByteCode_append(sByteCode* self, void* code, uint size);

extern sVarTable gGVTable;       // global variable table

//////////////////////////////////////////////////
// vm.c
//////////////////////////////////////////////////
MVALUE* object_to_ptr(CLObject obj);

extern MVALUE* gCLStack;
extern uint gCLStackSize;
extern MVALUE* gCLStackPtr;

#endif
