/*

MIT License

Copyright (c) 2020-2021 Graeme Elsworthy <github@sharkshead.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "shalelib.h"

BTree btree;
Exception slexception;
bool useMutex;
Number *trueValue;
Number *falseValue;

// LexInfo and Exception classes. These tie an execution error to the input.

LexInfo::LexInfo(const char *f, int ln, const char *l, int i) : filename(f), lineNumber(ln), line(l), index(i) { }
const char *LexInfo::getFilename() { return filename; }
int LexInfo::getLineNumber() { return lineNumber; }
const char *LexInfo::getLine() { return line; }
int LexInfo::getIndex() { return index; }

Exception::Exception() : lexInfo((LexInfo *) 0), message("internal error") { }
void Exception::chuck(const char *m, LexInfo *li) { message = m; lexInfo = li; throw this; }
void Exception::printError() {
  const char *f;
  int i;

  if(lexInfo != (LexInfo *) 0) {
    f = lexInfo->getFilename();
    printf("\nError, line %d, file %s\n", lexInfo->getLineNumber(), f != (const char *) 0 ? f : "<stdin>");
    printf("%s\n", lexInfo->getLine());
    for(i = 0; i < lexInfo->getIndex(); i++) printf(" ");
    printf("^ %s\n", message != (const char *) 0 ? message : "(no message)");
  } else {
    printf("Error: %s\n", message != (const char *) 0 ? message : "(no message)");
  }
}

// This implements a cache of pre-used objects to cut down on malloc()/free() calls.

ObjectBag::ObjectBag() : object((Object *) 0), next((ObjectBag *) 0) { }
ObjectBag::~ObjectBag() { if(object != (Object *) 0) delete(object); }

CacheDebug::CacheDebug() : used(0), news(0) { }
void CacheDebug::incUsed() { used++; }
void CacheDebug::decUsed() { used--; }
void CacheDebug::incNew() { news++; }
void CacheDebug::debug() { printf("count %d, free %d", news, used); }

Cache::Cache() : usedNumbers((ObjectBag *) 0), usedStrings((ObjectBag *) 0), usedPointers((ObjectBag *) 0), unusedBags((ObjectBag *) 0) { }
Cache::~Cache() {
  ObjectBag *ob;
  ObjectBag *t;

  ob = usedNumbers;
  while(ob != (ObjectBag *) 0) {
    t = ob->next;
    // delete(ob);
    ob = t;
  }

  ob = usedStrings;
  while(ob != (ObjectBag *) 0) {
    t = ob->next;
    // delete(ob);
    ob = t;
  }

  ob = usedPointers;
  while(ob != (ObjectBag *) 0) {
    t = ob->next;
    // delete(ob);
    ob = t;
  }

// fixme
//  ob = usedBags;
//  while(ob != (ObjectBag *) 0) {
//    t = ob->next;
//    delete(ob);
//    ob = t;
//  }
}

void Cache::incUnused() { unused++; }
void Cache::decUnused() { unused--; }

Number *Cache::newNumber(INT i) {
  ObjectBag *nb;
  Number *ret;

  if(usedNumbers != (ObjectBag *) 0) {
    nb = usedNumbers;
    usedNumbers = usedNumbers->next;
    nb->next = unusedBags;
    unusedBags = nb;
    ret = (Number *) nb->object;
    nb->object = (Object *) 0;
    ret->setInt(i);
    numbers.decUsed();
    incUnused();
  } else {
    ret = new Number(i, this);
    numbers.incNew();
  }

  return ret;
}

Number *Cache::newNumber(double d) {
  ObjectBag *nb;
  Number *ret;

  if(usedNumbers != (ObjectBag *) 0) {
    nb = usedNumbers;
    usedNumbers = usedNumbers->next;
    nb->next = unusedBags;
    unusedBags = nb;
    ret = (Number *) nb->object;
    nb->object = (Object *) 0;
    ret->setDouble(d);
    numbers.decUsed();
    incUnused();
  } else {
    ret = new Number(d, this);
    numbers.incNew();
  }

  return ret;
}

void Cache::deleteNumber(Number *n) {
  ObjectBag *ob;

  if(unusedBags != (ObjectBag *) 0) {
    ob = unusedBags;
    unusedBags = unusedBags->next;
    decUnused();
  } else {
    ob = new ObjectBag;
  }
  ob->object = n;
  ob->next = usedNumbers;
  usedNumbers = ob;
  numbers.incUsed();
}

String *Cache::newString(const char *s) { return newString(s, false); }

String *Cache::newString(const char *s, bool rsf) {
  ObjectBag *ob;
  String *ret;

  if(usedStrings != (ObjectBag *) 0) {
    ob = usedStrings;
    usedStrings = usedStrings->next;
    ob->next = unusedBags;
    unusedBags = ob;
    ret = (String *) ob->object;
    ob->object = (Object *) 0;
    ret->setString(s);
    ret->setRemoveStringFlag(rsf);
    strings.decUsed();
    incUnused();
  } else {
    ret = new String(s, this, rsf);
    strings.incNew();
  }

  return ret;
}

void Cache::deleteString(String *str) {
  ObjectBag *ob;

  if(str->getRemoveStringFlag()) free((void *) str->getValue());
  if(unusedBags != (ObjectBag *) 0) {
    ob = unusedBags;
    unusedBags = unusedBags->next;
    decUnused();
  } else {
    ob = new ObjectBag;
  }
  ob->object = str;
  ob->next = usedStrings;
  usedStrings = ob;
  strings.incUsed();
}

Pointer *Cache::newPointer(Object *o) {
  ObjectBag *pb;
  Pointer *ret;

  if(usedPointers != (ObjectBag *) 0) {
    pb = usedPointers;
    usedPointers = usedPointers->next;
    pb->next = unusedBags;
    unusedBags = pb;
    ret = (Pointer *) pb->object;
    pb->object = (Object *) 0;
    ret->setObject(o);
    pointers.decUsed();
    incUnused();
  } else {
    ret = new Pointer(o, this);
    pointers.incNew();
  }

  return ret;
}

void Cache::deletePointer(Pointer *p) {
  ObjectBag *ob;

  if(unusedBags != (ObjectBag *) 0) {
    ob = unusedBags;
    unusedBags = unusedBags->next;
    decUnused();
  } else {
    ob = new ObjectBag;
  }
  ob->object = p;
  ob->next = usedPointers;
  usedPointers = ob;
  pointers.incUsed();
}

void Cache::debug() {
  printf("Number: "); numbers.debug(); printf(".  ");
  printf("String: "); strings.debug(); printf(".  ");
  printf("Pointer: "); pointers.debug(); printf(".  ");
  printf("Free bags: %d\n", unused);
}

// Object class

Object::Object(Cache *c) : cache(c), isStatic(false), referenceCount(0), mutex((pthread_mutex_t *) 0) { }
Object::Object(Cache *c, ObjectOption oo) : cache(c), referenceCount(0) { isStatic = (oo == IS_STATIC); mutex = (pthread_mutex_t *) 0; if(oo == ALLOCATE_MUTEX) { allocateMutex(); } }
Object::~Object() { deallocateMutex(); }
Number *Object::getNumber(LexInfo *li, ExecutionEnvironment *ee) { slexception.chuck("number not found", li); return (Number *) 0; }
String *Object::getString(LexInfo *li, ExecutionEnvironment *ee) { slexception.chuck("string not found", li); return (String *) 0; }
bool Object::isName() { return false; }
Name *Object::getName(LexInfo *li, ExecutionEnvironment *ee) { slexception.chuck("name not found", li); return (Name *) 0; }
Code *Object::getCode(LexInfo *li, ExecutionEnvironment *ee) { slexception.chuck("code not found", li); return (Code *) 0; }
Pointer *Object::getPointer(LexInfo *li, ExecutionEnvironment *ee) { slexception.chuck("pointer not found", li); return (Pointer *) 0; }
bool Object::isDynamic() { return ! isStatic; }
void Object::hold() {
  if(! isStatic) {
    if(useMutex && (mutex != (pthread_mutex_t *) 0)) pthread_mutex_lock(mutex);
    referenceCount++;
    if(useMutex && (mutex != (pthread_mutex_t *) 0)) pthread_mutex_unlock(mutex);
  }
}
void Object::release(LexInfo *li) {
  if(! isStatic) {
    if(useMutex && (mutex != (pthread_mutex_t *) 0)) pthread_mutex_lock(mutex);
    if(referenceCount < 0) slexception.chuck("reference error", li);
    if(referenceCount == 0) {
      delete(this);
    } else {
      referenceCount--;
      if(useMutex && (mutex != (pthread_mutex_t *) 0)) pthread_mutex_unlock(mutex);
    }
  }
}
void Object::allocateMutex() {
  if(! isStatic) {
    if(mutex == (pthread_mutex_t *) 0) {
      mutex = new pthread_mutex_t;
      pthread_mutex_init(mutex, NULL);
    }
  }
}
void Object::deallocateMutex() {
  if(mutex != (pthread_mutex_t *) 0) {
    pthread_mutex_destroy(mutex);
    delete(mutex);
    mutex = (pthread_mutex_t *) 0;
  }
}

// Number class

Number::Number(INT i, Cache *c) : Object(c), intRep(true), valueInt(i) { }
Number::Number(INT i, Cache *c, ObjectOption oo) : Object(c, oo), intRep(true), valueInt(i) { }
Number::Number(double d, Cache *c) : Object(c), intRep(false), valueDouble(d) { }
Number::Number(double d, Cache *c, ObjectOption oo) : Object(c, oo), intRep(false), valueDouble(d) { }
Number *Number::getNumber(LexInfo *li, ExecutionEnvironment *ee) { this->hold(); return this; }
bool Number::isInt() { return intRep; }
INT Number::getInt() { if(intRep) return valueInt; return valueDouble; }
void Number::setInt(INT i) { intRep = true; valueInt = i; }
double Number::getDouble() { if(intRep) return valueInt; return valueDouble; }
void Number::setDouble(double d) { intRep = false; valueDouble = d; }
void Number::release(LexInfo *li) {
  if(isDynamic()) {
    if(useMutex && (mutex != (pthread_mutex_t *) 0)) pthread_mutex_lock(mutex);
    if(referenceCount < 0) slexception.chuck("reference error", li);
    if(referenceCount == 0) { this->deallocateMutex(); cache->deleteNumber(this); } else referenceCount--;
    if(useMutex && (mutex != (pthread_mutex_t *) 0)) pthread_mutex_unlock(mutex);
  }
}
void Number::debug() { char fmt[32]; printf("Number: "); if(intRep) { sprintf(fmt, "%%%sd\n", PCTD); printf(fmt, valueInt); } else printf("%0.3f\n", valueDouble); }

// String class

String::String(const char *s, Cache *c) : Object(c), str(s), removeStringFlag(false) { }
String::String(const char *s, Cache *c, ObjectOption oo) : Object(c, oo), str(s), removeStringFlag(false) { }
String::String(const char *s, Cache *c, bool rsf) : Object(c), str(s), removeStringFlag(rsf) { }
String::~String() { if(removeStringFlag) free((void *) str); }
String *String::getString(LexInfo *li, ExecutionEnvironment *ee) { this->hold(); return this; }
const char *String::getValue() { return str; }
bool String::getRemoveStringFlag() { return removeStringFlag; }
void String::setString(const char *s) { str = s; }
void String::setRemoveStringFlag(bool rsf) { removeStringFlag = rsf; }
void String::release(LexInfo *li) {
  if(isDynamic()) {
    if(useMutex && (mutex != (pthread_mutex_t *) 0)) pthread_mutex_lock(mutex);
    if(referenceCount < 0) slexception.chuck("reference error", li);
    if(referenceCount == 0) { this->deallocateMutex(); cache->deleteString(this); } else referenceCount--;
    if(useMutex && (mutex != (pthread_mutex_t *) 0)) pthread_mutex_unlock(mutex);
  }
}
void String::debug() { printf("String: %s\n", str); }

// Name class

Name::Name(const char *n, Cache *c) : Object(c) {
  int i;

  for(i = 0; (i < (MAX_NAME_LENGTH - 1)) && (n[i] != 0); i++) {
    name[i] = n[i];
  }
  name[i] = 0;
}

Name::Name(const char *n, Cache *c, ObjectOption oo) : Object(c, oo) {
  int i;

  for(i = 0; (i < (MAX_NAME_LENGTH - 1)) && (n[i] != 0); i++) {
    name[i] = n[i];
  }
  name[i] = 0;
}

bool Name::isName() {
  return true;
}

Name *Name::getName(LexInfo *li, ExecutionEnvironment *ee) {
  return this;
}

char *Name::getValue() {
  return name;
}

Number *Name::getNumber(LexInfo *li, ExecutionEnvironment *ee) {
  static char buf[128];
  Variable *v = ee->variableStack.findVariable(name);
  if(v == (Variable *) 0) { sprintf(buf, "variable error: %s not found", name); slexception.chuck(buf, li); }
  if(! v->isInitialised()) { sprintf(buf, "variable error: %s not initialised", name); slexception.chuck(buf, li); }
  return v->getObject()->getNumber(li, ee);
}

String *Name::getString(LexInfo *li, ExecutionEnvironment *ee) {
  static char buf[128];
  Variable *v = ee->variableStack.findVariable(name);
  if(v == (Variable *) 0) { sprintf(buf, "variable error: %s not found", name); slexception.chuck(buf, li); }
  if(! v->isInitialised()) { sprintf(buf, "variable error: %s not initialised", name); slexception.chuck(buf, li); }
  return v->getObject()->getString(li, ee);
}

Code *Name::getCode(LexInfo *li, ExecutionEnvironment *ee) {
  static char buf[128];
  Variable *v = ee->variableStack.findVariable(name);
  if(v == (Variable *) 0) { sprintf(buf, "variable error: %s not found", name); slexception.chuck(buf, li); }
  if(! v->isInitialised()) { sprintf(buf, "variable error: %s not initialised", name); slexception.chuck(buf, li); }
  return v->getObject()->getCode(li, ee);
}

Pointer *Name::getPointer(LexInfo *li, ExecutionEnvironment *ee) {
  static char buf[128];
  Variable *v = ee->variableStack.findVariable(name);
  if(v == (Variable *) 0) { sprintf(buf, "variable error: %s not found", name); slexception.chuck(buf, li); }
  if(! v->isInitialised()) { sprintf(buf, "variable error: %s not initialised", name); slexception.chuck(buf, li); }
  return v->getObject()->getPointer(li, ee);
}

void Name::debug() { printf("Name: %s\n", name); }

// Code class

Code::Code(OperationList *ol, Cache *c) : Object(c), operationList(ol) { }
Code::Code(OperationList *ol, Cache *c, ObjectOption oo) : Object(c, oo), operationList(ol) { }
Code *Code::getCode(LexInfo *li, ExecutionEnvironment *e) { this->hold(); return this; }
OperationList *Code::getOperationList() { return operationList; }
OperatorReturn Code::action(ExecutionEnvironment *ee) { return operationList->action(ee); }
void Code::debug() { printf("Code\n"); }

// Pointer class

Pointer::Pointer(Object *o, Cache *c) : Object(c), object(o) { object->hold(); }
Pointer *Pointer::getPointer(LexInfo *li, ExecutionEnvironment *ee) { this->hold(); return this; }
void Pointer::setObject(Object *o) { if(object != (Object *) 0) object->release((LexInfo *) 0); object = o; object->hold(); }
Object *Pointer::getObject() { return object; }
void Pointer::hold() { Object::hold(); if(object != (Object *) 0) object->hold(); }
void Pointer::release(LexInfo *li) {
  if(isDynamic()) {
    if(useMutex && (mutex != (pthread_mutex_t *) 0)) pthread_mutex_lock(mutex);
    if(referenceCount < 0) slexception.chuck("reference error", li);
    if(object != (Object *) 0) object->release(li);
    if(referenceCount == 0) { this->deallocateMutex(); cache->deletePointer(this); } else referenceCount--;
    if(useMutex && (mutex != (pthread_mutex_t *) 0)) pthread_mutex_unlock(mutex);
  }
}
void Pointer::debug() { printf("Pointer\n"); }

// Operation classes

Operation::Operation(LexInfo *li) : lexInfo(li) { }

LexInfo *Operation::getLexInfo() {
  return lexInfo;
}

bool Operation::isVar() {
  return false;
}

bool Operation::isFunction() {
  return false;
}

// OperationListItem class

OperationListItem::OperationListItem(Operation *op) : operation(op), next((OperationListItem *) 0) { }

OperationListItem *OperationListItem::getNext() {
  return next;
}

void OperationListItem::setNext(OperationListItem *oli) {
  next = oli;
}

Operation *OperationListItem::getOperation() {
  return operation;
}

// OperationList class

OperationList::OperationList() : head((OperationListItem *) 0), tail((OperationListItem *) 0), newVariableStack(false), isFn(false) { }

void OperationList::addOperation(Operation *op) {
  OperationListItem *oli = new OperationListItem(op);
  if(head == (OperationListItem *) 0) { head = tail = oli; }
  else { tail->setNext(oli); tail = oli; }
  if(op->isVar()) newVariableStack = true;
  if(op->isFunction()) isFn = true;
}

bool OperationList::isFunction() {
  return isFn;
}

OperationListItem *OperationList::getOperationList() {
  return head;
}

OperatorReturn OperationList::action(ExecutionEnvironment *ee) {
  OperatorReturn ret;

  ret = or_continue;
  if(newVariableStack) ee->variableStack.addVariableStack();
  for(OperationListItem *oli = head; oli != (OperationListItem *) 0; oli = oli->getNext()) {
    if((ret = oli->getOperation()->action(ee)) != or_continue) {
      break;
    }
  }
  if(newVariableStack) ee->variableStack.popVariableStack();
  if((ret == or_return) && isFn) ret = or_continue;
  return ret;
}

OperatorReturn OperationList::actionLatest(ExecutionEnvironment *ee) {
  OperationListItem *oli;
  OperatorReturn ret;

  ret = or_continue;
  if(newVariableStack && ee->variableStack.isEmpty()) ee->variableStack.addVariableStack();
  oli = tail;
  if(oli != (OperationListItem *) 0) {
    ret = oli->getOperation()->action(ee);
  }
  return ret;
}

// ObjectList classes

ObjectListItem::ObjectListItem(Object *o) : object(o), next((ObjectListItem *) 0) { object->hold(); }

ObjectListItem *ObjectListItem::getNext() {
  return next;
}

void ObjectListItem::setNext(ObjectListItem *n) {
  next = n;
}

Object *ObjectListItem::getObject() {
  return object;
}

ObjectList::ObjectList() : length(0), head((ObjectListItem *) 0), tail((ObjectListItem *) 0) { }

void ObjectList::addObject(Object *o) {
  ObjectListItem *oli = new ObjectListItem(o);
  if(head == (ObjectListItem *) 0) { head = tail = oli; }
  else { tail->setNext(oli); tail = oli; }
  length++;
}

int ObjectList::getLength() {
  return length;
}

Object *ObjectList::getObject(int n) {
  ObjectListItem *oli;
  int i;

  for(i = 0, oli = head; (i != n) && (oli != (ObjectListItem *) 0); oli = oli->getNext(), i++) ;
  if(oli == (ObjectListItem *) 0) slexception.chuck("array out of bounds", (LexInfo *) 0);

  return oli->getObject();
}

// Push class

Push::Push(Object *o, LexInfo *li) : Operation(li), object(o) { object->hold(); }

OperatorReturn Push::action(ExecutionEnvironment *ee) {
  object->hold();
  ee->stack.push(object);
  return or_continue;
}

// Stack classes

Pop::Pop(LexInfo *li) : Operation(li) { }

OperatorReturn Pop::action(ExecutionEnvironment *ee) {
  Object *o;

  o = ee->stack.pop(getLexInfo());
  o->release(getLexInfo());

  return or_continue;
}

Swap::Swap(LexInfo *li) : Operation(li) { }

OperatorReturn Swap::action(ExecutionEnvironment *ee) {
  Object *o1;
  Object *o2;

  o1 = ee->stack.pop(getLexInfo());
  o2 = ee->stack.pop(getLexInfo());

  ee->stack.push(o1);
  ee->stack.push(o2);

  return or_continue;
}

Dup::Dup(LexInfo *li) : Operation(li) { }

OperatorReturn Dup::action(ExecutionEnvironment *ee) {
  Object *o;

  o = ee->stack.pop(getLexInfo());
  o->hold();
  ee->stack.push(o);
  ee->stack.push(o);

  return or_continue;
}

// Arithmetic classes

Int::Int(LexInfo *li) : Operation(li) { }

OperatorReturn Int::action(ExecutionEnvironment *ee) {
  Object *o;
  Number *n;

  o = ee->stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo(), ee);
  ee->stack.push(ee->cache.newNumber(n->getInt()));
  n->release(getLexInfo());
  o->release(getLexInfo());

  return or_continue;
}

Double::Double(LexInfo *li) : Operation(li) { }

OperatorReturn Double::action(ExecutionEnvironment *ee) {
  Object *o;
  Number *n;

  o = ee->stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo(), ee);
  ee->stack.push(ee->cache.newNumber(n->getDouble()));
  n->release(getLexInfo());
  o->release(getLexInfo());

  return or_continue;
}

Plus::Plus(LexInfo *li) : Operation(li) { }

OperatorReturn Plus::action(ExecutionEnvironment *ee) {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;
  Number *res;
  Code *c1;
  Code *c2;
  OperationList *ol;
  OperationListItem *oli;
  bool found;

  o2 = ee->stack.pop(getLexInfo());
  o1 = ee->stack.pop(getLexInfo());

  found = false;

  try {
    n1 = o1->getNumber(getLexInfo(), ee);
    n2 = o2->getNumber(getLexInfo(), ee);

    if(n1->isInt()) {
      if(n2->isInt()) res = ee->cache.newNumber(n1->getInt() + n2->getInt());
      else res = ee->cache.newNumber(n1->getInt() + n2->getDouble());
    } else {
      if(n2->isInt()) res = ee->cache.newNumber(n1->getDouble() + n2->getInt());
      else res = ee->cache.newNumber(n1->getDouble() + n2->getDouble());
    }
    ee->stack.push(res);

    n1->release(getLexInfo());
    n2->release(getLexInfo());

    found = true;
  } catch (Exception *e) { }

  if(! found) {
    try {
      c1 = o1->getCode(getLexInfo(), ee);
      c2 = o2->getCode(getLexInfo(), ee);

      ol = new OperationList;
      for(oli = c1->getOperationList()->getOperationList(); oli != (OperationListItem *) 0; oli = oli->getNext()) {
        ol->addOperation(oli->getOperation());
      }
      for(oli = c2->getOperationList()->getOperationList(); oli != (OperationListItem *) 0; oli = oli->getNext()) {
        ol->addOperation(oli->getOperation());
      }
      ee->stack.push(new Code(ol, &ee->cache));

      c1->release(getLexInfo());
      c2->release(getLexInfo());

      found = true;
    } catch(Exception *e) { }
  }

  if(! found) slexception.chuck("unknown operands", getLexInfo());

  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return or_continue;
}

Minus::Minus(LexInfo *li) : Operation(li) { }

OperatorReturn Minus::action(ExecutionEnvironment *ee) {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;
  Number *res;

  o2 = ee->stack.pop(getLexInfo());
  o1 = ee->stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo(), ee);
  n2 = o2->getNumber(getLexInfo(), ee);

  if(n1->isInt()) {
    if(n2->isInt()) res = ee->cache.newNumber(n1->getInt() - n2->getInt());
    else res = ee->cache.newNumber(n1->getInt() - n2->getDouble());
  } else {
    if(n2->isInt()) res = ee->cache.newNumber(n1->getDouble() - n2->getInt());
    else res = ee->cache.newNumber(n1->getDouble() - n2->getDouble());
  }
  ee->stack.push(res);

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return or_continue;
}

Times::Times(LexInfo *li) : Operation(li) { }

OperatorReturn Times::action(ExecutionEnvironment *ee) {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;
  Number *res;

  o2 = ee->stack.pop(getLexInfo());
  o1 = ee->stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo(), ee);
  n2 = o2->getNumber(getLexInfo(), ee);

  if(n1->isInt()) {
    if(n2->isInt()) res = ee->cache.newNumber(n1->getInt() * n2->getInt());
    else res = ee->cache.newNumber(n1->getInt() * n2->getDouble());
  } else {
    if(n2->isInt()) res = ee->cache.newNumber(n1->getDouble() * n2->getInt());
    else res = ee->cache.newNumber(n1->getDouble() * n2->getDouble());
  }
  ee->stack.push(res);

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return or_continue;
}

Divide::Divide(LexInfo *li) : Operation(li) { }

OperatorReturn Divide::action(ExecutionEnvironment *ee) {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;
  Number *res;

  o2 = ee->stack.pop(getLexInfo());
  o1 = ee->stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo(), ee);
  n2 = o2->getNumber(getLexInfo(), ee);

  if(n1->isInt()) {
    if(n2->isInt()) res = ee->cache.newNumber(n1->getInt() / n2->getInt());
    else res = ee->cache.newNumber(n1->getInt() / n2->getDouble());
  } else {
    if(n2->isInt()) res = ee->cache.newNumber(n1->getDouble() / n2->getInt());
    else res = ee->cache.newNumber(n1->getDouble() / n2->getDouble());
  }
  ee->stack.push(res);

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return or_continue;
}

Mod::Mod(LexInfo *li) : Operation(li) { }

OperatorReturn Mod::action(ExecutionEnvironment *ee) {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;

  o2 = ee->stack.pop(getLexInfo());
  o1 = ee->stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo(), ee);
  n2 = o2->getNumber(getLexInfo(), ee);

  ee->stack.push(ee->cache.newNumber(n1->getInt() % n2->getInt()));

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return or_continue;
}

BitwiseAnd::BitwiseAnd(LexInfo *li) : Operation(li) { }

OperatorReturn BitwiseAnd::action(ExecutionEnvironment *ee) {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;

  o2 = ee->stack.pop(getLexInfo());
  o1 = ee->stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo(), ee);
  n2 = o2->getNumber(getLexInfo(), ee);

  ee->stack.push(ee->cache.newNumber(n1->getInt() & n2->getInt()));

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return or_continue;
}

BitwiseOr::BitwiseOr(LexInfo *li) : Operation(li) { }

OperatorReturn BitwiseOr::action(ExecutionEnvironment *ee) {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;

  o2 = ee->stack.pop(getLexInfo());
  o1 = ee->stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo(), ee);
  n2 = o2->getNumber(getLexInfo(), ee);

  ee->stack.push(ee->cache.newNumber(n1->getInt() | n2->getInt()));

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return or_continue;
}

BitwiseXor::BitwiseXor(LexInfo *li) : Operation(li) { }

OperatorReturn BitwiseXor::action(ExecutionEnvironment *ee) {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;

  o2 = ee->stack.pop(getLexInfo());
  o1 = ee->stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo(), ee);
  n2 = o2->getNumber(getLexInfo(), ee);

  ee->stack.push(ee->cache.newNumber(n1->getInt() ^ n2->getInt()));

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return or_continue;
}

BitwiseNot::BitwiseNot(LexInfo *li) : Operation(li) { }

OperatorReturn BitwiseNot::action(ExecutionEnvironment *ee) {
  Object *o1;
  Number *n1;

  o1 = ee->stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo(), ee);

  ee->stack.push(ee->cache.newNumber(~n1->getInt()));

  n1->release(getLexInfo());
  o1->release(getLexInfo());

  return or_continue;
}

LeftShift::LeftShift(LexInfo *li) : Operation(li) { }

OperatorReturn LeftShift::action(ExecutionEnvironment *ee) {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;

  o2 = ee->stack.pop(getLexInfo());
  o1 = ee->stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo(), ee);
  n2 = o2->getNumber(getLexInfo(), ee);

  ee->stack.push(ee->cache.newNumber(n1->getInt() << n2->getInt()));

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return or_continue;
}

RightShift::RightShift(LexInfo *li) : Operation(li) { }

OperatorReturn RightShift::action(ExecutionEnvironment *ee) {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;

  o2 = ee->stack.pop(getLexInfo());
  o1 = ee->stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo(), ee);
  n2 = o2->getNumber(getLexInfo(), ee);

  ee->stack.push(ee->cache.newNumber(n1->getInt() >> n2->getInt()));

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return or_continue;
}

// The Function class

Function::Function(LexInfo *li) : Operation(li) { }

OperatorReturn Function::action(ExecutionEnvironment *ee) { return or_continue; }

bool Function::isFunction() { return true; }

// Var class

Var::Var(LexInfo *li) : Operation(li) { }

OperatorReturn Var::action(ExecutionEnvironment *ee) {
  Object *o;

  o = ee->stack.pop(getLexInfo());
  ee->variableStack.addVariable(o->getName(getLexInfo(), ee)->getValue(), getLexInfo());
  o->release(getLexInfo());

  return or_continue;
}

bool Var::isVar() { return true; }

// Assign class

Assign::Assign(LexInfo *li) : Operation(li) { }

OperatorReturn Assign::action(ExecutionEnvironment *ee) {
  Object *var;
  Object *val;
  Number *number;
  String *string;
  Code *code;
  Pointer *pointer;
  Object *o;
  Variable *v;
  bool varfound;
  bool valfound;
  bool allocateMutex;
  static char message[1024];

  val = ee->stack.pop(getLexInfo());
  var = ee->stack.pop(getLexInfo());
  varfound = false;
  valfound = false;

  // Is this a variable we're assigning?
  try {
    v = ee->variableStack.findVariable(var->getName(getLexInfo(), ee)->getValue());
    if(v != (Variable *) 0) {
      varfound = true;

      // Not elegant, but if this is going into the BTree then allocate a mutex as this is potentially shared between threads.
      allocateMutex = (v->getName()[0] == '/');
        
      try {
        number = val->getNumber(getLexInfo(), ee);
        if(allocateMutex) number->allocateMutex();
        v->setObject(number);
        number->release(getLexInfo());
        valfound = true;
      } catch(Exception *e) { }

      if(! valfound) {
        try {
          string = val->getString(getLexInfo(), ee);
          if(allocateMutex) string->allocateMutex();
          v->setObject(string);
          string->release(getLexInfo());
          valfound = true;
        } catch(Exception *e) { }
      }

      if(! valfound) {
        try {
          code = val->getCode(getLexInfo(), ee);
          if(allocateMutex) code->allocateMutex();
          v->setObject(code);
          code->release(getLexInfo());
          valfound = true;
        } catch(Exception *e) { }
      }

      if(! valfound) {
        try {
          pointer = val->getPointer(getLexInfo(), ee);
          if(allocateMutex) pointer->allocateMutex();
          v->setObject(pointer);
          pointer->release(getLexInfo());
          valfound = true;
        } catch(Exception *e) { }
      }
    }
  } catch(Exception *e) { }

  if(! valfound) {
    if(varfound) {
      slexception.chuck("assignment error, value not found", getLexInfo());
    } else {
      slexception.chuck("assignment error, variable not found", getLexInfo());
    }
  }

  val->release(getLexInfo());
  var->release(getLexInfo());

  return or_continue;
}

// PointerAssign class

PointerAssign::PointerAssign(LexInfo *li) : Operation(li) { }

OperatorReturn PointerAssign::action(ExecutionEnvironment *ee) {
  Object *var;
  Object *val;
  Variable *v;
  Pointer *p;
  bool found;

  val = ee->stack.pop(getLexInfo());
  var = ee->stack.pop(getLexInfo());
  found = false;

  try {
    v = ee->variableStack.findVariable(var->getName(getLexInfo(), ee)->getValue());
    if(v != (Variable *) 0) {
      p = ee->cache.newPointer(val);
      if(v->getName()[0] == '/') p->allocateMutex();
      v->setObject(p);
      p->release(getLexInfo());
      found = true;
    }
  } catch(Exception *e) { }

  if(! found) slexception.chuck("pointer assignment error", getLexInfo());

  val->release(getLexInfo());
  var->release(getLexInfo());

  return or_continue;
}

// PointerDereference class

PointerDereference::PointerDereference(LexInfo *li) : Operation(li) { }

OperatorReturn PointerDereference::action(ExecutionEnvironment *ee) {
  Object *var;
  Pointer *p;
  Object *o;

  var = ee->stack.pop(getLexInfo());

  p = var->getPointer(getLexInfo(), ee);
  o = p->getObject();
  o->hold();
  ee->stack.push(o);
  p->release(getLexInfo());

  var->release(getLexInfo());

  return or_continue;
}

// PlusPlus class

PlusPlus::PlusPlus(LexInfo *li) : Operation(li) { }

OperatorReturn PlusPlus::action(ExecutionEnvironment *ee) {
  Object *o;
  Object *no;
  Variable *v;
  Number *n;
  Number *newn;
  static char buf[128];
  char *vname;

  o = ee->stack.pop(getLexInfo());

  vname = o->getName(getLexInfo(), ee)->getValue();
  v = ee->variableStack.findVariable(vname);
  if(v == (Variable *) 0) slexception.chuck("variable error", getLexInfo());
  no = v->getObject();
  if(no == (Object *) 0) {
    sprintf(buf, "variable %s undefined", vname);
    slexception.chuck(buf, getLexInfo());
  }
  n = no->getNumber(getLexInfo(), ee);

  if(n->isInt()) v->setObject(newn = ee->cache.newNumber(n->getInt() + (INT) 1));
  else v->setObject(newn = ee->cache.newNumber(n->getDouble() + 1.0));

  newn->release(getLexInfo());
  n->release(getLexInfo());

  o->release(getLexInfo());

  return or_continue;
}

// MinusMinus class

MinusMinus::MinusMinus(LexInfo *li) : Operation(li) { }

OperatorReturn MinusMinus::action(ExecutionEnvironment *ee) {
  Object *o;
  Object *no;
  Variable *v;
  Number *n;
  Number *newn;
  static char buf[128];
  char *vname;

  o = ee->stack.pop(getLexInfo());

  vname = o->getName(getLexInfo(), ee)->getValue();
  v = ee->variableStack.findVariable(vname);
  if(v == (Variable *) 0) slexception.chuck("variable error", getLexInfo());
  no = v->getObject();
  if(no == (Object *) 0) {
    sprintf(buf, "variable %s undefined", vname);
    slexception.chuck(buf, getLexInfo());
  }
  n = no->getNumber(getLexInfo(), ee);

  if(n->isInt()) v->setObject(newn = ee->cache.newNumber(n->getInt() - (INT) 1));
  else v->setObject(newn = ee->cache.newNumber(n->getDouble() - 1.0));

  newn->release(getLexInfo());
  n->release(getLexInfo());

  o->release(getLexInfo());

  return or_continue;
}

// Logical operators

LessThan::LessThan(LexInfo *li) : Operation(li) { }

OperatorReturn LessThan::action(ExecutionEnvironment *ee) {
  Object *a;
  Object *b;
  Number *an;
  Number *bn;
  bool r;

  b = ee->stack.pop(getLexInfo());
  a = ee->stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo(), ee);
  bn = b->getNumber(getLexInfo(), ee);

  if(an->isInt()) {
    if(bn->isInt()) r = (an->getInt() < bn->getInt());
    else r = (an->getInt() < bn->getDouble());
  } else {
    if(bn->isInt()) r = (an->getDouble() < bn->getInt());
    else r = (an->getDouble() < bn->getDouble());
  }
  ee->stack.push(r ? trueValue : falseValue);

  an->release(getLexInfo());
  bn->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return or_continue;
}

LessThanOrEquals::LessThanOrEquals(LexInfo *li) : Operation(li) { }

OperatorReturn LessThanOrEquals::action(ExecutionEnvironment *ee) {
  Object *a;
  Object *b;
  Number *an;
  Number *bn;
  bool r;

  b = ee->stack.pop(getLexInfo());
  a = ee->stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo(), ee);
  bn = b->getNumber(getLexInfo(), ee);

  if(an->isInt()) {
    if(bn->isInt()) r = (an->getInt() <= bn->getInt());
    else r = (an->getInt() <= bn->getDouble());
  } else {
    if(bn->isInt()) r = (an->getDouble() <= bn->getInt());
    else r = (an->getDouble() <= bn->getDouble());
  }
  ee->stack.push(r ? trueValue : falseValue);

  an->release(getLexInfo());
  bn->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return or_continue;
}

Equals::Equals(LexInfo *li) : Operation(li) { }

OperatorReturn Equals::action(ExecutionEnvironment *ee) {
  Object *a;
  Object *b;
  Number *an;
  Number *bn;
  bool r;

  b = ee->stack.pop(getLexInfo());
  a = ee->stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo(), ee);
  bn = b->getNumber(getLexInfo(), ee);

  if(an->isInt()) {
    if(bn->isInt()) r = (an->getInt() == bn->getInt());
    else r = (an->getInt() == bn->getDouble());
  } else {
    if(bn->isInt()) r = (an->getDouble() == bn->getInt());
    else r = (an->getDouble() == bn->getDouble());
  }
  ee->stack.push(r ? trueValue : falseValue);

  an->release(getLexInfo());
  bn->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return or_continue;
}

NotEquals::NotEquals(LexInfo *li) : Operation(li) { }

OperatorReturn NotEquals::action(ExecutionEnvironment *ee) {
  Object *a;
  Object *b;
  Number *an;
  Number *bn;
  bool r;

  b = ee->stack.pop(getLexInfo());
  a = ee->stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo(), ee);
  bn = b->getNumber(getLexInfo(), ee);

  if(an->isInt()) {
    if(bn->isInt()) r = (an->getInt() != bn->getInt());
    else r = (an->getInt() != bn->getDouble());
  } else {
    if(bn->isInt()) r = (an->getDouble() != bn->getInt());
    else r = (an->getDouble() != bn->getDouble());
  }
  ee->stack.push(r ? trueValue : falseValue);

  an->release(getLexInfo());
  bn->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return or_continue;
}

GreaterThanOrEquals::GreaterThanOrEquals(LexInfo *li) : Operation(li) { }

OperatorReturn GreaterThanOrEquals::action(ExecutionEnvironment *ee) {
  Object *a;
  Object *b;
  Number *an;
  Number *bn;
  bool r;

  b = ee->stack.pop(getLexInfo());
  a = ee->stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo(), ee);
  bn = b->getNumber(getLexInfo(), ee);

  if(an->isInt()) {
    if(bn->isInt()) r = (an->getInt() >= bn->getInt());
    else r = (an->getInt() >= bn->getDouble());
  } else {
    if(bn->isInt()) r = (an->getDouble() >= bn->getInt());
    else r = (an->getDouble() >= bn->getDouble());
  }
  ee->stack.push(r ? trueValue : falseValue);

  an->release(getLexInfo());
  bn->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return or_continue;
}

GreaterThan::GreaterThan(LexInfo *li) : Operation(li) { }

OperatorReturn GreaterThan::action(ExecutionEnvironment *ee) {
  Object *a;
  Object *b;
  Number *an;
  Number *bn;
  bool r;

  b = ee->stack.pop(getLexInfo());
  a = ee->stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo(), ee);
  bn = b->getNumber(getLexInfo(), ee);

  if(an->isInt()) {
    if(bn->isInt()) r = (an->getInt() > bn->getInt());
    else r = (an->getInt() > bn->getDouble());
  } else {
    if(bn->isInt()) r = (an->getDouble() > bn->getInt());
    else r = (an->getDouble() > bn->getDouble());
  }
  ee->stack.push(r ? trueValue : falseValue);

  an->release(getLexInfo());
  bn->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return or_continue;
}

LogicalAnd::LogicalAnd(LexInfo *li) : Operation(li) { }

OperatorReturn LogicalAnd::action(ExecutionEnvironment *ee) {
  Object *a;
  Object *b;
  Object *co;
  Number *an;
  Number *bn;
  Code *c;
  bool found;
  bool r;
  OperatorReturn ret;

  b = ee->stack.pop(getLexInfo());
  a = ee->stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo(), ee);

  ret = or_continue;
  r = (an->getInt() != 0);
  if(r) {
    try {
      found = false;
      c = b->getCode(getLexInfo(), ee);
      ret = c->action(ee);
      co = ee->stack.pop(getLexInfo());
      bn = co->getNumber(getLexInfo(), ee);
      co->release(getLexInfo());
      c->release(getLexInfo());
      found = true;
    } catch(Exception *e) { }

    if(! found) {
      bn = b->getNumber(getLexInfo(), ee);
    }
    r = (bn->getInt() != 0);
    bn->release(getLexInfo());
  }

  ee->stack.push(r ? trueValue : falseValue);

  an->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return ret;
}

LogicalOr::LogicalOr(LexInfo *li) : Operation(li) { }

OperatorReturn LogicalOr::action(ExecutionEnvironment *ee) {
  Object *a;
  Object *b;
  Object *co;
  Number *an;
  Number *bn;
  Code *c;
  bool found;
  bool r;

  b = ee->stack.pop(getLexInfo());
  a = ee->stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo(), ee);

  r = (an->getInt() != 0);
  if(! r) {
    try {
      found = false;
      c = b->getCode(getLexInfo(), ee);
      c->action(ee);
      co = ee->stack.pop(getLexInfo());
      bn = co->getNumber(getLexInfo(), ee);
      co->release(getLexInfo());
      c->release(getLexInfo());
      found = true;
    } catch(Exception *e) { }

    if(! found) {
      bn = b->getNumber(getLexInfo(), ee);
    }
    r = (bn->getInt() != 0);
    bn->release(getLexInfo());
  }

  ee->stack.push(r ? trueValue : falseValue);

  an->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return or_continue;
}

LogicalNot::LogicalNot(LexInfo *li) : Operation(li) { }

OperatorReturn LogicalNot::action(ExecutionEnvironment *ee) {
  Object *a;
  Number *an;
  bool r;

  a = ee->stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo(), ee);

  if(an->isInt()) r = ! an->getInt();
  else r = ! an->getDouble();
  ee->stack.push(r ? trueValue : falseValue);

  an->release(getLexInfo());
  a->release(getLexInfo());

  return or_continue;
}

// Break class

Break::Break(LexInfo *li) : Operation(li) { }

OperatorReturn Break::action(ExecutionEnvironment *ee) {
  return or_break;
}

// Return class

Return::Return(LexInfo *li) : Operation(li) { }

OperatorReturn Return::action(ExecutionEnvironment *ee) {
  return or_return;
}

// Exit class

Exit::Exit(LexInfo *li) : Operation(li) { }

OperatorReturn Exit::action(ExecutionEnvironment *ee) {
  Object *o;
  Number *n;

  o = ee->stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo(), ee);
  exit(n->getInt());

  return or_continue;
}

// While class

While::While(LexInfo *li) : Operation(li) { }

OperatorReturn While::action(ExecutionEnvironment *ee) {
  Object *cond;
  Object *body;
  Code *condCode;
  Code *bodyCode;
  Object *c;
  Number *n;
  INT num;
  OperatorReturn ret;

  body = ee->stack.pop(getLexInfo());
  cond = ee->stack.pop(getLexInfo());
  condCode = cond->getCode(getLexInfo(), ee);
  bodyCode = body->getCode(getLexInfo(), ee);

  ret = or_continue;
  while(true) {
    condCode->action(ee);
    c = ee->stack.pop(getLexInfo());
    n = c->getNumber(getLexInfo(), ee);
    num = n->getInt();
    n->release(getLexInfo());
    c->release(getLexInfo());
    if(num == 0) break;
    if((ret = bodyCode->action(ee)) != or_continue) {
      if(ret == or_break) ret = or_continue;
      break;
    }
  }

  condCode->release(getLexInfo());
  bodyCode->release(getLexInfo());
  cond->release(getLexInfo());
  body->release(getLexInfo());

  return ret;
}

// If class

If::If(LexInfo *li) : Operation(li) { }

OperatorReturn If::action(ExecutionEnvironment *ee) {
  Object *cond;
  Object *thenPart;
  Object *elsePart;
  Code *thenCode;
  Code *elseCode;
  Number *n;
  OperatorReturn ret;

  elsePart = ee->stack.pop(getLexInfo());
  thenPart = ee->stack.pop(getLexInfo());
  cond = ee->stack.pop(getLexInfo());
  thenCode = thenPart->getCode(getLexInfo(), ee);
  elseCode = elsePart->getCode(getLexInfo(), ee);
  n = cond->getNumber(getLexInfo(), ee);

  if(n->isInt()) {
    if(n->getInt() == 0) ret = elseCode->action(ee);
    else ret = thenCode->action(ee);
  } else {
    if(n->getDouble() == 0) ret = elseCode->action(ee);
    else ret = thenCode->action(ee);
  }

  n->release(getLexInfo());
  thenCode->release(getLexInfo());
  elseCode->release(getLexInfo());
  thenPart->release(getLexInfo());
  elsePart->release(getLexInfo());
  cond->release(getLexInfo());

  return ret;
}

// IfThen class

IfThen::IfThen(LexInfo *li) : Operation(li) { }

OperatorReturn IfThen::action(ExecutionEnvironment *ee) {
  Object *cond;
  Object *thenPart;
  Code *thenCode;
  Number *n;
  OperatorReturn ret;

  thenPart = ee->stack.pop(getLexInfo());
  cond = ee->stack.pop(getLexInfo());
  thenCode = thenPart->getCode(getLexInfo(), ee);
  n = cond->getNumber(getLexInfo(), ee);
  ret = or_continue;

  if(n->isInt()) {
    if(n->getInt() != 0) ret = thenCode->action(ee);
  } else {
    if(n->getDouble() != 0) ret = thenCode->action(ee);
  }

  n->release(getLexInfo());
  thenCode->release(getLexInfo());
  cond->release(getLexInfo());
  thenPart->release(getLexInfo());

  return ret;
}

// Value class

Value::Value(LexInfo *li) : Operation(li) { }

OperatorReturn Value::action(ExecutionEnvironment *ee) {
  Object *o;

  o = ee->stack.pop(getLexInfo());

  try {
    ee->stack.push(o->getNumber(getLexInfo(), ee));
    o->release(getLexInfo());
    return or_continue;
  } catch (Exception *e) { }

  try {
    ee->stack.push(o->getString(getLexInfo(), ee));
    o->release(getLexInfo());
    return or_continue;
  } catch (Exception *e) { }

  try {
    ee->stack.push(o->getPointer(getLexInfo(), ee));
    o->release(getLexInfo());
    return or_continue;
  } catch (Exception *e) { }

  try {
    ee->stack.push(o->getCode(getLexInfo(), ee));
    o->release(getLexInfo());
    return or_continue;
  } catch (Exception *e) { }

  slexception.chuck("value error", getLexInfo());

  return or_continue;
}

// ToName class

ToName::ToName(LexInfo *li) : Operation(li) { }

OperatorReturn ToName::action(ExecutionEnvironment *ee) {
  Object *o;
  Number *n;
  String *s;
  char buf[64];
  char fmt[32];

  o = ee->stack.pop(getLexInfo());

  try {
    n = o->getNumber(getLexInfo(), ee);
    if(n->isInt()) {
      sprintf(fmt, "%%%sd", PCTD);
      sprintf(buf, fmt, n->getInt());
    } else sprintf(buf, "%0.3f", n->getDouble());
    ee->stack.push(new Name(buf, &ee->cache));
    n->release(getLexInfo());
    o->release(getLexInfo());
    return or_continue;
  } catch (Exception *e) { }

  try {
    s = o->getString(getLexInfo(), ee);
    ee->stack.push(new Name(s->getValue(), &ee->cache));
    s->release(getLexInfo());
    o->release(getLexInfo());
    return or_continue;
  } catch(Exception *e) { }

  slexception.chuck("to name error", getLexInfo());

  return or_continue;
}

// Namespace class

Namespace::Namespace(LexInfo *li) : Operation(li) { }

OperatorReturn Namespace::action(ExecutionEnvironment *ee) {
  Object *nsobject;
  Object *inobject;
  Name *nsname;
  Number *nsnumber;
  String *nsstring;
  const char *nselementp;
  char nselement[1024];
  bool nsreleasestring;
  Name *inname;
  Number *innumber;
  String *instring;
  const char *inelementp;
  char inelement[1024];
  bool inreleasestring;
  bool found;
  char namebuf[1024];
  const char *p;
  int i;
  int j;
  char fmt[32];

  nsobject = ee->stack.pop(getLexInfo());
  inobject = ee->stack.pop(getLexInfo());

  found = false;
  nsreleasestring = false;

  try {
    nsname = nsobject->getName(getLexInfo(), ee);
    nselementp = nsname->getValue();
    found = true;
  } catch(Exception *e) { }

  if(! found) {
    try {
      nsnumber = nsobject->getNumber(getLexInfo(), ee);
      if(nsnumber->isInt()) {
        sprintf(fmt, "%%%sd", PCTD);
        sprintf(nselement, fmt, nsnumber->getInt());
      } else sprintf(nselement, "%0.3f", nsnumber->getDouble());
      nselementp = nselement;
      nsnumber->release(getLexInfo());
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) {
    try {
      nsstring = nsobject->getString(getLexInfo(), ee);
      nselementp = nsstring->getValue();
      nsreleasestring = true;
      found =  true;
    } catch(Exception *e) { }
  }

  if(! found) slexception.chuck("unknown namespace name", getLexInfo());

  found = false;
  inreleasestring = false;

  try {
    inname = inobject->getName(getLexInfo(), ee);
    inelementp = inname->getValue();
    found = true;
  } catch(Exception *e) { }

  if(! found) {
    try {
      innumber = inobject->getNumber(getLexInfo(), ee);
      if(innumber->isInt()) {
        sprintf(fmt, "%%%sd", PCTD);
        sprintf(inelement, fmt, innumber->getInt());
      } else sprintf(inelement, "%0.3f", innumber->getDouble());
      inelementp = inelement;
      innumber->release(getLexInfo());
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) {
    try {
      instring = inobject->getString(getLexInfo(), ee);
      inelementp = instring->getValue();
      inreleasestring = true;
      found =  true;
    } catch(Exception *e) { }
  }

  if(! found) slexception.chuck("unknown index name", getLexInfo());

  i = 0;
  p = inelementp;
  if(*p != '/') namebuf[i++] = '/';
  for(j = 0; (p[j] != 0) && (i < (MAX_NAME_LENGTH - 2)); j++, i++) namebuf[i] = p[j];
  if(p[j] != 0) slexception.chuck("name too long", getLexInfo());

  p = nselementp;
  if(*p != '/') namebuf[i++] = '/';
  for(j = 0; (p[j] != 0) && (i < (MAX_NAME_LENGTH - 1)); j++, i++) namebuf[i] = p[j];
  if(p[j] != 0) slexception.chuck("name too long", getLexInfo());

  namebuf[i] = 0;
  ee->stack.push(new Name(namebuf, &ee->cache));

  if(nsreleasestring) nsstring->release(getLexInfo());
  if(inreleasestring) instring->release(getLexInfo());

  inobject->release(getLexInfo());
  nsobject->release(getLexInfo());

  return or_continue;
}

// The Library class

Library::Library(LexInfo *li) : Operation(li) { }

OperatorReturn Library::action(ExecutionEnvironment *ee) {
  Object *o;
  Name *n;
  char pluginName[256];
  void *handle;
  void *(*slmain)();
  char *err;

  o = ee->stack.pop(getLexInfo());
  n = o->getName(getLexInfo(), ee);
  sprintf(pluginName, "/usr/local/lib/shale/%s.so", n->getValue());

  dlerror();

  handle = dlopen(pluginName, RTLD_NOW | RTLD_GLOBAL);
  if((err = dlerror()) != (char *) 0) slexception.chuck(err, getLexInfo());

  slmain = (void *(*)(void)) dlsym(handle, "slmain");
  if((err = dlerror()) != (char *) 0) slexception.chuck(err, getLexInfo());

  (*slmain)();

  o->release(getLexInfo());

  return or_continue;
}

// Execute class

Execute::Execute(LexInfo *li) : Operation(li) { }

OperatorReturn Execute::action(ExecutionEnvironment *ee) {
  Object *o;
  Code *code;
  OperatorReturn ret;

  o = ee->stack.pop(getLexInfo());
  code = o->getCode(getLexInfo(), ee);

  ret = code->action(ee);

  code->release(getLexInfo());
  o->release(getLexInfo());

  return ret;
}

// Print classes

Print::Print(bool nl, LexInfo *li) : Operation(li), newline(nl) { }

OperatorReturn Print::action(ExecutionEnvironment *ee) {
  Object *o;
  Number *n;
  String *s;
  bool found;
  char fmt[32];

  o = ee->stack.pop(getLexInfo());
  found = false;

  if(! found) {
    try {
      n = o->getNumber(getLexInfo(), ee);
      if(n->isInt()) {
        sprintf(fmt, "%%%sd", PCTD);
        printf(fmt, n->getInt());
      } else printf("%0.3f", n->getDouble());
      n->release(getLexInfo());
      found = true;
    } catch(Exception *e) {
    }
  }

  if(! found) {
    try {
      s = o->getString(getLexInfo(), ee);
      printf("%s", s->getValue());
      s->release(getLexInfo());
      found = true;
    } catch(Exception *e) {
    }
  }
  if(! found) slexception.chuck("print error", getLexInfo());

  if(found && newline) {
    printf("\n");
  }

  o->release(getLexInfo());

  return or_continue;
}

Printf::Printf(bool o, LexInfo *li) : Operation(li), output(o) { }

OperatorReturn Printf::action(ExecutionEnvironment *ee) {
  Object *formatObject;
  Object *o;
  String *format;
  Number *n;
  String *str;
  Name *name;
  const char *p;
  char *q;
  char buf[128];
  char res[10240];
  char *op;
  bool found;
  char fmt[32];

  formatObject = ee->stack.pop(getLexInfo());
  format = formatObject->getString(getLexInfo(), ee);

  op = res;
  *op = 0;

  for(p = format->getValue(); *p != 0; p++ ) {
    if(*p == '%') {
      q = buf;
      *q++ = *p++;
      while((*p != 0) && (*p != 'c') && (*p != 'd') && (*p != 'f') && (*p != 's') && (*p != 'x') && (*p != 'X') && (*p != 'p') && (*p != 'n') && (*p != '%')) *q++ = *p++;
      if(*p == 0) slexception.chuck("format error", getLexInfo());
      *q++ = *p;
      *q = 0;
      if(*p == '%') {
        *op++ = '%';
        *op = 0;
      } else {
        o = ee->stack.pop(getLexInfo());
        switch(*p) {
          case 'c':
            n = o->getNumber(getLexInfo(), ee);
            sprintf(op, buf, (char) n->getInt());
            n->release(getLexInfo());
            break;

          case 'd':
          case 'x':
          case 'X':
            q--;
            strcpy(q, PCTD);
            while(*q != 0) q++;
            *q++ = *p;
            *q = 0;
            n = o->getNumber(getLexInfo(), ee);
            sprintf(op, buf, n->getInt());
            n->release(getLexInfo());
            break;

          case 'f':
            n = o->getNumber(getLexInfo(), ee);
            sprintf(op, buf, n->getDouble());
            n->release(getLexInfo());
            break;

          case 's':
            str = o->getString(getLexInfo(), ee);
            sprintf(op, buf, str->getValue());
            str->release(getLexInfo());
            break;

          case 'p':
            found = false;
            try {
              n = o->getNumber(getLexInfo(), ee);
              if(n->isInt()) {
                sprintf(fmt, "%%%sd", PCTD);
                sprintf(op, fmt, n->getInt());
              } else sprintf(op, "%0.3f", n->getDouble());
              n->release(getLexInfo());
              found = true;
            } catch(Exception *e) { }

            if(! found) {
              try {
                str = o->getString(getLexInfo(), ee);
                sprintf(op, "%s", str->getValue());
                str->release(getLexInfo());
                found = true;
              } catch(Exception *e) { }
            }
            if(! found) slexception.chuck("print error", getLexInfo());
            break;

          case 'n':
            found = false;
            try {
              name = o->getName(getLexInfo(), ee);
              sprintf(op, "%s", name->getValue());
              found = true;
            } catch(Exception *e) { }
            if(! found) slexception.chuck("unknown %n type", getLexInfo());
            break;
        }
        while(*op != 0) op++;
        o->release(getLexInfo());
      }
    } else if(*p == '\\') {
      p++;
      if(*p == 'n') { *op++ = '\n'; *op = 0; }
      else { *op++ = *p; *op = 0; }
    } else {
      *op++ = *p;
      *op = 0;
    }
  }

  if(output) {
    printf("%s", res);
  } else {
    if((q = (char *) malloc(strlen(res) + 1)) == (char *) 0) slexception.chuck("malloc error", getLexInfo());
    strcpy(q, res);
    ee->stack.push(ee->cache.newString(q, true));
  }

  format->release(getLexInfo());
  formatObject->release(getLexInfo());

  return or_continue;
}

// Defined and Initialised classes

Defined::Defined(LexInfo *li) : Operation(li) { }

OperatorReturn Defined::action(ExecutionEnvironment *ee) {
  Object *o;
  Variable *v;
  INT n;
  Name *name;
  char *value;

  o = ee->stack.pop(getLexInfo());

  n = 1;
  if(o->isName()) {
    v = ee->variableStack.findVariable(o->getName(getLexInfo(), ee)->getValue());
    if(v == (Variable *) 0) n = 0;
  }
  ee->stack.push(ee->cache.newNumber(n));

  o->release(getLexInfo());

  return or_continue;
}

Initialised::Initialised(LexInfo *li) : Operation(li) { }

OperatorReturn Initialised::action(ExecutionEnvironment *ee) {
  Object *o;
  Variable *v;
  INT n;

  o = ee->stack.pop(getLexInfo());

  n = 1;
  if(o->isName()) {
    v = ee->variableStack.findVariable(o->getName(getLexInfo(), ee)->getValue());
    if((v == (Variable *) 0) || (! v->isInitialised())) n = 0;
  }
  ee->stack.push(ee->cache.newNumber(n));

  o->release(getLexInfo());

  return or_continue;
}

// PrintStack class

PrintStack::PrintStack(LexInfo *li) : Operation(li) { }

OperatorReturn PrintStack::action(ExecutionEnvironment *ee) {
  StackItem *si;
  Object *o;
  Name *na;
  String *s;
  Code *c;
  Pointer *p;
  Number *no;
  int i;
  bool found;
  char fmt[32];

  for(i = 0, si = ee->stack.getStack(); si != (StackItem *) 0; si = si->getDown(), i++) {
    printf("%d: ", i);

    o = si->getObject();
    found = false;

    try {
      na = o->getName(getLexInfo(), ee);
      printf("%s\n", na->getValue());
      found = true;
    } catch(Exception *e) { }

    if(! found) {
      try {
        s = o->getString(getLexInfo(), ee);
        printf("\"%s\"\n", s->getValue());
        s->release(getLexInfo());
        found = true;
      } catch(Exception *e) { }
    }

    if(! found) {
      try {
        c = o->getCode(getLexInfo(), ee);
        printf("{ ...code... }\n");
        c->release(getLexInfo());
        found = true;
      } catch(Exception *e) { }
    }

    if(! found) {
      try {
        p = o->getPointer(getLexInfo(), ee);
        printf("... pointer ...\n");
        p->release(getLexInfo());
        found = true;
      } catch(Exception *e) { }
    }

    if(! found) {
      try {
        no = o->getNumber(getLexInfo(), ee);
        if(no->isInt()) {
          sprintf(fmt, "%%%sd\n", PCTD);
          printf(fmt, no->getInt());
        } else printf("%0.3f\n", no->getDouble());
        no->release(getLexInfo());
        found = true;
      } catch(Exception *e) { }
    }

    if(! found) slexception.chuck("unknown stack entry", getLexInfo());
  }

  return or_continue;
}

// Debug class

Debug::Debug(LexInfo *li) : Operation(li) { }

OperatorReturn Debug::action(ExecutionEnvironment *ee) {
  ee->cache.debug();
  ee->stack.debug();
  btree.debug();

  return or_continue;
}

// BTree debug class

BTreeDebug::BTreeDebug(LexInfo *li) : Operation(li) { }

OperatorReturn BTreeDebug::action(ExecutionEnvironment *ee) {
  btree.print();

  return or_continue;
}

// Variable class

Variable::Variable(const char *n) : object((Object *) 0), next((Variable *) 0) {
  if((name = (char *) malloc(strlen(n) + 1)) == (char *) 0) slexception.chuck("variable error: malloc failed", (LexInfo *) 0);
  strcpy(name, n);
}

Variable::~Variable() {
  if(name != (char *) 0) free(name);
}

char *Variable::getName() {
  return name;
}

Object *Variable::getObject() {
  return object;
}

void Variable::setObject(Object *o) {
  if(object != (Object *) 0) object->release((LexInfo *) 0);
  object = o;
  object->hold();
}

bool Variable::isInitialised() {
  return object != (Object *) 0;
}

Variable *Variable::getNext() {
  return next;
}

void Variable::setNext(Variable *n) {
  next = n; 
}

// VariableStackItem class

VariableStackItem::VariableStackItem(VariableStackItem *d) : list((Variable *) 0), down(d) { }

VariableStackItem::~VariableStackItem() {
  Variable *t, *l = list;
  Object *o;

  while(l != (Variable *) 0) {
    o = l->getObject();
    if(o != (Object *) 0) o->release((LexInfo *) 0);
    t = l->getNext();
    delete(l);
    l = t;
  }
}

VariableStackItem *VariableStackItem::getDown() {
  return down;
}

void VariableStackItem::setDown(VariableStackItem *d) {
  down = d;
}

Variable *VariableStackItem::addVariable(char *v) {
  return addVariable(v, (LexInfo *) 0);
}

Variable *VariableStackItem::addVariable(char *v, LexInfo *li) {
  Variable *l;
  static char msg[64];

  if(v[0] == '/') {
    if(findVariable(v) == (Variable *) 0) {
      l = new Variable(v);
      btree.addVariable(l);
    } else {
      sprintf(msg, "variable %s already defined", v);
      slexception.chuck(msg, li);
    }
  } else {
    for(l = list; l != (Variable *) 0; l = l->getNext()) {
      if(strcmp(v, l->getName()) == 0) {
        sprintf(msg, "variable %s already defined", v);
        slexception.chuck(msg, li);
      }
    }
    l = new Variable(v);
    l->setNext(list);
    list = l;
  }

  return l;
}

Variable *VariableStackItem::findVariable(char *v) {
  Variable *l;

  if(v[0] == '/') {
    if((l = btree.findVariable(v)) != (Variable *) 0) return l;
  } else {
    for(l = list; l != (Variable *) 0; l = l->getNext()) if(strcmp(v, l->getName()) == 0) return l;
  }

  return (Variable *) 0;
}

// VariableStack class

VariableStack::VariableStack() : head((VariableStackItem *) 0) { }

void VariableStack::addVariableStack() {
  VariableStackItem *vsi = new VariableStackItem(head);
  head = vsi;
}

void VariableStack::popVariableStack() {
  VariableStackItem *vsi = head;
  if(vsi != (VariableStackItem *) 0) {
    head = vsi->getDown();
    delete(vsi);
  } else {
    slexception.chuck("variable stack error", (LexInfo *) 0);
  }
}

Variable *VariableStack::addVariable(char *n) {
  if(head != (VariableStackItem *) 0) return head->addVariable(n);
  slexception.chuck("variable stack error", (LexInfo *) 0);
  return (Variable *) 0;
}

Variable *VariableStack::addVariable(char *n, LexInfo *li) {
  if(head != (VariableStackItem *) 0) return head->addVariable(n, li);
  slexception.chuck("variable stack error", li);
  return (Variable *) 0;
}

Variable *VariableStack::findVariable(char *n) {
  Variable *v;
  VariableStackItem *vsi = head;

  if(*n == '/') {
    return btree.findVariable(n);
  } else {
    while(vsi != (VariableStackItem *) 0) {
      if((v = vsi->findVariable(n)) != (Variable *) 0) return v;
      vsi = vsi->getDown();
    }
  }
  return (Variable *) 0;
}

bool VariableStack::isEmpty() {
  return head == (VariableStackItem *) 0;
}

// The BTree classes

BTreeNode::BTreeNode(bool l) : leaf(l), number(0) { }

BTreeNode::BTreeNode(bool l, BTreeNode *left, Variable *d, BTreeNode *right) : leaf(l) {
  number = 1;
  pointer[0] = left;
  data[0] = d;
  pointer[1] = right;
}

bool BTreeNode::isLeaf() { return leaf; }

int BTreeNode::getNumber() { return number; }

Variable *BTreeNode::getData(int i) { return data[i]; }

void BTreeNode::setData(int i, Variable *d) { if((i >= 0) && (i < BTREE_DATA_COUNT)) data[i] = d; }

BTreeNode *BTreeNode::getPointer(int i) { return pointer[i]; }

void BTreeNode::setPointer(int i, BTreeNode *p) { if((i >= 0) && (i <= BTREE_DATA_COUNT)) pointer[i] = p; }

void BTreeNode::incNumber() { number++; }

bool BTreeNode::addData(Variable *d, BTreeNode **leftp, Variable **datap, BTreeNode **rightp, int *nodes) {
  BTreeNode *newLeft;
  Variable *newData;
  BTreeNode *newRight;
  int c;
  int n;
  bool ret;

  // Find where this data entry goes.
  for(n = 0; n < number; n++) {
    c = strcmp(d->getName(), data[n]->getName());
    if(c == 0) return false;
    if(c < 0) break;
  }
  // n is the spot where the new data must go.

  ret = true;

  if(leaf) {
    ret = addDataToNode((BTreeNode *) 0, d, (BTreeNode *) 0, leftp, datap, rightp, nodes);
  } else {
    newLeft = (BTreeNode *) 0;
    if(! pointer[n]->addData(d, &newLeft, &newData, &newRight, nodes)) return false;
    if(newLeft != (BTreeNode *) 0) {
      ret = addDataToNode(newLeft, newData, newRight, leftp, datap, rightp, nodes);
    }
  }

  return ret;
}

bool BTreeNode::addDataToNode(BTreeNode *l, Variable *d, BTreeNode *r, BTreeNode **leftp, Variable **datap, BTreeNode **rightp, int *nodes) {
  BTreeNode *newLeft;
  Variable *newData;
  BTreeNode *newRight;
  BTreeNode *bp;
  BTreeNode *lbp;
  BTreeNode *rbp;
  Variable *dp;
  int c;
  int n;
  int i;
  int j;

  // Find where this data entry goes.
  for(n = 0; n < number; n++) {
    c = strcmp(d->getName(), data[n]->getName());
    if(c == 0) return false;
    if(c < 0) break;
  }
  // n is the spot where the new data must go.

  if(leaf) {
    if(number == BTREE_DATA_COUNT) {
      // split leaf
      newLeft = new BTreeNode(true);
      newRight = new BTreeNode(true);
      (*nodes)++;
      for(i = 0; i <= BTREE_DATA_COUNT; i++) {
        if(i < n) dp = data[i];
        else if(i == n) dp = d;
        else dp = data[i - 1];
        if(i < (BTREE_DATA_COUNT / 2)) {
          newLeft->setData(i, dp);
          newLeft->incNumber();
        } else if(i == (BTREE_DATA_COUNT / 2)) {
          *datap = dp;
        } else {
          newRight->setData(i - (BTREE_DATA_COUNT / 2) - 1, dp);
          newRight->incNumber();
        }
        *leftp = newLeft;
        *rightp = newRight;
      }
    } else {
      for(i = number; i > n; i--) {
        data[i] = data[i - 1];
      }
      data[n] = d;
      number++;
    }
  } else {
    delete(pointer[n]);
    if(number == BTREE_DATA_COUNT) {
      // split non-leaf
      newLeft = new BTreeNode(false);
      newRight = new BTreeNode(false);
      (*nodes)++;
      for(i = 0; i <= BTREE_DATA_COUNT; i++) {
        if(i < n) {
          lbp = pointer[i];
          dp = data[i];
          if(i == (n - 1)) rbp = l;
          else rbp = pointer[i + 1];
        } else if(i == n) {
          lbp = l;
          dp = d;
          rbp = r;
        } else {
          if(i == (n + 1)) lbp = r;
          else lbp = pointer[i - 1];
          dp = data[i - 1];
          rbp = pointer[i];
        }
        if(i < (BTREE_DATA_COUNT / 2)) {
          newLeft->setPointer(i, lbp);
          newLeft->setData(i, dp);
          newLeft->incNumber();
          if(i == (BTREE_DATA_COUNT / 2 - 1)) newLeft->setPointer(i + 1, rbp);
        } else if(i == (BTREE_DATA_COUNT / 2)) {
          *datap = dp;
        } else {
          if(i == (BTREE_DATA_COUNT / 2 + 1)) newRight->setPointer(0, lbp);
          newRight->setData(i - BTREE_DATA_COUNT / 2 - 1, dp);
          newRight->setPointer(i - BTREE_DATA_COUNT / 2, rbp);
          newRight->incNumber();
        }
        *leftp = newLeft;
        *rightp = newRight;
      }
    } else {
      for(i = number + 1; i > n; i--) {
        if(i <= number) data[i] = data[i - 1];
        pointer[i] = pointer[i - 1];
      }
      pointer[n] = l;
      data[n] = d;
      pointer[n + 1] = r;
      number++;
    }
  }

  return true;
}

BTree::BTree() : tree((BTreeNode *) 0), depth(0), nodes(0), entries(0), mutex((pthread_rwlock_t *) 0) { }

bool BTree::addVariable(Variable *d) {
  BTreeNode *left;
  BTreeNode *right;
  Variable *data;

  if(mutex != (pthread_rwlock_t *) 0) pthread_rwlock_wrlock(mutex);

  if(tree == (BTreeNode *) 0) {
    tree = new BTreeNode(true, (BTreeNode *) 0, d, (BTreeNode *) 0);
    depth = 1;
    nodes = 1;
    entries = 1;
  } else {
    left = (BTreeNode *) 0;
    if(! tree->addData(d, &left, &data, &right, &nodes)) {
      if(mutex != (pthread_rwlock_t *) 0) pthread_rwlock_unlock(mutex);
      return false;
    }
    if(left != (BTreeNode *) 0) {
      delete(tree);
      tree = new BTreeNode(false, left, data, right);
      depth++;
      nodes++;
    }
    entries++;
  }

  if(mutex != (pthread_rwlock_t *) 0) pthread_rwlock_unlock(mutex);

  return true;
}

Variable *BTree::findVariable(const char *v) {
  BTreeNode *p;
  Variable *d;
  int c;
  int n;
  int i;

  if(tree == (BTreeNode *) 0) return (Variable *) 0;

  if(mutex != (pthread_rwlock_t *) 0) pthread_rwlock_rdlock(mutex);

  p = tree;
  while(p != (BTreeNode *) 0) {
    n = p->getNumber();
    for(i = 0; i < n; i++) {
      d = p->getData(i);
      c = strcmp(v, d->getName());
      if(c == 0) {
        if(mutex != (pthread_rwlock_t *) 0) pthread_rwlock_unlock(mutex);
        return d;
      }
      if(c < 0) {
        if(p->isLeaf()) {
          if(mutex != (pthread_rwlock_t *) 0) pthread_rwlock_unlock(mutex);
          return (Variable *) 0;
        }
        p = p->getPointer(i);
        break;
      }
    }
    if(i >= n) {
      if(p->isLeaf()) {
        if(mutex != (pthread_rwlock_t *) 0) pthread_rwlock_unlock(mutex);
        return (Variable *) 0;
      }
      p = p->getPointer(n);
    }
  }

  if(mutex != (pthread_rwlock_t *) 0) pthread_rwlock_unlock(mutex);

  return (Variable *) 0;
}

void BTree::setThreadSafe() {
  pthread_rwlockattr_t attr;

  mutex = new pthread_rwlock_t;
  pthread_rwlockattr_init(&attr);
  pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_PRIVATE);
#ifdef PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP
  pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
#else /* PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP */
#if defined __USE_POSIX199506 || defined __USE_UNIX98
  pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
#endif /* defined __USE_POSIX199506 || defined __USE_UNIX98 */
#endif /* PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP */
  pthread_rwlock_init(mutex, &attr);
}

void BTree::debug() {
  printf("BTree: depth %d, nodes %d, entries %d\n", depth, nodes, entries);
}

void BTree::print() {
  if(mutex != (pthread_rwlock_t *) 0) pthread_rwlock_rdlock(mutex);
  printTree(tree);
  if(mutex != (pthread_rwlock_t *) 0) pthread_rwlock_unlock(mutex);
}

void BTree::printTree(BTreeNode *t) {
  int n;

  for(n = 0; n < t->getNumber(); n++) {
    if(! t->isLeaf()) printTree(t->getPointer(n));
    printDetail(t, n);
  }
  if(! t->isLeaf()) printTree(t->getPointer(n));
}

void BTree::printDetail(BTreeNode *t, int i) {
  Variable *v;
  Object *o;
  Number *n;
  String *s;
  Pointer *p;
  Code *c;
  bool found;
  char fmt[32];

  v = t->getData(i);

  printf("%s: ", v->getName());
  o = v->getObject();
  if(o == (Object *) 0) {
    printf("...undefined...\n");
    return;
  }

  found = false;
  try {
    n = o->getNumber((LexInfo *) 0, (ExecutionEnvironment *) 0);
    if(n->isInt()) {
      sprintf(fmt, "%%%sd", PCTD);
      printf(fmt, n->getInt());
    } else printf("%0.3f", n->getDouble());
    n->release((LexInfo *) 0);
    found = true;
  } catch(Exception *e) { }

  if(! found) {
    try {
      s = o->getString((LexInfo *) 0, (ExecutionEnvironment *) 0);
      printf("\"%s\"", s->getValue());
      s->release((LexInfo *) 0);
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) {
    try {
      p = o->getPointer((LexInfo *) 0, (ExecutionEnvironment *) 0);
      printf("...pointer...");
      p->release((LexInfo *) 0);
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) {
    try {
      c = o->getCode((LexInfo *) 0, (ExecutionEnvironment *) 0);
      printf("{ ...code... }");
      c->release((LexInfo *) 0);
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) printf("...unknown");

  printf("\n");
}

// StackItem class

StackItem::StackItem() { }

Object *StackItem::getObject() {
  return object;
}

void StackItem::setObject(Object *o) {
  object = o;
}

StackItem *StackItem::getDown() {
  return down;
}

void StackItem::setDown(StackItem *d) { down = d; }

// Stack class

Stack::Stack() : stack((StackItem *) 0), used((StackItem *) 0), stackSize(0), usedSize(0) { }

void Stack::push(Object *o) {
  StackItem *si;

  if(used == (StackItem *) 0) {
    si = new StackItem;
  } else {
    si = used;
    used = si->getDown();
    usedSize--;
  }
  si->setObject(o);
  si->setDown(stack);
  stack = si;
  stackSize++;
}

Object *Stack::pop(LexInfo *li) {
  Object *o;
  StackItem *stackDown;

  if(stack == (StackItem *) 0) slexception.chuck("stack pop error", li);
  o = stack->getObject();
  stackDown = stack->getDown();
  stack->setDown(used);
  used = stack;
  stack = stackDown;
  stackSize--;
  usedSize++;

  return o;
}

StackItem *Stack::getStack() {
  return stack;
}

int Stack::getStackSize() {
  return stackSize;
}

int Stack::getUsedSize() {
  return usedSize;
}

void Stack::debug() {
  printf("Stack: depth %d, free %d\n", stackSize, usedSize);
}
