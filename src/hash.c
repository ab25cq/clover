#include "hash.h"
#include <stdio.h>
#include <string.h>

hash_it* hash_it_new(char* key, void* item, hash_it* coll_it, hash_it* next_it, hash_obj* hash) 
{
   hash_it* it = (hash_it*)MALLOC(sizeof(hash_it));

   it->mKey = STRDUP(key);

   it->mItem = item;
   it->mCollisionIt = coll_it;

   it->mNextIt = next_it;   

   return it;
}
   
void hash_it_release(hash_it* it, hash_obj* hash)
{
    FREE(it->mKey);
      
    FREE(it);
}
         
#ifndef MDEBUG
hash_obj* hash_new(int size)
{
   hash_obj* self = MALLOC(sizeof(hash_obj));

   self->mTableSize = size;
   self->mTable = (hash_it**)MALLOC(sizeof(hash_it*) * size);
   memset(self->mTable, 0, sizeof(hash_it*)*size);

   self->mEntryIt = NULL;

   self->mCounter = 0;

   return self;
}
#else
hash_obj* hash_new_debug(int size, const char* fname, int line, const char* func_name)
{
   hash_obj* self = debug_malloc(sizeof(hash_obj), fname, line, func_name);

   self->mTableSize = size;
   self->mTable = (hash_it**)MALLOC(sizeof(hash_it*) * size);
   memset(self->mTable, 0, sizeof(hash_it*)*size);

   self->mEntryIt = NULL;

   self->mCounter = 0;

   return self;
}
#endif

void hash_delete(hash_obj* self)
{
   hash_it* it = self->mEntryIt;

   while(it) {
      hash_it* next_it = it->mNextIt;
      hash_it_release(it, self);
      it = next_it;
   }
   
   FREE(self->mTable);
   
   FREE(self);
}

static unsigned int get_hash_value(hash_obj* self, char* key)
{
   unsigned int i = 0;
   while(*key) {
      i += *key;
      key++;
   }

   return i % self->mTableSize;
}

static void resize(hash_obj* self) 
{
    const int table_size = self->mTableSize;

    self->mTableSize *= 5;
    FREE(self->mTable);
    self->mTable = (hash_it**)MALLOC(sizeof(hash_it*) * self->mTableSize);
    memset(self->mTable, 0, sizeof(hash_it*)*self->mTableSize);

    hash_it* it = self->mEntryIt;
    while(it) {
        unsigned int hash_key = get_hash_value(self, it->mKey);

        it->mCollisionIt = self->mTable[hash_key];
        
        self->mTable[hash_key] = it;

        it = it->mNextIt;
    }
}

void hash_put(hash_obj* self, char* key, void* item)
{
    if(self->mCounter >= self->mTableSize) {
        resize(self);
    }
    
    unsigned int hash_key = get_hash_value(self, key);
    
    hash_it* it = self->mTable[ hash_key ];
    while(it) {
        if(strcmp(key, it->mKey) == 0) {
            it->mItem = item;
            return;
        }
        it = it->mCollisionIt;
    }
    
    hash_it* new_it
        = hash_it_new(key, item, self->mTable[hash_key], self->mEntryIt, self);
    
    self->mTable[hash_key] = new_it;
    self->mEntryIt = new_it;
    self->mCounter++;
}

static void erase_from_list(hash_obj* self, hash_it* rit)
{
   if(rit == self->mEntryIt) {
      self->mEntryIt = rit->mNextIt;
   }
   else {
      hash_it* it = self->mEntryIt;

      while(it->mNextIt) {
         if(it->mNextIt == rit) {
            it->mNextIt = it->mNextIt->mNextIt;
            break;
         }
            
         it = it->mNextIt;
      }
   }
}

BOOL hash_erase(hash_obj* self, char* key)
{
   const unsigned int hash_value = get_hash_value(self, key);
   hash_it* it = self->mTable[hash_value];
   
   if(it == NULL)
      ;
   else if(strcmp(it->mKey, key) == 0) {
      hash_it* it2 = self->mEntryIt;   
      self->mTable[ hash_value ] = it->mCollisionIt;

      erase_from_list(self, it);
      
      hash_it_release(it, self);

      self->mCounter--;

      return TRUE;
   }
   else {
      hash_it* prev_it = it;
      it = it->mCollisionIt;
      while(it) {
         if(strcmp(it->mKey, key) == 0) {
            prev_it->mCollisionIt = it->mCollisionIt;
            erase_from_list(self, it);
            hash_it_release(it, self);
            self->mCounter--;
            
            return TRUE;
         }
         
         prev_it = it;
         it = it->mCollisionIt;
      }
   }

   return FALSE;
}

void hash_clear(hash_obj* self)
{
   int i;
   int max;
   hash_it* it = self->mEntryIt;

   while(it) {
      hash_it* next_it = it->mNextIt;
      hash_it_release(it, self);
      it = next_it;
   }

   memset(self->mTable, 0, sizeof(hash_it*)*self->mTableSize);
   
   self->mEntryIt = NULL;
   self->mCounter = 0;
}

void hash_replace(hash_obj* self, char* key, void* item)
{
   hash_it* it = self->mTable[get_hash_value(self, key)];

   if(it) {
      do {
         if(strcmp(it->mKey, key) == 0) {
            it->mItem = item;
            break;
         }
   
         it = it->mCollisionIt;
      } while(it);
   }
}

void hash_show(hash_obj* self, char* fname)
{
   char tmp[8096];
   int i;
   int max;
   hash_it* it;
   xstrncpy(tmp, "", 8096);
   
   max = self->mTableSize;
   for(i=0; i<max; i++) {
      snprintf(tmp + strlen(tmp), 8096-strlen(tmp), "table[%d]: ", i);
      
      it = self->mTable[i];
      while(it) {
         snprintf(tmp + strlen(tmp), 8096-strlen(tmp), "item[\"%s\"], ", it->mKey);
         it = it->mCollisionIt;
      }
    
    xstrncat(tmp, "\n", 8096);
   }

    
    FILE* f = fopen(fname, "a");
    fprintf(f, "-------------------------------------\n");
    fprintf(f, "%s", tmp);
    fprintf(f, "-------------------------------------\n");
    it = self->mEntryIt;
    while(it) {
        fprintf(f, "%s\n", it->mKey);
        it = it->mNextIt;
    }
    fprintf(f, "-------------------------------------\n");
    fclose(f);
}

void* hash_item(hash_obj* self, char* key)
{
   hash_it* it = self->mTable[ get_hash_value(self, key) ];

   while(it) {
      if(strcmp(key, it->mKey) == 0) return it->mItem;
      it = it->mCollisionIt;
   }

   return NULL;
}

void* hash_item_addr(hash_obj* self, char* key)
{
   hash_it* it = self->mTable[ get_hash_value(self, key) ];
   while(it) {
      if(strcmp(key, it->mKey) == 0) return &it->mItem;
      it = it->mCollisionIt;
   }

   return NULL;
}
   
char* hash_key(hash_obj* self, void* item)
{
   hash_it* it = self->mEntryIt;

   while(it) {
      if(it->mItem == item) return it->mKey;
      it = it->mNextIt;
   }

   return NULL;
}

int hash_count(hash_obj* self)
{
   return self->mCounter;
}   

hash_it* hash_loop_begin(hash_obj* self) 
{ 
    return self->mEntryIt; 
}

void* hash_loop_item(hash_it* it) 
{ 
    return it->mItem; 
}

char* hash_loop_key(hash_it* it)
{ 
    return it->mKey; 
}

hash_it* hash_loop_next(hash_it* it) 
{ 
    return it->mNextIt; 
}

