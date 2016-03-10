#include "clover.h"
#include "common.h"

int um_is_none_ascii(enum eUtfMbsKind code, unsigned char c)
{
    if(code == kEucjp) {
        return (c >= 0xA1 && c <= 0xFE) || c == 0x8e;
//        return c >= 161 && c <= 254;
    }
    else if(code == kSjis) {
        return (c >= 0x81 && c <= 0x9f) || c >= 0xE0;
    }
    else if(code == kUtf8) {
        return c > 127;
    }

    return 0;
}

char* um_index2pointer(enum eUtfMbsKind code, char* mbs, int pos)
{
    if(code == kUtf8) {
        if(pos<0) return mbs;

        char* p = mbs;
        int n = 0;
    
        char* max = mbs + strlen(mbs);
        while(p < max) {
            if(n == pos) {
                return p;
            }
            if(((unsigned char)*p) > 127) {
                int size = ((*p & 0x80) >> 7) + ((*p & 0x40) >> 6) + ((*p & 0x20) >> 5) + ((*p & 0x10) >> 4);
                p+= size;
                n++;
            }
            else {
                p++;
                n++;
            }
/*
            if(gKitutukiSigInt) {
                return NULL;
            }
*/
        }

        return mbs + strlen(mbs);
    }
    else if(code == kByte) {
        return mbs + pos;
    }
    else {
        if(pos<0) return mbs;

        char* p = mbs;
        int c = 0;
        char* max = mbs + strlen(mbs);
        while(p < max) {
            if(c==pos) {
                return p;
            }

            if(um_is_none_ascii(code, *p)) {
                p+=2;
                c++;
            }
            else {
                p++;
                c++;
            }
        }

        return mbs + strlen(mbs);
    }
}

int um_strlen(enum eUtfMbsKind code, char* mbs)
{
    if(code == kByte) {
        return strlen(mbs);
    }
    else if(code == kUtf8) {
        int n = 0;
        char* p = mbs;

        char* max = strlen(mbs) + mbs;
        while(p < max) {
            if(((unsigned char)*p) > 127) {
                int size = ((*p & 0x80) >> 7) + ((*p & 0x40) >> 6) + ((*p & 0x20) >> 5) + ((*p & 0x10) >> 4);
                p+= size;
                n++;
            }
            else {
                p++;
                n++;
            }
        }

        return n;
    }
    else {
        int result = 0;
        char* p = mbs;
        while(*p) {
            if(um_is_none_ascii(code, *p) && *(p+1) != 0) {
                p+=2;
                result++;
            }
            else {
                p++;
                result++;
            }
        }
        return result;
    }
}

int um_pointer2index(enum eUtfMbsKind code, char* mbs, char* pointer)
{
    if(code == kEucjp || code == kSjis) {
        int result = 0;

        char* p = mbs;
        while(p < pointer) {
            if(um_is_none_ascii(code, *p)) {
                p+=2;
                result++;
            }
            else {
                p++;
                result++;
            }
        }

        return result;
    }
    else if(code == kByte) {
        return pointer - mbs;
    }
    else {
        char* mbs2;
        int result;

        mbs2 = MSTRDUP(mbs);
        *(mbs2 + (pointer-mbs)) = 0;

        result = um_strlen(code, mbs2);

        MFREE(mbs2);

        return result;
    }
}
