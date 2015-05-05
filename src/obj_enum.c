#include "clover.h"
#include "common.h"

BOOL Enum_toHash(MVALUE** stack_ptr, MVALUE* lvar, sVMInfo* info, CLObject vm_type, sCLClass* klass)
{
    /*
    CLObject hash_type_object;
    CLObject type_object;
    CLObject type_object2;
    CLObject hash;
    int i;

    vm_mutex_lock();

    hash_type_object = create_type_object_with_class_name("Hash");
    push_object(hash_type_object, info);

    CLTYPEOBJECT(hash_type_object)->mGenericsTypesNum = 2;

    CLTYPEOBJECT(hash_type_object)->mGenericsTypes[0] = gStringTypeObject;
    CLTYPEOBJECT(hash_type_object)->mGenericsTypes[1] = gIntTypeObject;

    if(!create_hash_object(&hash, hash_type_object, NULL, NULL, 0, info))
    {
        pop_object_except_top(info);
        vm_mutex_unlock();
        return FALSE;
    }

    push_object(hash, info);

    for(i=0; i<klass->mNumFields; i++) {
        if(!add_item_to_hash(hash, key, item, info)) {
            pop_object_except_top(info);
            pop_object_except_top(info);
            vm_mutex_unlock();
            return FALSE;
        }
    }

    pop_object();
    pop_object();

    vm_mutex_unlock();
*/
    return TRUE;
}

