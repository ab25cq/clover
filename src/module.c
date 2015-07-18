#include "clover.h"
#include "common.h"
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>

static BOOL load_module_from_file(ALLOC sCLModule** self, char* real_module_name);

sCLModule* gModules[CL_MODULE_HASH_SIZE];

void module_init()
{
    memset(gModules, 0, sizeof(gModules));
}

static BOOL append_module_to_table(char* name, sCLModule* module)
{
    int hash_value;
    sCLModule** p;

    hash_value = get_hash(name) % CL_MODULE_HASH_SIZE;

    p = gModules + hash_value;

    while(1) {
        if(*p == NULL) {
            *p = module;
            break;
        }
        else {
            if(strcmp((*p)->mName, name) == 0) {
                return FALSE;
            }
            else {
                p++;

                if(p == gModules + CL_MODULE_HASH_SIZE) {
                    p = gModules;
                }
                else if(p == gModules + hash_value) {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

static void create_real_module_name(char* result, int result_size, char* namespace, char* module_name)
{
    if(namespace[0] == 0) {
        xstrncpy(result, module_name, result_size);
    }
    else {
        xstrncpy(result, namespace, result_size);
        xstrncat(result, "::", result_size);
        xstrncat(result, module_name, result_size);
    }
}

// result: (NULL) overflow module table (sCLModule*) success
sCLModule* create_module_from_real_module_name(char* real_module_name)
{
    sCLModule* self;

    self = CALLOC(1, sizeof(sCLModule));

    sBuf_init(&self->mBody);

    xstrncpy(self->mName, real_module_name, CL_MODULE_NAME_MAX);

    if(!append_module_to_table(real_module_name, self)) {
        return NULL;
    }

    return self;
}

// result: (NULL) overflow module table (sCLModule*) success
sCLModule* create_module(char* namespace, char* name)
{
    char real_module_name[CL_MODULE_NAME_MAX+1];

    create_real_module_name(real_module_name, CL_MODULE_NAME_MAX, namespace, name);

    return create_module_from_real_module_name(real_module_name);
}

static void free_module(sCLModule* self)
{
    FREE(self->mBody.mBuf);

    FREE(self);
}

void module_final()
{
    sCLModule** p;

    p = gModules;

    while(p < gModules + CL_MODULE_HASH_SIZE) {
        if(*p != NULL) {
            free_module(*p);
        }

        p++;
    }
}

void append_character_to_module(sCLModule* self, char c)
{
    sBuf_append_char(&self->mBody, c);
}

void append_str_to_module(sCLModule* self, char* str)
{
    sBuf_append(&self->mBody, str, strlen(str));
}

sCLModule* get_module_from_real_module_name(char* real_module_name)
{
    int hash_value;
    sCLModule** p;

    hash_value = get_hash(real_module_name) % CL_MODULE_HASH_SIZE;

    p = gModules + hash_value;

    while(1) {
        if(*p == NULL) {
            sCLModule* module;

            if(!load_module_from_file(&module, real_module_name)) {
                break;
            }

            return module;
        }
        else {
            if(strcmp((*p)->mName, real_module_name) == 0) {
                return *p;
            }
            else {
                p++;

                if(p == gModules + CL_MODULE_HASH_SIZE) {
                    p = gModules;
                }
                else if(p == gModules + hash_value) {
                    break;
                }
            }
        }
    }

    return NULL;
}

sCLModule* get_module(char* namespace, char* name)
{
    char real_module_name[CL_MODULE_NAME_MAX+1];

    create_real_module_name(real_module_name, CL_MODULE_NAME_MAX, namespace, name);

    return get_module_from_real_module_name(real_module_name);
}

char* get_module_body(sCLModule* module)
{
    if(module) {
        return module->mBody.mBuf;
    }
    else {
        return NULL;
    }
}

static BOOL save_module_to_file(sCLModule* self)
{
    FILE* f;
    char fname[PATH_MAX];

    snprintf(fname, PATH_MAX, "%s.clm", self->mName);

    f = fopen(fname, "w+");

    if(f == NULL) { 
        return FALSE; 
    }

    fprintf(f, "%s", self->mBody.mBuf);

    fclose(f);

    return TRUE;
}

// result : (TRUE) found (FALSE) not found
static BOOL search_for_module_file_from_module_name(char* module_file, unsigned int module_file_size, char* real_module_name)
{
    int i;
    char* cwd;

    cwd = getenv("PWD");
    if(cwd == NULL) {
        fprintf(stderr, "PWD environment path is NULL\n");
        return FALSE;
    }

    /// default search path ///
    snprintf(module_file, module_file_size, "%s/%s.clm", DATAROOTDIR, real_module_name);

    if(access(module_file, F_OK) == 0) {
        return TRUE;
    }

    /// current working directory ///
    snprintf(module_file, module_file_size, "%s/%s.clm", cwd, real_module_name);

    if(access(module_file, F_OK) == 0) {
        return TRUE;
    }

    return FALSE;
}

static BOOL load_module_from_file(ALLOC sCLModule** self, char* real_module_name)
{
    char buf[BUFSIZ+1];
    int fd;
    char fname[PATH_MAX];

    if(!search_for_module_file_from_module_name(fname, PATH_MAX, real_module_name))
    {
        return FALSE;
    }

    /// load from file ///
    *self = create_module_from_real_module_name(real_module_name);

    fd = open(fname, O_RDONLY);

    if(fd < 0) {
        return FALSE;
    }

    while(1) {
        int size;

        size = read(fd, buf, BUFSIZ);
        if(size < 0) {
            close(fd);
            return FALSE;
        }

        buf[size] = 0;

        if(size < BUFSIZ) {
            append_str_to_module(*self, buf);
            break;
        }
        else {
            append_str_to_module(*self, buf);
        }
    }

    close(fd);

    return TRUE;
}

// result (TRUE): success (FALSE): failed to write module to the file
void save_all_modified_modules()
{
    sCLModule** p;

    p = gModules;

    while(p < gModules + CL_MODULE_HASH_SIZE) {
        if(*p != NULL) {
            if((*p)->mModified) {
                if(!save_module_to_file((*p))) {
                    printf("failed to write this module(%s)\n", (*p)->mName);
                }
            }
        }

        p++;
    }
}

void this_module_is_modified(sCLModule* self)
{
    self->mModified = TRUE;
}

void create_cl_type_from_module(sCLType* cl_type, sCLModule* module, sCLClass* klass)
{
    cl_type->mClassNameOffset = append_str_to_constant_pool(&klass->mConstPool, module->mName, FALSE);
    cl_type->mStar = FALSE;
    cl_type->mGenericsTypesNum = 0;
}

sCLModule* get_module_from_cl_type(sCLClass* klass, sCLType* cl_type)
{
    char* module_name;

    module_name = CONS_str(&klass->mConstPool, cl_type->mClassNameOffset);

    return get_module_from_real_module_name(module_name);
}
