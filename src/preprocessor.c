#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

static BOOL call_preprocessor(sBuf* command, sBuf* output)
{
    FILE* f;
    char buf[BUFSIZ];

    f = popen(command->mBuf, "r");
    if(f == NULL) {
        parser_err_msg_without_line("popen(2) is failed on #shell");
        return FALSE;
    }
    while(1) {
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

BOOL preprocessor(sBuf* source, sBuf* source2)
{
    char* p;
    BOOL dquort;

    p = source->mBuf;

    dquort = FALSE;

    while(*p) {
        if(*p == '"') {
            dquort = !dquort;
            sBuf_append_char(source2, *p);
            p++;
        }
        else if(!dquort && ((*p == '\n' && *(p+1) == '#') || (p == source->mBuf && *p == '#'))) 
        {
            if(*p == '\n') {
                sBuf_append_char(source2, '\n');
                p+=2;
            }
            else {
                p++;
            }

            if(memcmp(p, "preprocessor", 12) == 0) {
                sBuf command;

                p+=12;

                /// read one line ///
                sBuf_init(&command);

                while(1) {
                    if(memcmp(p, "#endpreprocessor", 16) == 0) {
                        p+=16;
                        break;
                    }
                    else if(*p == 0) {
                        parser_err_msg_without_line("Clover reads out the source file before #endpreprocessor");
                        FREE(command.mBuf);
                        return FALSE;
                    }
                    else if(*p == '\n') {
                        sBuf_append_char(source2, *p);
                        sBuf_append_char(&command, *p);
                        p++;
                    }
                    else {
                        sBuf_append_char(&command, *p);
                        p++;
                    }
                }

                /// compile ///
                if(!call_preprocessor(&command, source2)) {
                    FREE(command.mBuf);
                    return FALSE;
                }

                FREE(command.mBuf);
            }
            else {
                sBuf_append_char(source2, '#');
            }
        }
        else {
            sBuf_append_char(source2, *p);
            p++;
        }
    }

    return TRUE;
}
