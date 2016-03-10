#ifndef DEBUG_H
#define DEBUG_H

#include <stdlib.h>
#include <assert.h>

#ifndef BOOL 
#define BOOL int
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef ALLOC
#define ALLOC                   // indicated this memory should be freed after used
#endif

#ifndef MANAGED
#define MANAGED                 // indicated this memory is managed inside the function or object
#endif

#ifndef NULLABLE
#define NULLABLE
#endif

#if !defined(MDEBUG)

#   define CHECKML_BEGIN
#   define CHECKML_END

ALLOC void* xmalloc(size_t size);
ALLOC char* xstrdup(char* str);
ALLOC void* xrealloc(void* ptr, size_t size);
ALLOC void* xcalloc(size_t count, size_t size);

#   define MMALLOC(o) xmalloc(o)
#   define MSTRDUP(o) xstrdup(o)
#   define MREALLOC(o, o2) xrealloc(o, o2)
#   define MCALLOC(o, o2) xcalloc(o, o2)
#   define MFREE(o) free(o)

#   define MASSERT(o)

#else 

void debug_init();
void debug_final();

ALLOC void* debug_malloc(size_t size, const char* file_name, int line, const char* func_name);
ALLOC void* debug_calloc(size_t count, size_t size, const char* file_name, int line, const char* func_name);
ALLOC char* debug_strdup(char* str, const char* file_name, int line, const char* func_name);
ALLOC void* debug_realloc(void* ptr, size_t size, const char* file_name, int line, const char* func_name);
void debug_free(MANAGED void* memory, const char* file_name, int line, const char* func_name);
void debug_assert(int value, const char* file_name, int line, const char* func_name);

#    define CHECKML_BEGIN debug_init();
#    define CHECKML_END debug_final();

#   define MMALLOC(o) debug_malloc(o, __FILE__, __LINE__, __FUNCTION__)
#   define MCALLOC(o, o2) debug_calloc(o, o2, __FILE__, __LINE__, __FUNCTION__)
#   define MSTRDUP(o) debug_strdup(o, __FILE__, __LINE__, __FUNCTION__)
#   define MREALLOC(o, o2) debug_realloc(o, o2, __FILE__, __LINE__, __FUNCTION__)
#   define MFREE(o) debug_free(o, __FILE__, __LINE__, __FUNCTION__)

#   define MASSERT(o) debug_assert((int)(o), __FILE__, __LINE__, __FUNCTION__)

#endif

#endif
