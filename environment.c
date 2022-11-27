#include "shirp.h"

#define INITIAL_BUCKET_SIZE 16
#define TOMBSTONE ((void *)-1)
#define FNV_PRIME 16777619
#define FNV_OFFSET_BASIS 2166136261UL
#define HIGH_WATERMARK 70

static uint32_t fnv_hash(char *str, size_t len) {
  uint32_t hash = FNV_OFFSET_BASIS;
  for (size_t i = 0; i < len; i++) {
    hash ^= (unsigned char)str[i];
    hash *= FNV_PRIME;
  }
  return hash;
}

HashTable *new_hash_table() {
  HashTable *ht = (HashTable *)shirp_calloc(1, sizeof(HashTable));
  ht->capacity = INITIAL_BUCKET_SIZE;
  ht->buckets = (Entry **)shirp_calloc(ht->capacity, sizeof(Entry *));
  return ht;
}

void hashtable_resize(HashTable *ht, size_t new_capacity) {
  Entry **new_buckets = (Entry **)shirp_calloc(new_capacity, sizeof(Entry *));
  for (size_t i = 0; i < ht->capacity; i++) {
    Entry *entry = ht->buckets[i];
    if (entry == NULL || entry == TOMBSTONE) {
      continue;
    }
    uint32_t hash = fnv_hash(entry->key, entry->keylen);
    size_t index = hash % new_capacity;
    while (new_buckets[index] != NULL) {
      index = (index + 1) % new_capacity;
    }
    new_buckets[index] = entry;
  }
  free(ht->buckets);
  ht->buckets = new_buckets;
  ht->capacity = new_capacity;
}

void hashtable_insert(HashTable *ht, char *key, size_t keylen, void *val) {
  if (ht->used * 100 / ht->capacity >= HIGH_WATERMARK) {
    hashtable_resize(ht, ht->capacity * 2);
  }
  uint32_t hash = fnv_hash(key, keylen);
  size_t index = hash % ht->capacity;
  Entry *entry = (Entry *)shirp_malloc(sizeof(Entry));
  entry->key = key;
  entry->keylen = keylen;
  entry->val = val;
  while (ht->buckets[index] != NULL) {
    if (ht->buckets[index] == TOMBSTONE) {
      ht->buckets[index] = entry;
      ht->used++;
      return;
    } else if (ht->buckets[index]->keylen == keylen &&
               match_str(ht->buckets[index]->key, key, keylen)) {
      ht->buckets[index]->val = val;
      return;
    }
    index = (index + 1) % ht->capacity;
  }
  ht->buckets[index] = entry;
  ht->used++;
}

static size_t get_placeholder_index(HashTable *ht, char *key, size_t keylen) {
  uint32_t hash = fnv_hash(key, keylen);
  size_t index = hash % ht->capacity;
  while (ht->buckets[index] != NULL) {
    if (ht->buckets[index] != TOMBSTONE &&
        ht->buckets[index]->keylen == keylen &&
        match_str(ht->buckets[index]->key, key, keylen)) {
      return index;
    }
    index = (index + 1) % ht->capacity;
  }
  return (size_t)-1;
}

void hashtable_delete(HashTable *ht, char *key, size_t keylen) {
  size_t index = get_placeholder_index(ht, key, keylen);
  ht->buckets[index] = TOMBSTONE;
}

Entry *hashtable_get(HashTable *ht, char *key, size_t keylen) {
  size_t index = get_placeholder_index(ht, key, keylen);
  if (index == (size_t)-1)
    return NULL;
  return ht->buckets[index];
}

void free_table(HashTable *ht) {
  for (size_t i = 0; i < ht->capacity; i++) {
    if (ht->buckets[i] != NULL && ht->buckets[i] != TOMBSTONE) {
      free(ht->buckets[i]);
    }
  }
  free(ht->buckets);
  free(ht);
}

Frame *push_new_frame(Frame *outer) {
  Frame *frame = (Frame *)shirp_malloc(sizeof(Frame));
  frame->outer = outer;
  frame->table = new_hash_table();
  frame->is_held = false;
  return frame;
}

Frame *pop_frame(Frame *frame) {
  Frame *outer = frame->outer;
  if (frame->is_held) {
    outer->is_held = true;
  } else {
    free_table(frame->table);
    free(frame);
  }
  return outer;
}

void frame_insert_obj(Frame *frame, char *key, size_t keylen, void *val) {
  hashtable_insert(frame->table, key, keylen, val);
  debug_log("`%.*s` has been registerred to table %p", keylen, key,
            frame->table);
}

void *frame_get_obj(Frame *frame, char *key, size_t keylen) {
  debug_log("Frame searching for object: `%.*s`", keylen, key);
  while (frame) {
    Entry *entry = hashtable_get(frame->table, key, keylen);
    if (entry) {
      debug_log("Key found! %.*s", keylen, key);
      return entry->val;
    }
    frame = frame->outer;
  }
  debug_log("Key not found in Frames: %.*s", keylen, key);
  return NULL;
}

/* test
int main() {
  HashTable *ht = new_hash_table();
  hashtable_insert(ht, "foo", 3, (void *)2);
  hashtable_insert(ht, "hogehoge", 8, (void *)8);
  hashtable_delete(ht, "asdf", 4);
  hashtable_delete(ht, "sadhjfgas", 9);
  hashtable_delete(ht, "sadfasdf", 8);
  hashtable_delete(ht, "sadfqsdf", 8);
  hashtable_delete(ht, "ow", 2);
  hashtable_delete(ht, "a", 1);
  hashtable_insert(ht, "ow", 2, (void *)15);
  hashtable_insert(ht, "a", 1, (void *)18);
  hashtable_delete(ht, "a", 1);
  printf("%p\n", hashtable_get(ht, "foo", 3)->val);
  printf("%p\n", hashtable_get(ht, "a", 1));
  printf("%p\n", hashtable_get(ht, "hogehoge", 8)->val);
  printf("%p\n", hashtable_get(ht, "ow", 2)->val);
  printf("%p\n", hashtable_get(ht, "aw", 2));
  hashtable_delete(ht, "aw", 2);
  printf("%p\n", hashtable_get(ht, "aw", 2));
  return 0;
}
*/