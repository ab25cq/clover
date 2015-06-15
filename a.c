#include <oniguruma.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    regex_t* regex;
    int result;
    char* regex_str;
    OnigOptionType option;
    OnigEncoding enc;
    OnigErrorInfo err_info;

    option = ONIG_OPTION_DEFAULT;
    enc = ONIG_ENCODING_UTF8;

    regex_str = "XXX";
    
    result = onig_new(&regex
        , (const OnigUChar*) regex_str
        , (const OnigUChar*) regex_str + strlen(regex_str)
        , option
        , enc
        , ONIG_SYNTAX_DEFAULT
        , &err_info);

    if(result == ONIG_NORMAL) {
        char* str = "AXXX";
        char* p = str;
        OnigRegion* region;

        region = onig_region_new();

        int r = onig_search(regex
                , str
                , str + strlen(str)
                , p
                , p + strlen(p)
                , region, ONIG_OPTION_NONE);

        printf("r %d\n", r);
    }

    exit(0);
}
