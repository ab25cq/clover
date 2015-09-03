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
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>

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

char** gClassNames;

char* on_complete(const char* text, int a)
{
    char* p;
    char* text2;
    char** candidates;
    int num_candidates;
    BOOL inputing_method_name;
    sCLNodeType* type_;
    FILE* f;
    FILE* f2;
    char command[1024];
    sBuf output;
    char fname[PATH_MAX];
    char* home;

    /// get source stat ///
    inputing_method_name = FALSE;

    text2 = STRDUP((char*)rl_line_buffer);

    p = text2 + strlen(text2) -1;
    while(p >= text2) {
        if(isalpha(*p)) {
            p--;
        }
        else {
            break;
        }
    }

    if(*p == '.') {
        inputing_method_name = TRUE;
        *p = 0;
    }

    /// parse source ///
    home = getenv("HOME");

    assert(home != NULL);

    snprintf(fname, PATH_MAX, "%s/.clover/pclover_tmp", home);

    f2 = fopen(fname, "w");
    if(f2 == NULL) {
        fprintf(stderr, "fopen(3) is failed");
        FREE(text2);
        return NULL;
    }
    fprintf(f2, "%s", text2);
    fclose(f2);

    snprintf(command, 1024, "pclover get_type %s¥n", fname);
    
    f = popen(command, "r");

    if(f == NULL) {
        fprintf(stderr, "popen(2) is failed");
        FREE(text2);
        return NULL;
    }

    sBuf_init(&output);

    while(1) {
        char buf[BUFSIZ];
        int size;
        
        size = fread(buf, 1, BUFSIZ, f);
        sBuf_append(&output, buf, size);
        
        if(size < BUFSIZ) {
            break;
        }
    }
    (void)pclose(f);

    FREE(output.mBuf);
    FREE(text2);

    candidates = NULL;
    num_candidates = 0;

/*
    /// class completion ///
    if(0) {
    }
    /// method completion ///
    else if(inputing_method_name) {
        sNodeTree* node_tree;
        sCLNodeType* type;
        sCLClass* klass;

        if(num_nodes > 0) {
            sCLClass* klass;
            sCLNodeType* type;

            node_tree = gNodes + nodes[num_nodes-1];

            type = type_;
            if(type) {
                klass = type->mClass;
            }

            if(klass) {
                candidates = ALLOC ALLOC get_method_names_with_arguments(klass);
                num_candidates = klass->mNumMethods;
            }
        }
    }
*/

    /// sort ///

    /// go ///
    if(candidates) {
        /// get candidates ///
        char* candidate;
        char** p2;
        char** candidates2;
        int num_candidates2;
        int j;

        p2 = candidates;

        candidates2 = CALLOC(1, sizeof(char*)*(num_candidates+1));
        num_candidates2 = 0;

        while((candidate = *p2) != NULL) {
            int len_candidate;
            int len_text;

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

            candidate = *candidates2;

            parenthesis = strstr(candidate, "(");

            if(parenthesis) {
                len_candidate = parenthesis - candidate;
            }
            else {
                len_candidate = strlen(candidate);
            }
            len_text = strlen(text);

            appended_chars = CALLOC(1, len_candidate-len_text+2);
            memcpy(appended_chars, candidate+len_text, len_candidate-len_text);
            appended_chars[len_candidate-len_text] = 0;

            rl_insert_text(appended_chars);

            FREE(appended_chars);

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
                    appended_chars = CALLOC(1, same_len-len_text+2);
                    memcpy(appended_chars, candidate+len_text, same_len-len_text);
                    appended_chars[same_len-len_text] = 0;

                    rl_insert_text(appended_chars);

                    FREE(appended_chars);
                }
            }
            else if(same_len == 0) {
                display_candidates(candidates2);
                rl_forced_update_display();
            }
        }

        FREE(candidates2);

        for(j=0; j<num_candidates; j++) {
            FREE(candidates[j]);
        }
        FREE(candidates);
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
    char cmd[512];
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
    snprintf(cmd, 512, "cclover --output-value '%s'", fname);
    system(cmd);

    /// eval ///
    if(!cl_eval_file(fname)) {
        return FALSE;
    }

    return TRUE;
}

int main(int argc, char** argv)
{
    CHECKML_BEGIN

    setlocale(LC_ALL, "");
    srandom((unsigned)time(NULL));

    set_signal_for_interpreter();

    if(!cl_init(1024, 512)) {
        exit(1);
    }

    if(!cl_load_fundamental_classes()) {
        fprintf(stderr, "can't load fundamental class\n");
        exit(1);
    }

    rl_basic_word_break_characters = "\t\n;.+-/*%&|{}?<>:[]^()'\"!";
    rl_completer_word_break_characters = "\t\n;.+-/*%&|{}?<>:[]^()'\"!";
    //rl_attempted_completion_function = on_complete;
    rl_completion_entry_function = on_complete;

    while(1) {
        char* line;

        gClassNames = ALLOC get_class_names();

        line = readline("clover > ");

        if(line == NULL) {
            FREE(gClassNames);
            break;
        }

        if(!eval_str(line)) {
            fprintf(stderr, "error\n");
        }
        else {
            add_history(line);
        }

        FREE(gClassNames);

        free(line);
    }

    cl_final();

    CHECKML_END

    exit(0);
}
