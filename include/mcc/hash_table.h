#ifndef MCC_HASH_TABLE_H
#define MCC_HASH_TABLE_H

#include "str.h"

typedef struct HashMap HashMap;

struct HashMap {
  struct HashNode* root;
};

typedef struct HashNode HashNode;

void* hashmap_lookup(const HashMap* table, StringView key);

void hashmap_insert(HashMap* table, StringView key, void* value_ptr,
                    Arena* arena);

#endif // MCC_HASH_TABLE_H
