#include "config.h"
#include "clover.h"
#include "common.h"
#include <locale.h>

#if !defined(__CYGWIN__)
#include <term.h>
#endif

#if defined(__LINUX__) || defined(__DARWIN__)
#include <signal.h>
#endif

#include <termios.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <dirent.h>
#include <oniguruma.h>
#include <sys/stat.h>
#include <pwd.h>
#include <limits.h>

#if defined(HAVE_CURSES_H)
#include <curses.h>
#elif defined(HAVE_NCURSES_H)
#include <ncurses.h>
#elif defined(HAVE_NCURSES_NCURSES_H)
#include <ncurses/ncurses.h>
#endif

#include <histedit.h>

static EditLine* gEditLine;
static HistoryW* gHistory;

static void sig_int_editline(int signo)
{
    printf("\r\033[0K");
    //tty_rawmode(gEditLine);  // I don't know why tty settings is changed by CTRL-C signal handler
}

static void editline_signal()
{
    struct sigaction sa2;

    memset(&sa2, 0, sizeof(sa2));
    sa2.sa_handler = sig_int_editline;
    //sa2.sa_flags |= SA_RESTART;

    if(sigaction(SIGINT, &sa2, NULL) < 0) {
        perror("sigaction2");
        exit(1);
    }

    memset(&sa2, 0, sizeof(sa2));
    sa2.sa_handler = SIG_IGN;
    sa2.sa_flags = 0;
    if(sigaction(SIGTSTP, &sa2, NULL) < 0) {
        perror("sigaction2");
        exit(1);
    }

    memset(&sa2, 0, sizeof(sa2));
    sa2.sa_handler = SIG_IGN;
    sa2.sa_flags = 0;
    if(sigaction(SIGUSR1, &sa2, NULL) < 0) {
        perror("sigaction2");
        exit(1);
    }
    sa2.sa_handler = SIG_IGN;
    sa2.sa_flags = 0;
    if(sigaction(SIGQUIT, &sa2, NULL) < 0) {
        perror("sigaction2");
        exit(1);
    }
}

static wchar_t* gEditlinePrompt = NULL;
static wchar_t* gEditlineRPrompt = NULL;

static wchar_t* editline_prompt(EditLine* el)
{
    return gEditlinePrompt;
}

static wchar_t* editline_rprompt(EditLine* el)
{
    return gEditlineRPrompt;
}

ALLOC char* editline(char* prompt, char* rprompt)
{
    int len;
    wchar_t* wprompt;
    wchar_t* wrprompt;
    int numc;
    const wchar_t* line;
    char* result;

    editline_signal();

    if(prompt == NULL) prompt = " > ";

    len = strlen(prompt) + 1;
    wprompt = MALLOC(sizeof(wchar_t)*len);
    mbstowcs(wprompt, prompt, len);
    gEditlinePrompt = wprompt;

    if(rprompt) {
        const int len2 = strlen(rprompt) + 1;

        wrprompt = MALLOC(sizeof(wchar_t)*len2);
        mbstowcs(wrprompt, rprompt, len);
        gEditlineRPrompt = wrprompt;
        el_wset(gEditLine, EL_RPROMPT_ESC, editline_rprompt, '\1');
    }
    else {
        wrprompt = NULL;
        el_wset(gEditLine, EL_RPROMPT_ESC, NULL);
    }

    numc = 0;
    do {
        line = el_wgets(gEditLine, &numc);
    } while(numc == -1);

    if(numc == 0 && line == NULL) {
        result = NULL;
    }
    else {
        HistEventW ev;
        int size;

        history_w(gHistory, &ev, H_ENTER, line);

        size = MB_LEN_MAX * (wcslen(line) + 1);
        result = MALLOC(size);
        wcstombs(result, line, size);
    }

    FREE(wprompt);
    if(wrprompt) FREE(wrprompt);

    return ALLOC result;
}

static void cl_editline_history_init()
{
    /// set history size ///
    int history_size;
    char* histsize_env;
    char histfile[PATH_MAX]; 
    char* histfile_env;
    HistEventW ev;
    
    histsize_env = getenv("CLOVER_HISTSIZE");
    if(histsize_env) {
        char buf[256];

        history_size = atoi(histsize_env);
        if(history_size < 0) history_size = 1000;
        if(history_size > 50000) history_size = 50000;

        snprintf(buf, 256, "%d", history_size);
        setenv("CLOVER_HISTSIZE", buf, 1);
    }
    else {
        char buf[256];

        history_size = 1000;
        snprintf(buf, 256, "%d", history_size);
        setenv("CLOVER_HISTSIZE", buf, 1);
    }

    /// set history file name ///
    histfile_env = getenv("CLOVER_HISTFILE");
    if(histfile_env == NULL) {
        char* home = getenv("HOME");

        if(home) {
            char clover_home[PATH_MAX];

            snprintf(histfile, PATH_MAX, "%s/.clover/history", home);
            snprintf(clover_home, PATH_MAX, "%s/.clover", home);
            mkdir(clover_home, 0700);
        }
        else {
            fprintf(stderr, "HOME evironment path is NULL. exited\n");
            exit(1);
        }
    }
    else {
        xstrncpy(histfile, histfile_env, PATH_MAX);
    }

    gHistory = history_winit();
    history_w(gHistory, &ev, H_SETSIZE, history_size);
    history_w(gHistory, &ev, H_LOAD, histfile);

    el_wset(gEditLine, EL_HIST, history_w, gHistory);
}

void cl_editline_init()
{
    /// editline init ///
    gEditLine = el_init("clover", stdin, stdout, stderr);

    el_wset(gEditLine, EL_EDITOR, L"emacs");
    el_wset(gEditLine, EL_SIGNAL, 1);
    el_wset(gEditLine, EL_PROMPT_ESC, editline_prompt, '\1');

    el_source(gEditLine, NULL);
    cl_editline_history_init();
}

void cl_editline_final()
{
    int i;
    char* histfile;

    /// write history ///
    histfile = getenv("CLOVER_HISTFILE");

    if(histfile) {
        HistEventW ev;
        history_w(gHistory, &ev, H_SAVE, histfile);
    }
    else {
        char* home = getenv("HOME");

        if(home) {
            char path[PATH_MAX];
            HistEventW ev;

            snprintf(path, PATH_MAX, "%s/.clover/history", home);
            history_w(gHistory, &ev, H_SAVE, path);
        }
        else {
            fprintf(stderr, "HOME evironment path is NULL. exited\n");
            exit(1);
        }
    }

    /// history end ///
    history_wend(gHistory);

    /// Editline end ///
    el_end(gEditLine);
}

int main(int argc, char** argv) 
{
    CHECKML_BEGIN

    setlocale(LC_ALL, "");

    cl_init(1024, 1024, 1024, 512, TRUE);
    cl_editline_init();

    while(1) {
        char* line;
        int sline;

        line = ALLOC editline("clover : ", NULL);

        if(line == NULL) {
            break;
        }

        sline = 1;
        (void)cl_eval(line, "cmdline", &sline);

        if(line) { FREE(line); }
    }

    cl_editline_final();
    cl_final();

    CHECKML_END

    exit(0);
}
