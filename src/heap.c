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
    void* result;

    const unsigned int index = obj - FIRST_OBJ;
    result = (void*)(gCLHeap.mCurrentMem + gCLHeap.mHandles[index].mOffset);

    return result;
}

BOOL is_valid_object(CLObject obj)
{
    BOOL result;

    result = obj >= FIRST_OBJ && obj < FIRST_OBJ + gCLHeap.mNumHandles;

    return result;
}

// result --> (0: not found) (non 0: found)
CLObject get_object_from_mvalue(MVALUE mvalue)
{
    if(is_valid_object(mvalue.mObjectValue.mValue)) {
        return mvalue.mObjectValue.mValue;
    }
    else {
        return 0;
    }
}

static unsigned int get_heap_mem_size(CLObject object)
{
    return CLOBJECT_HEADER(object)->mHeapMemSize;
}

void mark_object(CLObject obj, unsigned char* mark_flg)
{
    if(is_valid_object(obj)) {
        if(mark_flg[obj - FIRST_OBJ] == FALSE) {
            sCLClass* klass;
            CLObject type_object;

            mark_flg[obj - FIRST_OBJ] = TRUE;

            klass = CLOBJECT_HEADER(obj)->mClass;
            type_object = CLOBJECT_HEADER(obj)->mType;

            /// mark type object ///
            mark_object(type_object, mark_flg);

            /// mark objects which is contained in ///
            if(klass && klass->mMarkFun) {
                klass->mMarkFun(obj, mark_flg);
            }
        }
    }
}

static void mark(unsigned char* mark_flg, CLObject type_object)
{
    int i;
    sVMInfo* it;

    /// mark type object ///
    mark_object(type_object, mark_flg);

    /// mark type object of global value ///
    mark_object(gTypeObject, mark_flg);
    mark_object(gIntTypeObject, mark_flg);
    mark_object(gShortTypeObject, mark_flg);
    mark_object(gUIntTypeObject, mark_flg);
    mark_object(gLongTypeObject, mark_flg);
    mark_object(gCharTypeObject, mark_flg);
    mark_object(gStringTypeObject, mark_flg);
    mark_object(gFloatTypeObject, mark_flg);
    mark_object(gDoubleTypeObject, mark_flg);
    mark_object(gBoolTypeObject, mark_flg);
    mark_object(gPointerTypeObject, mark_flg);
    mark_object(gByteTypeObject, mark_flg);
    mark_object(gBytesTypeObject, mark_flg);
    mark_object(gBlockTypeObject, mark_flg);
    mark_object(gArrayTypeObject, mark_flg);
    mark_object(gRangeTypeObject, mark_flg);
    mark_object(gNullTypeObject, mark_flg);
    mark_object(gOnigurumaRegexTypeObject, mark_flg);
    mark_object(gHashTypeObject, mark_flg);
    mark_object(gAnonymousTypeObject, mark_flg);

    /// mark stack ///
    it = gHeadVMInfo;
    while(it) {
        int len;
        CLObject obj;

        len = it->stack_ptr - it->stack;

        for(i=0; i<len; i++) {
            obj = it->stack[i].mObjectValue.mValue;
            mark_object(obj, mark_flg);
        }

        /// thread obj ///
        obj = it->thread_obj;
        mark_object(obj, mark_flg);

        /// thread block obj ///
        obj = it->thread_block_obj;
        mark_object(obj, mark_flg);

        /// vm_types ///
        for(i=0; i<it->num_vm_types; i++) {
            obj = it->vm_types[i];
            mark_object(obj, mark_flg);
        }

        it = it->next_info;
    }

    /// mark class fields ///
    mark_class_fields(mark_flg);
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

                /// call the destructor ///
                if(klass && klass->mFreeFun) {
                    klass->mFreeFun(obj);
                }

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

    mark_flg = CALLOC(1, gCLHeap.mNumHandles);

    compaction(mark_flg);

    FREE(mark_flg);
}

static void gc(CLObject type_object)
{
    unsigned char* mark_flg;

    mark_flg = CALLOC(1, gCLHeap.mNumHandles);

    mark(mark_flg, type_object);
    compaction(mark_flg);

    FREE(mark_flg);
}

void cl_gc()
{
    gc(0);
}

CLObject alloc_heap_mem(int size, CLObject type_object)
{
    int handle;
    CLObject obj;

    if(gCLHeap.mMemLen + size >= gCLHeap.mMemSize) {
        gc(type_object);

        /// create new space of object ///
        if(gCLHeap.mMemLen + size >= gCLHeap.mMemSize) {
            BOOL current_is_mem_a;
            int new_heap_size;

            current_is_mem_a = gCLHeap.mMem == gCLHeap.mCurrentMem;

            new_heap_size = (gCLHeap.mMemSize + size) * 2;


            gCLHeap.mMem = xxrealloc(gCLHeap.mMem, gCLHeap.mMemSize, new_heap_size);
            memset(gCLHeap.mMem + gCLHeap.mMemSize, 0, new_heap_size - gCLHeap.mMemSize);

            gCLHeap.mMemB = xxrealloc(gCLHeap.mMemB, gCLHeap.mMemSize, new_heap_size);
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


            gCLHeap.mHandles = xxrealloc(gCLHeap.mHandles, gCLHeap.mSizeHandles, sizeof(sHandle)*new_offset_size);
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
    CLOBJECT_HEADER(obj)->mType = type_object;
    if(type_object == 0) {
        CLOBJECT_HEADER(obj)->mClass = NULL;
    }
    else {
        CLOBJECT_HEADER(obj)->mClass = CLTYPEOBJECT(type_object)->mClass;
    }
    CLOBJECT_HEADER(obj)->mExistence = 0;

    return obj;
}

void show_heap(sVMInfo* info)
{
    int i;

    vm_mutex_lock();

    VMLOG(info, "offsetnum %d\n", gCLHeap.mNumHandles);
    for(i=0; i<gCLHeap.mNumHandles; i++) {
        CLObject obj = i + FIRST_OBJ;

        if(gCLHeap.mHandles[i].mOffset == -1) {
            VMLOG(info, "%ld --> (ptr null)\n", obj);
        }
        else {
            void* data;
            sCLClass* klass;
            unsigned int existance_count;

            data = (void*)(gCLHeap.mCurrentMem + gCLHeap.mHandles[i].mOffset);
            klass = CLOBJECT_HEADER(obj)->mClass;

            VMLOG(info, "%ld --> (size %d) ", obj, CLOBJECT_HEADER(obj)->mHeapMemSize);
            show_object_value(info, obj);
        }
    }

    vm_mutex_unlock();
}
