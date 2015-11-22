バイナリファイル src/alias.o に一致しました
バイナリファイル src/buffer.o に一致しました
src/c.cl:print("preprocessor test2...");
src/c.cl:println("TRUE");
バイナリファイル src/c_to_clover.o に一致しました
バイナリファイル src/compile.o に一致しました
src/compiler.c:printf("parse phase %d\n", i);
バイナリファイル src/debug.o に一致しました
バイナリファイル src/errmsg.o に一致しました
バイナリファイル src/heap.o に一致しました
バイナリファイル src/interface.o に一致しました
バイナリファイル src/klass.o に一致しました
src/klass_ctime.c:printf("generics_param_num %d\n", generics_param_num);
src/klass_ctime.c:printf("caller_class %s generics_param_num %d\n", REAL_CLASS_NAME(caller_class), generics_param_num);
バイナリファイル src/klass_ctime.o に一致しました
バイナリファイル src/load_class.o に一致しました
バイナリファイル src/main.o に一致しました
バイナリファイル src/module.o に一致しました
バイナリファイル src/namespace.o に一致しました
バイナリファイル src/node.o に一致しました
バイナリファイル src/node_tree.o に一致しました
src/node_type.c:puts("AAA2");
src/node_type.c:puts("BBB2");
src/node_type.c:puts("CCC2");
src/node_type.c:printf("generics_param_types->mNumImplementsTypes %d\n", generics_param_types->mNumImplementsTypes);
src/node_type.c:printf("num_implements_types2 %d\n", num_implements_types2);
src/node_type.c:puts("AAAA");
src/node_type.c:puts("BBB");
src/node_type.c:puts("CCC");
バイナリファイル src/node_type.o に一致しました
バイナリファイル src/obj_anonymous.o に一致しました
バイナリファイル src/obj_array.o に一致しました
バイナリファイル src/obj_block.o に一致しました
バイナリファイル src/obj_bool.o に一致しました
バイナリファイル src/obj_byte.o に一致しました
バイナリファイル src/obj_bytes.o に一致しました
バイナリファイル src/obj_char.o に一致しました
バイナリファイル src/obj_class_object.o に一致しました
バイナリファイル src/obj_clover.o に一致しました
バイナリファイル src/obj_double.o に一致しました
バイナリファイル src/obj_enum.o に一致しました
バイナリファイル src/obj_field.o に一致しました
バイナリファイル src/obj_file.o に一致しました
バイナリファイル src/obj_float.o に一致しました
バイナリファイル src/obj_hash.o に一致しました
バイナリファイル src/obj_int.o に一致しました
バイナリファイル src/obj_long.o に一致しました
バイナリファイル src/obj_method.o に一致しました
バイナリファイル src/obj_mutex.o に一致しました
バイナリファイル src/obj_null.o に一致しました
バイナリファイル src/obj_oniguruma_regex.o に一致しました
バイナリファイル src/obj_pointer.o に一致しました
バイナリファイル src/obj_range.o に一致しました
バイナリファイル src/obj_short.o に一致しました
バイナリファイル src/obj_string.o に一致しました
バイナリファイル src/obj_system.o に一致しました
バイナリファイル src/obj_thread.o に一致しました
バイナリファイル src/obj_tm.o に一致しました
バイナリファイル src/obj_type_object.o に一致しました
バイナリファイル src/obj_uint.o に一致しました
バイナリファイル src/obj_user_object.o に一致しました
バイナリファイル src/obj_void.o に一致しました
バイナリファイル src/obj_wait_status.o に一致しました
バイナリファイル src/parse.o に一致しました
バイナリファイル src/preprocessor.o に一致しました
src/tags:p	common.h	/^    char** p;$/;"	m	struct:sParserInfoStruct
src/tags:param_initializer	vm.c	/^static BOOL param_initializer(sConst* constant, sByteCode* code, int lv_num, int max_stack, sVMInfo* info, CLObject vm_type)$/;"	f	file:
src/tags:params_of_cl_type_to_params_of_node_type	node.c	/^static BOOL params_of_cl_type_to_params_of_node_type(ALLOC sCLNodeType** result, sCLType* params, int num_params, sCLClass* klass)$/;"	f	file:
src/tags:parent	clover.h	/^    struct sVMType* parent;$/;"	m	struct:sVMType	typeref:struct:sVMType::sVMType
src/tags:parse	compiler.c	/^static BOOL parse(sParserInfo* info, enum eCompileType compile_type, int parse_phase_num)$/;"	f	file:
src/tags:parse_alias	compiler.c	/^static BOOL parse_alias(sParserInfo* info, int parse_phase_num, int sline_top)$/;"	f	file:
src/tags:parse_annotation	parser.c	/^void parse_annotation(char** p, char* sname, int* sline, int* err_num)$/;"	f
src/tags:parse_block	compile.c	/^BOOL parse_block(unsigned int* block_id, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType* block_type, sCLMethod* method, sVarTable* lv_table)$/;"	f
src/tags:parse_block_object	compile.c	/^BOOL parse_block_object(unsigned int* block_id, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLNodeType* block_type, sCLMethod* method, sVarTable* lv_table, int sline_top, int num_params, sCLNodeType** class_params)$/;"	f
src/tags:parse_block_params	parser.c	/^static BOOL parse_block_params(sCLNodeType** class_params, int* num_params, sParserInfo* info, sVarTable* new_table, int sline_top)$/;"	f	file:
src/tags:parse_class	compiler.c	/^static BOOL parse_class(sParserInfo* info, BOOL private_, BOOL mixin_, BOOL abstract_, BOOL dynamic_typing_, BOOL final_, BOOL struct_, enum eCompileType compile_type, int parse_phase_num, BOOL interface)$/;"	f	file:
src/tags:parse_constructor	compiler.c	/^static BOOL parse_constructor(sParserInfo* info, sCLNodeType* result_type, char* name, BOOL mixin_, BOOL native_, BOOL synchronized_, BOOL generics_newable, sClassCompileData* class_compile_data, int parse_phase_num, int sline_top, BOOL interface)$/;"	f	file:
src/tags:parse_declaration_of_method_block	compiler.c	/^static BOOL parse_declaration_of_method_block(sParserInfo* info, sVarTable* lv_table, char* block_name, sCLNodeType** bt_result_type, sCLNodeType** bt_class_params, int* bt_num_params, int size_bt_class_params, int sline_top, int* block_num)$/;"	f	file:
src/tags:parse_generics_param_types	compiler.c	/^static BOOL parse_generics_param_types(sParserInfo* info, int* generics_param_types_num, sCLNodeGenericsParamTypes generics_param_types[CL_GENERICS_CLASS_PARAM_MAX], int parse_phase_num, BOOL get_param_number_only)$/;"	f	file:
src/tags:parse_generics_types_name	parser.c	/^BOOL parse_generics_types_name(char** p, char* sname, int* sline, int* err_num, char* generics_types_num, sCLNodeType** generics_types, char* current_namespace, sCLClass* klass, sCLMethod* method, BOOL skip)$/;"	f
src/tags:parse_method	compiler.c	/^static BOOL parse_method(sParserInfo* info, BOOL static_, BOOL private_, BOOL protected_, BOOL native_, BOOL mixin_, BOOL synchronized_, BOOL virtual_, BOOL abstract_, sCLNodeType* result_type, char* name, sClassCompileData* class_compile_data, int parse_phase_num, int sline_top, BOOL interface)$/;"	f	file:
src/tags:parse_module	compiler.c	/^static BOOL parse_module(sParserInfo* info, enum eCompileType compile_type, int parse_phase_num)$/;"	f	file:
src/tags:parse_namespace	compiler.c	/^static BOOL parse_namespace(sParserInfo* info, enum eCompileType compile_type, int parse_phase_num)$/;"	f	file:
src/tags:parse_namespace_and_class	parser.c	/^BOOL parse_namespace_and_class(sCLClass** result, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLMethod* method, BOOL skip, BOOL* star)$/;"	f
src/tags:parse_namespace_and_class_and_generics_type	parser.c	/^BOOL parse_namespace_and_class_and_generics_type(ALLOC sCLNodeType** type, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLMethod* method, BOOL skip)$/;"	f
src/tags:parse_namespace_and_class_and_generics_type_without_generics_check	parser.c	/^BOOL parse_namespace_and_class_and_generics_type_without_generics_check(ALLOC sCLNodeType** type, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLMethod* method, BOOL skip)$/;"	f
src/tags:parse_params	parser.c	/^BOOL parse_params(sCLNodeType** class_params, int* num_params, int size_params, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLMethod* method, sVarTable* lv_table, char close_character, int sline_top)$/;"	f
src/tags:parse_params_with_initializer	parser.c	/^BOOL parse_params_with_initializer(sCLNodeType** class_params, sByteCode* code_params, int* max_stack_params, int* lv_num_params, int* num_params, int size_params, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, char close_character, int sline_top, BOOL* variable_arguments)$/;"	f
src/tags:parse_quote	parser.c	/^static void parse_quote(char** p, int* sline, BOOL* quote)$/;"	f	file:
src/tags:parse_throws	compiler.c	/^static BOOL parse_throws(sParserInfo* info, sCLClass* exception_class[CL_METHOD_EXCEPTION_MAX], int* exception_num)$/;"	f	file:
src/tags:parse_word	parser.c	/^BOOL parse_word(char* buf, int buf_size, char** p, char* sname, int* sline, int* err_num, BOOL print_out_err_msg)$/;"	f
src/tags:parser_err_msg	parser.c	/^void parser_err_msg(char* msg, char* sname, int sline_top)$/;"	f
src/tags:parser_err_msg_format	parser.c	/^void parser_err_msg_format(char* sname, int sline, char* msg, ...)$/;"	f
src/tags:parser_operator_method_name	compiler.c	/^static void parser_operator_method_name(char* name, int name_size, sParserInfo* info)$/;"	f	file:
src/tags:pop_object	vm.c	/^CLObject pop_object(sVMInfo* info)$/;"	f
src/tags:pop_object_except_top	vm.c	/^void pop_object_except_top(sVMInfo* info)$/;"	f
src/tags:pop_object_n	vm.c	/^void pop_object_n(sVMInfo* info, int n)$/;"	f
src/tags:pop_vminfo	vm.c	/^static void pop_vminfo(sVMInfo* info)$/;"	f	file:
src/tags:postposition_operator	parser.c	/^static BOOL postposition_operator(unsigned int* node, sParserInfo* info, int sline_top, BOOL* quote, sVarTable* lv_table)$/;"	f	file:
src/tags:prepare_for_break_labels	node.c	/^static void prepare_for_break_labels(unsigned int** break_labels_before, int** break_labels_len_before, unsigned int break_labels[], int* break_labels_len, sCompileInfo* info)$/;"	f	file:
src/tags:prepare_for_continue_labels	node.c	/^static void prepare_for_continue_labels(unsigned int** continue_labels_before, int** continue_labels_len_before, unsigned int continue_labels[], int* continue_labels_len, sCompileInfo* info)$/;"	f	file:
src/tags:push_object	vm.c	/^void push_object(CLObject object, sVMInfo* info)$/;"	f
src/tags:push_vminfo	vm.c	/^void push_vminfo(sVMInfo* info)$/;"	f
src/tags:put_fun_to_hash	klass.c	/^static void put_fun_to_hash(char* path, fNativeMethod fun)$/;"	f	file:
src/tags:put_to_array	obj_array.c	/^static void put_to_array(CLObject self, int index, CLObject item)$/;"	f	file:
バイナリファイル src/type.o に一致しました
バイナリファイル src/utf_mb_str.o に一致しました
バイナリファイル src/vm.o に一致しました
バイナリファイル src/vtable.o に一致しました
バイナリファイル src/xfunc.o に一致しました
