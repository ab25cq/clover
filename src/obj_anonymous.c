#include "clover.h"
#include "common.h"

void initialize_hidden_class_method_of_anonymous(sCLClass* klass)
{
    klass->mFreeFun = NULL;
    klass->mShowFun = NULL;
    klass->mMarkFun = NULL;
    klass->mCreateFun = NULL;

    if(klass->mFlags & CLASS_FLAGS_NATIVE_BOSS) {
        gAnonymousClass = klass;
        gAnonymousTypeObject = create_type_object(gAnonymousClass);
    }
}

