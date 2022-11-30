#include "shirp.h"

GC *gc;
extern Frame *env;

void GC_init() { gc = (GC *)shirp_calloc(1, sizeof(GC)); }

void GC_collect(Obj *obj) {
  WorkList *wl = (WorkList *)shirp_calloc(1, sizeof(WorkList));
  wl->obj = obj;
  if (gc->head)
    gc->tail = gc->tail->next = wl;
  else
    gc->head = gc->tail = wl;
  ++gc->size;
}

void mark_object(Obj *obj) {
  if (!obj)
    return;
  WorkList *head = gc->head;
  while (head) {
    if (obj->typ == CONS_TY) {
      mark_object(obj->car);
      mark_object(obj->cdr);
    }
    if (head->obj == obj) {
      head->is_marked = true;
#ifdef DEBUG
      print_obj(obj);
#endif
      debug_log(" is used!");
      return;
    }
    head = head->next;
  }
}

void mark_frame(Frame *frame) {
  for (size_t i = 0; i < frame->table->capacity; i++) {
    Entry *entry = frame->table->buckets[i];
    if (entry && entry != TOMBSTONE) {
      Obj *obj = (Obj *)entry->val;
      mark_object(obj);
      if (obj->typ == LAMBDA_TY)
        mark_frame(obj->saved_env);
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
    if (!head->is_marked) {
#ifdef DEBUG
      print_obj(head->obj);
#endif
      debug_log(" will be freed!");
      sweep_worklist_item(prev, head);
      --gc->size;
    } else {
      head->is_marked = false;
      prev = head;
    }
    head = next;
  }
}

void GC_mark_and_sweep() {
  debug_log("GC: mark and sweep");
  GC_mark();
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