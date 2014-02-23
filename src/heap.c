#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>

#define FIRST_OBJ 1234

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

static void gc_all();

void heap_final()
{
    gc_all();
    FREE(gCLHeap.mMem);
    FREE(gCLHeap.mMemB);
    FREE(gCLHeap.mHandles);
}


void* object_to_ptr(CLObject obj) 
{
    const unsigned int index = obj - FIRST_OBJ;
    return (void*)(gCLHeap.mCurrentMem + gCLHeap.mHandles[index].mOffset);
}

CLObject alloc_heap_mem(unsigned int size, sCLClass* klass)
{
    int handle;
    CLObject obj;

    if(gCLHeap.mMemLen + size >= gCLHeap.mMemSize) {
        cl_gc();

        /// create new space of object ///
        if(gCLHeap.mMemLen + size >= gCLHeap.mMemSize) {
            BOOL current_is_mem_a;

            current_is_mem_a = gCLHeap.mMem == gCLHeap.mCurrentMem;

            const int new_heap_size = (gCLHeap.mMemSize + size) * 2;

            gCLHeap.mMem = REALLOC(gCLHeap.mMem, new_heap_size);
            memset(gCLHeap.mMem + gCLHeap.mMemSize, 0, new_heap_size - gCLHeap.mMemSize);

            gCLHeap.mMemB = REALLOC(gCLHeap.mMemB, new_heap_size);
            memset(gCLHeap.mMemB + gCLHeap.mMemSize, 0, new_heap_size - gCLHeap.mMemSize);

            gCLHeap.mMemSize = new_heap_size;

            if(current_is_mem_a) {
                gCLHeap.mCurrentMem = gCLHeap.mMem;
                gCLHeap.mSleepMem = gCLHeap.mMemB;
            }
            else {
                gCLHeap.mCurrentMem = gCLHeap.mMemB;
                gCLHeap.mSleepMem = gCLHeap.mMem;
            }
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

    CLOBJECT_HEADER(obj)->mHeapMemSize = size;
    CLOBJECT_HEADER(obj)->mClass = klass;
    CLOBJECT_HEADER(obj)->mExistence = 0;

    return obj;
}

static BOOL is_valid_object(CLObject obj)
{
    return obj >= FIRST_OBJ && obj < FIRST_OBJ + gCLHeap.mNumHandles;
}

static unsigned int get_heap_mem_size(CLObject object)
{
    return CLOBJECT_HEADER(object)->mHeapMemSize;
}

void mark_object(CLObject obj, unsigned char* mark_flg)
{
    if(is_valid_object(obj)) {
        sCLClass* klass;

        mark_flg[obj - FIRST_OBJ] = TRUE;

        klass = CLOBJECT_HEADER(obj)->mClass;

        /// mark objects which is contained in ///
        if(klass && klass->mMarkFun) { klass->mMarkFun(obj, mark_flg); }
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

    /// mark class fields ///
}

static void compaction(unsigned char* mark_flg)
{
    int i;
    unsigned char* mem;

//if(gCode) printf("con0 gCode->code %ld gCode %p\n", gCode->mCode, &gCode->mCode);
//printf("gCLHeap.mMem %p gCLHeap.mMemB %p gCLHeap.mCurrentMem %p gCLHeap.mSleepMem %p gCLHeap.mMemSize %d\n", gCLHeap.mMem, gCLHeap.mMemB, gCLHeap.mCurrentMem, gCLHeap.mSleepMem, gCLHeap.mMemSize);
    memset(gCLHeap.mSleepMem, 0, gCLHeap.mMemSize);
//if(gCode) printf("con0.1 gCode->code %ld\n", gCode->mCode);
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

                /// call the destructor ///
                if(klass && klass->mFreeFun) klass->mFreeFun(obj);

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

                obj_size = get_heap_mem_size(obj);
                
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

static void gc_all()
{
    unsigned char* mark_flg;
//cl_print("cl_gc...");

    mark_flg = CALLOC(1, gCLHeap.mNumHandles);

    compaction(mark_flg);

    FREE(mark_flg);
}

void cl_gc()
{
    unsigned char* mark_flg;
//cl_print("cl_gc...");

    mark_flg = CALLOC(1, gCLHeap.mNumHandles);

    mark(mark_flg);
    compaction(mark_flg);

    FREE(mark_flg);
}

void show_heap()
{
    int i;

    cl_print("offsetnum %d\n", gCLHeap.mNumHandles);
    for(i=0; i<gCLHeap.mNumHandles; i++) {
        CLObject obj = i + FIRST_OBJ;

        if(gCLHeap.mHandles[i].mOffset == -1) {
            cl_print("%ld --> (ptr null)\n", obj);
        }
        else {
            void* data;
            sCLClass* klass;
            unsigned int existance_count;

            data = (void*)(gCLHeap.mCurrentMem + gCLHeap.mHandles[i].mOffset);
            klass = CLOBJECT_HEADER(obj)->mClass;

            if(klass && is_valid_class_pointer(klass)) {
                cl_print("%ld --> (ptr %p) (size %d) (class %s)\n", obj, data, CLOBJECT_HEADER(obj)->mHeapMemSize, REAL_CLASS_NAME(klass));

                if(klass->mShowFun) { klass->mShowFun(obj); }
            }
            else {
                cl_print("%ld --> (ptr %p) (size %d)\n", obj, data, CLOBJECT_HEADER(obj)->mHeapMemSize);
            }
        }
    }
}
