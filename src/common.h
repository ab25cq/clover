#ifndef COMMON_H
#define COMMON_H

#define CLOVER_PARAM_MAX 16

void parser_init(BOOL load_foundamental_class);
void parser_final();

MVALUE* cl_object_to_ptr(CLObject obj) ;

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

void class_init(BOOL load_foundamental_class);

// result: (-1) --> not found (non -1) --> index
uint cl_get_method_num_params(sCLClass* klass, uint method_index);

// result (NULL) --> not found (pointer of sCLClass) --> found
sCLClass* cl_get_method_param_types(sCLClass* klass, uint method_index, uint param_num);

// result: (NULL) --> not found (sCLClass pointer) --> found
sCLClass* cl_get_method_result_type(sCLClass* klass, uint method_index);

void show_constants(sConst* constant);
void sConst_append_str(sConst* constant, uchar* str);
void sConst_append(sConst* self, void* data, uint size);
void sConst_append_wstr(sConst* constant, uchar* str);

#endif
