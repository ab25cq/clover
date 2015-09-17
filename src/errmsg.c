#include "clover.h"
#include "common.h"

BOOL gParserOutput = TRUE;

void parser_err_msg(char* msg, char* sname, int sline_top)
{
    if(gParserOutput) {
        fprintf(stderr, "%s %d: %s\n", sname, sline_top, msg);
    }
}

void parser_err_msg_format(char* sname, int sline, char* msg, ...)
{
    if(gParserOutput) {
        char msg2[1024];

        va_list args;
        va_start(args, msg);
        vsnprintf(msg2, 1024, msg, args);
        va_end(args);

        printf("%s %d: %s\n", sname, sline, msg2);
    }
}

void parser_err_msg_without_line(char* msg, ...)
{
    if(gParserOutput) {
        char msg2[1024];

        va_list args;
        va_start(args, msg);
        vsnprintf(msg2, 1024, msg, args);
        va_end(args);

        printf("%s", msg2);
    }
}

void show_node_type_for_errmsg(sCLNodeType* node_type)
{
    int i;

    if(node_type == NULL) {
        parser_err_msg_without_line("NULL");
    }
    else if(node_type->mGenericsTypesNum == 0) {
        if(node_type->mClass) {
            parser_err_msg_without_line("%s", REAL_CLASS_NAME(node_type->mClass));
        }
        else {
            parser_err_msg_without_line("NULL");
        }
    }
    else {
        if(node_type->mClass == NULL) {
            parser_err_msg_without_line("NULL<");
        }
        else {
            parser_err_msg_without_line("%s<", REAL_CLASS_NAME(node_type->mClass));
        }
        for(i=0; i<node_type->mGenericsTypesNum; i++) {
            show_node_type_for_errmsg(node_type->mGenericsTypes[i]);
            if(i != node_type->mGenericsTypesNum-1) { parser_err_msg_without_line(","); }
        }
        parser_err_msg_without_line(">");
    }
}

void show_type_for_errmsg(sCLClass* klass, sCLType* type)
{
    sCLNodeType* node_type;

    node_type = ALLOC create_node_type_from_cl_type(type, klass);

    show_node_type_for_errmsg(node_type);
}

void show_method_for_errmsg(sCLClass* klass, sCLMethod* method)
{
    int i;

    /// result ///
    show_type_for_errmsg(klass, &method->mResultType);
    parser_err_msg_without_line(" ");

    /// name ///
    parser_err_msg_without_line("%s(", METHOD_NAME2(klass, method));

    /// params ///
    for(i=0; i<method->mNumParams; i++) {
        show_type_for_errmsg(klass, &method->mParamTypes[i]);

        if(i != method->mNumParams-1) parser_err_msg_without_line(",");
    }

    /// block ///
    if(method->mNumBlockType == 0) {
        parser_err_msg_without_line(") with no block\n");
    }
    else {
        parser_err_msg_without_line(") with ");
        show_type_for_errmsg(klass, &method->mBlockType.mResultType);
        parser_err_msg_without_line(" block {|");

        for(i=0; i<method->mBlockType.mNumParams; i++) {
            show_type_for_errmsg(klass, &method->mBlockType.mParamTypes[i]);

            if(i != method->mBlockType.mNumParams-1) parser_err_msg_without_line(",");
        }

        parser_err_msg_without_line("|}\n");
    }
}

void show_all_method_for_errmsg(sCLClass* klass, char* method_name)
{
    int i;
    for(i=klass->mNumMethods-1; i>=0; i--) {                    // search for method in reverse because we want to get last defined method
        if(strcmp(METHOD_NAME(klass, i), method_name) == 0) {
            sCLMethod* method;
            
            method = klass->mMethods + i;

            show_method_for_errmsg(klass, method);
        }
    }
}
