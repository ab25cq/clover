#include "clover.h"
#include "common.h"
#include <locale.h>
#define _GNU_SOURCE
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>

static int mgetmaxx()
{
    struct winsize ws;
    ioctl(1, TIOCGWINSZ, &ws);

    return ws.ws_col;
}

static int mgetmaxy()
{
    struct winsize ws;
    ioctl(1, TIOCGWINSZ, &ws);

    return ws.ws_row;
}

void display_candidates(char** candidates)
{
    char* candidate;
    char** p;
    int max_len;
    int maxx;
    int cols;
    int n;

    p = candidates;

    max_len = -1;

    while((candidate = *p) != NULL) {
        int len;

        len = strlen(candidate);

        if(len > max_len) {
            max_len = len;
        }
        p++;
    }

    maxx = mgetmaxx();

    cols = maxx / (max_len + 2);

    if(cols == 0) {
        cols = 1;
    }

    n = 0;

    puts("");

    p = candidates;
    while((candidate = *p) != NULL) {
        char format[128];
        format[0] = '%';
        format[1] = '-';
        snprintf(format + 2, 128-2, "%d", max_len + 2);
        xstrncat(format, "s", 128);

        printf(format, candidate);
        n++;
        if(n == cols) {
            puts("");
            n = 0;
        }
        p++;
    }

    puts("");
}

static void get_class_name_or_namespace_name(char* result, char** p)
{
    if(isalpha(**p)) {
        while(isalpha(**p) || **p == '_' || isdigit(**p)) {
            *result = **p;
            result++;
            (*p)++;
        }
    }

    if(**p == '$') {
        *result = **p;
        result++;
        (*p)++;

       while(isdigit(**p)) {
           *result = **p;
           result++;
           (*p)++;
       }
    }

    while(**p == ' ' || **p == '\t' || **p == '\n') (*p)++;

    *result = 0;
}

static BOOL parse_class_name(char** p, sCLClass** klass)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX+1];

    get_class_name_or_namespace_name(real_class_name, p);

    if(**p == ':' && *(*p+1) == ':') {
        char real_class_name2[CL_REAL_CLASS_NAME_MAX];

        (*p)+=2;
        while(**p == ' ' || **p == '\t' || **p == '\n') (*p)++;

        get_class_name_or_namespace_name(real_class_name2, p);

        xstrncat(real_class_name, "::", CL_REAL_CLASS_NAME_MAX);
        xstrncat(real_class_name, real_class_name2, CL_REAL_CLASS_NAME_MAX);
    }

    *klass = cl_get_class(real_class_name);

    return TRUE;
}

ALLOC char* cl_type_to_buffer(sCLType* cl_type, sCLClass* klass)
{
    int i;
    sBuf buf;

    sBuf_init(&buf);

    if(cl_type == NULL) {
        sBuf_append_str(&buf, "NULL");
    }
    else if(cl_type->mGenericsTypesNum == 0) {
        sBuf_append_str(&buf, CONS_str(&klass->mConstPool, cl_type->mClassNameOffset));
    }
    else {
        if(cl_type->mClassNameOffset == 0) {
            sBuf_append_str(&buf, "NULL<");
        }
        else {
            sBuf_append_str(&buf, CONS_str(&klass->mConstPool, cl_type->mClassNameOffset));
            sBuf_append_str(&buf, "<");
        }
        for(i=0; i<cl_type->mGenericsTypesNum; i++) {
            char* result;

            result = cl_type_to_buffer(cl_type->mGenericsTypes[i], klass);

            sBuf_append_str(&buf, result);

            MFREE(result);
            if(i != cl_type->mGenericsTypesNum-1) { sBuf_append_str(&buf, ","); }
        }
        sBuf_append_str(&buf, ">");
    }

    return buf.mBuf;
}

ALLOC ALLOC char** get_method_names_with_arguments(sCLClass* klass, BOOL class_method, int* num_methods)
{
    int i;
    char** result;
    int result_size;
    int result_num;

    result_size = 128;
    result = MCALLOC(1, sizeof(char*)*result_size);
    result_num = 0;

    *num_methods = 0;

    for(i=0; i<klass->mNumMethods; i++) {
        sCLMethod* method;
        sBuf buf;
        int j;

        method = klass->mMethods + i;

        if((class_method && (method->mFlags & CL_CLASS_METHOD)) || (!class_method && !(method->mFlags & CL_CLASS_METHOD))) 
        {
            sBuf_init(&buf);

            sBuf_append_str(&buf, METHOD_NAME2(klass, method));
            sBuf_append_str(&buf, "(");

            for(j=0; j<method->mNumParams; j++) {
                sCLType* param;
                char* param_type;
                sCLNodeType* node_type;
                char* argment_names;

                param = method->mParamTypes + j;

                argment_names = ALLOC cl_type_to_buffer(param, klass);

                sBuf_append_str(&buf, argment_names);

                if(j!=method->mNumParams-1) sBuf_append_str(&buf, ",");

                MFREE(argment_names);
            }

            sBuf_append_str(&buf, ")");

            *(result+result_num) = MANAGED buf.mBuf;
            result_num++;

            if(result_num >= result_size) {
                result_size *= 2;
                result = MREALLOC(result, sizeof(char*)*result_size);
            }
        }
    }

    for(i=0; i<klass->mNumFields; i++) {
        sCLField* field;

        field = klass->mFields + i;

        if(field->mFlags & CL_STATIC_FIELD) {
            *(result+result_num) = MSTRDUP(FIELD_NAME(klass, i));
            result_num++;

            if(result_num >= result_size) {
                result_size *= 2;
                result = MREALLOC(result, sizeof(char*)*result_size);
            }
        }
    }

    *num_methods = result_num;

    *(result+result_num) = NULL;
    result_num++;

    if(result_num >= result_size) {
        result_size *= 2;
        result = MREALLOC(result, sizeof(char*)*result_size);
    }

    return result;
}

static BOOL run_parser(char* parser_command, char* source, ALLOC sBuf* output)
{
    char* home;
    char fname[PATH_MAX];
    FILE* f;
    FILE* f2;
    char command[1024];

    home = getenv("HOME");

    assert(home != NULL);

    snprintf(fname, PATH_MAX, "%s/.clover/psclover_tmp", home);

    f2 = fopen(fname, "w");
    if(f2 == NULL) {
        fprintf(stderr, "fopen(3) is failed");
        return FALSE;
    }
    fprintf(f2, "%s", source);
    fclose(f2);

    snprintf(command, 1024, "%s %s", parser_command, fname);

    f = popen(command, "r");

    if(f == NULL) {
        fprintf(stderr, "popen(2) is failed");
        return FALSE;
    }

    sBuf_init(output);

    while(1) {
        char buf[BUFSIZ];
        int size;
        
        size = fread(buf, 1, BUFSIZ, f);
        sBuf_append(output, buf, size);
        
        if(size < BUFSIZ) {
            break;
        }
    }
    (void)pclose(f);

    return TRUE;
}

char** gCandidates = NULL;
int gNumCandidates = 0;

BOOL gInputingMethod = FALSE;
BOOL gInputingPath = FALSE;
BOOL gInputingClass = FALSE;

char* on_complete(const char* text, int a)
{
    if(gInputingMethod) {
        rl_completion_append_character = '(';
    }
    else if(gInputingPath) {
        rl_completion_append_character = '"';
    }
    else if(gInputingClass) {
        rl_completion_append_character = 0;
    }

    /// sort ///

    /// go ///
    if(gCandidates) {
        /// get candidates ///
        char* candidate;
        char** p2;
        char** candidates2;
        int num_candidates2;
        int j;

        p2 = gCandidates;

        candidates2 = MCALLOC(1, sizeof(char*)*(gNumCandidates+1));
        num_candidates2 = 0;

        while(p2 < gCandidates + gNumCandidates) {
            int len_candidate;
            int len_text;

            candidate = *p2;

            len_candidate = strlen(candidate);
            len_text = strlen(text);

            if(len_candidate >= len_text && strncmp(candidate, text, len_text) == 0) 
            {
                candidates2[num_candidates2++] = candidate;
            }
            p2++;
        }

        candidates2[num_candidates2] = NULL;

        if(num_candidates2 == 0) {
        }
        else if(num_candidates2 == 1) {
            char* appended_chars;
            int len_candidate;
            int len_text;
            char* parenthesis;
            char appended_chars2[32];
            BOOL flg_field;

            candidate = *candidates2;

            flg_field = strstr(candidate, "(") == NULL;

            if(rl_completion_append_character == '(') {
                parenthesis = strstr(candidate, "(");

                if(parenthesis) {
                    len_candidate = parenthesis - candidate;
                }
                else {
                    len_candidate = strlen(candidate);
                }
            }
            else {
                len_candidate = strlen(candidate);
            }

            len_text = strlen(text);

            appended_chars = MCALLOC(1, len_candidate-len_text+2);
            memcpy(appended_chars, candidate+len_text, len_candidate-len_text);
            appended_chars[len_candidate-len_text] = 0;

            rl_insert_text(appended_chars);

            MFREE(appended_chars);

            /// path completion ///
            if(rl_completion_append_character == '"') {
                int len;

                len = strlen(candidate);
                if(candidate[len-1] != '/') {
                    appended_chars2[0] = rl_completion_append_character;
                    appended_chars2[1] = 0;

                    rl_insert_text(appended_chars2);
                }
            }
            else if(flg_field) {
                appended_chars2[0] = '.';
                appended_chars2[1] = 0;

                rl_insert_text(appended_chars2);
            }
            else if(rl_completion_append_character != 0) {
                appended_chars2[0] = rl_completion_append_character;
                appended_chars2[1] = 0;

                rl_insert_text(appended_chars2);
            }

            display_candidates(candidates2);
            rl_forced_update_display();
        }
        else {
            /// get same text ///
            char* candidate_before;
            int same_len;

            candidate_before = NULL;
            same_len = -1;
            p2 = candidates2;

            while((candidate = *p2) != NULL) {
                int i;
                int len_candidate;
                int len_candidate_before;

                if(candidate_before) {
                    int len;
                    int same_len2;
                    char* parenthesis;

                    parenthesis = strstr(candidate, "(");
                    if(parenthesis) {
                        len_candidate = parenthesis - candidate;
                    }
                    else {
                        len_candidate = strlen(candidate);
                    }

                    parenthesis = strstr(candidate_before, "(");
                    if(parenthesis) {
                        len_candidate_before = parenthesis - candidate_before;
                    }
                    else {
                        len_candidate_before = strlen(candidate_before);
                    }


                    if(len_candidate < len_candidate_before) {
                        len = len_candidate;
                    }
                    else {
                        len = len_candidate_before;
                    }

                    same_len2 = len;

                    for(i=0; i<len; i++) {
                        if(candidate[i] != candidate_before[i]) {
                            same_len2 = i;
                            break;
                        }
                    }

                    if(same_len == -1 || same_len2 < same_len) {
                        same_len = same_len2;
                    }
                }
                
                candidate_before = *p2;
                p2++;
            }

            candidate = *candidates2;

            if(same_len > 0) {
                char* appended_chars;
                int len_candidate;
                int len_text;

                len_candidate = strlen(candidate);
                len_text = strlen(text);

                if(same_len - len_text == 0) {
                    display_candidates(candidates2);
                    rl_forced_update_display();
                }
                else {
                    appended_chars = MCALLOC(1, same_len-len_text+2);
                    memcpy(appended_chars, candidate+len_text, same_len-len_text);
                    appended_chars[same_len-len_text] = 0;

                    rl_insert_text(appended_chars);

                    MFREE(appended_chars);
                }
            }
            else if(same_len == 0) {
                display_candidates(candidates2);
                rl_forced_update_display();
            }
        }

        MFREE(candidates2);

        for(j=0; j<gNumCandidates; j++) {
            MFREE(gCandidates[j]);
        }
        MFREE(gCandidates);
    }

    return 0;
}

void sig_int()
{
    rl_reset_line_state();
    rl_replace_line("", 0);
    rl_point = 0;
    puts("");
    rl_redisplay();
}

static void set_signal_for_interpreter() 
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_handler = sig_int;
    if(sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction2");
        exit(1);
    }
}

static BOOL eval_str(char* str)
{
    char fname[PATH_MAX];
    char cmd[1024];
    int value;
    FILE* f;
    char* home;

    /// make source ///
    home = getenv("HOME");

    while(1) {
        value = rand() % 9999;
        snprintf(fname, PATH_MAX, "%s/.clover/tmpfiles/interpreter%d.cl", home, value);

        if(access(fname, F_OK) != 0) {
            break;
        }
    }

    f = fopen(fname, "w");
    if(f == NULL) {
        fprintf(stderr, "Clover can't open %s\n", fname);
        return FALSE;
    }

    fprintf(f, "%s", str);
    fclose(f);

    /// compile ///
    snprintf(cmd, 1024, "cclover --output-value --no-delete-tmp-files '%s'", fname);
    system(cmd);

    /// eval ///
    if(!cl_eval_file(fname)) {
        return FALSE;
    }

    return TRUE;
}

static ALLOC char* line_buffer_from_head_to_cursor_point()
{
    char* result;

    MASSERT(rl_point >= 0 && rl_point <= strlen(rl_line_buffer));

    result = MCALLOC(1, strlen(rl_line_buffer)+1);
    memcpy(result, rl_line_buffer, rl_point);
    result[rl_point] = 0;

    return result;
}

static char** get_var_names(int* num_var_names)
{
    char* source;
    sBuf output;
    char** var_names;
    char* p;
    char* head_of_line;
    char** result;
    int result_size;
    
    source = ALLOC line_buffer_from_head_to_cursor_point();

    if(!run_parser("psclover --no-output get_variable_names", source, ALLOC &output)) {
        MFREE(source);
        *num_var_names = 0;
        return NULL;
    }

    MFREE(source);

    result_size = 128;
    result = MMALLOC(sizeof(char*)*result_size);

    p = output.mBuf;

    *num_var_names = 0;
    head_of_line = p;

    while(1) {
        if(*p == '\n') {
            char* line;
            int size;

            size = p - head_of_line;

            line = MMALLOC(size + 1);
            memcpy(line, head_of_line, size);
            line[size] = 0;

            result[*num_var_names] = MANAGED line;
            (*num_var_names)++;

            if(*num_var_names >= result_size) {
                result_size *= 2;
                result = MREALLOC(result, sizeof(char*)*result_size);
            }

            p++;
            head_of_line = p;
        }
        else if(*p == '\0') {
            char* line;
            int size;

            size = p - head_of_line;

            line = MMALLOC(size + 1);
            memcpy(line, head_of_line, size);
            line[size] = 0;

            result[*num_var_names] = MANAGED line;
            (*num_var_names)++;

            if(*num_var_names >= result_size) {
                result_size *= 2;
                result = MREALLOC(result, sizeof(char*)*result_size);
            }
            break;
        }
        else {
            p++;
        }
    }

    MFREE(output.mBuf);

    return result;
}

static int my_complete_internal(int count, int key)
{
    char* p;
    char* text2;
    char* text3;
    BOOL inputing_method_name;
    sBuf output;
    sCLClass* klass;
    sCLClass* type_class;
    BOOL class_method;
    BOOL inputing_path;
    char* line;

    gInputingMethod = FALSE;
    gInputingPath = FALSE;
    gInputingClass = FALSE;

    /// get source stat ///
    inputing_method_name = FALSE;

    /// parse source ///
    line = ALLOC line_buffer_from_head_to_cursor_point();

    text2 = MSTRDUP(line);
    text3 = MSTRDUP(line);

    p = text2 + strlen(text2) -1;
    while(p >= text2) {
        if(isalnum(*p) || *p == '_') {
            p--;
        }
        else {
            break;
        }
    }

    if(*p == '.') {
        inputing_method_name = TRUE;
        *(p+1) = 0;
        text3[p-text2] = 0;
    }

    /// parse source ///
    if(!run_parser("psclover --no-output get_type", text2, ALLOC &output)) {
        MFREE(text3);
        MFREE(text2);
        MFREE(line);
        return 0;
    }

    if(output.mBuf[0] == 0) {
        MFREE(output.mBuf);

        if(!run_parser("psclover --no-output get_type", text3, ALLOC &output)) {
            MFREE(text3);
            MFREE(text2);
            MFREE(line);
            return 0;
        }
    }

    p = output.mBuf;

    if(!parse_class_name(&p, &klass)) {
        MFREE(output.mBuf);
        MFREE(text3);
        MFREE(text2);
        MFREE(line);
        return 0;
    }

    MFREE(output.mBuf);

    type_class = cl_get_class("Type");

    if(klass == type_class) {
        if(!run_parser("psclover --no-output get_class_type", text3, ALLOC &output)) {
            MFREE(text3);
            MFREE(text2);
            MFREE(line);
            return 0;
        }

        p = output.mBuf;

        if(!parse_class_name(&p, &klass)) {
            MFREE(output.mBuf);
            MFREE(text3);
            MFREE(text2);
            MFREE(line);
            return 0;
        }

        MFREE(output.mBuf);

        class_method = TRUE;
    }
    else {
        class_method = FALSE;
    }

    MFREE(text2);
    MFREE(text3);

    if(!run_parser("psclover --no-output inputing_path", line, ALLOC &output)) {
        MFREE(line);
        return 0;
    }

    inputing_path = strstr(output.mBuf, "true") == output.mBuf;

    MFREE(output.mBuf);

    gCandidates = NULL;
    gNumCandidates = 0;

    /// path completion ///
    if(inputing_path) {
        DIR* result_opendir;
        char path[PATH_MAX];

        gInputingPath = TRUE;

        p = (char*)line + strlen(line);
        while(p >= line) {
            if(*p == '"') {
                break;
            }
            else {
                p--;
            }
        }

        if(*(p + 1) == 0) {
            result_opendir = opendir(".");
            path[0] = 0;
        }
        else {
            text2 = MSTRDUP(p + 1);

            if(text2[0] == '~') {
                char text3[PATH_MAX];
                char* home;

                home = getenv("HOME");

                if(home) {
                    if(text2[1] == '/') {
                        snprintf(text3, PATH_MAX, "%s/%s", home, text2 + 2);
                    }
                    else {
                        snprintf(text3, PATH_MAX, "%s/%s", home, text2 + 1);
                    }

                    rl_delete_text(rl_point-strlen(text2), rl_point);
                    rl_point -=strlen(text2);
                    rl_insert_text(text3);

                    result_opendir = opendir(text3);

                    xstrncpy(path, text3, PATH_MAX);
                }
                else {
                    result_opendir = opendir(text2);

                    xstrncpy(path, text2, PATH_MAX);
                }
            }

            if(text2[strlen(text2)-1] == '/') {
                result_opendir = opendir(text2);

                xstrncpy(path, text2, PATH_MAX);
            }
            else {
                char* dirname_;

                dirname_ = dirname(text2);
                result_opendir = opendir(dirname_);

                if(strcmp(dirname_, ".") == 0) {
                    path[0] = 0;
                }
                else {
                    xstrncpy(path, dirname_, PATH_MAX);

                    if(dirname_[strlen(dirname_)-1] != '/' ) {
                        xstrncat(path, "/", PATH_MAX);
                    }
                }
            }

            MFREE(text2);
        }

        if(result_opendir) {
            int n;
            int size;

            n = 0;
            size = 128;

            gCandidates = MCALLOC(1, sizeof(char*)*size);

            while(1) {
                struct dirent* result_readdir;
                int len;
                char* candidate;

                result_readdir = readdir(result_opendir);

                if(result_readdir == NULL) {
                    break;
                }

                if(strcmp(result_readdir->d_name, ".") != 0 && strcmp(result_readdir->d_name, "..") != 0)
                {
                    struct stat stat_;
                    len = strlen(path) + strlen(result_readdir->d_name) + 2 + 1 + 1;

                    candidate = MMALLOC(len);

                    xstrncpy(candidate, path, len);
                    xstrncat(candidate, result_readdir->d_name, len);

                    if(stat(candidate, &stat_) == 0) {
                        if(S_ISDIR(stat_.st_mode)) {
                            xstrncat(candidate, "/", len);
                        }

                        gCandidates[n++] = MANAGED candidate;

                        if(n >= size) {
                            size *= 2;
                            gCandidates = MREALLOC(gCandidates, sizeof(char*)*size);
                        }
                    }
                }
            }

            gCandidates[n] = NULL;

            gNumCandidates = n;

            closedir(result_opendir);
        }

        rl_completer_word_break_characters = "\t\n\"";
    }
    /// method completion ///
    else if(inputing_method_name) {
        int num_methods;

        /// class method ///
        if(class_method) {
            /// castamize completion ///
            if(klass->mCompletionMethodIndexOfClassMethod != -1) {
                sCLMethod* method;
                CLObject result_value;

                method = klass->mMethods + klass->mCompletionMethodIndexOfClassMethod;
                if(cl_excute_method(method, klass, NULL, &result_value))
                {
                    int i;
                    int n;
                    int len;

                    n = 0;

                    len = CLARRAY(result_value)->mLen;
                    gCandidates = MCALLOC(1, sizeof(char*)*(len+1));
                    for(i=0; i<len; i++) {
                        CLObject string_object;
                        char* str;

                        string_object = CLARRAY_ITEMS2(result_value, i).mObjectValue.mValue;
                        if(string_object_to_str(ALLOC &str, string_object))
                        {
                            gCandidates[n++] = MANAGED str;
                        }
                    }

                    gCandidates[n] = NULL;

                    gNumCandidates = n;
                }
            }
            else {
                num_methods = 0;
                gCandidates = ALLOC ALLOC get_method_names_with_arguments(klass, TRUE, &num_methods);
                gNumCandidates = num_methods;
            }
        }
        /// method ///
        else if(klass) {
            if(klass->mCompletionMethodIndex != -1) {
                sCLMethod* method;
                CLObject result_value;

                method = klass->mMethods + klass->mCompletionMethodIndex;
                if(cl_excute_method(method, klass, NULL, &result_value))
                {
                    int i;
                    int n;
                    int len;

                    n = 0;

                    len = CLARRAY(result_value)->mLen;
                    gCandidates = MCALLOC(1, sizeof(char*)*(len+1));
                    for(i=0; i<len; i++) {
                        CLObject string_object;
                        char* str;

                        string_object = CLARRAY_ITEMS2(result_value, i).mObjectValue.mValue;
                        if(string_object_to_str(ALLOC &str, string_object))
                        {
                            gCandidates[n++] = MANAGED str;
                        }
                    }

                    gCandidates[n] = NULL;

                    gNumCandidates = n;
                }
            }
            else {
                num_methods = 0;
                gCandidates = ALLOC ALLOC get_method_names_with_arguments(klass, FALSE, &num_methods);
                gNumCandidates = num_methods;
            }
        }

        rl_completer_word_break_characters = "\t\n.";

        gInputingMethod = TRUE;
    }
    /// class completion ///
    else {
        char** class_names;
        char** var_names;
        int num_class_names;
        int num_var_names;
        int i;

        gInputingClass = TRUE;
        
        class_names = ALLOC get_class_names(&num_class_names);

        var_names = ALLOC ALLOC get_var_names(&num_var_names);

        gCandidates = MCALLOC(1, sizeof(char*)*(num_class_names+num_var_names+1));

        for(i=0; i<num_class_names; i++) {
            gCandidates[i] = MSTRDUP(class_names[i]);
        }

        for(i=0; i<num_var_names; i++) {
            gCandidates[i+num_class_names] = MSTRDUP(var_names[i]);
        }

        gCandidates[i+num_class_names] = NULL;

        gNumCandidates = num_class_names + num_var_names;

        MFREE(class_names);
        if(var_names) {
            for(i=0; i<num_var_names; i++) {
                MFREE(var_names[i]);
            }
            MFREE(var_names);
        }

        rl_completer_word_break_characters = " \t\n.(!\"#$%&')-=~^|\{`@[]}*+;:?/><,";
    }

    MFREE(line);

    return rl_complete(0, key);
    //return rl_insert_completions(count, key);
}

static int my_bind_cr(int count, int key) 
{
    char* source;
    sBuf output;
    
    source = rl_line_buffer;

    if(!run_parser("psclover --no-output inputing_block", source, ALLOC &output)) {
        return 0;
    }

    if(strcmp(output.mBuf,"false\n") == 0) {
        puts("");
        rl_done = 1;
    }
    else {
        rl_insert_text("\n");
    }

    MFREE(output.mBuf);

    return 0;
}

int main(int argc, char** argv)
{
    CHECKML_BEGIN

    setlocale(LC_ALL, "");
    srandom((unsigned)time(NULL));

    set_signal_for_interpreter();

    if(!cl_init(1024, 512, argc, argv)) {
        exit(1);
    }

    if(!cl_load_fundamental_classes()) {
        fprintf(stderr, "can't load fundamental class\n");
        exit(1);
    }

    rl_basic_word_break_characters = "\t\n.";
    //rl_attempted_completion_function = on_complete;
    rl_completion_entry_function = on_complete;

    rl_bind_key('\t', my_complete_internal);
    rl_bind_key('\n', my_bind_cr);
    rl_bind_key('\r', my_bind_cr);

    printf("Welcome to Clover version %s\n", getenv("CLOVER_VERSION"));

    while(1) {
        char* line;

        line = readline("clover > ");

        if(line == NULL) {
            break;
        }

        if(!eval_str(line)) {
            fprintf(stderr, "error\n");
        }

        add_history(line);

        free(line);
    }

    cl_final();

    CHECKML_END

    exit(0);
}
