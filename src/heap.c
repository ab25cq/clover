#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

typedef struct {
    uchar* mMem;
    uchar* mMemB;

    uchar* mCurrentMem;
    uchar* mSleepMem;

    uint mMemLen;
    uint mMemSize;

    uint* mHandles;      // -1 for freed memory
    uint mSizeHandles;
    uint mNumHandles;
} sCLHeapManager;

enum eArrayType { kATObject, kATWChar };

static sCLHeapManager gCLHeap;

void heap_init(int heap_size, int size_hadles)
{
    gCLHeap.mMem = CALLOC(1, heap_size);
    gCLHeap.mMemSize = heap_size;
    gCLHeap.mMemLen = 0;
    gCLHeap.mMemB = CALLOC(1, heap_size);

    gCLHeap.mCurrentMem = gCLHeap.mMem;
    gCLHeap.mSleepMem = gCLHeap.mMemB;

    gCLHeap.mHandles = CALLOC(1, sizeof(uint)*size_hadles);
    gCLHeap.mSizeHandles = size_hadles;
    gCLHeap.mNumHandles = 0;
}

void heap_final()
{
    FREE(gCLHeap.mMem);
    FREE(gCLHeap.mMemB);
    FREE(gCLHeap.mHandles);
}

#define FIRST_OBJ 1234

MVALUE* object_to_ptr(CLObject obj) 
{
    const uint index = obj - FIRST_OBJ;
    return (MVALUE*)(gCLHeap.mCurrentMem + gCLHeap.mHandles[index]);
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

            gCLHeap.mMemB = REALLOC(gCLHeap.mMemB, new_heap_size);
            memset(gCLHeap.mMemB + gCLHeap.mMemSize, 0, new_heap_size - gCLHeap.mMemSize);

            gCLHeap.mMemSize = new_heap_size;
        }
    }

    /// searching a free handle ///
    int handle;
    int i;
    for(i=0; i<gCLHeap.mNumHandles; i++) {
        if(gCLHeap.mHandles[i] == -1) {
            handle = i;
            break;
        }
    }

    /// no free handle ///
    if(i == gCLHeap.mNumHandles) {
        if(gCLHeap.mNumHandles == gCLHeap.mSizeHandles) {
            const int new_offset_size = (gCLHeap.mSizeHandles + 1) * 2;

            gCLHeap.mHandles = REALLOC(gCLHeap.mHandles, sizeof(uint)*new_offset_size);
            memset(gCLHeap.mHandles + gCLHeap.mSizeHandles, 0, new_offset_size - gCLHeap.mSizeHandles);
            gCLHeap.mSizeHandles = new_offset_size;
        }

        handle = gCLHeap.mNumHandles;
        gCLHeap.mNumHandles++;
    }
    
    CLObject obj = handle + FIRST_OBJ;

    gCLHeap.mHandles[handle] = gCLHeap.mMemLen;
    gCLHeap.mMemLen += size;

    return obj;
}

uint object_size(sCLClass* klass)
{
    uint size = (uint)sizeof(MVALUE)*klass->mNumFields;
    size += sizeof(MVALUE)* OBJECT_HEADER_NUM;

    /// align to 4 byte boundry
    size = (size + 3) & ~3;

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

    CLCLASS(obj) = NULL;  // pointer to class is NULL for array.
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
    return obj >= FIRST_OBJ && obj < FIRST_OBJ + gCLHeap.mNumHandles;
}

static void mark_object(CLObject obj, uchar* mark_flg)
{
    if(is_valid_object(obj)) {
        mark_flg[obj - FIRST_OBJ] = TRUE;

        sCLClass* klass = CLCLASS(obj);

        /// mark fields ///
        if(klass) {
            int i;
            for(i=0; i<klass->mNumFields; i++) {
                CLObject obj2 = CLFIELD(obj, i).mObjectValue;

                mark_object(obj2, mark_flg);
            }
        }
    }
}

static void mark(uchar* mark_flg)
{
    /// mark stack ///
    int i;
    const int len = gCLStackPtr - gCLStack;
    for(i=0; i<len; i++) {
        CLObject obj = gCLStack[i].mObjectValue;
        mark_object(obj, mark_flg);
    }
}

static void compaction(uchar* mark_flg)
{
puts("compaction...");
sleep(3);
    memset(gCLHeap.mSleepMem, 0, gCLHeap.mMemSize);
    gCLHeap.mMemLen = 0;

    int i;
    for(i=0; i<gCLHeap.mNumHandles; i++) {
        if(gCLHeap.mHandles[i] != -1) {
            MVALUE* data = (MVALUE*)(gCLHeap.mCurrentMem + gCLHeap.mHandles[i]);
            sCLClass* klass = CLPCLASS(data);
            CLObject obj = i + FIRST_OBJ;

            /// this is not a marking object ///
            if(!mark_flg[i]) {
                /// destroy object ///
                /// freeObject(obj);

                gCLHeap.mHandles[i] = -1;
            }
            /// this is a marking object ///
            else {
                CLPEXISTENCE(data)++;

                int obj_size;

                /// NULL value for klass is array
                if(klass == NULL) {
                    obj_size = array_size(CLPATYPE(data), CLPALEN(data));
                }
                else {
                    obj_size = object_size(klass);
                }
                
                /// copy object to new heap
                void* src = gCLHeap.mCurrentMem + gCLHeap.mHandles[i];
                void* dst = gCLHeap.mSleepMem + gCLHeap.mMemLen;

                memcpy(dst, src, obj_size);
                //if(src != dst) { memmove(dst, src, obj_size); }

                gCLHeap.mHandles[i] = gCLHeap.mMemLen;
                gCLHeap.mMemLen += obj_size;
            }
        }
    }

    uchar* mem = gCLHeap.mSleepMem;
    gCLHeap.mSleepMem = gCLHeap.mCurrentMem;
    gCLHeap.mCurrentMem = mem;
}

void cl_gc()
{
puts("cl_gc...");
sleep(3);
    uchar* mark_flg = CALLOC(1, gCLHeap.mNumHandles);

    mark(mark_flg);
    compaction(mark_flg);

    FREE(mark_flg);
}

void show_heap()
{
    printf("offsetnum %d\n", gCLHeap.mNumHandles);
    int i;
    for(i=0; i<gCLHeap.mNumHandles; i++) {
        CLObject obj = i + FIRST_OBJ;

        if(gCLHeap.mHandles[i] == -1) {
            printf("%ld --> (ptr null)\n", obj);
        }
        else {
            MVALUE* data = (MVALUE*)(gCLHeap.mCurrentMem + gCLHeap.mHandles[i]);
            sCLClass* klass = CLPCLASS(data);
            uint existance_count = CLPEXISTENCE(data);

            printf("%ld --> (ptr %p) (handle %d) (class %p) (existance count %d)\n", obj, data, gCLHeap.mHandles[i], klass, existance_count);

            /// array ///
            if(klass == 0) {
                const uint obj_size = array_size(CLPATYPE(data), CLPALEN(data));
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

                        printf("  --> (obj_size %d) (type %d) (len %d) (%s)\n", obj_size, type, len, str);

                        FREE(data2);
                        FREE(str);
                        }
                        break;
                }
            }
            /// object ///
            else {
                const uint obj_size = object_size(klass);
                printf("   --> (obj_size %d)\n", obj_size);

                int j;
                for(j=0; j<klass->mNumFields; j++) {
                    printf("field#%d %d\n", j, CLFIELD(obj, j).mIntValue);
                }
            }
        }
    }

}
