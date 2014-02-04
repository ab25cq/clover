#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

struct sHandle_ {
    int mOffset;                 // -1 for FreeHandle
    int mNextFreeHandle;         // -1 for NULL. index of mHandles
};

typedef struct sHandle_ sHandle;

struct sCLHeapManager_ {
    unsigned char* mMem;
    unsigned char* mMemB;

    unsigned char* mCurrentMem;
    unsigned char* mSleepMem;

    unsigned int mMemLen;
    unsigned int mMemSize;

    sHandle* mHandles;
    int mSizeHandles;
    int mNumHandles;

    int mFreeHandles;    // -1 for NULL. index of mHandles
};

typedef struct sCLHeapManager_ sCLHeapManager;

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

void* object_to_ptr(CLObject obj) 
{
    const unsigned int index = obj - FIRST_OBJ;
    return (void*)(gCLHeap.mCurrentMem + gCLHeap.mHandles[index].mOffset);
}

CLObject alloc_heap_mem(unsigned int size)
{
    int handle;
    CLObject obj;

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
    
    obj = handle + FIRST_OBJ;

    gCLHeap.mHandles[handle].mOffset = gCLHeap.mMemLen;
    gCLHeap.mMemLen += size;

    return obj;
}

static BOOL is_valid_object(CLObject obj)
{
    return obj >= FIRST_OBJ && obj < FIRST_OBJ + gCLHeap.mNumHandles;
}

static unsigned int get_heap_mem_size(sCLClass* klass, CLObject object)
{
    int obj_size;

    if(klass == gStringType.mClass) {
        obj_size = string_size(object);
    }
    else if(klass == gArrayType.mClass) {
        obj_size = array_size(object);
    }
    else if(klass == gHashType.mClass) {
        obj_size = 0;
    }
    else {
        obj_size = object_size(klass);
    }

    return obj_size;
}

void mark_object(CLObject obj, unsigned char* mark_flg)
{
    if(is_valid_object(obj)) {
        sCLClass* klass;

        mark_flg[obj - FIRST_OBJ] = TRUE;

        klass = CLOBJECT_HEADER(obj)->mClass;

        /// mark objects which is contained in ///
        if(klass != gStringType.mClass) {
            if(klass == gHashType.mClass) {
            }
            else if(klass == gArrayType.mClass) {
                mark_array_object(obj, mark_flg);
            }
            else {
                mark_user_object(obj, mark_flg);
            }
        }
    }
}

static void mark(unsigned char* mark_flg)
{
    int i;
    const int len = gCLStackPtr - gCLStack;

    /// mark stack ///
    for(i=0; i<len; i++) {
        CLObject obj = gCLStack[i].mObjectValue;
        mark_object(obj, mark_flg);
    }
}

static void compaction(unsigned char* mark_flg)
{
    int i;
    unsigned char* mem;

    memset(gCLHeap.mSleepMem, 0, gCLHeap.mMemSize);
    gCLHeap.mMemLen = 0;

    for(i=0; i<gCLHeap.mNumHandles; i++) {
        if(gCLHeap.mHandles[i].mOffset != -1) {
            void* data;
            sCLClass* klass;
            CLObject obj;

            data = (void*)(gCLHeap.mCurrentMem + gCLHeap.mHandles[i].mOffset);
            klass = ((sCLObjectHeader*)data)->mClass;
            obj = i + FIRST_OBJ;

            /// this is not a marked object ///
            if(!mark_flg[i]) {
                int top_of_free_handle;

                /// call destructor ///
                if(klass->mFreeFun) klass->mFreeFun(obj);

                gCLHeap.mHandles[i].mOffset = -1;
                
                /// chain free handles ///
                top_of_free_handle = gCLHeap.mFreeHandles;
                gCLHeap.mFreeHandles = i;
                gCLHeap.mHandles[i].mNextFreeHandle = top_of_free_handle;
            }
            /// this is a marked object ///
            else {
                void* src;
                void* dst;
                int obj_size;

                ((sCLObjectHeader*)data)->mExistence++;

                obj_size = get_heap_mem_size(klass, obj);
                
                /// copy object to new heap
                src = gCLHeap.mCurrentMem + gCLHeap.mHandles[i].mOffset;
                dst = gCLHeap.mSleepMem + gCLHeap.mMemLen;

                memcpy(dst, src, obj_size);

                gCLHeap.mHandles[i].mOffset = gCLHeap.mMemLen;
                gCLHeap.mMemLen += obj_size;
            }
        }
    }

    //// now sleep memory is current ///
    mem = gCLHeap.mSleepMem;
    gCLHeap.mSleepMem = gCLHeap.mCurrentMem;
    gCLHeap.mCurrentMem = mem;
}

void cl_gc()
{
    unsigned char* mark_flg;
puts("cl_gc...");

    mark_flg = CALLOC(1, gCLHeap.mNumHandles);

    mark(mark_flg);
    compaction(mark_flg);

    FREE(mark_flg);
}

void show_heap()
{
    int i;

    printf("offsetnum %d\n", gCLHeap.mNumHandles);
    for(i=0; i<gCLHeap.mNumHandles; i++) {
        CLObject obj = i + FIRST_OBJ;

        if(gCLHeap.mHandles[i].mOffset == -1) {
            printf("%ld --> (ptr null)\n", obj);
        }
        else {
            MVALUE* data;
            sCLClass* klass;
            unsigned int existance_count;

            data = (MVALUE*)(gCLHeap.mCurrentMem + gCLHeap.mHandles[i].mOffset);
            klass = CLOBJECT_HEADER(obj)->mClass;
            existance_count = CLOBJECT_HEADER(obj)->mExistence;

            printf("%ld --> (ptr %p) (size %d) (class %p) (existance count %d)", obj, data, get_heap_mem_size(klass, obj), klass, existance_count);

            if(klass == gStringType.mClass) {
                unsigned int obj_size;
                int len;
                wchar_t* data2;
                int size;
                char* str;

                printf(" class name (String) ");
                obj_size = string_size(obj);

                len = CLSTRING(obj)->mLen;

                data2 = MALLOC(sizeof(wchar_t)*len + 1);
                memcpy(data2, CLSTRING(obj)->mChars, sizeof(wchar_t)*len);
                data2[len] = 0;

                size = (len + 1) * MB_LEN_MAX;
                str = MALLOC(size);
                wcstombs(str, data2, size);

                printf(" (len %d) (%s)\n", len, str);

                FREE(data2);
                FREE(str);
            }
            else if(klass == gArrayType.mClass) {
                int j;

                printf(" class name (Array) ");
                printf(" (size %d) (len %d)\n", CLARRAY(obj)->mSize, CLARRAY(obj)->mLen);

                for(j=0; j<CLARRAY(obj)->mLen; j++) {
                    printf("item##%d %d\n", j, CLARRAY_ITEMS(obj, j).mIntValue);
                }
            }
            else if(klass == gHashType.mClass) {
            }
            /// object ///
            else {
                int j;

                printf(" class name (%s)\n", REAL_CLASS_NAME(klass));

                for(j=0; j<get_field_num_including_super_classes(klass); j++) {
                    printf("field#%d %d\n", j, CLOBJECT(obj)->mFields[j].mIntValue);
                }
            }
        }
    }

}
