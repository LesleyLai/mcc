#include <mcc/hash_table.h>

// Hash table implementation adapted from
// https://nullprogram.com/blog/2025/01/19/

uint64_t hash64(StringView s)
{
  uint64_t h = 0x100;
  for (size_t i = 0; i < s.size; i++) {
    h ^= s.start[i] & 255;
    h *= 1111111111111111111;
  }
  return h;
}

typedef struct HashNode HashNode;

struct HashNode {
  HashNode* child[4];
  StringView key;
  void* value_ptr;
};

// Returns the Node that contains the value pointer, or nullptr otherwise
static HashNode** hashmap_lookup_node(HashMap* map, StringView key)
{
  HashNode** node = &map->root;

  for (uint64_t h = hash64(key); *node != nullptr; h <<= 2) {
    if (str_eq(key, (*node)->key)) { return node; }
    node = &(*node)->child[h >> 62];
  }
  return node;
}

void* hashmap_lookup(const HashMap* map, StringView key)
{
  HashNode** node_ptr = hashmap_lookup_node((HashMap*)map, key);
  if (*node_ptr == nullptr) { return nullptr; }
  return (*node_ptr)->value_ptr;
}

bool hashmap_try_insert(HashMap* map, StringView key, void* value_ptr,
                        Arena* arena)
{
  HashNode** node_ptr = hashmap_lookup_node(map, key);
  if (*node_ptr != nullptr) { return false; }

  *node_ptr = ARENA_ALLOC_OBJECT(arena, HashNode);
  **node_ptr = (HashNode){.key = key, .value_ptr = value_ptr};

  return true;
}
