#ifndef CLOVER_HASH_H
#define CLOVER_HASH_H 1

#include "debug.h"

typedef struct _hash_it {
   char* mKey;
   
   void* mItem;
   struct _hash_it* mCollisionIt;

   struct _hash_it* mNextIt;
} hash_it;

typedef struct {
   hash_it** mTable;
   int mTableSize;

   hash_it* mEntryIt;

   int mCounter;
} hash_obj;

#ifndef MDEBUG
hash_obj* hash_new(int size);
#define HASH_NEW(size) hash_new(size)
#else
hash_obj* hash_new_debug(int size, const char* fname, int line, const char* func_name);
#define HASH_NEW(size) hash_new_debug(size, __FILE__, __LINE__, __FUNCTION__)
#endif

void hash_delete(hash_obj* self);

void hash_put(hash_obj* self, char* key, void* item);
BOOL hash_erase(hash_obj* self, char* key);
void hash_clear(hash_obj* self);
void hash_replace(hash_obj* self, char* key, void* item);

void hash_show(hash_obj* self, char* fname);

void* hash_item(hash_obj* self, char* key);
void* hash_item_addr(hash_obj* self, char* key);
char* hash_key(hash_obj* self, void* item);
int hash_count(hash_obj* self);

hash_it* hash_loop_begin(hash_obj* self);
void* hash_loop_item(hash_it* it);
char* hash_loop_key(hash_it* it);
hash_it* hash_loop_next(hash_it* it);
unsigned int hash_size(hash_obj* self);

/*
 * access all items
 *
 * hash_it* i = hash_loop_begin(hash);
 * while(it) {
 *     void* obj = hash_loop_item(it);
 *
 *     // using obj
 *
 *     it = hash_loop_next(it);
 * }
 *
 */

#endif
