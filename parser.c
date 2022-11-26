#include "shirp.h"

Obj *new_obj(ObjType typ) {
  Obj *obj = (Obj *)shirp_malloc(sizeof(Obj));
  obj->typ = typ;
  return obj;
}