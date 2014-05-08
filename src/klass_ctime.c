
sCLClass* cl_get_class(char* real_class_name, BOOL auto_load)
{
    unsigned int hash;
    sCLClass* klass;
    
    hash = get_hash(real_class_name) % CLASS_HASH_SIZE;

    klass = gClassHashList[hash];

    while(klass) {
        if(strcmp(REAL_CLASS_NAME(klass), real_class_name) == 0) {
            return klass;
        }
        else {
            klass = klass->mNextClass;
        }
    }

    if(auto_load) {
        return load_class_from_classpath(real_class_name, TRUE);
    }

    return klass;
}
