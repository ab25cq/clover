#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

typedef struct _sFreedMemory {
    uint mOffset;
    uint mSize;

    struct _sFreedMemory* mNext;
} sFreedMemory;

typedef struct {
    uchar* mMem;
    uint mMemLen;
    uint mMemSize;

    sFreedMemory* mFreedMemory;

    uint* mOffset;      // -1 for freed memory
    uint mOffsetSize;
    uint mOffsetNum;

    uint* mFreedOffset;
    uint mFreedOffsetSize;
    uint mFreedOffsetNum;
} sCLHeapManager;

enum eArrayType { kATObject, kATWChar };

static sCLHeapManager gCLHeap;

void heap_init(int heap_size, int num_handles)
{
    gCLHeap.mMem = CALLOC(1, heap_size);
    gCLHeap.mMemSize = heap_size;
    gCLHeap.mMemLen = 0;

    gCLHeap.mOffset = CALLOC(1, sizeof(uint)*num_handles);
    gCLHeap.mOffsetSize = num_handles;
    gCLHeap.mOffsetNum = 0;
    
    gCLHeap.mFreedOffset = CALLOC(1, sizeof(uint)*num_handles);
    gCLHeap.mFreedOffsetSize = num_handles;
    gCLHeap.mFreedOffsetNum = 0;

    gCLHeap.mFreedMemory = NULL;
}

void heap_final()
{
    FREE(gCLHeap.mMem);
    FREE(gCLHeap.mOffset);
    FREE(gCLHeap.mFreedOffset);

    sFreedMemory* freed_memory = gCLHeap.mFreedMemory;
    while(freed_memory) {
        sFreedMemory* next = freed_memory->mNext;
        FREE(freed_memory);
        freed_memory = next;
    }
}

#define FIRST_OBJ 1234

MVALUE* object_to_ptr(CLObject obj) 
{
    const uint index = obj - FIRST_OBJ;
    return (MVALUE*)gCLHeap.mMem + gCLHeap.mOffset[index];
}

CLObject alloc_heap_mem(uint size)
{
    if(gCLHeap.mMemLen + size >= gCLHeap.mMemSize) {
        cl_gc();

        /// create new space of object ///
        if(gCLHeap.mMemLen + size >= gCLHeap.mMemSize) {
            const int new_heap_size = (gCLHeap.mMemSize + size) * 2;

            gCLHeap.mMem = REALLOC(gCLHeap.mMem, new_heap_size);
            memset(gCLHeap.mMem + gCLHeap.mMemSize, 0, new_heap_size - gCLHeap.mMemSize);
            gCLHeap.mMemSize = new_heap_size;
        }
    }

    uint offset;
    if(gCLHeap.mFreedOffsetNum > 0) {
puts("Free Node...");
        offset = gCLHeap.mFreedOffset[gCLHeap.mFreedOffsetNum-1];
        gCLHeap.mFreedOffsetNum--;
    }
    else {
        if(gCLHeap.mOffsetNum == gCLHeap.mOffsetSize) {
puts("REALLOC offset...");
sleep(3);
            const int new_offset_size = (gCLHeap.mOffsetSize + 1) * 2;

            gCLHeap.mOffset = REALLOC(gCLHeap.mOffset, sizeof(uint)*new_offset_size);
            memset(gCLHeap.mOffset + gCLHeap.mOffsetSize, 0, new_offset_size - gCLHeap.mOffsetSize);
            gCLHeap.mOffsetSize = new_offset_size;
        }

        offset = gCLHeap.mOffsetNum;
        gCLHeap.mOffsetNum++;
    }

    const uint obj = offset + FIRST_OBJ;

    gCLHeap.mOffset[offset] = gCLHeap.mMemLen;
    gCLHeap.mMemLen += size;

    return obj;
}

uint object_size(sCLClass* klass)
{
    uint size = (uint)sizeof(CLObject)*klass->mNumFields;
    size += sizeof(MVALUE)* OBJECT_HEADER_NUM;

    return size;
}

CLObject create_object(sCLClass* klass)
{
    CLObject object = alloc_heap_mem(object_size(klass));
    CLCLASS(object) = klass;

    return object;
}

static uint array_size(enum eArrayType type, uint len)
{
    uint size;
    switch(type) {
        case kATObject:
            size = 4 * len;
            break;

        case kATWChar:
            size = sizeof(wchar_t) * len;
            break;

        default:
            fprintf(stderr, "unexpected error on array_size\n");
            exit(1);
    }
    size += sizeof(MVALUE) * ARRAY_HEADER_NUM;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

    return size;
}

static CLObject alloc_array(enum eArrayType type, uint len)
{
    CLObject obj;

    uint size = array_size(type, len);

    obj = alloc_heap_mem(size);

    CLCLASS(obj) = 0;  // pointer to class is 0 for array.
    CLATYPE(obj) = type;
    CLALEN(obj) = len;

    return obj;
}

CLObject create_string_object(wchar_t* str, uint len)
{
    CLObject obj;

    obj = alloc_array(kATWChar, len);

    wchar_t* data = CLASTART(obj);

    int i;
    for(i=0; i<len; i++) {
        data[i] = str[i];
    }

    return obj;
}

static BOOL is_valid_object(CLObject obj)
{
    return obj >= FIRST_OBJ && obj < FIRST_OBJ + gCLHeap.mOffsetNum;
}

static void mark(uchar* mark_flg)
{
    /// mark stack ///
    int i;
    const int len = gCLStackPtr - gCLStack;
    for(i=0; i<len; i++) {
        CLObject obj = gCLStack[i].mObjectValue;
        if(is_valid_object(obj)) {
            mark_flg[obj - FIRST_OBJ] = TRUE;
        }
    }
}

static void compaction(uchar* mark_flg)
{
puts("compaction...");
sleep(3);
    int prev_mem_len = gCLHeap.mMemLen;
    gCLHeap.mMemLen = 0;

    int i;
    for(i=0; i<gCLHeap.mOffsetNum; i++) {
        if(gCLHeap.mOffset[i] != -1) {
            MVALUE* data = (MVALUE*)gCLHeap.mMem + gCLHeap.mOffset[i];
            sCLClass* klass = CLPCLASS(data);
            CLObject obj = i + FIRST_OBJ;

            /// this is not a marking object ///
            if(!mark_flg[i]) {
                /// destroy object ///
                /// freeObject(obj);

                gCLHeap.mOffset[i] = -1;

                if(gCLHeap.mFreedOffsetNum == gCLHeap.mFreedOffsetSize) {
                    const int new_offset_size = (gCLHeap.mFreedOffsetSize + 1) * 2;

                    gCLHeap.mFreedOffset = REALLOC(gCLHeap.mFreedOffset, sizeof(uint)*new_offset_size);
                    memset(gCLHeap.mFreedOffset + gCLHeap.mFreedOffsetSize, 0, new_offset_size - gCLHeap.mFreedOffsetSize);
                    gCLHeap.mFreedOffsetSize = new_offset_size;
                }
            
                gCLHeap.mFreedOffset[gCLHeap.mFreedOffsetNum] = i;
                gCLHeap.mFreedOffsetNum++;
            }
            /// this is a marking object ///
            else {
                CLPEXISTENCE(data)++;

                int obj_size;

                /// 0 value for klass is array
                if(klass == NULL) {
                    obj_size = array_size(CLPATYPE(data), CLPALEN(data));
                }
                else {
                    /// obj_size = cl_klass_size(klass);
                }
                
                /// copy object to new heap
                void* src = gCLHeap.mMem + gCLHeap.mOffset[i];
                void* dst = gCLHeap.mMem + gCLHeap.mMemLen;

                if(src != dst) { memmove(dst, src, obj_size); }

                gCLHeap.mOffset[i] = gCLHeap.mMemLen;
                gCLHeap.mMemLen += obj_size;
            }
        }
    }

    memset(gCLHeap.mMem + gCLHeap.mMemLen, 0, prev_mem_len - gCLHeap.mMemLen);
}

void cl_gc()
{
puts("cl_gc...");
sleep(3);
    uchar* mark_flg = MALLOC(gCLHeap.mOffsetNum);
    memset(mark_flg, 0, gCLHeap.mOffsetNum);

    mark(mark_flg);
    compaction(mark_flg);

    FREE(mark_flg);
}

void show_heap()
{
    printf("offsetnum %d freed_offset_num %d\n", gCLHeap.mOffsetNum, gCLHeap.mFreedOffsetNum);
    int i;
    for(i=0; i<gCLHeap.mOffsetNum; i++) {
        CLObject obj = i + FIRST_OBJ;

        if(gCLHeap.mOffset[i] == -1) {
            printf("%ld --> (ptr null)\n", obj);
        }
        else {
            MVALUE* data = (MVALUE*)gCLHeap.mMem + gCLHeap.mOffset[i];
            sCLClass* klass = CLPCLASS(data);
            uint existance_count = CLPEXISTENCE(data);

            printf("%ld --> (ptr %p) (offset %d) (class %p) (existance count %d)\n", obj, data, gCLHeap.mOffset[i], klass, existance_count);

            /// array ///
            if(klass == 0) {
                enum eArrayType type = CLPATYPE(data);
                int len = CLPALEN(data);
                
                switch(type) {
                    case kATObject:
                        break;

                    case kATWChar: {
                        wchar_t* data2 = MALLOC(sizeof(wchar_t)*len + 1);
                        memcpy(data2, CLPASTART(data), sizeof(wchar_t)*len);
                        data2[len] = 0;

                        const int size = (len + 1) * MB_LEN_MAX;
                        uchar* str = MALLOC(size);
                        wcstombs(str, data2, size);

                        printf("  --> (type %d) (len %d) (%s)\n", type, len, str);

                        FREE(data2);
                        FREE(str);
                        }
                        break;
                }
            }
        }
    }

}
