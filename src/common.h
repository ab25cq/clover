#ifndef COMMON_H
#define COMMON_H

#define CLOVER_PARAM_MAX 16
#define OBJECT_HEADER_NUM 4

#define CLEXISTENCE(obj) (cl_object_to_ptr(obj))[0].mIntValue
#define CLCLASS(obj) (cl_object_to_ptr(obj))[1].mClassRef
#define CLATYPE(obj) (cl_object_to_ptr(obj))[2].mIntValue
#define CLALEN(obj) (cl_object_to_ptr(obj))[3].mIntValue
#define CLASTART(obj) (void*)(cl_object_to_ptr(obj) + 4)

#define CLPEXISTENCE(data) ((MVALUE*)data)[0].mIntValue
#define CLPCLASS(data) ((MVALUE*)data)[1].mClassRef
#define CLPATYPE(data) ((MVALUE*)data)[2].mIntValue
#define CLPALEN(data) ((MVALUE*)data)[3].mIntValue
#define CLPASTART(data) (void*)(((MVALUE*)data) + 4)

//////////////////////////////////////////////////
// klass.c
//////////////////////////////////////////////////
extern sCLClass* gIntClass;      // foudamental classes
extern sCLClass* gStringClass;
extern sCLClass* gFloatClass;
extern sCLClass* gVoidClass;
extern sCLClass* gCloverClass;

#define CLASS_HASH_SIZE 128

sCLClass* alloc_class(uchar* class_name);        // result must be not NULL
void class_init(BOOL load_foundamental_class);
uint get_hash(uchar* name);
void show_class(sCLClass* klass);
void* alloc_class_part(uint size);
ALLOC uchar* native_load_class(uchar* file_name);
void show_constants(sConst* constant);

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

// result: (-1) --> not found (non -1) --> method index
int get_method_index(sCLClass* klass, uchar* method_name);

// result: (-1) --> not found (non -1) --> index
int get_method_num_params(sCLClass* klass, uint method_index);

// result (NULL) --> not found (pointer of sCLClass) --> found
sCLClass* get_method_param_types(sCLClass* klass, uint method_index, uint param_num);

// result: (NULL) --> not found (sCLClass pointer) --> found
sCLClass* get_method_result_type(sCLClass* klass, uint method_index);

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
void sConst_append_str(sConst* constant, uchar* str);
void sConst_append(sConst* self, void* data, uint size);
void sConst_append_wstr(sConst* constant, uchar* str);

void sByteCode_init(sByteCode* self);
BOOL compile_method(sCLMethod* method, sCLClass* klass, char** p, char* sname, int* sline, int* err_num, sVarTable* lv_table);

extern sVarTable gGVTable;       // global variable table

//////////////////////////////////////////////////
// vm.c
//////////////////////////////////////////////////
MVALUE* cl_object_to_ptr(CLObject obj);

#endif
