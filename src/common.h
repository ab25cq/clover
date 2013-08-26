#ifndef COMMON_H
#define COMMON_H

#define CLOVER_PARAM_MAX 16

void parser_final();
void parser_init();

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

#endif
