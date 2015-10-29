#include "clover.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libgen.h>

//////////////////////////////////////////////////
// native class
//////////////////////////////////////////////////
#define NATIVE_CLASS_HASH_SIZE 512

struct sNativeClassHashItem {
    char* mClassName;
    fNativeClassInitializar mFun;
};

static struct sNativeClassHashItem gNativeClassHash[NATIVE_CLASS_HASH_SIZE];

static unsigned int get_hash_key_for_native_class(char* path)
{
    unsigned int key;
    char* p;

    p = path;

    key = 0;

    while(*p) {
        key += *p++;
    }

    return key % NATIVE_CLASS_HASH_SIZE;
}

static void put_fun_to_hash_for_native_class(char* class_name, fNativeClassInitializar fun)
{
    unsigned int key, key2;

    key = get_hash_key_for_native_class(class_name);

    key2 = key;

    while(1) {
        if(gNativeClassHash[key2].mClassName == NULL) {
            gNativeClassHash[key2].mClassName = STRDUP(class_name);
            gNativeClassHash[key2].mFun = fun;
            break;
        }
        else {
            key2++;

            if(key2 >= NATIVE_CLASS_HASH_SIZE) {
                key2 = 0;
            }
            else if(key2 == key) {
                fprintf(stderr, "overflow native class number");
                exit(1);
            }
        }
    }
}

static fNativeClassInitializar get_native_class_initializar(char* class_name)
{
    unsigned int key, key2;

    key = get_hash_key_for_native_class(class_name);

    key2 = key;

    while(1) {
        if(gNativeClassHash[key2].mClassName == NULL) {
            return NULL;
        }
        else if(strcmp(gNativeClassHash[key2].mClassName, class_name) == 0) {
            return gNativeClassHash[key2].mFun;
        }
        else {
            key2++;

            if(key2 >= NATIVE_CLASS_HASH_SIZE) {
                key2 = 0;
            }
            else if(key2 == key) {
                return NULL;
            }
        }
    }
}

struct sNativeClassStruct {
    const char* mClassName;
    fNativeClassInitializar mFun;
};

typedef struct sNativeClassStruct sNativeClass;

static sNativeClass gNativeClasses[] = {
    {"int", initialize_hidden_class_method_of_immediate_int },
    {"byte", initialize_hidden_class_method_of_immediate_byte },
    {"short", initialize_hidden_class_method_of_immediate_short },
    {"uint", initialize_hidden_class_method_of_immediate_uint },
    {"long", initialize_hidden_class_method_of_immediate_long },
    {"char", initialize_hidden_class_method_of_immediate_char },
    {"float", initialize_hidden_class_method_of_immediate_float },
    {"double", initialize_hidden_class_method_of_immediate_double },
    {"pointer", initialize_hidden_class_method_of_pointer },
    {"String", initialize_hidden_class_method_of_string },
    {"anonymous", initialize_hidden_class_method_of_anonymous },
    {"void", initialize_hidden_class_method_of_immediate_void },
    {"bool", initialize_hidden_class_method_of_immediate_bool },
    {"Null", initialize_hidden_class_method_of_immediate_null },
    {"Array$1", initialize_hidden_class_method_of_array },
    {"Range", initialize_hidden_class_method_of_range },
    {"Class", initialize_hidden_class_method_of_class_object },
    {"Field", initialize_hidden_class_method_of_field_object },
    {"Method", initialize_hidden_class_method_of_method_object },
    {"Hash$2", initialize_hidden_class_method_of_hash },
    {"Bytes", initialize_hidden_class_method_of_bytes },
    {"Block", initialize_hidden_class_method_of_block },
    {"Thread", initialize_hidden_class_method_of_thread },
    {"OnigurumaRegex", initialize_hidden_class_method_of_oniguruma_regex },
    {"Type", initialize_hidden_class_method_of_type },
    {"Mutex", initialize_hidden_class_method_of_mutex },

    { "", 0 }  // sentinel
};

//////////////////////////////////////////////////
// native method
//////////////////////////////////////////////////
#define NATIVE_METHOD_HASH_SIZE 1280

struct sNativeMethodHashItem {
    char* mPath;
    fNativeMethod mFun;
};

static struct sNativeMethodHashItem gNativeMethodHash[NATIVE_METHOD_HASH_SIZE];

static unsigned int get_hash_key_for_native_method(char* path)
{
    unsigned int key;
    char* p;

    p = path;

    key = 0;

    while(*p) {
        key += *p++;
    }

    return key % NATIVE_METHOD_HASH_SIZE;
}

static void put_fun_to_hash_for_native_method(char* path, fNativeMethod fun)
{
    unsigned int key, key2;

    key = get_hash_key_for_native_method(path);

    key2 = key;

    while(1) {
        if(gNativeMethodHash[key2].mPath == NULL) {
            gNativeMethodHash[key2].mPath = STRDUP(path);
            gNativeMethodHash[key2].mFun = fun;
            break;
        }
        else {
            key2++;

            if(key2 >= NATIVE_METHOD_HASH_SIZE) {
                key2 = 0;
            }
            else if(key2 == key) {
                fprintf(stderr, "overflow native methods number");
                exit(1);
            }
        }
    }
}

static fNativeMethod get_native_method(char* path)
{
    unsigned int key, key2;

    key = get_hash_key_for_native_method(path);

    key2 = key;

    while(1) {
        if(gNativeMethodHash[key2].mPath == NULL) {
            return NULL;
        }
        else if(strcmp(gNativeMethodHash[key2].mPath, path) == 0) {
            return gNativeMethodHash[key2].mFun;
        }
        else {
            key2++;

            if(key2 >= NATIVE_METHOD_HASH_SIZE) {
                key2 = 0;
            }
            else if(key2 == key) {
                return NULL;
            }
        }
    }
}

struct sNativeMethodStruct {
    const char* mPath;
    fNativeMethod mFun;
};

typedef struct sNativeMethodStruct sNativeMethod;

// manually sort is needed
static sNativeMethod gNativeMethods[] = {
    { "int.toByte()", int_toByte },
    { "int.toString()", int_toString },
    { "int.setValue(int)", int_setValue },
    { "int.toFloat()", int_toFloat },
    { "int.toShort()", int_toShort },
    { "int.toUInt()", int_toUInt },
    { "int.toLong()", int_toLong },
    { "int.toChar()", int_toChar },
    { "int.toDouble()", int_toDouble },
    { "byte.toInt()", byte_toInt },
    { "byte.toLong()", byte_toLong },
    { "byte.setValue(byte)", byte_setValue },
    { "short.toLong()", short_toLong },
    { "short.toInt()", short_toInt },
    { "short.setValue(short)", short_setValue },
    { "long.toChar()", long_toChar },
    { "long.toDouble()", long_toDouble },
    { "long.toFloat()", long_toFloat },
    { "long.toUInt()", long_toUInt },
    { "long.toShort()", long_toShort },
    { "long.toByte()", long_toByte },
    { "long.toInt()", long_toInt },
    { "long.toString()", long_toString },
    { "long.setValue(long)", long_setValue },
    { "char.toString()", char_toString },
    { "char.setValue(char)", char_setValue },
    { "char.toInt()", char_toInt },
    { "uint.toInt()", uint_toInt },
    { "uint.toLong()", uint_toLong },
    { "uint.setValue(uint)", uint_setValue },
    { "uint.toString()", uint_toString },
    { "float.toInt()", float_toInt },
    { "float.toDouble()", float_toDouble },
    { "float.toString()", float_toString },
    { "float.setValue(float)", float_setValue },
    { "double.toFloat()", double_toFloat },
    { "double.toInt()", double_toInt },
    { "double.toString()", double_toString },
    { "double.setValue(double)", double_setValue },
    { "Thread.join()", Thread_join },
    { "Mutex.Mutex()", Mutex_Mutex },
    { "Array$1.length()", Array_length },
    { "Bytes.length()", Bytes_length },
    { "String.toInt()", String_toInt },
    { "String.toDouble()", String_toDouble },
    { "String.replace(int,char)", String_replace },
    { "String.toBytes()", String_toBytes }, 
    { "bool.setValue(bool)", bool_setValue },
    { "pointer.setValue(pointer)", pointer_setValue },
    { "pointer.toString()", pointer_toString },
    { "pointer.getByte()", pointer_getByte },
    { "pointer.getShort()", pointer_getShort },
    { "pointer.getUInt()", pointer_getUInt },
    { "pointer.getLong()", pointer_getLong },
    { "pointer.forward(int)", pointer_forward },
    { "pointer.backward(int)", pointer_backward },
    { "pointer.equals(pointer)", pointer_equals },
    { "Bytes.char(int)", Bytes_char },
    { "Bytes.toPointer()", Bytes_toPointer },
    { "String.length()", String_length },
    { "String.char(int)", String_char },
    { "Array$1.items(int)", Array_items },
    { "Bytes.toString()", Bytes_toString },
    { "System.isatty(int)", System_isatty },
    { "System.mkdir(Path,mode_t)", System_mkdir },
    { "System.rmdir(Path)", System_rmdir },
    { "System.chroot(Path)", System_chroot },
    { "System.chdir(Path)", System_chdir },
    { "System.link(Path,Path)", System_link },
    { "System.symlink(Path,Path)", System_symlink },
    { "System.getcwd()", System_getcwd },
    { "System.system(String)", System_system },
    { "System.exit(int)", System_exit },
    { "System.sleep(int)", System_sleep },
    { "System.nanosleep(int)", System_nanosleep },
    { "System.msleep(int)", System_msleep },
    { "System.getenv(String)", System_getenv },
    { "System.srand(int)", System_srand },
    { "System.rand()", System_rand },
    { "System.execv(String,Array$1)", System_execv },
    { "System.execvp(String,Array$1)", System_execvp },
    { "System.fork()bool{}", System_fork },
    { "System.wait()", System_wait },
    { "System.waitpid(pid_t,WaitOption)", System_waitpid },
    { "System.open(Path,FileMode,int)", System_open },
    { "System.write(int,Bytes)", System_write },
    { "System.close(int)", System_close },
    { "System.read(int,Bytes,int)", System_read },
    { "System.pipe(int,int)", System_pipe },
    { "System.dup2(int,int)", System_dup2 },
    { "System.getpid()", System_getpid },
    { "System.getppid()", System_getppid },
    { "System.getpgid(pid_t)", System_getpgid },
    { "System.setpgid(pid_t,pid_t)", System_setpgid },
    { "System.tcsetpgrp(int,pid_t)", System_tcsetpgrp },
    { "System.stat(Path,stat)", System_stat },
    { "System.lstat(Path,stat)", System_lstat },
    { "System.readlink(Path)", System_readlink },
    { "System.rename(Path,Path)", System_rename },
    { "System.time()", System_time },
    { "System.basename(Path)", System_basename },
    { "System.dirname(Path)", System_dirname },
    { "System.chmod(Path,mode_t)", System_chmod },
    { "System.lchmod(Path,mode_t)", System_lchmod },
    { "System.chown(Path,uid_t,gid_t)", System_chown },
    { "System.lchown(Path,uid_t,gid_t)", System_lchown },
    { "System.getuid()", System_getuid },
    { "System.getgid()", System_getgid },
    { "System.unlink(Path)", System_unlink },
    { "System.access(Path,AccessMode)", System_access },
    { "System.utime(Path,utimbuf)", System_utime },
    { "System.mktime(tm)", System_mktime },
    { "System.fnmatch(String,Path,FnmatchFlags)", System_fnmatch },
    { "System.truncate(Path,off_t)", System_truncate },
    { "System.umask(mode_t)", System_umask },
    { "System.flock(int,FileLockOperation)", System_flock },
    { "System.opendir(Path)", System_opendir },
    { "System.readdir(DIR)", System_readdir },
    { "System.closedir(DIR)", System_closedir },
    { "Object.type()", Object_type },
    { "Mutex.run()bool{}", Mutex_run },
    { "Hash$2.setValue(Hash$2)", Hash_setValue },
    { "Type.toString()", Type_toString },
    { "Array$1.setValue(Array$1)", Array_setValue },
    { "Clover.print(String)", Clover_print },
    { "Clover.gc()", Clover_gc },
    { "Array$1.add(GenericsParam0)", Array_add },
    { "Bytes.setValue(Bytes)", Bytes_setValue },
    { "Clover.showClasses()", Clover_showClasses },
    { "Bytes.replace(int,byte)", Bytes_replace },
    { "String.setValue(String)", String_setValue },
    { "Type.equals(Type)", Type_equals } ,
    { "Thread._constructor()bool{}", Thread_Thread },
    { "Clover.outputToString()bool{}", Clover_outputToString }, 
    { "Object.ID()", Object_ID },
    { "Type.class()", Type_class },
    { "Type.genericsParam(int)", Type_genericsParam },
    { "Type.genericsParamNumber()", Type_genericsParamNumber },
    { "Type.parentClass()", Type_parentClass },
    { "Type.parentClassNumber()", Type_parentClassNumber },
    { "Array$1.setItem(int,GenericsParam0)", Array_setItem },
    { "Range.head()", Range_head },
    { "Range.tail()", Range_tail },
    { "String.cmp(String,bool)", String_cmp },
    { "Bytes.cmp(Bytes)", Bytes_cmp },
    { "OnigurumaRegex.setValue(OnigurumaRegex)", OnigurumaRegex_setValue },
    { "OnigurumaRegex.ignoreCase()", OnigurumaRegex_ignoreCase },
    { "OnigurumaRegex.source()", OnigurumaRegex_source },
    { "OnigurumaRegex.global()", OnigurumaRegex_global },
    { "OnigurumaRegex.multiLine()", OnigurumaRegex_multiLine },
    { "OnigurumaRegex.encode()", OnigurumaRegex_encode },
    { "OnigurumaRegex.compile(String,bool,bool,bool,Encoding)", OnigurumaRegex_compile },
    { "Hash$2.put(GenericsParam0,GenericsParam1)", Hash_put },
    { "Hash$2.assoc(GenericsParam0)", Hash_assoc },
    { "Hash$2.length()", Hash_length },
    { "Hash$2.each()bool{GenericsParam0,GenericsParam1}", Hash_each },
    { "Object.fields(int)", Object_fields },
    { "Object.numFields()", Object_numFields },
    { "Object.setField(int,anonymous)", Object_setField },
    { "Class.newInstance()", Class_newInstance },
    { "Class.isNativeClass()", Class_isNativeClass },
    { "Class.toType()", Class_toType },
    { "Class.toString()", Class_toString },
    { "Type.classObject()", Type_classObject },
    { "Type.setValue(Type)", Type_setValue },
    { "Class.setValue(Class)", Class_setValue },
    { "Thread.setValue(Thread)", Thread_setValue },
    { "Range.setValue(Range)", Range_setValue },
    { "Range.setHead(int)", Range_setHead },
    { "Range.setTail(int)", Range_setTail },
    { "Mutex.setValue(Mutex)", Mutex_setValue },
    { "Field.isStaticField()", Field_isStaticField },
    { "Field.isPrivateField()", Field_isPrivateField },
    { "Field.isProtectedField()", Field_isProtectedField },
    { "Field.name()", Field_name },
    { "Field.setValue(Field)", Field_setValue },
    { "Field.fieldType()", Field_fieldType },
    { "Class.fields()", Class_fields },
    { "Class.methods()", Class_methods },
    { "Method.setValue(Method)", Method_setValue },
    { "Method.isNativeMethod()", Method_isNativeMethod },
    { "Method.isClassMethod()", Method_isClassMethod },
    { "Method.isPrivateMethod()", Method_isPrivateMethod },
    { "Method.isConstructor()", Method_isConstructor },
    { "Method.isSynchronizedMethod()", Method_isSyncronizedMethod },
    { "Method.isVirtualMethod()", Method_isVirtualMethod },
    { "Method.isAbstractMethod()", Method_isAbstractMethod },
    { "Method.isGenericsNewableConstructor()", Method_isGenericsNewableConstructor },
    { "Method.isProtectedMethod()", Method_isProtectedMethod },
    { "Method.isParamVariableArguments()", Method_isParamVariableArguments },
    { "Method.name()", Method_name },
    { "Method.path()", Method_path },
    { "Method.resultType()", Method_resultType },
    { "Method.parametors()", Method_parametors },
    { "Method.blockResultType()", Method_blockResultType },
    { "Method.blockParametors()", Method_blockParametors },
    { "Method.exceptions()", Method_exceptions },
    { "Class.isInterface()", Class_isInterface },
    { "Class.isAbstractClass()", Class_isAbstractClass },
    { "Class.isFinalClass()", Class_isFinalClass },
    { "Class.isStruct()", Class_isStruct },
    { "Class.superClasses()", Class_superClasses },
    { "Class.implementedInterfaces()", Class_implementedInterfaces },
    { "Class.classDependences()", Class_classDependences },
    { "Class.genericsParametorTypes()", Class_genericsParametorTypes },
    { "Field.get(Object)", Field_get } ,
    { "Field.set(Object,anonymous)", Field_set } ,
    { "Method.invokeMethod(Object,Array$1)", Method_invokeMethod } ,
    { "Type.createFromString(String)", Type_createFromString },
    { "Type.substitutionPosibility(Type,bool)", Type_substitutionPosibility },
    { "Hash$2.erase(GenericsParam0)", Hash_erase },
    { "Enum.toHash()", Enum_toHash },

    { "String.match(Regex,int,int)bool{int,int,Array$1}", String_match },
    { "String.matchReverse(Regex,int,int)bool{int,int,Array$1}", String_matchReverse },
    { "OnigurumaRegex.setEncode(Encoding)", OnigurumaRegex_setEncode },
    { "OnigurumaRegex.setGlobal(bool)", OnigurumaRegex_setGlobal },
    { "OnigurumaRegex.setMultiLine(bool)", OnigurumaRegex_setMultiLine },
    { "OnigurumaRegex.setIgnoreCase(bool)", OnigurumaRegex_setIgnoreCase },
    { "Block.parametors()", Block_parametors },
    { "Block.resultType()", Block_resultType },
    { "WaitStatus.exited()", WaitStatus_exited },
    { "WaitStatus.exitStatus()", WaitStatus_exitStatus },
    { "WaitStatus.signaled()", WaitStatus_signaled },
    { "WaitStatus.signalNumber()", WaitStatus_signalNumber },
    { "tm._constructor(time_t)", tm_tm },
    { "Clover.printf(String,Array$1)", Clover_printf },
    { "Clover.sprintf(String,Array$1)", Clover_sprintf },

    { "", 0 }  // sentinel
};

//////////////////////////////////////////////////
// get class
//////////////////////////////////////////////////
unsigned int get_hash(char* name)
{
    unsigned int hash;
    char* p;
    
    hash = 0;
    p = name;
    while(*p) {
        hash += *p++;
    }

    return hash;
}

sCLClass* gClassHashList[CLASS_HASH_SIZE];

BOOL is_valid_class_pointer(void* class_pointer)
{
    int i;

    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;

            klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass;
                
                next_klass = klass->mNextClass;
                if(klass == class_pointer) {
                    return TRUE;
                }
                klass = next_klass;
            }
        }
    }

    return FALSE;
}

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class(char* real_class_name)
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

    return klass;
}

BOOL is_dynamic_typing_class(sCLClass* klass)
{
    int i;

    for(i=0; i<klass->mNumSuperClasses; i++) {
        sCLClass* super;
        char* real_class_name;

        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);

        super = cl_get_class(real_class_name);

        ASSERT(super != NULL);

        if(super->mFlags & CLASS_FLAGS_DYNAMIC_TYPING) {
            return TRUE;
        }
    }

    if(klass->mFlags & CLASS_FLAGS_DYNAMIC_TYPING) {
        return TRUE;
    }

    return FALSE;
}

BOOL is_generics_param_class(sCLClass* klass)
{
    return klass && klass->mFlags & CLASS_FLAGS_GENERICS_PARAM;
}

void create_real_class_name(char* result, int result_size, char* namespace, char* class_name, int parametor_num)
{
    if(namespace[0] == 0) {
        if(parametor_num == 0) {
            xstrncpy(result, class_name, result_size);
        }
        else {
            snprintf(result, result_size, "%s$%d", class_name, parametor_num);
        }
    }
    else {
        if(parametor_num == 0) {
            xstrncpy(result, namespace, result_size);
            xstrncat(result, "::", result_size);
            xstrncat(result, class_name, result_size);
        }
        else {
            snprintf(result, result_size, "%s::%s$%d", namespace, class_name, parametor_num);
        }
    }
}

// result: (NULL) --> not found (non NULL) --> (sCLClass*)
sCLClass* cl_get_class_with_namespace(char* namespace, char* class_name, int parametor_num)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    sCLClass* result;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name, parametor_num);

    result = cl_get_class(real_class_name);
    if(result == NULL) {
        /// default namespace ///
        create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, "", class_name, parametor_num);
        return cl_get_class(real_class_name);
    }
    else {
        return result;
    }
}

//////////////////////////////////////////////////
// alloc class
//////////////////////////////////////////////////
static void add_class_to_class_table(char* namespace, char* class_name, sCLClass* klass, int parametor_num)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    unsigned int hash;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name, parametor_num);

    /// added this to class table ///
    hash = get_hash(real_class_name) % CLASS_HASH_SIZE;
    klass->mNextClass = gClassHashList[hash];
    gClassHashList[hash] = klass;
}

static void remove_class_from_class_table(char* namespace, char* class_name, int parametor_num)
{
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];
    unsigned int hash;
    sCLClass* klass;
    sCLClass* previous_klass;

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name, parametor_num);

    /// added this to class table ///
    hash = get_hash(real_class_name) % CLASS_HASH_SIZE;

    klass = gClassHashList[hash];
    previous_klass = NULL;
    while(klass) {
        if(strcmp(REAL_CLASS_NAME(klass), real_class_name) == 0) {
            if(previous_klass) {
                previous_klass->mNextClass = klass->mNextClass;
            }
            else {
                gClassHashList[hash] = klass->mNextClass;
            }
        }

        previous_klass = klass;
        klass = klass->mNextClass;
    }
}

sCLClass* gVoidClass;
sCLClass* gIntClass;
sCLClass* gByteClass;
sCLClass* gShortClass;
sCLClass* gUIntClass;
sCLClass* gLongClass;
sCLClass* gCharClass;
sCLClass* gIntClass;
sCLClass* gFloatClass;
sCLClass* gDoubleClass;
sCLClass* gBoolClass;
sCLClass* gPointerClass;
sCLClass* gNullClass;
sCLClass* gObjectClass;
sCLClass* gArrayClass;
sCLClass* gRangeClass;
sCLClass* gBytesClass;
sCLClass* gHashClass;
sCLClass* gBlockClass;
sCLClass* gTypeClass;
sCLClass* gStringClass;
sCLClass* gThreadClass;
sCLClass* gOnigurumaRegexClass;
sCLClass* gGParamClass[CL_GENERICS_CLASS_PARAM_MAX];
sCLClass* gAnonymousClass;
sCLClass* gExceptionClass;

static void initialize_hidden_class_method_and_flags(sCLClass* klass)
{
    if(klass->mFlags & CLASS_FLAGS_NATIVE) {
        fNativeClassInitializar fun;

        fun = get_native_class_initializar(REAL_CLASS_NAME(klass));

        /// a children of native class ///
        if(fun == NULL) {
            int i;
            for(i=klass->mNumSuperClasses-1; i>=0; i--) {
                char* real_class_name;

                real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
                
                fun = get_native_class_initializar(real_class_name);

                if(fun) {
                    fun(klass);
                }
            }
        }
        else {
            fun(klass);
        }
    }
    else {
        initialize_hidden_class_method_of_user_object(klass);
    }
}

// result should be not NULL
sCLClass* alloc_class(char* namespace, char* class_name, BOOL private_, BOOL abstract_, BOOL interface, BOOL dynamic_typing_, BOOL final_, BOOL native_, BOOL struct_, BOOL enum_, int parametor_num)
{
    sCLClass* klass;
    sCLClass* klass2;
    int i;
    char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];

    /// if there is a class which is entried already, return the class
    klass2 = cl_get_class_with_namespace(namespace, class_name, parametor_num);
    
    if(klass2) {
        return klass2;
    }

    klass = CALLOC(1, sizeof(sCLClass));

    sConst_init(&klass->mConstPool);

    klass->mFieldsInitialized = FALSE;

    klass->mFlags = (long long)(private_ ? CLASS_FLAGS_PRIVATE:0) | (long long)(interface ? CLASS_FLAGS_INTERFACE:0) | (long long)(abstract_ ? CLASS_FLAGS_ABSTRACT:0) | (long long)(dynamic_typing_ ? CLASS_FLAGS_DYNAMIC_TYPING:0) | (long long)(final_ ? CLASS_FLAGS_FINAL:0) | (long long)(struct_ ? CLASS_FLAGS_STRUCT:0) | (long long)(enum_ ? CLASS_FLAGS_ENUM:0) | (long long)(native_ ? CLASS_FLAGS_NATIVE|CLASS_FLAGS_NATIVE_BOSS:0);

    klass->mSizeMethods = 4;
    klass->mMethods = CALLOC(1, sizeof(sCLMethod)*klass->mSizeMethods);
    klass->mSizeFields = 4;
    klass->mFields = CALLOC(1, sizeof(sCLField)*klass->mSizeFields);

    memset(klass->mSuperClasses, 0, sizeof(klass->mSuperClasses));
    klass->mNumSuperClasses = 0;

    memset(klass->mImplementedInterfaces, 0, sizeof(klass->mImplementedInterfaces));
    klass->mNumImplementedInterfaces = 0;

    memset(klass->mIncludedModules, 0, sizeof(klass->mIncludedModules));
    
    klass->mNumIncludedModules = 0;

    klass->mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, class_name, FALSE);  // class name
    klass->mNameSpaceOffset = append_str_to_constant_pool(&klass->mConstPool, namespace, FALSE);   // namespace 

    create_real_class_name(real_class_name, CL_REAL_CLASS_NAME_MAX, namespace, class_name, parametor_num);
    klass->mRealClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, real_class_name, FALSE);  // real class name

    add_class_to_class_table(namespace, class_name, klass, parametor_num);

    klass->mSizeDependences = 4;
    klass->mDependencesOffset = CALLOC(1, sizeof(int)*klass->mSizeDependences);
    klass->mNumDependences = 0;

    klass->mNumVirtualMethodMap = 0;
    klass->mSizeVirtualMethodMap = 4;
    klass->mVirtualMethodMap = CALLOC(1, sizeof(sVMethodMap)*klass->mSizeVirtualMethodMap);

    klass->mCloneMethodIndex = -1;
    klass->mInitializeMethodIndex = -1;
    klass->mMethodMissingMethodIndex = -1;
    klass->mMethodMissingMethodIndexOfClassMethod = -1;
    klass->mCompletionMethodIndex = -1;
    klass->mCompletionMethodIndexOfClassMethod = -1;

    klass->mNumLoadedMethods = 0;
    klass->mMethodIndexOfCompileTime = 0;

    return klass;
}

static void free_class(sCLClass* klass)
{
    sConst_free(&klass->mConstPool);

    if(klass->mMethods) {
        int i;

        for(i=0; i<klass->mNumMethods; i++) {
            sCLMethod* method = klass->mMethods + i;
            if(method->mParamTypes) FREE(method->mParamTypes);
            if(method->mParamInitializers) {
                int i;
                for(i=0; i<method->mNumParams; i++) {
                    if(method->mParamInitializers[i].mInitializer.mCode) {
                        sByteCode_free(&method->mParamInitializers[i].mInitializer);
                    }
                }
                FREE(method->mParamInitializers);
            }

            if(!(method->mFlags & CL_NATIVE_METHOD) && method->uCode.mByteCodes.mCode != NULL)
            {
                sByteCode_free(&method->uCode.mByteCodes);
            }

            if(method->mNumBlockType > 0) {
                if(method->mBlockType.mParamTypes) FREE(method->mBlockType.mParamTypes);
            }
        }
        FREE(klass->mMethods);
    }

    if(klass->mFields) {
        int i;
        for(i=0; i<klass->mNumFields; i++) {
            sCLField* field = klass->mFields + i;

            if(field->mInitializer.mSize > 0) {
                sByteCode_free(&field->mInitializer);
            }
        }
        FREE(klass->mFields);
    }

    if(klass->mDependencesOffset) {
        FREE(klass->mDependencesOffset);
    }

    if(klass->mVirtualMethodMap) {
        FREE(klass->mVirtualMethodMap);
    }

    FREE(klass);
}

//////////////////////////////////////////////////
// fields
//////////////////////////////////////////////////
int get_static_fields_num(sCLClass* klass)
{
    int static_field_num;
    int i;

    static_field_num = 0;
    for(i=0; i<klass->mNumFields; i++) {
        sCLField* cl_field;

        cl_field = klass->mFields + i;

        if(cl_field->mFlags & CL_STATIC_FIELD) {
            static_field_num++;
        }
    }

    return static_field_num;
}

static int get_static_fields_num_including_super_class(sCLClass* klass)
{
    int i;
    int static_field_num;

    static_field_num = 0;
    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        static_field_num += get_static_fields_num(super_class);
    }

    static_field_num += get_static_fields_num(klass);

    return static_field_num;
}

BOOL run_fields_initializer(CLObject object, sCLClass* klass, CLObject vm_type)
{
    int i;
    int static_field_num;

    static_field_num = 0;

    for(i=0; i<klass->mNumFields; i++) {
        sCLField* cl_field;
        int max_stack;
        int lv_num;

        cl_field = klass->mFields + i;

        if(cl_field->mFlags & CL_STATIC_FIELD) {
            static_field_num++;
        }
        else if(cl_field->mInitializer.mSize > 0) {
            MVALUE result;

            lv_num = cl_field->mInitializerLVNum;
            max_stack = cl_field->mInitializerMaxStack;

            if(!field_initializer(&result, &cl_field->mInitializer, &klass->mConstPool, lv_num, max_stack, vm_type)) {
                return FALSE;
            }

            CLUSEROBJECT(object)->mFields[i-static_field_num] = result;
        }
        else {
            CLObject null_object;

            null_object = create_null_object();
            CLUSEROBJECT(object)->mFields[i-static_field_num].mObjectValue.mValue = null_object;
        }
    }

    return TRUE;
}

static BOOL run_class_fields_initializer(sCLClass* klass)
{
    int i;

    for(i=0; i<klass->mNumFields; i++) {
        sCLField* cl_field;
        int max_stack;
        int lv_num;

        cl_field = klass->mFields + i;

        if(cl_field->mFlags & CL_STATIC_FIELD && cl_field->uValue.mStaticField.mObjectValue.mValue == 0) {
            MVALUE result;

            lv_num = cl_field->mInitializerLVNum;
            max_stack = cl_field->mInitializerMaxStack;

            if(!field_initializer(&result, &cl_field->mInitializer, &klass->mConstPool, lv_num, max_stack, 0)) {
                return FALSE;
            }

            cl_field->uValue.mStaticField = result;
        }
    }

    klass->mFieldsInitialized = TRUE;

    return TRUE;
}

static void mark_class_fields_of_class(sCLClass* klass, unsigned char* mark_flg)
{
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        sCLField* field = &klass->mFields[i];

        if(field->mFlags & CL_STATIC_FIELD) {
            mark_object(field->uValue.mStaticField.mObjectValue.mValue, mark_flg);
        }
    }
}

static void mark_class_fields_of_class_and_super_class(sCLClass* klass, unsigned char* mark_flg)
{
    int i;

    for(i=klass->mNumSuperClasses-1; i>=0; i--) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        mark_class_fields_of_class(super_class, mark_flg);
    }

    mark_class_fields_of_class(klass, mark_flg);
}

void mark_class_fields(unsigned char* mark_flg)
{
    int i;
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;
            
            klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass;
                
                next_klass = klass->mNextClass;
                mark_class_fields_of_class_and_super_class(klass, mark_flg);
                klass = next_klass;
            }
        }
    }
}

static int get_sum_of_fields_on_super_classes(sCLClass* klass)
{
    int sum = 0;
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        sum += super_class->mNumFields;
    }

    return sum;
}

// return field number
int get_field_num_including_super_classes(sCLClass* klass)
{
    return get_sum_of_fields_on_super_classes(klass) + klass->mNumFields;
}

// return field number
int get_field_num_including_super_classes_without_class_field(sCLClass* klass)
{
    return get_field_num_including_super_classes(klass) - get_static_fields_num_including_super_class(klass);
}

// result: (NULL) --> not found (non NULL) --> field
sCLField* get_field(sCLClass* klass, char* field_name, BOOL class_field)
{
    int i;
    for(i=0; i<klass->mNumFields; i++) {
        sCLField* field;
        BOOL class_field2;

        field = klass->mFields + i;
        class_field2 = field->mFlags & CL_STATIC_FIELD;
        if(strcmp(FIELD_NAME(klass, i), field_name) == 0 && class_field == class_field2) {
            return klass->mFields + i;
        }
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// methods
///////////////////////////////////////////////////////////////////////////////
void alloc_bytecode_of_method(sCLMethod* method)
{
    sByteCode_init(&method->uCode.mByteCodes);
}

static BOOL solve_generics_types_of_class(sCLClass* klass, sCLClass** result, CLObject type_object)
{
    int i;

    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
        if(klass == gGParamClass[i]) {
            if(i < CLTYPEOBJECT(type_object)->mGenericsTypesNum) {
                CLObject type_object2;

                type_object2 = CLTYPEOBJECT(type_object)->mGenericsTypes[i];
                *result = CLTYPEOBJECT(type_object2)->mClass;
                return TRUE;
            }
            else {
                *result = klass; // error
                return FALSE;
            }
        }
    }

    *result = klass; // no solved

    return TRUE;
}

static BOOL check_method_params(sCLMethod* method, sCLClass* klass, char* method_name, CLObject* class_params, int num_params, BOOL search_for_class_method, int block_num, int block_num_params, CLObject* block_param_type, CLObject block_type, CLObject type_object, sVMInfo* info)
{
    if(strcmp(METHOD_NAME2(klass, method), method_name) ==0) {
        if((search_for_class_method && (method->mFlags & CL_CLASS_METHOD)) || (!search_for_class_method && !(method->mFlags & CL_CLASS_METHOD))) 
        {
            /// type checking ///
            if(method->mNumParams == -1) {              // no type checking of method params
                return TRUE;
            }
            else if(num_params <= method->mNumParams && num_params + method->mNumParamInitializer >= method->mNumParams) {
            //else if(num_params <= method->mNumParams) {
                int j, k;

                for(j=0; j<num_params; j++ ) {
                    CLObject klass_of_param;
                    CLObject solved_klass_of_param;
                    CLObject klass_of_param2;

                    klass_of_param = create_type_object_from_cl_type(klass, &method->mParamTypes[j], info);

                    push_object(klass_of_param, info);

                    klass_of_param2 = class_params[j];

                    if(!solve_generics_types_of_type_object(klass_of_param, &solved_klass_of_param, type_object, info))
                    {
                        pop_object(info);
                        return FALSE;
                    }

                    push_object(solved_klass_of_param, info);

                    if(!substitution_posibility_of_type_object(solved_klass_of_param, klass_of_param2, TRUE))
                    {
                        pop_object(info);
                        pop_object(info);
                        return FALSE;
                    }

                    pop_object(info);
                    pop_object(info);
                }

                if(block_num == method->mNumBlockType && block_num_params == method->mBlockType.mNumParams) {
                    if(block_num > 0) {
                        CLObject klass_of_param;
                        CLObject solved_klass_of_param;
                        CLObject klass_of_param2;

                        klass_of_param = create_type_object_from_cl_type(klass, &method->mBlockType.mResultType, info);
                        push_object(klass_of_param, info);

                        klass_of_param2 = block_type;

                        if(!solve_generics_types_of_type_object(klass_of_param, &solved_klass_of_param, type_object, info))
                        {
                            pop_object(info);
                            return FALSE;
                        }
                        push_object(solved_klass_of_param, info);

                        if(!substitution_posibility_of_type_object(solved_klass_of_param, klass_of_param2, TRUE))
                        {
                            pop_object(info);
                            pop_object(info);
                            return FALSE;
                        }
                        pop_object(info);
                        pop_object(info);
                    }
                    
                    for(k=0; k<block_num_params; k++) {
                        CLObject klass_of_param;
                        CLObject solved_klass_of_param;
                        CLObject klass_of_param2;
                        sCLClass* solved_klass_of_param2;

                        klass_of_param = create_type_object_from_cl_type(klass, &method->mBlockType.mParamTypes[k], info);

                        push_object(klass_of_param, info);

                        klass_of_param2 = block_param_type[k];

                        if(!solve_generics_types_of_type_object(klass_of_param, &solved_klass_of_param, type_object, info))
                        {
                            pop_object(info);
                            return FALSE;
                        }

                        push_object(solved_klass_of_param, info);
                        
                        if(!substitution_posibility_of_type_object(solved_klass_of_param, klass_of_param2, TRUE))
                        {
                            pop_object(info);
                            pop_object(info);
                            return FALSE;
                        }

                        pop_object(info);
                        pop_object(info);
                    }

                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

static sCLMethod* search_for_method_from_virtual_method_table(CLObject type_object, char* method_name, CLObject* class_params, int num_params, BOOL search_for_class_method, int block_num, int block_num_params, CLObject* block_param_type, CLObject block_type, sVMInfo* info)
{
    int hash;
    sVMethodMap* item;
    int i;
    sCLClass* klass;

    klass = CLTYPEOBJECT(type_object)->mClass;

    hash = get_hash(method_name) % klass->mSizeVirtualMethodMap;

    item = klass->mVirtualMethodMap + hash;

    while(1) {
        if(item->mMethodName[0] == 0) {
            break;
        }
        else {
            sCLMethod* method;

            method = klass->mMethods + item->mMethodIndex;

            if(check_method_params(method, klass, method_name, class_params, num_params, search_for_class_method, block_num, block_num_params, block_param_type, block_type, type_object, info))
            {
                return method;
            }
            else {
                item++;

                if(item == klass->mVirtualMethodMap + klass->mSizeVirtualMethodMap) {
                    item = klass->mVirtualMethodMap;
                }
                else if(item == klass->mVirtualMethodMap + hash) {
                    break;
                }
            }
        }
    }

    return NULL;
}

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class
sCLMethod* get_virtual_method_with_params(CLObject type_object, char* method_name, CLObject* class_params, int num_params, sCLClass** founded_class, BOOL search_for_class_method, int block_num, int block_num_params, CLObject* block_param_type, CLObject block_type,sVMInfo* info)
{
    sCLMethod* result;
    int i;
    sCLClass* klass;

    push_object(type_object, info);

    klass = CLTYPEOBJECT(type_object)->mClass;

    result = search_for_method_from_virtual_method_table(type_object, method_name, class_params, num_params, search_for_class_method, block_num, block_num_params, block_param_type, block_type, info);

    *founded_class = klass;
    
    if(result == NULL) {
        int i;

        for(i=klass->mNumSuperClasses-1; i>=0; i--) {
            CLObject new_type_object;
            CLObject solved_new_type_object;

            new_type_object = get_type_object_from_cl_type(&klass->mSuperClasses[i], klass, info);

            push_object(new_type_object, info);

            if(!solve_generics_types_of_type_object(new_type_object, ALLOC &solved_new_type_object, type_object, info)) {
                remove_object(info, 2);
                return NULL;
            }

            pop_object(info);
            pop_object(info);

            type_object = solved_new_type_object;
            push_object(type_object, info);

            result = search_for_method_from_virtual_method_table(type_object, method_name, class_params, num_params, search_for_class_method, block_num, block_num_params, block_param_type, block_type, info);

            if(result) {
                *founded_class = CLTYPEOBJECT(type_object)->mClass;
                break;
            }
        }
    }

    //ASSERT(result != NULL);

    pop_object(info);

    return result;
}

// result: (NULL) not found the method (sCLMethod*) found method. (sCLClass** founded_class) was setted on the method owner class

// result: (TRUE) found (FALSE) not found
BOOL search_for_super_class(sCLClass* klass, sCLClass* searched_class)
{
    int i;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        if(super_class == searched_class) {
            return TRUE;                // found
        }
    }

    return FALSE;
}

static BOOL search_for_implemeted_interface_core(sCLClass* klass, sCLClass* interface) 
{
    int i;

    for(i=0; i<klass->mNumImplementedInterfaces; i++) {
        char* real_class_name;
        sCLClass* interface2;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mImplementedInterfaces[i].mClassNameOffset);
        interface2 = cl_get_class(real_class_name);

        ASSERT(interface2 != NULL);     // checked on load time

        if(interface == interface2) {
            return TRUE;                // found
        }

        /// search for the super class of this interface ///
        if(search_for_super_class(interface2, interface)) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL search_for_implemeted_interface(sCLClass* klass, sCLClass* interface)
{
    int i;

    for(i=0; i<klass->mNumSuperClasses; i++) {
        char* real_class_name;
        sCLClass* super_class;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[i].mClassNameOffset);
        super_class = cl_get_class(real_class_name);

        ASSERT(super_class != NULL);     // checked on load time

        if(search_for_implemeted_interface_core(super_class, interface)) {
            return TRUE;
        }
    }

    if(search_for_implemeted_interface_core(klass, interface)) {
        return TRUE;
    }

    return FALSE;
}


//////////////////////////////////////////////////
// load and save class
//////////////////////////////////////////////////
static BOOL read_from_file(int f, void* buf, size_t size)
{
    size_t size2;

    size2 = read(f, buf, size);

    if(size2 < size) {
        return FALSE;
    }

    return TRUE;
}

static BOOL read_char_from_file(int fd, char* c)
{
    return read_from_file(fd, c, sizeof(char));
}

static BOOL read_int_from_file(int fd, int* n)
{
    return read_from_file(fd, n, sizeof(int));
}

static BOOL read_long_long_from_file(int fd, long long* n)
{
    return read_from_file(fd, n, sizeof(long long));
}

static BOOL read_type_from_file(int fd, sCLType* type)
{
    int j;
    char c;
    int n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    type->mClassNameOffset = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    type->mStar = n;

    if(!read_char_from_file(fd, &c)) {
        return FALSE;
    }

    type->mGenericsTypesNum = c;
    for(j=0; j<type->mGenericsTypesNum; j++) {
        type->mGenericsTypes[j] = allocate_cl_type();
        if(!read_type_from_file(fd, type->mGenericsTypes[j])) {
            return FALSE;
        }
    }

    return TRUE;
}

static BOOL read_params_from_file(int fd, int* num_params, sCLType** param_types)
{
    int j;
    int n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    
    *num_params = n;

    if(*num_params > 0) {
        *param_types = CALLOC(1, sizeof(sCLType)*(*num_params));
        for(j=0; j<*num_params; j++) {
            if(!read_type_from_file(fd, (*param_types) + j)) {
                return FALSE;
            }
        }
    }
    else {
        *param_types = NULL;
    }

    return TRUE;
}

static BOOL read_param_initializer_from_file(int fd, sCLParamInitializer* param_initializer)
{
    int len_bytecodes;
    int n;

    if(!read_int_from_file(fd, &len_bytecodes)) {
        return FALSE;
    }

    if(len_bytecodes > 0) {
        int* bytecodes;

        sByteCode_init(&param_initializer->mInitializer);
        bytecodes = MALLOC(sizeof(int)*len_bytecodes);

        if(!read_from_file(fd, bytecodes, sizeof(int)*len_bytecodes)) {
            FREE(bytecodes);
            return FALSE;
        }

        append_buf_to_bytecodes(&param_initializer->mInitializer, bytecodes, len_bytecodes, FALSE);

        FREE(bytecodes);
    }
    else {
        param_initializer->mInitializer.mCode = NULL;
    }

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    param_initializer->mMaxStack = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    param_initializer->mLVNum = n;

    return TRUE;
}

static BOOL read_param_initializers_from_file(int fd, int* num_params, sCLParamInitializer** param_initializer)
{
    int j;
    int n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    
    *num_params = n;

    if(*num_params > 0) {
        *param_initializer = CALLOC(1, sizeof(sCLParamInitializer)*(*num_params));
        for(j=0; j<*num_params; j++) {
            if(!read_param_initializer_from_file(fd, (*param_initializer) + j)) 
            {
                return FALSE;
            }
        }
    }
    else {
        *param_initializer = NULL;
    }

    return TRUE;
}

static BOOL read_block_type_from_file(int fd, sCLBlockType* block_type)
{
    int n;

    if(!read_type_from_file(fd, &block_type->mResultType)) {
        return FALSE;
    }

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    block_type->mNameOffset = n;

    if(!read_params_from_file(fd, &block_type->mNumParams, &block_type->mParamTypes)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL read_field_from_file(int fd, sCLField* field)
{
    int n;
    int len_bytecodes;
    int* bytecodes;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    field->mFlags = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    field->mNameOffset = n;

    field->uValue.mStaticField.mObjectValue.mValue = 0; //read_mvalue_from_buffer(p, file_data);

    /// initializer ////
    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    len_bytecodes = n;

    if(len_bytecodes > 0) {
        sByteCode_init(&field->mInitializer);
        bytecodes = MALLOC(sizeof(int)*len_bytecodes);

        if(!read_from_file(fd, bytecodes, sizeof(int)*len_bytecodes)) {
            FREE(bytecodes);
            return FALSE;
        }

        append_buf_to_bytecodes(&field->mInitializer, bytecodes, len_bytecodes, FALSE);

        FREE(bytecodes);
    }

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    field->mInitializerLVNum = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    field->mInitializerMaxStack = n;

    if(!read_type_from_file(fd, &field->mType)) {
        return FALSE;
    }

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    field->mFieldIndex = n;

    return TRUE;
}

static BOOL read_method_from_buffer(sCLClass* klass, sCLMethod* method, int fd)
{
    int n;
    int i;
    char c;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    method->mFlags = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    method->mNameOffset = n;
    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    method->mPathOffset = n;

    if(method->mFlags & CL_NATIVE_METHOD) {
        char* method_path;

        method_path = CONS_str(&klass->mConstPool, method->mPathOffset);
        method->uCode.mNativeMethod = get_native_method(method_path);
        if(method->uCode.mNativeMethod == NULL) {
            vm_error("native method(%s) is not found\n", method_path);
            exit(2);
        }
    }
    else {
        int len_bytecodes;

        alloc_bytecode_of_method(method);

        if(!read_int_from_file(fd, &n)) {
            return FALSE;
        }
        len_bytecodes = n;

        if(len_bytecodes > 0) {
            int* bytecodes = MALLOC(sizeof(int)*len_bytecodes);

            if(!read_from_file(fd, bytecodes, sizeof(int)*len_bytecodes)) {
                FREE(bytecodes);
                return FALSE;
            }

            append_buf_to_bytecodes(&method->uCode.mByteCodes, bytecodes, len_bytecodes, FALSE);

            FREE(bytecodes);
        }
    }

    if(!read_type_from_file(fd, &method->mResultType)) {
        return FALSE;
    }
    if(!read_params_from_file(fd, &method->mNumParams, &method->mParamTypes)) {
        return FALSE;
    }
    if(!read_param_initializers_from_file(fd, &method->mNumParams, &method->mParamInitializers)) {
        return FALSE;
    }

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    method->mNumLocals = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    method->mMaxStack = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    method->mNumBlockType = n;

    if(method->mNumBlockType >= 1) {
        if(!read_block_type_from_file(fd, &method->mBlockType)) {
            return FALSE;
        }
    }

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    method->mNumException = n;

    for(i=0; i<method->mNumException; i++) {
        if(!read_int_from_file(fd, &n)) {
            return FALSE;
        }
        method->mExceptionClassNameOffset[i] = n;
    }

    if(!read_char_from_file(fd, &c)) {
        return FALSE;
    }

    method->mNumParamInitializer = c;

    if(!read_char_from_file(fd, &c)) {
        return FALSE;
    }
    method->mGenericsTypesNum = c;
    for(i=0; i<method->mGenericsTypesNum; i++) {
        if(!read_generics_param_types(fd, &method->mGenericsTypes[i])) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL read_virtual_method_map(int fd, sCLClass* klass)
{
    int n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    klass->mSizeVirtualMethodMap = n;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }
    klass->mNumVirtualMethodMap = n;

    klass->mVirtualMethodMap = CALLOC(1, sizeof(sVMethodMap)*klass->mSizeVirtualMethodMap);

    return read_from_file(fd, klass->mVirtualMethodMap, sizeof(sVMethodMap)*klass->mSizeVirtualMethodMap);
}

BOOL read_generics_param_types(int fd, sCLGenericsParamTypes* generics_param_types)
{
    int i;
    int n;
    char c;

    if(!read_int_from_file(fd, &n)) {
        return FALSE;
    }

    generics_param_types->mNameOffset = n;

    if(!read_type_from_file(fd, &generics_param_types->mExtendsType)) {
        return FALSE;
    }

    if(!read_char_from_file(fd, &c)) {
        return FALSE;
    }

    generics_param_types->mNumImplementsTypes = c;

    for(i=0; i<generics_param_types->mNumImplementsTypes; i++) {
        if(!read_type_from_file(fd, generics_param_types->mImplementsTypes + i)) {
            return FALSE;
        }
    }

    return TRUE;
}

static sCLClass* read_class_from_file(int fd)
{
    int i;
    sCLClass* klass;
    int const_pool_len;
    char* buf;
    int n;
    char c;
    long long n2;

    klass = CALLOC(1, sizeof(sCLClass));

    klass->mFieldsInitialized = FALSE;

    sConst_init(&klass->mConstPool);

    if(!read_long_long_from_file(fd, &n2)) {
        return NULL;
    }

    klass->mFlags = n2;

    /// load constant pool ///
    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }
    const_pool_len = n;

    buf = MALLOC(const_pool_len);

    if(!read_from_file(fd, buf, const_pool_len)) {
        FREE(buf);
        return NULL;
    }

    append_buf_to_constant_pool(&klass->mConstPool, buf, const_pool_len, FALSE);

    FREE(buf);

    /// load namespace offset ///
    if(!read_char_from_file(fd, &c)) {
        return NULL;
    }
    klass->mNameSpaceOffset = c;

    /// load class name offset ///
    if(!read_char_from_file(fd, &c)) {
        return NULL;
    }
    klass->mClassNameOffset = c;

    /// load real class name offset //
    if(!read_char_from_file(fd, &c)) {
        return NULL;
    }
    klass->mRealClassNameOffset = c;

    /// load fields ///
    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }
    klass->mSizeFields = klass->mNumFields = n;

    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }

    klass->mNumFieldsIncludingSuperClasses = n;

    klass->mFields = CALLOC(1, sizeof(sCLField)*klass->mSizeFields);

    for(i=0; i<klass->mNumFields; i++) {
        if(!read_field_from_file(fd, klass->mFields + i)) {
            return FALSE;
        }
    }

    /// load methods ///
    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }
    klass->mSizeMethods = klass->mNumMethods = n;

    klass->mMethods = CALLOC(1, sizeof(sCLMethod)*klass->mSizeMethods);

    for(i=0; i<klass->mNumMethods; i++) {
        if(!read_method_from_buffer(klass, klass->mMethods + i, fd)) {
            return NULL;
        }
    }

    /// load super classes ///
    if(!read_type_from_file(fd, &klass->mSuperClass)) {
        return NULL;
    }
    
    if(!read_char_from_file(fd, &c)) {
        return NULL;
    }
    klass->mNumSuperClasses = c;
    for(i=0; i<klass->mNumSuperClasses; i++) {
        if(!read_type_from_file(fd, klass->mSuperClasses + i)) {
            return NULL;
        }
    }

    /// load implement interfaces ///
    if(!read_char_from_file(fd, &c)) {
        return NULL;
    }
    klass->mNumImplementedInterfaces = c;
    for(i=0; i<klass->mNumImplementedInterfaces; i++) {
        if(!read_type_from_file(fd, &klass->mImplementedInterfaces[i])) {
            return NULL;
        }
    }

    /// load included module ///
    if(!read_char_from_file(fd, &c)) {
        return NULL;
    }
    klass->mNumIncludedModules = c;
    for(i=0; i<klass->mNumIncludedModules; i++) {
        if(!read_type_from_file(fd, &klass->mIncludedModules[i])) {
            return NULL;
        }
    }

    /// load class params of generics ///
    if(!read_char_from_file(fd, &c)) {
        return NULL;
    }
    klass->mGenericsTypesNum = c;
    for(i=0; i<klass->mGenericsTypesNum; i++) {
        if(!read_generics_param_types(fd, &klass->mGenericsTypes[i])) {
            return NULL;
        }
    }

    /// load dependences ///
    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }

    klass->mNumDependences = n;
    klass->mSizeDependences = klass->mNumDependences;
    klass->mDependencesOffset = CALLOC(1, sizeof(int)*klass->mNumDependences);
    for(i=0; i<klass->mNumDependences; i++) {
        if(!read_int_from_file(fd, &n)) {
            return NULL;
        }
        klass->mDependencesOffset[i] = n;
    }

    /// load virtual method table ///
    if(!read_virtual_method_map(fd, klass)) {
        return NULL;
    }

    /// load clone method index ///
    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }

    klass->mCloneMethodIndex = n;

    /// load initialize method index ///
    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }

    klass->mInitializeMethodIndex = n;

    /// load method missing method index ///
    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }

    klass->mMethodMissingMethodIndex = n;

    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }

    klass->mMethodMissingMethodIndexOfClassMethod = n;

    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }

    klass->mCompletionMethodIndex = n;

    if(!read_int_from_file(fd, &n)) {
        return NULL;
    }

    klass->mCompletionMethodIndexOfClassMethod = n;

    return klass;
}

static BOOL check_dependece_offsets(sCLClass* klass)
{
    int i;

    for(i=0; i<klass->mNumDependences; i++) {
        sCLClass* dependence_class;

        dependence_class = cl_get_class(CONS_str(&klass->mConstPool, klass->mDependencesOffset[i]));
        if(dependence_class == NULL) {
            if(load_class_from_classpath(CONS_str(&klass->mConstPool, klass->mDependencesOffset[i]), TRUE, -1) == NULL)
            {
                vm_error("can't load class %s\n", CONS_str(&klass->mConstPool, klass->mDependencesOffset[i]));
                return FALSE;
            }
        }
    }

    return TRUE;
}

// result: (NULL) --> file not found (sCLClass*) loaded class
static sCLClass* load_class(char* file_name, BOOL solve_dependences, int parametor_num)
{
    sCLClass* klass;
    sCLClass* klass2;
    int fd;
    char c;
    int size;

    /// check the existance of the load class ///
    fd = open(file_name, O_RDONLY);

    if(fd < 0) {
        return NULL;
    }

    /// check magic number. Is this Clover object file? ///
    if(!read_from_file(fd, &c, 1) || c != 12) { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 17) { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 33) { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 79) { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'C') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'L') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'O') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'V') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'E') { close(fd); return NULL; }
    if(!read_from_file(fd, &c, 1) || c != 'R') { close(fd); return NULL; }
    klass = read_class_from_file(fd);
    close(fd);

    if(klass == NULL) {
        return NULL;
    }

    //// add class to class table ///
    add_class_to_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass), klass, parametor_num);

    /// set hidden flags to native classes ///
    initialize_hidden_class_method_and_flags(klass);

    if(solve_dependences) {
        if(!check_dependece_offsets(klass)) {
            vm_error("can't load class %s because of dependences\n", REAL_CLASS_NAME(klass));
            remove_class_from_class_table(NAMESPACE_NAME(klass), CLASS_NAME(klass), parametor_num);
            free_class(klass);
            return NULL;
        }
    }

    return klass;
}

// result : (TRUE) found (FALSE) not found
// set class file path on class_file arguments
static BOOL search_for_class_file_from_class_name(char* class_file, unsigned int class_file_size, char* real_class_name, int mixin_version)
{
    int i;
    char* cwd;

    cwd = getenv("PWD");

    if(mixin_version == -1) {
        /// current working directory ///
        if(cwd) {
            for(i=CLASS_VERSION_MAX; i>=1; i--) {
                if(i == 1) {
                    snprintf(class_file, class_file_size, "%s/%s.clo", cwd, real_class_name);
                }
                else {
                    snprintf(class_file, class_file_size, "%s/%s#%d.clo", cwd, real_class_name, i);
                }

                if(access(class_file, F_OK) == 0) {
                    return TRUE;
                }
            }
        }

        /// default search path ///
        for(i=CLASS_VERSION_MAX; i>=1; i--) {
            if(i == 1) {
                snprintf(class_file, class_file_size, "%s/%s.clo", DATAROOTDIR, real_class_name);
            }
            else {
                snprintf(class_file, class_file_size, "%s/%s#%d.clo", DATAROOTDIR, real_class_name, i);
            }

            if(access(class_file, F_OK) == 0) {
                return TRUE;
            }
        }
    }
    else {
        int version;

        version = mixin_version;

        /// current working directory ///
        if(cwd) {
            if(version == 1) {
                snprintf(class_file, class_file_size, "%s/%s.clo", cwd, real_class_name);
            }
            else {
                snprintf(class_file, class_file_size, "%s/%s#%d.clo", cwd, real_class_name, version);
            }

            if(access(class_file, F_OK) == 0) {
                return TRUE;
            }
        }

        /// default search path ///
        if(version == 1) {
            snprintf(class_file, class_file_size, "%s/%s.clo", DATAROOTDIR, real_class_name);
        }
        else {
            snprintf(class_file, class_file_size, "%s/%s#%d.clo", DATAROOTDIR, real_class_name, version);
        }

        if(access(class_file, F_OK) == 0) {
            return TRUE;
        }
    }


    return FALSE;
}

static void get_namespace_and_class_name_from_real_class_name(char* namespace, char* class_name, int* parametor_num, char* real_class_name)
{
    char* p;
    char* p2;
    char* p3;
    BOOL flag;

    p2 = strstr(real_class_name, "::");

    if(p2) {
        memcpy(namespace, real_class_name, p2 - real_class_name);
        namespace[p2 -real_class_name] = 0;

        p3 = strstr(p2 + 2, "$");

        if(p3) {
            xstrncpy(class_name, p2 + 2, CL_CLASS_NAME_MAX);
            class_name[p3 - p2+2] = 0;

            *parametor_num = atoi(p3 + 1);
        }
        else {
            xstrncpy(class_name, p2 + 2, CL_CLASS_NAME_MAX);
            *parametor_num = 0;
        }
    }
    else {
        p3 = strstr(real_class_name, "$");

        if(p3) {
            *namespace = 0;
            xstrncpy(class_name, real_class_name, CL_CLASS_NAME_MAX);
            class_name[p3 - real_class_name] = 0;

            *parametor_num = atoi(p3 + 1);
        }
        else {
            *namespace = 0;
            xstrncpy(class_name, real_class_name, CL_CLASS_NAME_MAX);
            *parametor_num = 0;
        }
    }
}

// result: (NULL) --> file not found (sCLClass*) loaded class
sCLClass* load_class_from_classpath(char* real_class_name, BOOL solve_dependences, int mixin_version)
{
    char class_file_path[PATH_MAX];
    sCLClass* klass;
    char class_name[CL_CLASS_NAME_MAX + 1];
    char namespace[CL_NAMESPACE_NAME_MAX + 1];
    int parametor_num;

    if(!search_for_class_file_from_class_name(class_file_path, PATH_MAX, real_class_name, mixin_version)) {
        return NULL;
    }

    get_namespace_and_class_name_from_real_class_name(namespace, class_name, &parametor_num, real_class_name);

    /// if there is a class which has been entried to class table already, return the class
    klass = cl_get_class_with_namespace(namespace, class_name, parametor_num);

    if(klass) {
        return klass;
    }

    return load_class(class_file_path, solve_dependences, parametor_num);
}

void unload_class(char* namespace, char* class_name, int parametor_num)
{
    remove_class_from_class_table(namespace, class_name, parametor_num);
}

//////////////////////////////////////////////////
// show class
//////////////////////////////////////////////////
void show_class_list(sVMInfo* info)
{
    int i;

    cl_print(info, "-+- class list -+-\n");
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;

            klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass;
                
                next_klass = klass->mNextClass;
                cl_print(info, "%s\n", REAL_CLASS_NAME(klass));
                klass = next_klass;
            }
        }
    }
}

//////////////////////////////////////////////////
// accessor function
//////////////////////////////////////////////////
ALLOC char** get_class_names(int* num_class_names)
{
    int i;
    char** result;
    int result_size;
    int result_num;

    result_size = 128;
    result = CALLOC(1, sizeof(char*)*result_size);
    result_num = 0;

    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;
            
            klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass;
                
                next_klass = klass->mNextClass;
                *(result+result_num) = CONS_str(&klass->mConstPool, klass->mClassNameOffset);
                result_num++;

                if(result_num >= result_size) {
                    result_size *= 2;
                    result = REALLOC(result, sizeof(char*)*result_size);
                }
                klass = next_klass;
            }
        }
    }

    *num_class_names = result_num;

    *(result+result_num) = NULL;
    result_num++;

    if(result_num >= result_size) {
        result_size *= 2;
        result = REALLOC(result, sizeof(char*)*result_size);
    }

    return result;
}

// result: (NULL) not found (sCLClass*) found
sCLClass* get_super(sCLClass* klass)
{
    if(klass->mNumSuperClasses > 0) {
        char* real_class_name;
        
        real_class_name = CONS_str(&klass->mConstPool, klass->mSuperClasses[klass->mNumSuperClasses-1].mClassNameOffset);
        return cl_get_class(real_class_name);
    }
    else {
        return NULL;
    }
}

//////////////////////////////////////////////////
// initialization and finalization
//////////////////////////////////////////////////
CLObject gTypeObject = 0;
CLObject gIntTypeObject = 0;
CLObject gAnonymousTypeObject = 0;
CLObject gByteTypeObject = 0;
CLObject gShortTypeObject = 0;
CLObject gUIntTypeObject = 0;
CLObject gLongTypeObject = 0;
CLObject gCharTypeObject = 0;
CLObject gStringTypeObject = 0;
CLObject gArrayTypeObject = 0;
CLObject gHashTypeObject = 0;
CLObject gRangeTypeObject = 0;
CLObject gFloatTypeObject = 0;
CLObject gDoubleTypeObject = 0;
CLObject gBoolTypeObject = 0;
CLObject gPointerTypeObject = 0;
CLObject gBytesTypeObject = 0;
CLObject gBlockTypeObject = 0;
CLObject gNullTypeObject = 0;
CLObject gOnigurumaRegexTypeObject = 0;

void class_init()
{
    sNativeMethod* p;
    sNativeClass* p2;

    memset(gNativeMethodHash, 0, sizeof(gNativeMethodHash));
    memset(gNativeClassHash, 0, sizeof(gNativeClassHash));

    p = gNativeMethods;

    while(p->mPath[0] != 0) {
        put_fun_to_hash_for_native_method((char*)p->mPath, p->mFun);
        p++;
    }

    p2 = gNativeClasses;
    while(p2->mClassName[0] != 0) {
        put_fun_to_hash_for_native_class((char*)p2->mClassName, p2->mFun);
        p2++;
    }
}

void cl_unload_all_classes()
{
    int i;
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;
            
            klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass;
                
                next_klass = klass->mNextClass;
                free_class(klass);
                klass = next_klass;
            }

            gClassHashList[i] = NULL;
        }
    }
}

void class_final()
{
    int i;

    cl_unload_all_classes();
    for(i=0; i<NATIVE_CLASS_HASH_SIZE; i++) {
        if(gNativeClassHash[i].mClassName) {
            FREE(gNativeClassHash[i].mClassName);
        }
    }
    for(i=0; i<NATIVE_METHOD_HASH_SIZE; i++) {
        if(gNativeMethodHash[i].mPath) {
            FREE(gNativeMethodHash[i].mPath);
        }
    }
}

BOOL call_initialize_method(sCLClass* klass)
{
    sCLMethod* method;
    CLObject result_value;
    int i;
    BOOL result;

    if(klass->mInitializeMethodIndex == -1) {
        return TRUE;
    }

    method = klass->mMethods + klass->mInitializeMethodIndex;

    result = cl_excute_method(method, klass, klass, NULL, &result_value);

    if(!result) {
        if(result_value && check_type_without_info(result_value, "Exception"))
        {
            fprintf(stderr, "class %s method %s:\n", REAL_CLASS_NAME(klass), METHOD_NAME2(klass, method));
            output_exception_message(result_value);
        }

        return FALSE;
    }
    if(result_value && check_type_without_info(result_value, "bool") && !CLBOOL(result_value)->mValue) 
    {
        fprintf(stderr, "The result of initialize method is false\n");
        return FALSE;
    }

    return result;
}

BOOL run_all_loaded_class_fields_initializer() 
{
    int i;
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;
            
            klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass;
                
                next_klass = klass->mNextClass;
                if(!klass->mFieldsInitialized) {
                    if(!run_class_fields_initializer(klass)) {
                        return FALSE;
                    }
                }
                klass = next_klass;
            }
        }
    }

    return TRUE;
}

BOOL run_all_loaded_class_initialize_method() 
{
    int i;
    for(i=0; i<CLASS_HASH_SIZE; i++) {
        if(gClassHashList[i]) {
            sCLClass* klass;
            
            klass = gClassHashList[i];
            while(klass) {
                sCLClass* next_klass;
                
                next_klass = klass->mNextClass;
                if(!call_initialize_method(klass)) {
                    return FALSE;
                }
                klass = next_klass;
            }
        }
    }

    return TRUE;
}

BOOL cl_load_fundamental_classes()
{
    int i;

    for(i=0; i<CL_GENERICS_CLASS_PARAM_MAX; i++) {
        char real_class_name[CL_REAL_CLASS_NAME_MAX + 1];

        snprintf(real_class_name, CL_REAL_CLASS_NAME_MAX, "GenericsParam%d", i);

        load_class_from_classpath(real_class_name, TRUE, -1);
    }

    load_class_from_classpath("Object", TRUE, -1);
    load_class_from_classpath("long", TRUE, -1);
    load_class_from_classpath("short", TRUE, -1);
    load_class_from_classpath("int", TRUE, -1);
    load_class_from_classpath("byte", TRUE, -1);
    load_class_from_classpath("uint", TRUE, -1);
    load_class_from_classpath("char", TRUE, -1);
    load_class_from_classpath("float", TRUE, -1);
    load_class_from_classpath("double", TRUE, -1);
    load_class_from_classpath("bool", TRUE, -1);
    load_class_from_classpath("pointer", TRUE, -1);
    load_class_from_classpath("Encoding", TRUE, -1);
    load_class_from_classpath("Regex", TRUE, -1);
    load_class_from_classpath("OnigurumaRegex", TRUE, -1);
    load_class_from_classpath("String", TRUE, -1);
    load_class_from_classpath("Bytes", TRUE, -1);
    load_class_from_classpath("Range", TRUE, -1);
    load_class_from_classpath("Array$1", TRUE, -1);
    load_class_from_classpath("Hash$2", TRUE, -1);
    load_class_from_classpath("Tuple$1", TRUE, -1);
    load_class_from_classpath("Tuple$2", TRUE, -1);
    load_class_from_classpath("Tuple$3", TRUE, -1);
    load_class_from_classpath("Tuple$4", TRUE, -1);
    load_class_from_classpath("Tuple$5", TRUE, -1);
    load_class_from_classpath("Tuple$6", TRUE, -1);
    load_class_from_classpath("Tuple$7", TRUE, -1);
    load_class_from_classpath("Tuple$8", TRUE, -1);
    load_class_from_classpath("Field", TRUE, -1);
    load_class_from_classpath("Method", TRUE, -1);
    load_class_from_classpath("Block", TRUE, -1);
    load_class_from_classpath("GenericsParametor", TRUE, -1);
    load_class_from_classpath("Class", TRUE, -1);
    load_class_from_classpath("Type", TRUE, -1);
    load_class_from_classpath("Enum", TRUE, -1);
    load_class_from_classpath("Null", TRUE, -1);
    load_class_from_classpath("anonymous", TRUE, -1);
    load_class_from_classpath("void", TRUE, -1);
    load_class_from_classpath("Thread", TRUE, -1);
    load_class_from_classpath("Mutex", TRUE, -1);
    load_class_from_classpath("Clover", TRUE, -1);
    load_class_from_classpath("System", TRUE, -1);

    gExceptionClass = load_class_from_classpath("Exception", TRUE, -1);
    load_class_from_classpath("SystemException", TRUE, -1);
    load_class_from_classpath("NullPointerException", TRUE, -1);
    load_class_from_classpath("RangeException", TRUE, -1);
    load_class_from_classpath("ConvertingStringCodeException", TRUE, -1);
    load_class_from_classpath("ClassNotFoundException", TRUE, -1);
    load_class_from_classpath("IOException", TRUE, -1);
    load_class_from_classpath("OverflowException", TRUE, -1);
    load_class_from_classpath("CantSolveGenericsType", TRUE, -1);
    load_class_from_classpath("TypeError", TRUE, -1);
    load_class_from_classpath("MethodMissingException", TRUE, -1);
    load_class_from_classpath("DivisionByZeroException", TRUE, -1);
    load_class_from_classpath("OverflowStackSizeException", TRUE, -1);
    load_class_from_classpath("InvalidRegexException", TRUE, -1);
    load_class_from_classpath("KeyNotFoundException", TRUE, -1);
    load_class_from_classpath("KeyOverlappingException", TRUE, -1);
    load_class_from_classpath("OutOfRangeOfStackException", TRUE, -1);
    load_class_from_classpath("OutOfRangeOfFieldException", TRUE, -1);

    load_class_from_classpath("NativeClass", TRUE, -1);

    if(!run_all_loaded_class_fields_initializer()) {
        return FALSE;
    }
    if(!run_all_loaded_class_initialize_method()) {
        return FALSE;
    }

    return TRUE;
}
