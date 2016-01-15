#include "clover.h"
#include "common.h"
#include <ctype.h>
#include <limits.h>
#include <wchar.h>

//////////////////////////////////////////////////
// for parser
//////////////////////////////////////////////////
sCLNodeType* gParserGetClassType = NULL;
BOOL gParserInputingPath = FALSE;
unsigned int gParserLastNode = -1;
sVarTable* gParserVarTable = NULL;

//////////////////////////////////////////////////
// general parse tools
//////////////////////////////////////////////////
/*
// skip spaces
void skip_spaces(char** p)
{
    while(**p == ' ' || **p == '\t') {
        (*p)++;
    }
}
*/

void skip_spaces_and_lf(char** p, int* sline)
{
    while(**p == ' ' || **p == '\t' || (**p == '\n' && (*sline)++)) {
        (*p)++;
    }
}

void compile_error(char* msg, ...)
{
    char msg2[1024];

    va_list args;
    va_start(args, msg);
    vsnprintf(msg2, 1024, msg, args);
    va_end(args);

    fprintf(stderr, "%s", msg2);
}

BOOL parse_word(char* buf, int buf_size, char** p, char* sname, int* sline, int* err_num, BOOL print_out_err_msg)
{
    char* p2;

    buf[0] = 0;

    p2 = buf;

    if(isalpha(**p)) {
        while(isalnum(**p) || **p == '_') {
            if(p2 - buf < buf_size-1) {
                *p2++ = **p;
                (*p)++;
            }
            else {
                if(print_out_err_msg) {
                    parser_err_msg_format(sname, *sline, "length of word is too long");
                }
                return FALSE;
            }
        }
    }

    *p2 = 0;

    if(**p == 0 && buf[0] == 0) {
        if(print_out_err_msg) {
            parser_err_msg_format(sname, *sline, "require word(alphabet or number). this is the end of source");
        }
        return FALSE;
    }

    if(buf[0] == 0) {
        if(print_out_err_msg) {
            parser_err_msg_format(sname, *sline, "require word(alphabet or _ or number). this is (%c)", **p);
            (*err_num)++;
        }

        if(**p == '\n') (*sline)++;

        (*p)++;
    }

    return TRUE;
}

// characters is null-terminated
void expect_next_character_with_one_forward(char* characters, int* err_num, char** p, char* sname, int* sline)
{
    BOOL found;
    char* p2;

    skip_spaces_and_lf(p, sline);

    found = FALSE;
    p2 = characters;
    while(*p2) {
        if(**p == *p2) {
            found = TRUE;
        }
        p2++;
    }

    if(found) {
        (*p)++;
        skip_spaces_and_lf(p, sline);
    }
    else {
        parser_err_msg_format(sname, *sline, "expected that next character is %s", characters);
        (*err_num)++;
        (*p)++;
        skip_spaces_and_lf(p, sline);
    }
}

// characters is null-terminated
BOOL expect_next_character(char* characters, int* err_num, char** p, char* sname, int* sline)
{
    char err_characters[128];
    char* perr_characters;
    BOOL err;
    int sline_top;

    sline_top = *sline;

    perr_characters = err_characters;
    
    err = FALSE;
    while(1) {
        BOOL found;
        char* p2;

        if(**p == 0) {
            parser_err_msg_format(sname, sline_top, "clover has expected that next characters are '%s', but it arrived at source end", characters);
            (*err_num)++;
            return FALSE;
        }

        found = FALSE;
        p2 = characters;
        while(*p2) {
            if(**p == *p2) {
                found = TRUE;
                break;
            }
            else {
                p2++;
            }
        }

        if(found) {
            break;
        }
        else {
            err = TRUE;
            if(perr_characters - err_characters < 127) {
                *perr_characters = **p;
                perr_characters++;
            }
            if(**p == '\n') { (*sline)++; }
            (*p)++;
        }
    }

    if(err) {
        *perr_characters = 0;
        parser_err_msg_format(sname, sline_top, "Clover has expected that next characters are '%s', but there are some characters(%s) before them", characters, err_characters);
        (*err_num)++;
        (*p)++;
    }

    return TRUE;
}

/// result (-1): not found (>=0): found the generic type num
int get_generics_type_num(sCLClass* klass, char* type_name)
{
    int generics_type_num;

    /// get generics num ///
    generics_type_num = -1;

    if(klass != NULL) {
        if(klass->mGenericsTypesNum > 0) {
            int i;

            for(i=0; i<klass->mGenericsTypesNum; i++) {
                if(strcmp(type_name, CONS_str(&klass->mConstPool, klass->mGenericsTypes[i].mNameOffset)) == 0) {
                    generics_type_num = i;
                    break;
                }
            }
        }
    }

    return generics_type_num;
}

/// result (-1): not found (>=0): found the generic type num
int get_generics_type_num_of_method_scope(sCLClass* klass, sCLMethod* method, char* type_name)
{
    int generics_type_num;

    /// get generics num ///
    generics_type_num = -1;

    if(klass != NULL && method != NULL) {
        if(method->mGenericsTypesNum > 0) {
            int i;

            for(i=0; i<method->mGenericsTypesNum; i++) {
                if(strcmp(type_name, CONS_str(&klass->mConstPool, method->mGenericsTypes[i].mNameOffset)) == 0) 
                {
                    generics_type_num = i;
                    break;
                }
            }
        }
    }

    return generics_type_num;
}

static void add_dependences(sCLClass* klass, sCLClass* loaded_class)
{
    /// compiling script(cl) file ///
    if(klass == NULL) {
        if(loaded_class) {
            add_loaded_class_to_table(loaded_class);
        }
    }
    /// compiling class script(clc) file ///
    else {
        if(loaded_class) {
            add_dependence_class(klass, loaded_class);
        }
    }
}

static void class_not_found(char* namespace, char* class_name, sCLClass** result, char* sname, int* sline, int* err_num, sCLClass* klass, int parametor_num)
{
    *result = load_class_with_namespace_on_compile_time(namespace, class_name, TRUE, parametor_num, -1);

    if(*result == NULL) {
        parser_err_msg_format(sname, *sline, "can't solve this class name(%s::%s)", namespace, class_name);
        (*err_num)++;
    }
}

// result: (FALSE) there is an error (TRUE) success
// result class is setted on first parametor
BOOL parse_namespace_and_class(sCLClass** result, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLMethod* method, BOOL skip, BOOL* star, BOOL* self_class)
{
    char buf[WORDSIZ];
    int generics_type_num;
    int generics_type_num_of_method_scope;

    /// a first word ///
    if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) {
        return FALSE;
    }
    skip_spaces_and_lf(p, sline);

    /// get generics type num ///
    generics_type_num = get_generics_type_num(klass, buf);

    /// it is a generics type ///
    if(generics_type_num >= 0) {
        *result = gGParamTypes[generics_type_num]->mClass;
    }
    /// special class names ///
    else if(strcmp(buf, "Self") == 0) {
        if(klass == NULL) {
            parser_err_msg_format(sname, *sline, "Self is NULL");
            return FALSE;
        }
        *result = klass;
        *self_class = TRUE;
    }
    /// it is not a generics type ///
    else {
        /// a second word ///
        if(**p == ':' && *(*p + 1) == ':') {
            char buf2[WORDSIZ];
            int parametor_num;

            (*p)+=2;
            skip_spaces_and_lf(p, sline);

            if(!parse_word(buf2, WORDSIZ, p, sname, sline, err_num, TRUE)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(**p == '$') {
                char num[128];
                char* p2;

                (*p)++;
                skip_spaces_and_lf(p, sline);

                p2 = num;
                while(isdigit(**p)) {
                    *p2++ = **p;
                    (*p)++;

                    if(p2 - num >= 128) {
                        parser_err_msg_format(sname, *sline, "overflow generics param numbre");
                        return FALSE;
                    }
                }
                *p2 = 0;

                skip_spaces_and_lf(p, sline);

                parametor_num = atoi(num);

                if(parametor_num >= CL_GENERICS_CLASS_PARAM_MAX) {
                    parser_err_msg_format(sname, *sline, "invalid generics parametor num");
                    (*err_num)++;
                }
            }
            else if(**p == '<') {
                /// parse foward to get generics parametor number ///
                
                char* p_saved;
                int sline_saved;
                sCLNodeType* type_tmp;

                p_saved = *p;
                sline_saved = *sline;
                type_tmp = alloc_node_type();

                if(!parse_generics_types_name(p, sname, sline, err_num, &type_tmp->mGenericsTypesNum, type_tmp->mGenericsTypes, current_namespace, klass, method, TRUE)) 
                {
                    return FALSE;
                }

                *p = p_saved;
                *sline = sline_saved;
                parametor_num = type_tmp->mGenericsTypesNum;

                *result = cl_get_class_with_argument_namespace_only(buf, buf2, parametor_num);
            }
            else {
                *result = cl_get_class_with_argument_namespace_only(buf, buf2, 0);

                parametor_num = 0;
            }

            if(!skip) {
                if(*result == NULL) {
                    class_not_found(buf, buf2, result, sname, sline, err_num, klass, parametor_num);
                }

                add_dependences(klass, *result);
            }
        }
        else {
            int parametor_num;

            if(**p == '$') {
                char num[128];
                char* p2;

                (*p)++;

                p2 = num;
                while(isdigit(**p)) {
                    *p2++ = **p;
                    (*p)++;

                    if(p2 - num >= 128) {
                        parser_err_msg_format(sname, *sline, "overflow generics param numbre");
                        return FALSE;
                    }
                }
                *p2 = 0;

                skip_spaces_and_lf(p, sline);

                parametor_num = atoi(num);

                if(parametor_num >= CL_GENERICS_CLASS_PARAM_MAX) {
                    parser_err_msg_format(sname, *sline, "invalid generics parametor num");
                    (*err_num)++;
                }
            }
            else if(**p == '<') {
                /// parse foward to get generics parametor number ///
                
                char* p_saved;
                int sline_saved;
                sCLNodeType* type_tmp;

                p_saved = *p;
                sline_saved = *sline;
                type_tmp = alloc_node_type();

                if(!parse_generics_types_name(p, sname, sline, err_num, &type_tmp->mGenericsTypesNum, type_tmp->mGenericsTypes, current_namespace, klass, method, TRUE)) 
                {
                    return FALSE;
                }

                *p = p_saved;
                *sline = sline_saved;
                parametor_num = type_tmp->mGenericsTypesNum;

                *result = cl_get_class_with_namespace(current_namespace, buf, parametor_num);
            }
            else {
                *result = cl_get_class_with_namespace(current_namespace, buf, 0);

                parametor_num = 0;
            }

            if(!skip) {
                if(*result == NULL) {
                    class_not_found(current_namespace, buf, result, sname, sline, err_num, klass, parametor_num);
                }

                add_dependences(klass, *result);
            }
        }

        if(**p == '*') {
            (*p)++;
            skip_spaces_and_lf(p, sline);

            *star = TRUE;
        }
    }

    return TRUE;
}

BOOL parse_module_name(sCLModule** result, char** p, char* sname, int* sline, int* err_num, char* current_namespace, BOOL no_err_msg)
{
    char buf[WORDSIZ];
    char buf2[WORDSIZ];

    /// module name or namespace name///
    if(!parse_word(buf, WORDSIZ, p, sname, sline, err_num, TRUE)) 
    {
        return FALSE;
    }
    skip_spaces_and_lf(p, sline);

    if(**p == ':' && *(*p+1) == ':') {
        (*p) += 2;
        skip_spaces_and_lf(p, sline);

        /// module name ///
        if(!parse_word(buf2, WORDSIZ, p, sname, sline, err_num, TRUE)) 
        {
            return FALSE;
        }
        skip_spaces_and_lf(p, sline);

        *result = get_module(buf, buf2);

        if(*result == NULL) {
            parser_err_msg_format(sname, *sline, "Clover can't found this module(%s::%s)", buf, buf2);
            (*err_num)++;
        }
    }
    else {
        *result = get_module(current_namespace, buf);

        if(*result == NULL) {
            *result = get_module("", buf);

            if(!no_err_msg && *result == NULL) {
                parser_err_msg_format(sname, *sline, "Clover can't found this module(%s)", buf);
                (*err_num)++;
            }
        }
    }

    return TRUE;
}

BOOL parse_generics_types_name(char** p, char* sname, int* sline, int* err_num, char* generics_types_num, sCLNodeType** generics_types, char* current_namespace, sCLClass* klass, sCLMethod* method, BOOL skip)
{
    if(**p == '<') {
        (*p)++;
        skip_spaces_and_lf(p, sline);

        while(1) {
            sCLNodeType* node_type;

            if(!parse_namespace_and_class_and_generics_type(ALLOC &node_type, p, sname, sline, err_num, current_namespace, klass, method, skip))
            {
                return FALSE;
            }

            generics_types[*generics_types_num] = MANAGED node_type;
            (*generics_types_num)++;

            if(*generics_types_num > CL_GENERICS_CLASS_PARAM_MAX) {
                parser_err_msg_format(sname, *sline, "Overflow generics types number");
                return FALSE;
            }

            if(**p == 0) {
                parser_err_msg_format(sname, *sline, "It arrived at the end of source before > closing");
                return FALSE;
            }
            else if(**p == '>') {
                break;
            }
            else {
                expect_next_character_with_one_forward(",", err_num, p, sname, sline);
            }
        }

        expect_next_character_with_one_forward(">", err_num, p, sname, sline);
    }
    else {
        *generics_types_num = 0;
    }

    return TRUE;
}

void parse_annotation(char** p, char* sname, int* sline, int* err_num)
{
    if(**p == '@') {
        (*p)++;

        while(isalpha(**p) || **p == '_') {
            (*p)++;
        }
    }
    skip_spaces_and_lf(p, sline);
}

// result: (FALSE) there is an error (TRUE) success
// result type is setted on first parametor
BOOL parse_namespace_and_class_and_generics_type(ALLOC sCLNodeType** type, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLMethod* method, BOOL skip)
{
    BOOL star;
    BOOL self_class;

    star = FALSE;
    self_class = FALSE;
    *type = alloc_node_type();

    if(!parse_namespace_and_class(&(*type)->mClass, p, sname, sline, err_num, current_namespace, klass, method, skip, &star, &self_class))
    {
        return FALSE;
    }

    (*type)->mStar = star;

    if(self_class) {
        int i;

        (*type)->mGenericsTypesNum = klass->mGenericsTypesNum;
        for(i=0; i<klass->mGenericsTypesNum; i++) {
            (*type)->mGenericsTypes[i] = alloc_node_type();
            (*type)->mGenericsTypes[i]->mClass = gGParamClass[i];
            (*type)->mGenericsTypes[i]->mGenericsTypesNum = 0;
        }
    }
    else {
        if(!parse_generics_types_name(p, sname, sline, err_num, &(*type)->mGenericsTypesNum, (*type)->mGenericsTypes, current_namespace, klass, method, skip))
        {
            return FALSE;
        }
    }

    parse_annotation(p, sname, sline, err_num);

    if(!skip && (*type)->mClass && gParsePhaseNum >= PARSE_PHASE_ADD_METHODS_AND_FIELDS) {
        if(!check_valid_generics_type(*type, sname, sline, err_num, klass, method)) {
            return FALSE;
        }
    }

    if(star && !skip && (*type)->mClass) {
        if(!check_valid_star_type((*type)->mClass)) {
            parser_err_msg_format(sname, *sline, "%s does not define clone method", REAL_CLASS_NAME((*type)->mClass));
            (*err_num)++;
        }
    }

    return TRUE;
}

static BOOL parse_namespace_and_class_and_generics_type_without_generics_check(ALLOC sCLNodeType** type, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLMethod* method, BOOL skip)
{
    BOOL star;
    BOOL self_class;

    star = FALSE;
    self_class = FALSE;

    *type = alloc_node_type();

    if(!parse_namespace_and_class(&(*type)->mClass, p, sname, sline, err_num, current_namespace, klass, method, skip, &star, &self_class))
    {
        return FALSE;
    }

    (*type)->mStar = star;

    if(self_class) {
        int i;

        (*type)->mGenericsTypesNum = klass->mGenericsTypesNum;
        for(i=0; i<klass->mGenericsTypesNum; i++) {
            (*type)->mGenericsTypes[i] = alloc_node_type();
            (*type)->mGenericsTypes[i]->mClass = gGParamClass[i];
            (*type)->mGenericsTypes[i]->mGenericsTypesNum = 0;
        }
    }
    else {
        if(!parse_generics_types_name(p, sname, sline, err_num, &(*type)->mGenericsTypesNum, (*type)->mGenericsTypes, current_namespace, klass, method, skip))
        {
            return FALSE;
        }
    }

    return TRUE;
}

static void parse_quote(char** p, int* sline, BOOL* quote)
{
    if(**p == '\\') {
        (*p)++;
        skip_spaces_and_lf(p, sline);

        *quote = TRUE;
    }
    else {
        *quote = FALSE;
    }
}

BOOL delete_comment(sBuf* source, sBuf* source2)
{
    char* p;
    BOOL in_string;

    p = source->mBuf;

    in_string = FALSE;

    while(*p) {
        if(*p == '"') {
            in_string = !in_string;
            sBuf_append_char(source2, *p);
            p++;
        }
        else if(!in_string && ((p == source->mBuf && *p =='/' && *(p+1) == '/')
            || (*p != 'r' && *(p+1) == '/' && *(p+2) == '/')))
        {
            if(*p == '\n') {
                sBuf_append_char(source2, '\n');   // no delete line field for error message
            }

            if(p == source->mBuf) {
                p+=2;
            }
            else {
                p+=3;
            }

            while(1) {
                if(*p == 0) {
                    break;
                }
                else if(*p == '\n') {
                    //p++;      // no delete line field for error message
                    break;
                }
                else {
                    p++;
                }
            }
        }
        else if(!in_string && *p == '/' && *(p+1) == '*') {
            int nest;

            p+=2;
            nest = 0;
            while(1) {
                if(*p == '"') {
                    p++;
                    in_string = !in_string;
                }
                else if(*p == 0) {
                    compile_error("there is not a comment end until source end\n");
                    return FALSE;
                }
                else if(!in_string && *p == '/' && *(p+1) == '*') {
                    p+=2;
                    nest++;
                }
                else if(!in_string && *p == '*' && *(p+1) == '/') {
                    p+=2;
                    if(nest == 0) {
                        break;
                    }

                    nest--;
                }
                else if(*p == '\n') {
                    sBuf_append_char(source2, *p);   // no delete line field for error message
                    p++;
                }
                else {
                    p++;
                }
            }
        }
        else {
            sBuf_append_char(source2, *p);
            p++;
        }
    }

    return TRUE;
}

BOOL parse_params(sCLNodeType** class_params, int* num_params, int size_params, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLClass* klass, sCLMethod* method, sVarTable* lv_table, char close_character, int sline_top)
{
    if(**p == close_character) {
        (*p)++;
        skip_spaces_and_lf(p, sline);
    }
    else {
        while(1) {
            sCLNodeType* param_type;
            char param_name[CL_METHOD_NAME_MAX+1];
            char close_characters[64];

            /// class and generics types ///
            if(!parse_namespace_and_class_and_generics_type(ALLOC &param_type, p, sname, sline, err_num, current_namespace, klass, method, FALSE)) 
            {
                return FALSE;
            }

            /// name ///
            if(!parse_word(param_name, CL_METHOD_NAME_MAX, p, sname, sline, err_num, TRUE)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(param_type->mClass) {
                class_params[*num_params] = MANAGED param_type;
                (*num_params)++;

                if(*num_params >= size_params) {
                    parser_err_msg_format(sname, sline_top, "overflow param number");
                    (*err_num)++;
                    return TRUE;
                }

                if(lv_table) {
                    if(!add_variable_to_table(lv_table, param_name, param_type)) {
                        parser_err_msg_format(sname, sline_top, "there is a same name variable(%s) or overflow local variable table", param_name);

                        (*err_num)++;
                        return TRUE;
                    }
                }
            }

            snprintf(close_characters, 64, "%c,", close_character);
            if(!expect_next_character(close_characters, err_num, p, sname, sline)) {
                return FALSE;
            }

            if(**p == close_character) {
                (*p)++;
                skip_spaces_and_lf(p, sline);
                break;
            }
            else if(**p == ',') {
                (*p)++;
                skip_spaces_and_lf(p, sline);
            }
        }
    }

    return TRUE;
}

BOOL parse_params_with_initializer(sCLNodeType** class_params, ALLOC sByteCode* code_params, int* max_stack_params, int* lv_num_params, int* num_params, int size_params, char** p, char* sname, int* sline, int* err_num, char* current_namespace, sCLNodeType* klass, sCLMethod* method, sVarTable* lv_table, char close_character, int sline_top, BOOL* variable_arguments, BOOL parse_only_to_param_initializer)
{
    *variable_arguments = FALSE;

    if(**p == close_character) {
        (*p)++;
        skip_spaces_and_lf(p, sline);
    }
    else {
        while(1) {
            sCLNodeType* param_type;
            char param_name[CL_METHOD_NAME_MAX+1];
            char close_characters[64];
            sByteCode initializer;
            sCLNodeType* initializer_code_type;
            int max_stack;
            int lv_num;

            param_type = NULL;

            /// class and generics types ///
            if(!parse_namespace_and_class_and_generics_type(ALLOC &param_type, p, sname, sline, err_num, current_namespace, klass ? klass->mClass:NULL, method, FALSE)) 
            {
                return FALSE;
            }

            /// name ///
            if(!parse_word(param_name, CL_METHOD_NAME_MAX, p, sname, sline, err_num, TRUE)) {
                return FALSE;
            }
            skip_spaces_and_lf(p, sline);

            if(**p == '=') {
                (*p)++;
                skip_spaces_and_lf(p, sline);

                if(!compile_param_initializer(ALLOC &initializer, ALLOC &initializer_code_type, &max_stack, &lv_num, klass, p, sname, sline, err_num, current_namespace, parse_only_to_param_initializer))
                {
                    return FALSE;
                }

                /// type checking
                if(!parse_only_to_param_initializer && (param_type->mClass == NULL || !substitution_posibility(param_type, initializer_code_type))) 
                {
                    parser_err_msg_format(sname, *sline, "type error");
                    parser_err_msg_without_line("left type is ");
                    show_node_type_for_errmsg(param_type);
                    parser_err_msg_without_line(". right type is ");
                    show_node_type_for_errmsg(initializer_code_type);
                    parser_err_msg_without_line("\n");

                    (*err_num)++;
                }
            }
            else {
                memset(&initializer, 0, sizeof(sByteCode));
                max_stack = 0;
                lv_num = 0;
            }

            if(param_type->mClass) {
                class_params[*num_params] = param_type;
                code_params[*num_params] = MANAGED initializer;
                max_stack_params[*num_params] = max_stack;
                lv_num_params[*num_params] = lv_num;

                (*num_params)++;

                if(*num_params >= size_params) {
                    parser_err_msg_format(sname, sline_top, "overflow param number");
                    (*err_num)++;
                    return TRUE;
                }

                if(lv_table) {
                    if(!add_variable_to_table(lv_table, param_name, param_type)) {
                        parser_err_msg_format(sname, sline_top, "there is a same name variable(%s) or overflow local variable table", param_name);

                        (*err_num)++;
                        return TRUE;
                    }
                }
            }

            snprintf(close_characters, 64, "%c,", close_character);
            if(!expect_next_character(close_characters, err_num, p, sname, sline)) {
                return FALSE;
            }

            if(**p == close_character) {
                (*p)++;
                skip_spaces_and_lf(p, sline);
                break;
            }
            else if(**p == ',') {
                (*p)++;
                skip_spaces_and_lf(p, sline);

                if(**p == '.' && *(*p+1) == '.' && *(*p+2) == '.') {
                    (*p)+=3;
                    skip_spaces_and_lf(p, sline);

                    sCLNodeType* last_param;

                    if(*num_params == 0) {
                        parser_err_msg_format(sname, sline_top, "Clover requires to define Array<anonymous> type with variable arguments at last declaration of arguments");
                        (*err_num)++;
                        return TRUE;
                    }

                    last_param = class_params[*num_params -1];

                    if(!(last_param->mClass == gArrayClass && last_param->mGenericsTypesNum == 1 && last_param->mGenericsTypes[0]->mGenericsTypesNum == 0 && last_param->mGenericsTypes[0]->mClass == gAnonymousClass))
                    {
                        parser_err_msg_format(sname, sline_top, "Clover requires to define Array<anonymous> type with variable arguments at last declaration of arguments");
                        (*err_num)++;
                        return TRUE;
                    }

                    *variable_arguments = TRUE;

                    if(**p == close_character) {
                        (*p)++;
                        skip_spaces_and_lf(p, sline);
                        break;
                    }
                    else {
                        parser_err_msg_format(sname, sline_top, "Clover requires to %c character after variable argments", close_character);
                        (*err_num)++;
                    }
                }
            }
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////
// parse the source and make nodes
//////////////////////////////////////////////////
static BOOL parse_block_params(sCLNodeType** class_params, int* num_params, sParserInfo* info, sVarTable* new_table, int sline_top);

static BOOL get_params(sParserInfo* info, unsigned int* res_node, char start_brace, char end_brace, sVarTable* lv_table, unsigned int* block_object, unsigned int* block_node, BOOL block_caller_existance)
{
    int params_num;

    params_num = 0;

    *res_node = 0;
    if(**info->p == start_brace) {
        char* p2;
        int sline_rewind;
        BOOL result;
        char buf[128];

        sCLNodeType* result_type;

        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        if(**info->p == end_brace) {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);
        }
        else {
            while(1) {
                unsigned int new_node;
                char buf[WORDSIZ];

                if(!node_expression_without_comma(&new_node, info, lv_table)) {
                    return FALSE;
                }

                skip_spaces_and_lf(info->p, info->sline);

                if(new_node) {
                    if(params_num < CL_METHOD_PARAM_MAX) {
                        *res_node = sNodeTree_create_param(*res_node, new_node,  0);
                        params_num++;
                    }
                    else {
                        parser_err_msg_format(info->sname, *info->sline, "parametor number is overflow");
                        (*info->err_num)++;
                        return FALSE;
                    }
                }

                /// anotation
                if(**info->p == '@') {
                    while(1) {
                        if(**info->p == ',' || **info->p == end_brace) {
                            break;
                        }
                        else if(**info->p == '\0') {
                            parser_err_msg_format(info->sname, *info->sline, "Clover expects next character is , or %c before the source end", end_brace);
                            (*info->err_num)++;
                        }
                        else {
                            (*info->p)++;
                        }
                    }
                }

                snprintf(buf, WORDSIZ, ",%c", end_brace);
                if(!expect_next_character(buf, info->err_num, info->p, info->sname, info->sline)) {
                    gParserLastNode = new_node;
                    return FALSE;
                }

                if(**info->p == ',') {
                    (*info->p)++;
                    skip_spaces_and_lf(info->p, info->sline);
                }
                else if(**info->p == end_brace) {
                    (*info->p)++;
                    skip_spaces_and_lf(info->p, info->sline);
                    break;
                }
            }
        }

        if(block_object && block_node) {
            p2 = *info->p;
            sline_rewind = *info->sline;

            result = parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, FALSE);
            skip_spaces_and_lf(info->p, info->sline);

            if(strcmp(buf, "with") == 0) {
                if(!node_expression_without_comma(block_node, info, lv_table)) {
                    return FALSE;
                }

                skip_spaces_and_lf(info->p, info->sline);

                if(*block_node == 0) {
                    parser_err_msg_format(info->sname, *info->sline, "require block variable after \"with\" keyword");
                    (*info->err_num)++;
                    return FALSE;
                }

                *block_object = 0;
            }
            else {
                *info->p = p2;
                *info->sline = sline_rewind;
                
                *block_node = 0;

                if(isalpha(**info->p)) {
                    result_type = NULL;

                    if(!parse_namespace_and_class_and_generics_type(&result_type, info->p, info->sname, info->sline, info->err_num, info->current_namespace, (info->klass ? info->klass->mClass:NULL), info->method, FALSE)) {
                        return FALSE;
                    }
                }
                else {
                    result_type = gVoidType;
                }

                /// method with block ///
                if(**info->p == '{') {
                    sVarTable* new_table;
                    int num_params;
                    sCLNodeType* class_params[CL_METHOD_PARAM_MAX];

                    /// appned break exsitance flag to result type ///
                    if(type_identity(result_type, gVoidType)) {
                        result_type = gBoolType;
                    }
                    else {
                        make_block_result(&result_type);
                    }

                    num_params = 0;

                    (*info->p)++;
                    skip_spaces_and_lf(info->p, info->sline);

                    new_table = init_method_block_vtable(lv_table);

                    if(block_caller_existance) {
                        sCLNodeType* dummy_type;

                        dummy_type = alloc_node_type();
                        dummy_type->mClass = NULL;
                        dummy_type->mGenericsTypesNum = 0;
                        if(!add_variable_to_table(new_table, "caller", dummy_type)) {
                            parser_err_msg_format(info->sname, *info->sline, "overflow the table or a variable which hash the same name exists");
                            (*info->err_num)++;
                        }
                    }

                    if(!parse_block_params(class_params, &num_params, info, new_table, *info->sline))
                    {
                        return FALSE;
                    }

                    if(!parse_block_object(block_object, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, result_type, info->method, new_table, *info->sline, num_params, class_params, block_caller_existance))
                    {
                        return FALSE;
                    }
                }
                else {
                    *block_object = 0;
                }
            }
        }
    }

    return TRUE;
}

static BOOL expression_node_while(unsigned int* node, sParserInfo* info, sCLNodeType* type, sVarTable* lv_table)
{
    unsigned int conditional;
    unsigned int block;

    BOOL result;
    sVarTable* new_table;

    /// new table ///
    new_table = init_block_vtable(lv_table);

    if(!expect_next_character("(", info->err_num, info->p, info->sname, info->sline)) {
        return FALSE;
    }
    (*info->p)++;

    /// conditional ///
    if(!node_expression(&conditional, info, new_table)) {
        return FALSE;
    }

    if(!expect_next_character(")", info->err_num, info->p, info->sname, info->sline)) {
        gParserLastNode = conditional;
        return FALSE;
    }
    (*info->p)++;
    skip_spaces_and_lf(info->p, info->sline);

    /// block ///
    if(!expect_next_character("{", info->err_num, info->p, info->sname, info->sline)) {
        return FALSE;
    }
    (*info->p)++;
    skip_spaces_and_lf(info->p, info->sline);

    if(!parse_block(&block, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, gVoidType, info->method, new_table)) {
        return FALSE;
    }

    /// entry new vtable to node block ///
    entry_vtable_to_node_block(block, new_table, lv_table);

    *node = sNodeTree_create_while(conditional, block, type);

    return TRUE;
}

static BOOL expression_node_do(unsigned int* node, sParserInfo* info, sCLNodeType* type, sVarTable* lv_table)
{
    unsigned int conditional;
    unsigned int block;

    char buf[WORDSIZ];
    BOOL result;
    sVarTable* new_table;

    /// new table ///
    new_table = init_block_vtable(lv_table);

    /// block ///
    if(!expect_next_character("{", info->err_num, info->p, info->sname, info->sline)) {
        return FALSE;
    }
    (*info->p)++;
    skip_spaces_and_lf(info->p, info->sline);

    if(!parse_block(&block, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, gVoidType, info->method, new_table)) {
        return FALSE;
    }

    /// while ///
    if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
        return FALSE;
    }
    skip_spaces_and_lf(info->p, info->sline);

    if(strcmp(buf, "while") != 0) {
        parser_err_msg_format(info->sname, *info->sline, "require \"while\" for \"do\" statment");
        (*info->err_num)++;
    }

    if(!expect_next_character("(", info->err_num, info->p, info->sname, info->sline)) {
        return FALSE;
    }
    (*info->p)++;

    /// conditional ///
    if(!node_expression(&conditional, info, new_table)) {
        return FALSE;
    }

    if(!expect_next_character(")", info->err_num, info->p, info->sname, info->sline)) {
        gParserLastNode = conditional;
        return FALSE;
    }
    (*info->p)++;
    skip_spaces_and_lf(info->p, info->sline);

    /// entry new vtable to node block ///
    entry_vtable_to_node_block(block, new_table, lv_table);

    *node = sNodeTree_create_do(conditional, block, type);

    return TRUE;
}

static BOOL expression_node_for(unsigned int* node, sParserInfo* info, sCLNodeType* type, sVarTable* lv_table)
{
    unsigned int conditional, conditional2, conditional3;
    unsigned int block;

    BOOL result;
    sVarTable* new_table;

    /// new table ///
    new_table = init_block_vtable(lv_table);

    if(!expect_next_character("(", info->err_num, info->p, info->sname, info->sline)) {
        return FALSE;
    }
    (*info->p)++;
    skip_spaces_and_lf(info->p, info->sline);

    /// initialization ///
    if(!node_expression(&conditional, info, new_table)) {
        return FALSE;
    }

    if(!expect_next_character(";", info->err_num, info->p, info->sname, info->sline)) {
        gParserLastNode = conditional;
        return FALSE;
    }
    (*info->p)++;
    skip_spaces_and_lf(info->p, info->sline);

    /// conditional ///
    if(!node_expression(&conditional2, info, new_table)) {
        return FALSE;
    }

    if(!expect_next_character(";", info->err_num, info->p, info->sname, info->sline)) {
        gParserLastNode = conditional2;
        return FALSE;
    }
    (*info->p)++;
    skip_spaces_and_lf(info->p, info->sline);

    /// finalization ///
    if(!node_expression(&conditional3, info, new_table)) {
        return FALSE;
    }

    if(!expect_next_character(")", info->err_num, info->p, info->sname, info->sline)) {
        gParserLastNode = conditional3;
        return FALSE;
    }
    (*info->p)++;
    skip_spaces_and_lf(info->p, info->sline);

    /// block ///
    if(!expect_next_character("{", info->err_num, info->p, info->sname, info->sline)) {
        return FALSE;
    }
    (*info->p)++;
    skip_spaces_and_lf(info->p, info->sline);

    if(!parse_block(&block, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, gVoidType, info->method, new_table)) {
        return FALSE;
    }

    /// entry new vtable to node block ///
    entry_vtable_to_node_block(block, new_table, lv_table);

    *node = sNodeTree_create_for(conditional, conditional2, conditional3, block, type);

    return TRUE;
}

static BOOL expression_node_if(unsigned int* node, sParserInfo* info, sCLNodeType* type, sVarTable* lv_table)
{
    unsigned int if_conditional;
    unsigned int if_block;
    unsigned int else_block;
    unsigned int else_if_block[CL_ELSE_IF_MAX];
    unsigned int else_if_conditional[CL_ELSE_IF_MAX];
    int else_if_num;
    char buf[WORDSIZ];
    char* p2;
    int sline_rewind;
    BOOL result;
    sVarTable* new_table;

    /// new table ///
    new_table = init_block_vtable(lv_table);

    if(!expect_next_character("(", info->err_num, info->p, info->sname, info->sline)) {
        return FALSE;
    }
    (*info->p)++;

    /// conditional ///
    if(!node_expression(&if_conditional, info, new_table)) {
        return FALSE;
    }

    if(!expect_next_character(")", info->err_num, info->p, info->sname, info->sline)) {
        gParserLastNode = if_conditional;
        return FALSE;
    }
    (*info->p)++;
    skip_spaces_and_lf(info->p, info->sline);

    /// block ///
    if(!expect_next_character("{", info->err_num, info->p, info->sname, info->sline)) {
        return FALSE;
    }
    (*info->p)++;
    skip_spaces_and_lf(info->p, info->sline);

    if(!parse_block(&if_block, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, type, info->method, new_table)) {
        return FALSE;
    }

    /// entry new vtable to node block ///
    entry_vtable_to_node_block(if_block, new_table, lv_table);

    /// "else" and "else if" block ///
    p2 = *info->p;
    sline_rewind = *info->sline;

    result = parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, FALSE);
    skip_spaces_and_lf(info->p, info->sline);

    else_if_num = 0;
    else_block = 0;

    if(result && strcmp(buf, "else") == 0) {
        while(1) {
            if(**info->p == '{') {
                /// new table ///
                new_table = init_block_vtable(lv_table);

                /// block ///
                if(!expect_next_character("{", info->err_num, info->p, info->sname, info->sline)) {
                    return FALSE;
                }
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                if(!parse_block(&else_block, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, type, info->method, new_table)) {
                    return FALSE;
                }

                /// entry new vtable to node block ///
                entry_vtable_to_node_block(else_block, new_table, lv_table);

                *node = sNodeTree_create_if(if_conditional, if_block, else_block, else_if_conditional, else_if_block, else_if_num, type);

                break;
            }
            else {
                p2 = *info->p;

                result = parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, FALSE);
                skip_spaces_and_lf(info->p, info->sline);

                if(result && strcmp(buf, "if") == 0) {
                    /// new table ///
                    new_table = init_block_vtable(lv_table);

                    if(!expect_next_character("(", info->err_num, info->p, info->sname, info->sline)) {
                        return FALSE;
                    }
                    (*info->p)++;

                    /// conditional ///
                    if(!node_expression(&else_if_conditional[else_if_num], info, new_table)) 
                    {
                        return FALSE;
                    }

                    if(!expect_next_character(")", info->err_num, info->p, info->sname, info->sline)) 
                    {
                        gParserLastNode = else_if_conditional[else_if_num];
                        return FALSE;
                    }
                    (*info->p)++;
                    skip_spaces_and_lf(info->p, info->sline);

                    /// else if block ///
                    if(!expect_next_character("{", info->err_num, info->p, info->sname, info->sline)) {
                        return FALSE;
                    }
                    (*info->p)++;
                    skip_spaces_and_lf(info->p, info->sline);

                    if(!parse_block(&else_if_block[else_if_num], info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, type, info->method, new_table)) {
                        return FALSE;
                    }

                    /// entry new vtable to node block ///
                    entry_vtable_to_node_block(else_if_block[else_if_num], new_table, lv_table);

                    else_if_num++;

                    /// "else" and "else if" block ///
                    p2 = *info->p;

                    result = parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, FALSE);
                    skip_spaces_and_lf(info->p, info->sline);

                    if(!result || strcmp(buf, "else") != 0) {
                        *info->p = p2;   // rewind
                        sline_rewind = *info->sline;

                        *node = sNodeTree_create_if(if_conditional, if_block, else_block, else_if_conditional, else_if_block, else_if_num, type);
                        break;
                    }
                }
                else {
                    parser_err_msg_format(info->sname, *info->sline, "require { or \"if\" after \"else\"");
                    (*info->err_num)++;
                    break;
                }
            }
        }
    }
    else {
        *info->p = p2;   // rewind
        *info->sline = sline_rewind;

        *node = sNodeTree_create_if(if_conditional, if_block, 0, NULL, NULL, 0, type);
    }

    return TRUE;
}

static BOOL expression_node_try(unsigned int* node, sParserInfo* info, int sline_top, sCLNodeType* finally_block_type, sVarTable* lv_table)
{
    unsigned int try_block;
    unsigned int catch_blocks[CL_CATCH_BLOCK_NUMBER_MAX];
    unsigned int finally_block;
    unsigned int catch_conditional;
    char buf[WORDSIZ];
    sVarTable* new_table;
    char* p2;
    int sline_rewind;
    sCLNodeType* exception_type[CL_CATCH_BLOCK_NUMBER_MAX];
    char exception_variable_name[CL_CATCH_BLOCK_NUMBER_MAX][CL_VARIABLE_NAME_MAX+1];
    int catch_block_number;
    BOOL result;

    /// try block ///
    if(!expect_next_character("{", info->err_num, info->p, info->sname, info->sline)) {
        return FALSE;
    }
    (*info->p)++;
    skip_spaces_and_lf(info->p, info->sline);

    /// parse try block ///
    new_table = init_block_vtable(lv_table);

    if(!parse_block_object(&try_block, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, gVoidType, info->method, new_table, sline_top, 0, NULL, FALSE))
    {
        return FALSE;
    }

    /// "catch" block ///
    catch_block_number = 0;

    while(1) {
        p2 = *info->p;                      // for rewind
        sline_rewind = *info->sline;

        result = parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, FALSE);
        skip_spaces_and_lf(info->p, info->sline);

        if(result && strcmp(buf, "catch") == 0) {
            sVarTable* new_table2;
            int num_params;
            sCLNodeType* class_params[CL_METHOD_PARAM_MAX];

            if(!expect_next_character("(", info->err_num, info->p, info->sname, info->sline)) 
            {
                return FALSE;
            }
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            /// class name ///
            if(!parse_namespace_and_class_and_generics_type(&exception_type[catch_block_number], info->p, info->sname, info->sline, info->err_num, info->current_namespace, (info->klass ? info->klass->mClass:NULL), info->method, FALSE)) 
            {
                return FALSE;
            }

            /// variable name ///
            if(!parse_word(exception_variable_name[catch_block_number], CL_VARIABLE_NAME_MAX, info->p, info->sname, info->sline, info->err_num, TRUE)) 
            {
                return FALSE;
            }
            skip_spaces_and_lf(info->p, info->sline);

            if(!expect_next_character(")", info->err_num, info->p, info->sname, info->sline)) {
                return FALSE;
            }
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expect_next_character("{", info->err_num, info->p, info->sname, info->sline)) {
                return FALSE;
            }
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            //// catch block ///
            if(exception_type[catch_block_number]->mClass) {
                class_params[0] = MANAGED clone_node_type(exception_type[catch_block_number]);
                num_params = 1;

                new_table2 = init_block_vtable(lv_table);

                if(!add_variable_to_table(new_table2, exception_variable_name[catch_block_number], exception_type[catch_block_number])) 
                {
                    parser_err_msg_format(info->sname, sline_top, "there is a same name variable(%s) or overflow local variable table", exception_variable_name[catch_block_number]);

                    (*info->err_num)++;
                    *node = 0;
                    return TRUE;
                }
            }

            if(!parse_block_object(&catch_blocks[catch_block_number], info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, gVoidType, info->method, new_table2, sline_top, num_params, class_params, FALSE))
            {
                return FALSE;
            }

            catch_block_number++;

            if(catch_block_number >= CL_CATCH_BLOCK_NUMBER_MAX) {
                parser_err_msg_format(info->sname, *info->sline, "catch block number overflow");
                (*info->err_num)++;
                *node = 0;
                return FALSE;
            }
        }
        else {
            /// rewind ///
            *info->p = p2;   // rewind
            *info->sline = sline_rewind;

            break;
        }
    }

    /// "finally" block ///
    p2 = *info->p;                      // for rewind
    sline_rewind = *info->sline;

    result = parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, FALSE);
    skip_spaces_and_lf(info->p, info->sline);

    if(result && strcmp(buf, "finally") == 0) {
        sVarTable* new_table3;

        if(!expect_next_character("{", info->err_num, info->p, info->sname, info->sline)) {
            return FALSE;
        }
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        //// finally block ///
        new_table3 = init_block_vtable(lv_table);

        if(!parse_block_object(&finally_block, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, finally_block_type, info->method, new_table3, sline_top, 0, NULL, FALSE))
        {
            return FALSE;
        }
    }
    else {
        *info->p = p2;   // rewind
        *info->sline = sline_rewind;

        finally_block = 0;
    }

    *node = sNodeTree_create_try(try_block, catch_blocks, catch_block_number, finally_block, exception_type, exception_variable_name);

    return TRUE;
}

static BOOL parse_block_params(sCLNodeType** class_params, int* num_params, sParserInfo* info, sVarTable* new_table, int sline_top)
{
    if(**info->p == '|') {
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        if(!parse_params(class_params, num_params, CL_METHOD_PARAM_MAX, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass:NULL, info->method, new_table, '|', sline_top)) 
        {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL after_class_name(sCLNodeType* type, unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    char buf[WORDSIZ];

    /// class method or class field ///
    if(**info->p == '.'&& *(*info->p+1) != '.') {
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        /// check valid generics type ///
        if(!check_valid_generics_type(type, info->sname, info->sline, info->err_num, info->klass ? info->klass->mClass:NULL, info->method)) {
            return FALSE;
        }

        if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);

        /// call class method ///
        if(**info->p == '(') {
            unsigned int param_node;
            unsigned int block;
            sCLNodeType* result_type;
            unsigned int block_node;
            unsigned int block_object;
            
            param_node = 0;
            if(!get_params(info, &param_node, '(', ')', lv_table, &block_object, &block_node, FALSE)) {
                return FALSE;
            }

            if(block_node != 0 && !(info->method->mFlags & CL_CLASS_METHOD)) {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't call class method with none class method block object");
                (*info->err_num)++;
            }

            *node = sNodeTree_create_class_method_call(buf, type, 0, param_node, 0, block_object, block_node);
        }
        /// access class field ///
        else {
            *node = sNodeTree_create_class_field(buf, type, 0, 0, 0);
        }
    }

    /// define block ///
    else if(**info->p == '{') {
        unsigned int block;
        sVarTable* new_table;
         
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        /// check valid generics type ///
        if(!check_valid_generics_type(type, info->sname, info->sline, info->err_num, info->klass ? info->klass->mClass:NULL, info->method)) {
            return FALSE;
        }

        /// new table ///
        new_table = init_block_vtable(lv_table);

        if(!parse_block(&block, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass, type, info->method, new_table)) {
            return FALSE;
        }

        /// entry new vtable to node block ///
        entry_vtable_to_node_block(block, new_table, lv_table);

        *node = sNodeTree_create_block(type, block);
    }

    /// define variable or loops with type ///
    else if(isalpha(**info->p)) {
        /// check valid generics type ///
        if(!check_valid_generics_type(type, info->sname, info->sline, info->err_num, info->klass ? info->klass->mClass:NULL, info->method)) {
            return FALSE;
        }

        if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);

        //// define variable ///
/*
        if(strcmp(buf, "if") == 0) {
            if(!expression_node_if(node, info, type, lv_table)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "while") == 0) {
            if(!expression_node_while(node, info, type, lv_table)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "do") == 0) {
            if(!expression_node_do(node, info, type, lv_table)) {
                return FALSE;
            }
        }
        else if(strcmp(buf, "for") == 0) {
            if(!expression_node_for(node, info, type, lv_table)) {
                return FALSE;
            }
        }
        if(strcmp(buf, "try") == 0) {
            if(!expression_node_try(node, info, sline_top, type, lv_table)) {
                return FALSE;
            }
        }
*/
        char* name;

        name = buf;

        if(lv_table == NULL) {
            parser_err_msg_format(info->sname, *info->sline, "there is not local variable table");
            (*info->err_num)++;

            *node = 0;
            return TRUE;
        }

        *node = sNodeTree_create_define_var(name, type, 0, 0, 0);

        if(!add_variable_to_table(lv_table, buf, type)) {
            parser_err_msg_format(info->sname, sline_top, "there is a same name variable(%s) or overflow local variable table", buf);

            (*info->err_num)++;
            *node = 0;
            return TRUE;
        }
    }
    /// class name ///
    else {
        *node = sNodeTree_create_class_name(type);
    }

    return TRUE;
}

static BOOL increment_and_decrement(enum eOperand op, unsigned int* node, unsigned int right, unsigned int middle, sParserInfo* info , int sline_top, sVarTable* lv_table, BOOL quote)
{
    if(*node > 0) {
        switch(gNodes[*node].mNodeType) {
            case NODE_TYPE_VARIABLE_NAME:
            case NODE_TYPE_FIELD:
            case NODE_TYPE_CLASS_FIELD:
                break;

            default:
                parser_err_msg("require variable name for ++ or --", info->sname, sline_top);
                (*info->err_num)++;
                break;
        }

        *node = sNodeTree_create_operand(op, *node, 0, 0, quote);
    }

    return TRUE;
}

// from left to right order
static BOOL postposition_operator(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(*node == 0) {
        return TRUE;
    }

    while(*info->p) {
        BOOL quote;
        char* p_saved;

        p_saved = *info->p;
        parse_quote(info->p, info->sline, &quote);

        /// call method or access field ///
        if((**info->p == '.' && *(*info->p+1) != '.') || (**info->p == '-' && *(*info->p+1) == '>')) 
        {
            if(quote) {
                parser_err_msg_format(info->sname, sline_top, "can't quote . operand");
                (*info->err_num)++;
                *node = 0;
                return TRUE;
            }

            if(**info->p == '.') {
                (*info->p)++;
            }
            else {
                (*info->p)+=2;
            }
            skip_spaces_and_lf(info->p, info->sline);

            if(isalpha(**info->p)) {
                char buf[WORDSIZ];

                if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);

                /// call methods ///
                if(**info->p == '(') {
                    unsigned int param_node;
                    unsigned int block;
                    sCLNodeType* result_type;
                    unsigned int block_object;
                    unsigned int block_node;

                    param_node = 0;

                    if(!get_params(info, &param_node, '(', ')', lv_table, &block_object, &block_node, TRUE)) {
                        return FALSE;
                    }

                    if(block_node != 0 && info->method->mFlags & CL_CLASS_METHOD) {
                        parser_err_msg_format(info->sname, *info->sline, "Clover can't call none class method with class method block object");
                        (*info->err_num)++;
                    }

                    *node = sNodeTree_create_method_call(buf, *node, param_node, 0, block_object, block_node);
                }
                /// access fields ///
                else {
                    *node = sNodeTree_create_fields(buf, *node, 0, 0);
                }
            }
            else {
                parser_err_msg("require method name or field name after .", info->sname, sline_top);
                (*info->err_num)++;

                *node = 0;
                break;
            }
        }
        /// indexing ///
        else if(**info->p == '[') {
            unsigned int param_node;
            unsigned int block_object;
            unsigned int block_node;

            if(quote) {
                parser_err_msg_format(info->sname, sline_top, "can't quote [ operand");
                (*info->err_num)++;
                *node = 0;
                return TRUE;
            }
            
            param_node = 0;
            if(!get_params(info, &param_node, '[', ']', lv_table, &block_object, &block_node, FALSE)) {
                return FALSE;
            }

            if(block_object != 0 || block_node != 0) {
                parser_err_msg_format(info->sname, *info->sline, "Clover can't get block with indexing operator");
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpIndexing, *node, param_node, 0, quote);
        }
        else if(**info->p == '+' && *(*info->p+1) == '+') {
            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!increment_and_decrement(kOpPlusPlus2, node, 0, 0, info, sline_top, lv_table, quote)) {
                return FALSE;
            }
        }
        else if(**info->p == '-' && *(*info->p+1) == '-') {
            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!increment_and_decrement(kOpMinusMinus2, node, 0, 0, info, sline_top, lv_table, quote)) {
                return FALSE;
            }
        }
        else {
            *info->p = p_saved;
            break;
        }
    }

    return TRUE;
}

static BOOL get_hex_number(char* buf, size_t buf_size, char* p2, unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    unsigned long value;

    while((**info->p >= '0' && **info->p <= '9') || (**info->p >= 'a' && **info->p <= 'f') || (**info->p >= 'A' && **info->p <= 'F')) {
        *p2++ = **info->p;
        (*info->p)++;

        if(p2 - buf >= buf_size) {
            parser_err_msg("overflow node of number",  info->sname, sline_top);
            return FALSE;
        }
    }
    *p2 = 0;

    value = strtoul(buf, NULL, 0);

    if(**info->p == 'y') {
        (*info->p)++;

        *node = sNodeTree_create_byte_value((unsigned char)value, 0, 0, 0);
    }
    else if(**info->p == 's') {
        (*info->p)++;

        *node = sNodeTree_create_short_value((unsigned short)value, 0, 0, 0);
    }
    else if(**info->p == 'u') {
        (*info->p)++;
        *node = sNodeTree_create_uint_value((unsigned int)value, 0, 0, 0);
    }
    else if(**info->p == 'l') {
        (*info->p)++;
        *node = sNodeTree_create_long_value((unsigned long)value, 0, 0, 0);
    }
    else {
        *node = sNodeTree_create_value((int)value, 0, 0, 0);
    }

    skip_spaces_and_lf(info->p, info->sline);
    return TRUE;
}

static BOOL get_oct_number(char* buf, size_t buf_size, char* p2, unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    unsigned long value;

    while(**info->p >= '0' && **info->p <= '7') {
        *p2++ = **info->p;
        (*info->p)++;

        if(p2 - buf >= buf_size) {
            parser_err_msg("overflow node of number",  info->sname, sline_top);
            return FALSE;
        }
    }
    *p2 = 0;

    value = strtoul(buf, NULL, 0);

    if(**info->p == 'y') {
        (*info->p)++;
        *node = sNodeTree_create_byte_value((unsigned char)value, 0, 0, 0);
    }
    else if(**info->p == 's') {
        (*info->p)++;
        *node = sNodeTree_create_short_value((unsigned short)value, 0, 0, 0);
    }
    else if(**info->p == 'u') {
        (*info->p)++;
        *node = sNodeTree_create_uint_value((unsigned int)value, 0, 0, 0);
    }
    else if(**info->p == 'l') {
        (*info->p)++;
        *node = sNodeTree_create_long_value((unsigned long)value, 0, 0, 0);
    }
    else {
        *node = sNodeTree_create_value((int)value, 0, 0, 0);
    }

    skip_spaces_and_lf(info->p, info->sline);

    return TRUE;
}

static BOOL get_number(char* buf, size_t buf_size, char* p2, unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    while(**info->p >= '0' && **info->p <= '9') {
        *p2++ = **info->p;
        (*info->p)++;

        if(p2 - buf >= buf_size) {
            parser_err_msg("overflow node of number",  info->sname, sline_top);
            return FALSE;
        }
    }
    *p2 = 0;

    if(**info->p == '.' && (*(*info->p+1) >= '0' && *(*info->p+1) <= '9')) {
        *p2++ = **info->p;
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        while(**info->p >= '0' && **info->p <= '9') {
            *p2++ = **info->p;
            (*info->p)++;

            if(p2 - buf >= buf_size) {
                parser_err_msg("overflow node of number",  info->sname, sline_top);
                return FALSE;
            }
        }
        *p2 = 0;
        skip_spaces_and_lf(info->p, info->sline);

        if(**info->p == 'f') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            *node = sNodeTree_create_fvalue(atof(buf), 0, 0, 0);
        }
        else {
            *node = sNodeTree_create_dvalue(atof(buf), 0, 0, 0);
        }

    }
    else if(**info->p == 'y') {
        (*info->p)++;
        unsigned long value = strtoul(buf, NULL, 0);
        *node = sNodeTree_create_byte_value((unsigned char)value, 0, 0, 0);
    }
    else if(**info->p == 's') {
        (*info->p)++;
        unsigned long value = strtoul(buf, NULL, 0);
        *node = sNodeTree_create_short_value((unsigned short)value, 0, 0, 0);
    }
    else if(**info->p == 'u') {
        (*info->p)++;
        unsigned long value = strtoul(buf, NULL, 0);
        *node = sNodeTree_create_uint_value((unsigned int)value, 0, 0, 0);
    }
    else if(**info->p == 'l') {
        (*info->p)++;
        unsigned long value = strtoul(buf, NULL, 0);
        *node = sNodeTree_create_long_value((unsigned long)value, 0, 0, 0);
    }
    else {
        *node = sNodeTree_create_value(atoi(buf), 0, 0, 0);
    }

    skip_spaces_and_lf(info->p, info->sline);

    return TRUE;
}

static BOOL reserved_words(BOOL* processed, char* buf, unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    *processed = TRUE;

    if(strcmp(buf, "new") == 0) {
        sCLNodeType* type;

        type = NULL;

        if(!parse_namespace_and_class_and_generics_type(&type, info->p, info->sname, info->sline, info->err_num, info->current_namespace, info->klass ? info->klass->mClass:NULL, info->method, FALSE)) {
            return FALSE;
        }

        if(**info->p == '(') {
            unsigned int param_node;
            unsigned int block_object;
            unsigned int block_node;

            param_node = 0;
            if(!get_params(info, &param_node, '(', ')', lv_table, &block_object, &block_node, TRUE)) {
                return FALSE;
            }

            if(type->mClass) {
                *node = sNodeTree_create_new_expression(type, 0, param_node, 0, block_object, block_node);
            }
            else {
                *node = 0;
            }
        }
        else {
            parser_err_msg("require (", info->sname, sline_top);
            (*info->err_num)++;

            *node = 0;
        }
    }
    else if(strcmp(buf, "super") == 0) {
        unsigned int param_node;
        char** p2;
        char buf2[CL_VARIABLE_NAME_MAX];
        unsigned int block_object;
        unsigned int block_node;
        sCLNodeType* result_type;
        BOOL block_caller_existance;
        
        if(!expect_next_character("(", info->err_num, info->p, info->sname, info->sline)) {
            return FALSE;
        }

        /// call super ///
        block_caller_existance = !(info->method->mFlags & CL_CLASS_METHOD);
        param_node = 0;
        if(!get_params(info, &param_node, '(', ')', lv_table, &block_object, &block_node, block_caller_existance)) {
            return FALSE;
        }

        *node = sNodeTree_create_super(0, param_node, 0, block_object, block_node);
    }
    else if(strcmp(buf, "mixin") == 0) {
        unsigned int param_node;
        unsigned int block_object;
        sCLNodeType* result_type;
        unsigned int block_node;
        BOOL block_caller_existance;
        
        if(!expect_next_character("(", info->err_num, info->p, info->sname, info->sline)) {
            return FALSE;
        }

        /// call mixin ///
        block_caller_existance = !(info->method->mFlags & CL_CLASS_METHOD);
        param_node = 0;
        if(!get_params(info, &param_node, '(', ')', lv_table, &block_object, &block_node, block_caller_existance)) {
            return FALSE;
        }

        *node = sNodeTree_create_inherit(0, param_node, 0, block_object, block_node);
    }
    else if(strcmp(buf, "return") == 0) {
        unsigned int rv_node;

        if(**info->p == ';') {
            rv_node = 0;
        }
        else {
            if(!node_expression(&rv_node, info, lv_table)) {
                return FALSE;
            }
            skip_spaces_and_lf(info->p, info->sline);

            if(rv_node == 0) {
                parser_err_msg("require expression as return operand", info->sname, sline_top);
                (*info->err_num)++;
            }
        }

        *node = sNodeTree_create_return(gNodes[rv_node].mType, rv_node, 0, 0);
    }
    else if(strcmp(buf, "revert") == 0) {
        unsigned int rv_node;

        if(**info->p == ';') {
            rv_node = 0;
        }
        else {
            if(!node_expression(&rv_node, info, lv_table)) {
                return FALSE;
            }
            skip_spaces_and_lf(info->p, info->sline);

            if(rv_node == 0) {
                parser_err_msg("require expression as return operand", info->sname, sline_top);
                (*info->err_num)++;
            }
        }

        *node = sNodeTree_create_revert(gNodes[rv_node].mType, rv_node, 0, 0);
    }
    else if(strcmp(buf, "throw") == 0) {
        unsigned int tv_node;

        if(**info->p == ';') {
            parser_err_msg("require expression as throw operand", info->sname, sline_top);
            (*info->err_num)++;
        }
        else if(**info->p == '(') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!node_expression(&tv_node, info, lv_table)) {
                return FALSE;
            }
            skip_spaces_and_lf(info->p, info->sline);

            if(!expect_next_character(")", info->err_num, info->p, info->sname, info->sline)) {
                return FALSE;
            }
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(tv_node == 0) {
                parser_err_msg("require expression as throw operand", info->sname, sline_top);
                (*info->err_num)++;
            }
        }
        else {
            if(!node_expression(&tv_node, info, lv_table)) {
                return FALSE;
            }
            skip_spaces_and_lf(info->p, info->sline);

            if(tv_node == 0) {
                parser_err_msg("require expression as throw operand", info->sname, sline_top);
                (*info->err_num)++;
            }
        }

        *node = sNodeTree_create_throw(gNodes[tv_node].mType, tv_node, 0, 0);
    }
    else if(strcmp(buf, "null") == 0) {
        *node = sNodeTree_create_null();
    }
    else if(strcmp(buf, "true") == 0) {
        *node = sNodeTree_create_true();
    }
    else if(strcmp(buf, "false") == 0) {
        *node = sNodeTree_create_false();
    }
    else if(strcmp(buf, "continue") == 0) {
        *node = sNodeTree_create_continue();
    }
    else if(strcmp(buf, "break") == 0) {
        unsigned int bv_node;

        if(**info->p == ';') {
            bv_node = 0;
        }
        else if(**info->p == '(') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!node_expression(&bv_node, info, lv_table)) {
                return FALSE;
            }
            skip_spaces_and_lf(info->p, info->sline);

            if(!expect_next_character(")", info->err_num, info->p, info->sname, info->sline)) {
                return FALSE;
            }
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(bv_node == 0) {
                parser_err_msg("require expression as ( operand", info->sname, sline_top);
                (*info->err_num)++;
            }
        }
        else {
            if(!node_expression(&bv_node, info, lv_table)) {
                return FALSE;
            }
            skip_spaces_and_lf(info->p, info->sline);

            if(bv_node == 0) {
                parser_err_msg("require expression as ( operand", info->sname, sline_top);
                (*info->err_num)++;
            }
        }

        *node = sNodeTree_create_break(gNodes[bv_node].mType, bv_node, 0, 0);
    }
    else if(strcmp(buf, "try") == 0) {
        if(!expression_node_try(node, info, sline_top, gVoidType, lv_table)) {
            return FALSE;
        }
    }
    else if(strcmp(buf, "if") == 0) {
        if(!expression_node_if(node, info, gVoidType, lv_table)) {
            return FALSE;
        }
    }
    else if(strcmp(buf, "while") == 0) {
        if(!expression_node_while(node, info, gVoidType, lv_table)) {
            return FALSE;
        }
    }
    else if(strcmp(buf, "do") == 0) {
        if(!expression_node_do(node, info, gVoidType, lv_table)) {
            return FALSE;
        }
    }
    else if(strcmp(buf, "for") == 0) {
        if(!expression_node_for(node, info, gVoidType, lv_table)) {
            return FALSE;
        }
    }
    else {
        *processed = FALSE;
    }

    return TRUE;
}

static BOOL alias_words(BOOL* processed, char* buf, unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    sCLMethod* alias_method;
    sCLClass* alias_class;

    *processed = TRUE;

    alias_method = get_method_from_alias_table(buf, &alias_class);

    if(alias_method) {
        sCLNodeType* type;
        unsigned int param_node;
        unsigned int block_object;
        sCLNodeType* result_type;
        unsigned int block_node;

        param_node = 0;
        if(!get_params(info, &param_node, '(', ')', lv_table, &block_object, &block_node, FALSE)) {
            return FALSE;
        }

        type = alloc_node_type();

        type->mClass = alias_class;
        type->mGenericsTypesNum = 0;

        *node = sNodeTree_create_class_method_call(buf, type, 0, param_node, 0, block_object, block_node);
    }
    else {
        *processed = FALSE;
    }

    return TRUE;
}

static BOOL expression_node(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if((**info->p == '-' && *(*info->p+1) != '=' && *(*info->p+1) != '-' && *(*info->p+1) != '>') || (**info->p == '+' && *(*info->p+1) != '=' && *(*info->p+1) != '+')) 
    {
        char buf[128];
        char* p2;

        p2 = buf;

        if(**info->p == '-') {
            *p2++ = '-';
            (*info->p)++;
        }
        else if(**info->p =='+') {
            (*info->p)++;
        }

        if(**info->p >= '0' && **info->p <= '9') {
            if(!get_number(buf, 128, p2, node, info, sline_top, lv_table)) {
                return FALSE;
            }
        }
        else if(**info->p == '0' && *(*info->p+1) == 'x') {
            *p2++ = **info->p;
            (*info->p)++;
            *p2++ = **info->p;
            (*info->p)++;

            if(!get_hex_number(buf, 128, p2, node, info, sline_top, lv_table)) {
                return FALSE;
            }
        }
        else if(**info->p == '0') {
            *p2++ = **info->p;
            (*info->p)++;

            if(!get_oct_number(buf, 128, p2, node, info, sline_top, lv_table)) {
                return FALSE;
            }
        }
        else { 
            parser_err_msg("require number after + or -", info->sname, sline_top);
            *node = 0;
            (*info->err_num)++;
        }
    }
    else if(**info->p == '0' && *(*info->p+1) == 'x') {
        char buf[128];
        char* p2;

        p2 = buf;

        *p2++ = **info->p;
        (*info->p)++;
        *p2++ = **info->p;
        (*info->p)++;

        if(!get_hex_number(buf, 128, p2, node, info, sline_top, lv_table)) {
            return FALSE;
        }
    }
    else if(**info->p == '0' && *(*info->p+1) >= '0' && *(*info->p+1) <= '9') {
        char buf[128];
        char* p2;

        p2 = buf;

        *p2++ = **info->p;
        (*info->p)++;

        if(!get_oct_number(buf, 128, p2, node, info, sline_top, lv_table)) {
            return FALSE;
        }
    }
    else if(**info->p >= '0' && **info->p <= '9') {
        char buf[128];
        char* p2;

        p2 = buf;

        if(!get_number(buf, 128, p2, node, info, sline_top, lv_table)) {
            return FALSE;
        }
    }
    else if(**info->p == '\'') {
        wchar_t c;

        (*info->p)++;

        if(**info->p == '\\') {
            (*info->p)++;

            switch(**info->p) {
                case 'n':
                    c = '\n';
                    (*info->p)++;
                    break;

                case 't':
                    c = '\t';
                    (*info->p)++;
                    break;

                case 'r':
                    c = '\r';
                    (*info->p)++;
                    break;

                case 'a':
                    c = '\a';
                    (*info->p)++;
                    break;

                case '\\':
                    c = '\\';
                    (*info->p)++;
                    break;

                default:
                    c = **info->p;
                    (*info->p)++;
                    break;
            }
        }
        else {
            unsigned char p2 = *(unsigned char*)*info->p;

            /// utf-8 character ///
            if(p2 > 127) {
                int size;
                char str[MB_LEN_MAX+1];

                size = ((p2 & 0x80) >> 7) + ((p2 & 0x40) >> 6) + ((p2 & 0x20) >> 5) + ((p2 & 0x10) >> 4);
                if(size > MB_LEN_MAX) {
                    parser_err_msg("invalid utf-8 character", info->sname, sline_top);
                    (*info->err_num)++;
                }
                else {
                    memcpy(str, *info->p, size);
                    str[size] = 0;

                    if(mbtowc(&c, str, size) < 0) {
                        parser_err_msg("invalid utf-8 character", info->sname, sline_top);
                        (*info->err_num)++;
                        c = 0;
                    }

                    (*info->p)+=size;
                }
            }
            /// ASCII character ///
            else {
                c = **info->p;
                (*info->p)++;
            }
        }

        if(**info->p != '\'') {
            parser_err_msg("close \' to make character value", info->sname, sline_top);
            (*info->err_num)++;
        }
        else {
            (*info->p)++;

            skip_spaces_and_lf(info->p, info->sline);

            *node = sNodeTree_create_character_value(c);
        }
    }
    else if(**info->p == '"') {
        sBuf value;

        (*info->p)++;

        sBuf_init(&value);

        while(1) {
            if(**info->p == '"') {
                (*info->p)++;
                break;
            }
            else if(**info->p == '\\') {
                (*info->p)++;
                switch(**info->p) {
                    case 'n':
                        sBuf_append_char(&value, '\n');
                        (*info->p)++;
                        break;

                    case 't':
                        sBuf_append_char(&value, '\t');
                        (*info->p)++;
                        break;

                    case 'r':
                        sBuf_append_char(&value, '\r');
                        (*info->p)++;
                        break;

                    case 'a':
                        sBuf_append_char(&value, '\a');
                        (*info->p)++;
                        break;

                    case '\\':
                        sBuf_append_char(&value, '\\');
                        (*info->p)++;
                        break;

                    default:
                        sBuf_append_char(&value, **info->p);
                        (*info->p)++;
                        break;
                }
            }
            else if(**info->p == 0) {
                parser_err_msg("close \" to make string value", info->sname, sline_top);
                return FALSE;
            }
            else {
                if(**info->p =='\n') (*info->sline)++;

                sBuf_append_char(&value, **info->p);
                (*info->p)++;
            }
        }

        skip_spaces_and_lf(info->p, info->sline);

        *node = sNodeTree_create_string_value(MANAGED value.mBuf, 0, 0, 0);
    }
    else if((**info->p == 'B' || **info->p == 'b') && *(*info->p+1) == '"') {
        sBuf value;

        (*info->p)+=2;

        sBuf_init(&value);

        while(1) {
            if(**info->p == '"') {
                (*info->p)++;
                break;
            }
            else if(**info->p == '\\') {
                (*info->p)++;
                switch(**info->p) {
                    case 'n':
                        sBuf_append_char(&value, '\n');
                        (*info->p)++;
                        break;

                    case 't':
                        sBuf_append_char(&value, '\t');
                        (*info->p)++;
                        break;

                    case 'r':
                        sBuf_append_char(&value, '\r');
                        (*info->p)++;
                        break;

                    case 'a':
                        sBuf_append_char(&value, '\a');
                        (*info->p)++;
                        break;

                    case '\\':
                        sBuf_append_char(&value, '\\');
                        (*info->p)++;
                        break;

                    default:
                        sBuf_append_char(&value, **info->p);
                        (*info->p)++;
                        break;
                }
            }
            else if(**info->p == 0) {
                parser_err_msg("close \" to make string value", info->sname, sline_top);
                return FALSE;
            }
            else {
                if(**info->p =='\n') (*info->sline)++;

                sBuf_append_char(&value, **info->p);
                (*info->p)++;
            }
        }

        skip_spaces_and_lf(info->p, info->sline);

        *node = sNodeTree_create_bytes_value(MANAGED value.mBuf, 0, 0, 0);
    }
    else if((**info->p == 'P' || **info->p == 'p') && *(*info->p+1) == '"') {
        sBuf value;

        gParserInputingPath = TRUE;

        (*info->p)+=2;

        sBuf_init(&value);

        while(1) {
            if(**info->p == '"') {
                gParserInputingPath = FALSE;
                (*info->p)++;
                break;
            }
            else if(**info->p == '\\') {
                (*info->p)++;
                switch(**info->p) {
                    case 'n':
                        sBuf_append_char(&value, '\n');
                        (*info->p)++;
                        break;

                    case 't':
                        sBuf_append_char(&value, '\t');
                        (*info->p)++;
                        break;

                    case 'r':
                        sBuf_append_char(&value, '\r');
                        (*info->p)++;
                        break;

                    case 'a':
                        sBuf_append_char(&value, '\a');
                        (*info->p)++;
                        break;

                    case '\\':
                        sBuf_append_char(&value, '\\');
                        (*info->p)++;
                        break;

                    default:
                        sBuf_append_char(&value, **info->p);
                        (*info->p)++;
                        break;
                }
            }
            else if(**info->p == 0) {
                parser_err_msg("close \" to make string value", info->sname, sline_top);
                return FALSE;
            }
            else {
                if(**info->p =='\n') (*info->sline)++;

                sBuf_append_char(&value, **info->p);
                (*info->p)++;
            }
        }

        skip_spaces_and_lf(info->p, info->sline);

        *node = sNodeTree_create_path_value(MANAGED value.mBuf, 0, 0, 0);
    }
    /// regex ///
    else if(**info->p == '/' || (**info->p == 'r' && *(*info->p+1) == '/')) {
        char regex[REGEX_LENGTH_MAX];
        char* p;
        BOOL global;
        BOOL multiline;
        BOOL ignore_case;

        p = regex;

        if(**info->p == 'r') {
            (*info->p)+=2;
        }
        else {
            (*info->p)++;
        }


        while(1) {
            if(**info->p == '/') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);
                break;
            }
            else if(**info->p == '\\' && *(*info->p+1) == '/') {
                *p++ = '/';

                if(p - regex >= REGEX_LENGTH_MAX) {
                    parser_err_msg_format(info->sname, *info->sline, "overflow regex length max");
                    (*info->err_num)++;
                    break;
                }

                (*info->p)+=2;
            }
/*
            else if(**info->p == '\\') {
                (*info->p)++;

                *p++ = **info->p;
                (*info->p)++;

                if(p - regex >= REGEX_LENGTH_MAX) {
                    parser_err_msg_format(info->sname, *info->sline, "overflow regex length max");
                    (*info->err_num)++;
                    break;
                }
            }
*/
            else if(**info->p == 0) {
                parser_err_msg_format(info->sname, sline_top, "invalid regex. Regex expression requires to close regex using /");
                (*info->err_num)++;
                break;
            }
            else {
                *p++ = **info->p;
                (*info->p)++;

                if(p - regex >= REGEX_LENGTH_MAX) {
                    parser_err_msg_format(info->sname, *info->sline, "overflow regex length max");
                    (*info->err_num)++;
                    break;
                }
            }
        }

        *p++ = 0;

        if(p - regex >= REGEX_LENGTH_MAX) {
            parser_err_msg_format(info->sname, *info->sline, "overflow regex length max");
            (*info->err_num)++;
        }

        global = FALSE;
        multiline = FALSE;
        ignore_case = FALSE;

        while(1) {
            if(**info->p == 'g') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                global = TRUE;
            }
            else if(**info->p == 'm') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                multiline = TRUE;
            }
            else if(**info->p == 'i') {
                (*info->p)++;
                skip_spaces_and_lf(info->p, info->sline);

                ignore_case = TRUE;
            }
            else {
                break;
            }
        }
        *node = sNodeTree_create_regex(regex, global, multiline, ignore_case);
    }
    /// array, hash ///
    else if(**info->p == '{') {
        int sline2;
        unsigned int new_node;
        unsigned int elements_num;
        int array_or_hash;      // 1:array 2:hash

        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        sline2 = *info->sline;

        new_node = 0;
        elements_num = 0;
        array_or_hash = 0;

        if(**info->p == '}') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);
        }
        else {
            while(1) {
                if(**info->p == 0) {
                    parser_err_msg_format(info->sname, sline2, "require } to end of the source");
                    (*info->err_num)++;
                    break;
                }
                else {
                    unsigned int new_node2;

                    if(!node_expression_without_comma(&new_node2, info, lv_table)) {
                        return FALSE;
                    }
                    skip_spaces_and_lf(info->p, info->sline);

                    /// hash value ///
                    if(**info->p == '=' && *(*info->p+1) == '>') {
                        unsigned int new_node3;

                        (*info->p)+=2;
                        skip_spaces_and_lf(info->p, info->sline);

                        if(array_or_hash == 0) {
                            array_or_hash = 2;
                        }
                        else if(array_or_hash == 1) {
                            parser_err_msg_format(info->sname, *info->sline, "This is hash mark");
                            (*info->err_num)++;
                        }
                        
                        if(new_node2) {
                            if(elements_num < CL_ARRAY_ELEMENTS_MAX*2) {
                                new_node = sNodeTree_create_param(new_node, new_node2,  0);
                                elements_num++;
                            }
                            else {
                                parser_err_msg_format(info->sname, *info->sline, "number of hash elements overflow");
                                return FALSE;
                            }
                        }
                        else {
                            parser_err_msg_format(info->sname, *info->sline, "Require key value of hash");
                            (*info->err_num)++;
                        }

                        if(!node_expression_without_comma(&new_node3, info, lv_table)) {
                            return FALSE;
                        }
                        skip_spaces_and_lf(info->p, info->sline);
                        
                        if(new_node3) {
                            if(elements_num < CL_ARRAY_ELEMENTS_MAX*2) {
                                new_node = sNodeTree_create_param(new_node, new_node3, 0);
                                elements_num++;
                            }
                            else {
                                parser_err_msg_format(info->sname, *info->sline, "number of array elements overflow");
                                return FALSE;
                            }
                        }
                        else {
                            parser_err_msg_format(info->sname, *info->sline, "Require item value of hash.");
                            (*info->err_num)++;
                        }
                    }
                    /// array value ///
                    else if(**info->p == ',' || ((array_or_hash == 1 || array_or_hash == 0) && **info->p == '}')) {
                        if(array_or_hash == 0) {
                            array_or_hash = 1;
                        }
                        else if(array_or_hash == 2) {
                            parser_err_msg_format(info->sname, *info->sline, "This is array mark.");
                            (*info->err_num)++;
                        }

                        if(new_node2) {
                            if(elements_num < CL_ARRAY_ELEMENTS_MAX) {
                                new_node = sNodeTree_create_param(new_node, new_node2,  0);
                                elements_num++;
                            }
                            else {
                                parser_err_msg_format(info->sname, *info->sline, "number of array elements overflow");
                                return FALSE;
                            }
                        }
                    }

                    if(**info->p == ',') {
                        (*info->p)++;
                        skip_spaces_and_lf(info->p, info->sline);
                    }
                    else if(**info->p == '}') {
                        (*info->p)++;
                        skip_spaces_and_lf(info->p, info->sline);
                        break;
                    }
                    else {
                        parser_err_msg_format(info->sname, *info->sline, "require , or } after an array element");
                        (*info->err_num)++;
                        break;
                    }
                }
            }
        }

        if(array_or_hash == 2) {
            *node = sNodeTree_create_hash(new_node, 0, 0);
        }
        else {
            *node = sNodeTree_create_array(new_node, 0, 0);
        }
    }
    /// words ///
    else if(isalpha(**info->p)) {
        BOOL processed;
        char buf[WORDSIZ];
        char* saved_p;
        int saved_sline;

        saved_p = *info->p;
        saved_sline = *info->sline;

        if(!parse_word(buf, WORDSIZ, info->p, info->sname, info->sline, info->err_num, TRUE)) {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);

        /// reserved words ///
        if(!reserved_words(&processed, buf, node, info, sline_top, lv_table)) {
            return FALSE;
        }

        if(processed) {  // reserved words or alias words is processed
        }
        /// local variable ///
        else if(lv_table && does_this_var_exist(lv_table, buf)) {
            /// calling block ///
            if(**info->p == '(') {
                unsigned int param_node;
                unsigned int block_object;
                unsigned int block_node;

                param_node = 0;
                if(!get_params(info, &param_node, '(', ')', lv_table, &block_object, &block_node, FALSE)) {
                    return FALSE;
                }

                if(block_object != 0 || block_node != 0) {
                    parser_err_msg_format(info->sname, *info->sline, "Clover can't get block with calling block");
                    (*info->err_num)++;
                }

                *node = sNodeTree_create_call_block(buf, 0, param_node, 0);
            }
            else {
                *node = sNodeTree_create_var(buf, 0, 0, 0);
            }
        }
        /// alias or class name or calling block ///
        else {
            /// alias ///
            if(!alias_words(&processed, buf, node, info, sline_top, lv_table)) 
            {
                return FALSE;
            }

            if(processed) {
            }
            else {
                sCLNodeType* type;

                *info->p = saved_p;                        // rewind
                *info->sline = saved_sline;

                if(!parse_namespace_and_class_and_generics_type_without_generics_check(ALLOC &type, info->p, info->sname, info->sline, info->err_num, info->current_namespace, (info->klass ? info->klass->mClass:NULL), info->method, FALSE))
                {
                    return FALSE;
                }

                gParserGetClassType = type;

                if(type->mClass == NULL) {
                    *node = 0;
                }
                else {
                    if(!after_class_name(type, node, info, sline_top, lv_table)) {
                        return FALSE;
                    }
                }
            }
        }
    }
    else if(**info->p == '(') {
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        if(!node_expression(node, info, lv_table)) {
            return FALSE;
        }
        skip_spaces_and_lf(info->p, info->sline);

        if(!expect_next_character(")", info->err_num, info->p, info->sname, info->sline)) {
            gParserLastNode = *node;
            return FALSE;
        }
        (*info->p)++;
        skip_spaces_and_lf(info->p, info->sline);

        if(*node == 0) {
            parser_err_msg("require expression as ( operand", info->sname, sline_top);
            (*info->err_num)++;
        }
    }
    else if(**info->p == '\n') {
        skip_spaces_and_lf(info->p, info->sline);

        return expression_node(node, info, sline_top, lv_table);
    }
    else if(**info->p == ';' || **info->p == '}' || **info->p == ')' || **info->p == 0) {
        *node = 0;
        return TRUE;
    }
    else {
        parser_err_msg_format(info->sname, *info->sline, "invalid character (character code %d) (%c)", **info->p, **info->p);
        *node = 0;
        if(**info->p == '\n') (*info->sline)++;
        (*info->p)++;
        (*info->err_num)++;

        while(**info->p && **info->p != ';') {
            if(**info->p == '\n') (*info->sline)++;
            (*info->p)++;
        }
    }

    /// postposition operator ///
    if(!postposition_operator(node, info, sline_top, lv_table))
    {
        return FALSE;
    }

    return TRUE;
}

// from right to left order 
static BOOL expression_monadic_operator(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    while(**info->p) {
        BOOL quote;
        parse_quote(info->p, info->sline, &quote);

        if(**info->p == '+' && *(*info->p+1) == '+') {
            (*info->p) +=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_monadic_operator(node, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require value", info->sname, sline_top);
                (*info->err_num)++;
            }

            if(!increment_and_decrement(kOpPlusPlus, node, 0, 0, info, sline_top, lv_table, quote)) {
                return FALSE;
            }
            break;
        }
        else if(**info->p == '-' && *(*info->p+1) == '-') {
            (*info->p) +=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_monadic_operator(node, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require value", info->sname, sline_top);
                (*info->err_num)++;
            }

            if(!increment_and_decrement(kOpMinusMinus, node, 0, 0, info, sline_top, lv_table, quote)) {
                return FALSE;
            }
            break;
        }
        else if(**info->p == '~') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_monadic_operator(node, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require value", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComplement, *node, 0, 0, quote);
            break;
        }
        else if(**info->p == '!') {
            (*info->p) ++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_monadic_operator(node, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require value", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpLogicalDenial, *node, 0, 0, quote);
            break;
        }
        else {
            if(quote) {
                parser_err_msg("Clover can't quote before expression node", info->sname, sline_top);
                (*info->err_num)++;
            }

            if(!expression_node(node, info, sline_top, lv_table)) {
                return FALSE;
            }
            break;
        }
    }


    return TRUE;
}

// from left to right order
static BOOL expression_mult_div(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(!expression_monadic_operator(node, info, sline_top, lv_table)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**info->p) {
        BOOL quote;
        char* p_saved;

        p_saved = *info->p;
        parse_quote(info->p, info->sline, &quote);

        if(**info->p == '*' && *(*info->p+1) != '=') {
            unsigned int right;

            right = 0;

            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);
            if(!expression_monadic_operator(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value1", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpMult, *node, right, 0, quote);
        }
        else if(**info->p == '/' && *(*info->p+1) != '=') {
            unsigned int right;

            right = 0;

            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_monadic_operator(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value2", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpDiv, *node, right, 0, quote);
        }
        else if(**info->p == '%' && *(*info->p+1) != '=') {
            unsigned int right;

            right = 0;

            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_monadic_operator(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value3", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpMod, *node, right, 0, quote);
        }
        else {
            *info->p = p_saved;
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_add_sub(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(!expression_mult_div(node, info, sline_top, lv_table)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**info->p) {
        BOOL quote;
        char* p_saved;

        p_saved = *info->p;
        parse_quote(info->p, info->sline, &quote);

        if(**info->p == '+' && *(*info->p+1) != '=' && *(*info->p+1) != '+') {
            unsigned int right;

            right = 0;

            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            right = 0;

            if(!expression_mult_div(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpAdd, *node, right, 0, quote);
        }
        else if(**info->p == '-' && *(*info->p+1) != '=' && *(*info->p+1) != '-' && *(*info->p+1) != '>') {
            unsigned int right;

            right = 0;

            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_mult_div(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value4", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpSub, *node, right, 0, quote);
        }
        else {
            *info->p = p_saved;
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_shift(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(!expression_add_sub(node, info, sline_top, lv_table)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**info->p) {
        BOOL quote;
        char* p_saved;

        p_saved = *info->p;
        parse_quote(info->p, info->sline, &quote);

        if(**info->p == '<' && *(*info->p+1) == '<' && *(*info->p+2) != '=') {
            unsigned int right;

            right = 0;

            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_add_sub(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value5", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpLeftShift, *node, right, 0, quote);
        }
        else if(**info->p == '>' && *(*info->p+1) == '>' && *(*info->p+2) != '=') {
            unsigned int right;

            right = 0;

            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_add_sub(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value6", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpRightShift, *node, right, 0, quote);
        }
        else {
            *info->p = p_saved;
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_comparison_operator(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(!expression_shift(node, info, sline_top, lv_table)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**info->p) {
        BOOL quote;
        char* p_saved;

        p_saved = *info->p;
        parse_quote(info->p, info->sline, &quote);

        if(**info->p == '>' && *(*info->p+1) == '=') {
            unsigned int right;

            right = 0;

            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_shift(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value7", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComparisonGreaterEqual, *node, right, 0, quote);
        }
        else if(**info->p == '<' && *(*info->p+1) == '=') {
            unsigned int right;

            right = 0;

            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_shift(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value8", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComparisonLesserEqual, *node, right, 0, quote);
        }
        else if(**info->p == '>' && *(*info->p+1) != '>') {
            unsigned int right;

            right = 0;

            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_shift(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value9", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComparisonGreater, *node, right, 0, quote);
        }
        else if(**info->p == '<' && *(*info->p+1) != '<') {
            unsigned int right;

            right = 0;

            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_shift(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value10", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComparisonLesser, *node, right, 0, quote);
        }
        else {
            *info->p = p_saved;
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_comparison_equal_operator(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(!expression_comparison_operator(node, info, sline_top, lv_table)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**info->p) {
        BOOL quote;
        char* p_saved;

        p_saved = *info->p;
        parse_quote(info->p, info->sline, &quote);

        if(**info->p == '=' && *(*info->p+1) == '=') {
            unsigned int right;

            right = 0;

            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_comparison_operator(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value11", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComparisonEqual, *node, right, 0, quote);
        }
        else if(**info->p == '!' && *(*info->p+1) == '=') {
            unsigned int right;

            right = 0;

            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_comparison_operator(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value12", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpComparisonNotEqual, *node, right, 0, quote);
        }
        else if(**info->p == '=' && *(*info->p+1) == '~') {
            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }

            if(**info->p == '(') {
                unsigned int param_node;

                param_node = 0;

                if(!get_params(info, &param_node, '(', ')', lv_table, NULL, NULL, FALSE))
                {
                    return FALSE;
                }

                if(param_node == 0) {
                    parser_err_msg("require parametor value", info->sname, sline_top);
                    (*info->err_num)++;
                }

                *node = sNodeTree_create_operand(kOpComparisonEqualTilda, *node, param_node, 0, quote);
            }
            else {
                unsigned int right;

                right = 0;
            
                if(!expression_comparison_operator(&right, info, sline_top, lv_table)) 
                {
                    return FALSE;
                }

                if(right == 0) {
                    parser_err_msg("require right value13", info->sname, sline_top);
                    (*info->err_num)++;
                }

                right = sNodeTree_create_param(0, right,  0);

                *node = sNodeTree_create_operand(kOpComparisonEqualTilda, *node, right, 0, quote);
            }
        }
        else {
            *info->p = p_saved;
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_and(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(!expression_comparison_equal_operator(node, info, sline_top, lv_table)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**info->p) {
        BOOL quote;
        char* p_saved;

        p_saved = *info->p;
        parse_quote(info->p, info->sline, &quote);

        if(**info->p == '&' && *(*info->p+1) != '&' && *(*info->p+1) != '=') {
            unsigned int right;

            right = 0;

            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_comparison_equal_operator(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value14", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpAnd, *node, right, 0, quote);
        }
        else {
            *info->p = p_saved;
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_xor(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(!expression_and(node, info, sline_top, lv_table)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**info->p) {
        BOOL quote;
        char* p_saved;

        p_saved = *info->p;
        parse_quote(info->p, info->sline, &quote);

        if(**info->p == '^' && *(*info->p+1) != '=') {
            unsigned int right;

            right = 0;

            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_and(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value15", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpXor, *node, right, 0, quote);
        }
        else {
            *info->p = p_saved;
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_or(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(!expression_xor(node, info, sline_top, lv_table)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**info->p) {
        BOOL quote;
        char* p_saved;

        p_saved = *info->p;
        parse_quote(info->p, info->sline, &quote);

        if(**info->p == '|' && *(*info->p+1) != '=' && *(*info->p+1) != '|') {
            unsigned int right;

            right = 0;

            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_xor(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value16", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpOr, *node, right, 0, quote);
        }
        else {
            *info->p = p_saved;
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_and_and(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(!expression_or(node, info, sline_top, lv_table)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**info->p) {
        BOOL quote;
        char* p_saved;

        p_saved = *info->p;
        parse_quote(info->p, info->sline, &quote);

        if(**info->p == '&' && *(*info->p+1) == '&') {
            unsigned int right;

            right = 0;

            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_or(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value17", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpAndAnd, *node, right, 0, quote);
        }
        else {
            *info->p = p_saved;
            break;
        }
    }

    return TRUE;
}

static BOOL expression_or_or(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(!expression_and_and(node, info, sline_top, lv_table)) 
    {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**info->p) {
        BOOL quote;
        char* p_saved;

        p_saved = *info->p;
        parse_quote(info->p, info->sline, &quote);

        if(**info->p == '|' && *(*info->p+1) == '|') {
            unsigned int right;

            right = 0;

            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_and_and(&right, info, sline_top, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value18", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpOrOr, *node, right, 0, quote);
        }
        else {
            *info->p = p_saved;
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_conditional_operator(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(!expression_or_or(node, info, sline_top, lv_table)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**info->p) {
        if(**info->p == '?') {
            unsigned int middle;
            unsigned int right;

            middle = 0;
            right = 0;

            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!node_expression(&middle, info, lv_table)) {
                return FALSE;
            }

            expect_next_character_with_one_forward(":", info->err_num, info->p, info->sname, info->sline);

            if(!node_expression(&right, info, lv_table)) {
                return FALSE;
            }

            if(*node == 0) {
                parser_err_msg("require left value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(middle == 0) {
                parser_err_msg("require middle value", info->sname, sline_top);
                (*info->err_num)++;
            }
            if(right == 0) {
                parser_err_msg("require right value19", info->sname, sline_top);
                (*info->err_num)++;
            }

            *node = sNodeTree_create_operand(kOpConditional, *node, right, middle, FALSE);
        }
        else {
            break;
        }
    }

    return TRUE;
}

static BOOL expression_range(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(!expression_conditional_operator(node, info, sline_top, lv_table)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**info->p) {
        if(**info->p == '.' && *(*info->p+1) == '.') {
            unsigned int tail;

            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!expression_conditional_operator(&tail, info, sline_top, lv_table)) {
                return FALSE;
            }

            *node = sNodeTree_create_range(*node, tail);
        }
        else {
            break;
        }
    }

    return TRUE;
}

// from left to right order
static BOOL expression_comma(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(!expression_range(node, info, sline_top, lv_table)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**info->p) {
        if(**info->p == ',') {
            int sline2;
            unsigned int elements_num;

            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            sline2 = *info->sline;

            *node = sNodeTree_create_param(0, *node,  0);
            elements_num = 1;

            while(**info->p) {
                unsigned int new_node2;

                if(!node_expression_without_comma(&new_node2, info, lv_table)) {
                    return FALSE;
                }
                skip_spaces_and_lf(info->p, info->sline);

                if(new_node2) {
                    if(elements_num < CL_TUPLE_ELEMENTS_MAX) {
                        *node = sNodeTree_create_param(*node, new_node2,  0);
                        elements_num++;
                    }
                    else {
                        parser_err_msg_format(info->sname, sline2, "number of tuple elements overflow");
                        return FALSE;
                    }
                }

                if(**info->p == ',') {
                    (*info->p)++;
                    skip_spaces_and_lf(info->p, info->sline);
                }
                else {
                    break;
                }
            }

            *node = sNodeTree_create_tuple(*node, 0, 0);
        }
        else {
            break;
        }
    }

    return TRUE;
}

static BOOL expression_substitution(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table);

static BOOL substitution_node(unsigned int* node, sParserInfo* info, enum eNodeSubstitutionType substitution_type, int sline_top, sVarTable* lv_table, BOOL quote)
{
    unsigned int right;

    right = 0;

    if(!expression_substitution(&right, info, sline_top, lv_table)) {
        return FALSE;
    }

    if(*node == 0) {
        parser_err_msg("require left value", info->sname, sline_top);
        (*info->err_num)++;
    }
    if(right == 0) {
        parser_err_msg("require right value20", info->sname, sline_top);
        (*info->err_num)++;
    }

    if(*node > 0 && right > 0) {
        switch(gNodes[*node].mNodeType) {
            case NODE_TYPE_VARIABLE_NAME:
                gNodes[*node].mNodeType = NODE_TYPE_STORE_VARIABLE_NAME;
                gNodes[*node].mRight = right;
                gNodes[*node].uValue.sVarName.mNodeSubstitutionType = substitution_type;
                gNodes[*node].uValue.sVarName.mQuote = quote;
                break;

            case NODE_TYPE_DEFINE_VARIABLE_NAME: {
                gNodes[*node].mNodeType = NODE_TYPE_DEFINE_AND_STORE_VARIABLE_NAME;
                gNodes[*node].mRight = right;
                gNodes[*node].uValue.sVarName.mNodeSubstitutionType = substitution_type;
                gNodes[*node].uValue.sVarName.mQuote = quote;
                }
                break;

            case NODE_TYPE_FIELD:
                gNodes[*node].mNodeType = NODE_TYPE_STORE_FIELD;
                gNodes[*node].mRight = right;
                gNodes[*node].uValue.sVarName.mNodeSubstitutionType = substitution_type;
                gNodes[*node].uValue.sVarName.mQuote = quote;
                break;

            case NODE_TYPE_CLASS_FIELD:
                gNodes[*node].mNodeType = NODE_TYPE_STORE_CLASS_FIELD;
                gNodes[*node].mRight = right;
                gNodes[*node].uValue.sVarName.mNodeSubstitutionType = substitution_type;
                gNodes[*node].uValue.sVarName.mQuote = quote;
                break;

            case NODE_TYPE_TUPLE_VALUE:
                gNodes[*node].mNodeType = NODE_TYPE_STORE_TUPLE;
                gNodes[*node].mRight = right;
                if(substitution_type != kNSNone) {
                    parser_err_msg("Clover can't store tuple value with operator +=, -=, *=, /=, and so on", info->sname, sline_top);
                    (*info->err_num)++;
                }
                break;

            /// a[x,y] = z
            case NODE_TYPE_OPERAND: {
                int left_node;

                left_node = gNodes[*node].mLeft;

                if(gNodes[*node].uValue.sOperand.mOperand == kOpIndexing && left_node != 0)
//                if(gNodes[*node].uValue.mOperand == kOpIndexing && left_node != 0 && (gNodes[left_node].mNodeType == NODE_TYPE_VARIABLE_NAME || gNodes[left_node].mNodeType == NODE_TYPE_FIELD || gNodes[left_node].mNodeType == NODE_TYPE_CLASS_FIELD))
                {
                    gNodes[*node].uValue.sOperand.mOperand = kOpSubstitutionIndexing;
                    gNodes[*node].mMiddle = right;
                    gNodes[*node].uValue.sVarName.mNodeSubstitutionType = substitution_type;
                    gNodes[*node].uValue.sVarName.mQuote = quote;
                }
                else {
                    parser_err_msg("require variable name on left node of equal", info->sname, sline_top);
                    (*info->err_num)++;
                }
                }
                break;

            default:
                parser_err_msg("require variable name on left node of equal", info->sname, sline_top);
                (*info->err_num)++;
                break;
        }
    }

    return TRUE;
}

// from right to left order
static BOOL expression_substitution(unsigned int* node, sParserInfo* info, int sline_top, sVarTable* lv_table)
{
    if(!expression_comma(node, info, sline_top, lv_table)) {
        return FALSE;
    }
    if(*node == 0) {
        return TRUE;
    }

    while(**info->p) {
        BOOL quote;
        char* p_saved;

        p_saved = *info->p;
        parse_quote(info->p, info->sline, &quote);

        if(**info->p == '+' && *(*info->p+1) == '=') {
            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!substitution_node(node, info, kNSPlus, sline_top, lv_table, quote)) 
            {
                return FALSE;
            }
        }
        else if(**info->p == '-' && *(*info->p+1) == '=') {
            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!substitution_node(node, info, kNSMinus, sline_top, lv_table, quote)) {
                return FALSE;
            }
        }
        else if(**info->p == '*' && *(*info->p+1) == '=') {
            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!substitution_node(node, info, kNSMult, sline_top, lv_table, quote)) {
                return FALSE;
            }
        }
        else if(**info->p == '/' && *(*info->p+1) == '=') {
            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!substitution_node(node, info, kNSDiv, sline_top, lv_table, quote)) {
                return FALSE;
            }
        }
        else if(**info->p == '%' && *(*info->p+1) == '=') {
            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!substitution_node(node, info, kNSMod, sline_top, lv_table, quote)) {
                return FALSE;
            }
        }
        else if(**info->p == '<' && *(*info->p+1) == '<' && *(*info->p+2) == '=') {
            (*info->p)+=3;
            skip_spaces_and_lf(info->p, info->sline);

            if(!substitution_node(node, info, kNSLShift, sline_top, lv_table, quote)) {
                return FALSE;
            }
        }
        else if(**info->p == '>' && *(*info->p+1) == '>' && *(*info->p+2) == '=') {
            (*info->p)+=3;
            skip_spaces_and_lf(info->p, info->sline);

            if(!substitution_node(node, info, kNSRShift, sline_top, lv_table, quote)) {
                return FALSE;
            }
        }
        else if(**info->p == '&' && *(*info->p+1) == '=') {
            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!substitution_node(node, info, kNSAnd, sline_top, lv_table, quote)) {
                return FALSE;
            }
        }
        else if(**info->p == '^' && *(*info->p+1) == '=') {
            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!substitution_node(node, info, kNSXor, sline_top, lv_table, quote)) {
                return FALSE;
            }
        }
        else if(**info->p == '|' && *(*info->p+1) == '=') {
            (*info->p)+=2;
            skip_spaces_and_lf(info->p, info->sline);

            if(!substitution_node(node, info, kNSOr, sline_top, lv_table, quote)) {
                return FALSE;
            }
        }
        else if(**info->p == '=' && *(*info->p+1) != '>') {
            (*info->p)++;
            skip_spaces_and_lf(info->p, info->sline);

            if(!substitution_node(node, info, kNSNone, sline_top, lv_table, quote)) {
                return FALSE;
            }
        }
        else {
            *info->p = p_saved;
            break;
        }
    }

    return TRUE;
}

BOOL node_expression(unsigned int* node, sParserInfo* info, sVarTable* lv_table)
{
    int sline_top;
    BOOL result;

    skip_spaces_and_lf(info->p, info->sline);

    sline_top = *info->sline;

    *node = 0;

    result = expression_substitution(node, info, sline_top, lv_table);

    return result;
}

BOOL node_expression_without_comma(unsigned int* node, sParserInfo* info, sVarTable* lv_table)
{
    int sline_top;
    BOOL result;

    skip_spaces_and_lf(info->p, info->sline);

    sline_top = *info->sline;

    *node = 0;

    result = expression_range(node, info, sline_top, lv_table);

    return result;
}
