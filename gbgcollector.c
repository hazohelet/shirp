#include "shirp.h"

GC *gc;
extern Frame *env;
extern Obj *builtin;

#define INITIAL_TABLE_SIZE 32
#define MARK ((void *)-2)

void GC_init() {
  gc = (GC *)shirp_calloc(1, sizeof(GC));
  gc->marked_table = new_hash_table(INITIAL_TABLE_SIZE);
}

void GC_register(Obj *obj) {
  WorkList *wl = (WorkList *)shirp_calloc(1, sizeof(WorkList));
  wl->obj = obj;
  if (gc->head)
    gc->tail = gc->tail->next = wl;
  else
    gc->head = gc->tail = wl;
  ++gc->size;
}

void hashtable_insert_ptr(HashTable *ht, void *ptr, void *val) {
  uintptr_t bit_ptr = (uintptr_t)ptr;
  size_t keylen = sizeof(uintptr_t);
  char *key = (char *)calloc(keylen, sizeof(char));
  for (size_t i = 0; i < keylen; ++i)
    key[i] = (char)(bit_ptr >> (i * 8));

  hashtable_insert(ht, key, keylen, val);
}

void *hashtable_get_ptr(HashTable *ht, void *ptr) {
  uintptr_t bit_ptr = (uintptr_t)ptr;
  size_t keylen = sizeof(uintptr_t);
  char *key = (char *)calloc(keylen, sizeof(char));
  for (size_t i = 0; i < keylen; ++i)
    key[i] = (char)(bit_ptr >> (i * 8));

  Entry *ent = hashtable_get(ht, key, keylen);
  free(key);
  if (ent)
    return ent->val;
  return NULL;
}

void hashtable_delete_ptr(HashTable *ht, void *ptr) {
  uintptr_t bit_ptr = (uintptr_t)ptr;
  size_t keylen = sizeof(uintptr_t);
  char *key = (char *)calloc(keylen, sizeof(char));
  for (size_t i = 0; i < keylen; ++i)
    key[i] = (char)(bit_ptr >> (i * 8));

  hashtable_delete(ht, key, keylen);
  free(key);
}

bool is_marked(Obj *obj) {
  return hashtable_get_ptr(gc->marked_table, obj) == MARK;
}

void mark_object(Obj *obj) {
  if (!obj || is_marked(obj))
    return;
  hashtable_insert_ptr(gc->marked_table, obj, MARK);
  if (obj->typ == CONS_TY) {
    mark_object(obj->exclusive.car);
    mark_object(obj->cdr);
  }
}

void mark_frame(Frame *frame) {
  // this is for avoiding infinite loop in lambda saved frames
  if (hashtable_get_ptr(gc->marked_table, frame) == MARK)
    return;
  // mark the frame to avoid revisit
  hashtable_insert_ptr(gc->marked_table, frame, MARK);
  for (size_t i = 0; i < frame->table->capacity; i++) {
    Entry *entry = frame->table->buckets[i];
    if (entry && entry != TOMBSTONE) {
      Obj *obj = *(Obj **)entry->val;
      mark_object(obj);
      if (obj->typ == LAMBDA_TY)
        mark_frame(obj->exclusive.saved_env);
    }
  }
}

void unmark_all() {
  for (size_t i = 0; i < gc->marked_table->capacity; i++) {
    Entry *entry = gc->marked_table->buckets[i];
    if (entry && entry != TOMBSTONE) {
      free(entry->key);
      free(entry);
      gc->marked_table->buckets[i] = TOMBSTONE;
    }
  }
}

void GC_mark() {
  Frame *frame = env;
  while (frame) {
    mark_frame(frame);
    frame = frame->outer;
  }
}

void sweep_worklist_item(WorkList *prev, WorkList *item) {
  if (!prev)
    gc->head = item->next;
  else
    prev->next = item->next;

  if (item == gc->tail)
    gc->tail = prev;

  free_obj(item->obj);
  free(item);
}

void GC_sweep() {
  WorkList *head = gc->head;
  WorkList *prev = NULL;
  while (head) {
    WorkList *next = head->next;
    if (!is_marked(head->obj)) {
#ifdef DEBUG
      print_obj(head->obj);
#endif
      debug_log(" will be freed!");
      sweep_worklist_item(prev, head);
      --gc->size;
    } else
      prev = head;
    head = next;
  }
  unmark_all();
}

void GC_collect() {
  debug_log("GC: mark");
  GC_mark();
  debug_log("GC: sweep");
  GC_sweep();
  debug_log("GC: finished");
}

void GC_dump() {
#ifndef DEBUG
  return;
#endif
  debug_log("---GC-DUMP--- size = %ld", gc->size);
  WorkList *head = gc->head;
  while (head) {
    println_obj(head->obj);
    head = head->next;
  }
  debug_log("---DUMPED---");
}