#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

typedef struct {
    int mOffset;                 // -1 for FreeHandle
    int mNextFreeHandle;         // -1 for NULL. index of mHandles
} sHandle;

typedef struct {
    uchar* mMem;
    uchar* mMemB;

    uchar* mCurrentMem;
    uchar* mSleepMem;

    uint mMemLen;
    uint mMemSize;

    sHandle* mHandles;
    int mSizeHandles;
    int mNumHandles;

    int mFreeHandles;    // -1 for NULL. index of mHandles
} sCLHeapManager;

static sCLHeapManager gCLHeap;

void heap_init(int heap_size, int size_hadles)
{
    gCLHeap.mMem = CALLOC(1, heap_size);
    gCLHeap.mMemSize = heap_size;
    gCLHeap.mMemLen = 0;
    gCLHeap.mMemB = CALLOC(1, heap_size);

    gCLHeap.mCurrentMem = gCLHeap.mMem;
    gCLHeap.mSleepMem = gCLHeap.mMemB;

    gCLHeap.mHandles = CALLOC(1, sizeof(sHandle)*size_hadles);
    gCLHeap.mSizeHandles = size_hadles;
    gCLHeap.mNumHandles = 0;

    gCLHeap.mFreeHandles = -1;   // -1 for NULL
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
    return (MVALUE*)(gCLHeap.mCurrentMem + gCLHeap.mHandles[index].mOffset);
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

    /// get a free handle from linked list ///
    int handle;
    if(gCLHeap.mFreeHandles >= 0) {
        handle = gCLHeap.mFreeHandles;
        gCLHeap.mFreeHandles = gCLHeap.mHandles[handle].mNextFreeHandle;
        gCLHeap.mHandles[handle].mNextFreeHandle = -1;
    }
    /// no free handle. get new one ///
    else {
        if(gCLHeap.mNumHandles == gCLHeap.mSizeHandles) {
            const int new_offset_size = (gCLHeap.mSizeHandles + 1) * 2;

            gCLHeap.mHandles = REALLOC(gCLHeap.mHandles, sizeof(sHandle)*new_offset_size);
            memset(gCLHeap.mHandles + gCLHeap.mSizeHandles, 0, sizeof(sHandle)*(new_offset_size - gCLHeap.mSizeHandles));
            gCLHeap.mSizeHandles = new_offset_size;
        }

        handle = gCLHeap.mNumHandles;
        gCLHeap.mNumHandles++;
    }
    
    CLObject obj = handle + FIRST_OBJ;

    gCLHeap.mHandles[handle].mOffset = gCLHeap.mMemLen;
    gCLHeap.mMemLen += size;

    return obj;
}

static uint object_size(sCLClass* klass)
{
    uint size = (uint)sizeof(MVALUE)*get_field_num_including_super_classes(klass);
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

static BOOL is_valid_object(CLObject obj)
{
    return obj >= FIRST_OBJ && obj < FIRST_OBJ + gCLHeap.mNumHandles;
}

static uint get_heap_size(sCLClass* klass, CLObject object)
{
    int obj_size;
    if(klass == gStringClass) {
        obj_size = string_size(object);
    }
    else if(klass == gArrayClass) {
        obj_size = array_size(object);
    }
    else if(klass == gHashClass) {
        obj_size = 0;
    }
    else {
        obj_size = object_size(klass);
    }

    return obj_size;
}

void mark_object(CLObject obj, uchar* mark_flg)
{
    if(is_valid_object(obj)) {
        mark_flg[obj - FIRST_OBJ] = TRUE;

        sCLClass* klass = CLCLASS(obj);

        /// mark objects which is contained in ///
        if(klass != gStringClass) {
            if(klass == gHashClass) {
            }
            else if(klass == gArrayClass) {
                mark_array_object(obj, mark_flg);
            }
            else {         // user object has fields
                int i;
                for(i=0; i<get_field_num_including_super_classes(klass); i++) {
                    CLObject obj2 = CLFIELD(obj, i).mObjectValue;

                    mark_object(obj2, mark_flg);
                }
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
    memset(gCLHeap.mSleepMem, 0, gCLHeap.mMemSize);
    gCLHeap.mMemLen = 0;

    int i;
    for(i=0; i<gCLHeap.mNumHandles; i++) {
        if(gCLHeap.mHandles[i].mOffset != -1) {
            MVALUE* data = (MVALUE*)(gCLHeap.mCurrentMem + gCLHeap.mHandles[i].mOffset);
            sCLClass* klass = CLPCLASS(data);
            CLObject obj = i + FIRST_OBJ;

            /// this is not a marking object ///
            if(!mark_flg[i]) {
                /// call destructor ///
                if(klass->mFreeFun) klass->mFreeFun(obj);

                gCLHeap.mHandles[i].mOffset = -1;
                
                /// chain free handles ///
                int top_of_free_handle = gCLHeap.mFreeHandles;
                gCLHeap.mFreeHandles = i;
                gCLHeap.mHandles[i].mNextFreeHandle = top_of_free_handle;
            }
            /// this is a marking object ///
            else {
                CLPEXISTENCE(data)++;

                int obj_size = get_heap_size(klass, obj);
                
                /// copy object to new heap
                void* src = gCLHeap.mCurrentMem + gCLHeap.mHandles[i].mOffset;
                void* dst = gCLHeap.mSleepMem + gCLHeap.mMemLen;

                memcpy(dst, src, obj_size);

                gCLHeap.mHandles[i].mOffset = gCLHeap.mMemLen;
                gCLHeap.mMemLen += obj_size;
            }
        }
    }

    //// now sleep memory is current ///
    uchar* mem = gCLHeap.mSleepMem;
    gCLHeap.mSleepMem = gCLHeap.mCurrentMem;
    gCLHeap.mCurrentMem = mem;
}

void cl_gc()
{
puts("cl_gc...");
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

        if(gCLHeap.mHandles[i].mOffset == -1) {
            printf("%ld --> (ptr null)\n", obj);
        }
        else {
            MVALUE* data = (MVALUE*)(gCLHeap.mCurrentMem + gCLHeap.mHandles[i].mOffset);
            sCLClass* klass = CLCLASS(obj);
            uint existance_count = CLEXISTENCE(obj);

            printf("%ld --> (ptr %p) (size %d) (class %p) (existance count %d)", obj, data, get_heap_size(klass, obj), klass, existance_count);

            if(klass == gStringClass) {
                printf(" class name (String) ");
                uint obj_size = string_size(obj);

                const int len = CLSTRING_LEN(obj);

                wchar_t* data2 = MALLOC(sizeof(wchar_t)*len + 1);
                memcpy(data2, CLSTRING_START(obj), sizeof(wchar_t)*len);
                data2[len] = 0;

                const int size = (len + 1) * MB_LEN_MAX;
                char* str = MALLOC(size);
                wcstombs(str, data2, size);

                printf(" (len %d) (%s)\n", len, str);

                FREE(data2);
                FREE(str);
            }
            else if(klass == gArrayClass) {
                printf(" class name (Array) ");
                printf(" (size %d) (len %d)\n", CLARRAY_SIZE(obj), CLARRAY_LEN(obj));

                int j;
                for(j=0; j<CLARRAY_LEN(obj); j++) {
                    printf("item##%d %d\n", j, CLARRAY_ITEM(obj, j).mIntValue);
                }
            }
            else if(klass == gHashClass) {
            }
            /// object ///
            else {
                printf(" class name (%s) ", REAL_CLASS_NAME(klass));

                int j;
                for(j=0; j<get_field_num_including_super_classes(klass); j++) {
                    printf("field#%d %d\n", j, CLFIELD(obj, j).mIntValue);
                }
            }
        }
    }

}
