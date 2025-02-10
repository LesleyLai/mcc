#ifndef MCC_HASH_TABLE_H
#define MCC_HASH_TABLE_H

#include "str.h"

typedef struct HashMap HashMap;

// maps StringView keys to void* values.
struct HashMap {
  struct HashNode* root;
};

// Returns a pointer to value if the key if found, or nullptr otherwise
void* hashmap_lookup(const HashMap* hashMap, StringView key);

/*
 * Attempts to insert a key-value pair into the hash map.
 *
 * If the key already exists, the function does nothing and returns false.
 * Otherwise, the key-value pair is inserted, and the function returns true.
 */
bool hashmap_try_insert(HashMap* map, StringView key, void* value_ptr,
                        Arena* arena);

#endif // MCC_HASH_TABLE_H
