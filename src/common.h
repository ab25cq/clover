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
#define CLASS_HASH_SIZE 128

sCLClass* alloc_class(uchar* class_name);        // result must be not NULL
void class_init(BOOL load_foundamental_class);
uint get_hash(uchar* name);
void show_class(sCLClass* klass);
void* alloc_class_part(uint size);
ALLOC uchar* native_load_class(uchar* file_name);
void show_constants(sConst* constant);

//////////////////////////////////////////////////
// compiler.c
//////////////////////////////////////////////////
typedef struct {
    uchar* mFieldName[CL_METHOD_NAME_MAX];
    uint mIndex;
    sCLClass* mClass;
} sField;

typedef struct {
    uchar* mClassName[CL_CLASS_NAME_MAX];
    sField mField[CL_FIELDS_MAX];
    uint mFieldNum;
} sFieldTable;

sFieldTable* get_field_table(uchar* class_name);
sField* get_field(sFieldTable* table, uchar* field_name);

//////////////////////////////////////////////////
// parser.c
//////////////////////////////////////////////////
void parser_init(BOOL load_foundamental_class);
void parser_final();
void skip_spaces_and_lf(char** p, uint* sline);
void parser_err_msg(char* msg, char* sname, int sline);
uint get_method_index(sCLClass* klass, uchar* method_name);
BOOL expect_next_character(uchar* characters, int* err_num, char** p, char* sname, int* sline);

void sConst_init(sConst* self);
void sConst_append_str(sConst* constant, uchar* str);
void sConst_append(sConst* self, void* data, uint size);
void sConst_append_wstr(sConst* constant, uchar* str);

void sByteCode_init(sByteCode* self);
BOOL compile_method(sCLMethod* method, sCLClass* klass, char** p, char* sname, int* sline, int* err_num, sFieldTable* field_table);

//////////////////////////////////////////////////
// vm.c
//////////////////////////////////////////////////
MVALUE* cl_object_to_ptr(CLObject obj);

#endif
