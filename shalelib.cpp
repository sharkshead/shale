#include "shalelib.h"

VariableStack variableStack;
BTree btree;
Stack stack;
Exception slexception;

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
    printf("Internal error: %s\n", message != (const char *) 0 ? message : "(no message)");
  }
}

// This implements a cache of pre-used objects to cut down on malloc()/free() calls.

ObjectBag::ObjectBag() : object((Object *) 0), next((ObjectBag *) 0) { }

CacheDebug::CacheDebug() : used(0), news(0) { }
void CacheDebug::incUsed() { used++; }
void CacheDebug::decUsed() { used--; }
void CacheDebug::incNew() { news++; }
void CacheDebug::debug() { printf("free %d, new %d", used, news); }

Cache::Cache() : usedNumbers((ObjectBag *) 0), usedStrings((ObjectBag *) 0), usedPointers((ObjectBag *) 0), unusedBags((ObjectBag *) 0) { }

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
    ret->setInt(i);
    numbers.decUsed();
    incUnused();
  } else {
    ret = new Number(i);
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
    ret->setDouble(d);
    numbers.decUsed();
    incUnused();
  } else {
    ret = new Number(d);
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
    ret->setString(s);
    ret->setRemoveStringFlag(rsf);
    strings.decUsed();
    incUnused();
  } else {
    ret = new String(s, rsf);
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
    ret->setObject(o);
    pointers.decUsed();
    incUnused();
  } else {
    ret = new Pointer(o);
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

Cache cache;

// Object class

Object::Object() : referenceCount(0) { }
Object::~Object() { }
Number *Object::getNumber(LexInfo *li) { slexception.chuck("number not found", li); return (Number *) 0; }
String *Object::getString(LexInfo *li) { slexception.chuck("string not found", li); return (String *) 0; }
bool Object::isName() { return false; }
Name *Object::getName(LexInfo *li) { slexception.chuck("name not found", li); return (Name *) 0; }
Code *Object::getCode(LexInfo *li) { slexception.chuck("code not found", li); return (Code *) 0; }
Pointer *Object::getPointer(LexInfo *li) { slexception.chuck("pointer not found", li); return (Pointer *) 0; }
void Object::hold() { referenceCount++; }
void Object::release(LexInfo *li) { if(referenceCount < 0) slexception.chuck("reference error", li); if(referenceCount == 0) delete(this); else referenceCount--; }

// Number class

Number::Number(INT i) : Object(), intRep(true), valueInt(i) { }
Number::Number(double d) : Object(), intRep(false), valueDouble(d) { }
Number *Number::getNumber(LexInfo *li) { this->hold(); return this; }
bool Number::isInt() { return intRep; }
INT Number::getInt() { return (intRep ? valueInt : valueDouble); }
void Number::setInt(INT i) { intRep = true; valueInt = i; }
double Number::getDouble() { return (intRep ? valueInt : valueDouble); }
void Number::setDouble(double d) { intRep = false; valueDouble = d; }
void Number::release(LexInfo *li) { if(referenceCount < 0) slexception.chuck("reference error", li); if(referenceCount == 0) cache.deleteNumber(this); else referenceCount--; }
void Number::debug() { printf("Number: "); if(intRep) printf("%ld\n", valueInt); else printf("%0.3f\n", valueDouble); }

// String class

String::String(const char *s) : Object(), str(s), removeStringFlag(false) { }
String::String(const char *s, bool rsf) : Object(), str(s), removeStringFlag(rsf) { }
String::~String() { if(removeStringFlag) free((void *) str); }
String *String::getString(LexInfo *li) { this->hold(); return this; }
const char *String::getValue() { return str; }
bool String::getRemoveStringFlag() { return removeStringFlag; }
void String::setString(const char *s) { str = s; }
void String::setRemoveStringFlag(bool rsf) { removeStringFlag = rsf; }
void String::release(LexInfo *li) { if(referenceCount < 0) slexception.chuck("reference error", li); if(referenceCount == 0) cache.deleteString(this); else referenceCount--; }
void String::debug() { printf("String: %s\n", str); }

// Name class

Name::Name(const char *n) : Object() {
  int i;

  for(i = 0; (i < 31) && (n[i] != 0); i++) {
    name[i] = n[i];
  }
  name[i] = 0;
}

bool Name::isName() {
  return true;
}

Name *Name::getName(LexInfo *li) {
  return this;
}

char *Name::getValue() {
  return name;
}

Number *Name::getNumber(LexInfo *li) {
  static char buf[128];
  Variable *v = variableStack.findVariable(name);
  if(v == (Variable *) 0) { sprintf(buf, "variable error: %s not found", name); slexception.chuck(buf, li); }
  if(! v->isInitialised()) { sprintf(buf, "variable error: %s not initialised", name); slexception.chuck(buf, li); }
  return v->getObject()->getNumber(li);
}

String *Name::getString(LexInfo *li) {
  static char buf[128];
  Variable *v = variableStack.findVariable(name);
  if(v == (Variable *) 0) { sprintf(buf, "variable error: %s not found", name); slexception.chuck(buf, li); }
  if(! v->isInitialised()) { sprintf(buf, "variable error: %s not initialised", name); slexception.chuck(buf, li); }
  return v->getObject()->getString(li);
}

Code *Name::getCode(LexInfo *li) {
  static char buf[128];
  Variable *v = variableStack.findVariable(name);
  if(v == (Variable *) 0) { sprintf(buf, "variable error: %s not found", name); slexception.chuck(buf, li); }
  if(! v->isInitialised()) { sprintf(buf, "variable error: %s not initialised", name); slexception.chuck(buf, li); }
  return v->getObject()->getCode(li);
}

Pointer *Name::getPointer(LexInfo *li) {
  static char buf[128];
  Variable *v = variableStack.findVariable(name);
  if(v == (Variable *) 0) { sprintf(buf, "variable error: %s not found", name); slexception.chuck(buf, li); }
  if(! v->isInitialised()) { sprintf(buf, "variable error: %s not initialised", name); slexception.chuck(buf, li); }
  return v->getObject()->getPointer(li);
}

void Name::debug() { printf("Name: %s\n", name); }

// Code class

Code::Code(OperationList *ol) : Object(), operationList(ol) { }
Code *Code::getCode(LexInfo *li) { this->hold(); return this; }
OperationList *Code::getOperationList() { return operationList; }
bool Code::action() { return operationList->action(); }
void Code::debug() { printf("Code\n"); }

// Pointer class

Pointer::Pointer(Object *o) : Object(), object(o) { object->hold(); }
Pointer *Pointer::getPointer(LexInfo *li) { this->hold(); return this; }
void Pointer::setObject(Object *o) { object = o; object->hold(); }
Object *Pointer::getObject() { return object; }
void Pointer::hold() { Object::hold(); if(object != (Object *) 0) object->hold(); }
void Pointer::release(LexInfo *li) {
  if(referenceCount < 0) slexception.chuck("reference error", li);
  if(object != (Object *) 0) object->release(li);
  if(referenceCount == 0) {
    cache.deletePointer(this);
  } else referenceCount--;
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

OperationList::OperationList() : head((OperationListItem *) 0), tail((OperationListItem *) 0), newVariableStack(false) { }

void OperationList::addOperation(Operation *op) {
  OperationListItem *oli = new OperationListItem(op);
  if(head == (OperationListItem *) 0) { head = tail = oli; }
  else { tail->setNext(oli); tail = oli; }
  if(op->isVar()) newVariableStack = true;
}

OperationListItem *OperationList::getOperationList() {
  return head;
}

bool OperationList::action() {
  bool ret;

  ret = true;
  if(newVariableStack) variableStack.addVariableStack();
  for(OperationListItem *oli = head; oli != (OperationListItem *) 0; oli = oli->getNext()) {
    if(! oli->getOperation()->action()) {
      ret = false;
      break;
    }
  }
  if(newVariableStack) variableStack.popVariableStack();
  return ret;
}

bool OperationList::actionLatest() {
  OperationListItem *oli;
  bool ret;

  ret = true;
  if(newVariableStack && variableStack.isEmpty()) variableStack.addVariableStack();
  oli = tail;
  if(oli != (OperationListItem *) 0) {
    if(! oli->getOperation()->action()) {
      ret = false;
    }
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

bool Push::action() {
  object->hold();
  stack.push(object);
  return true;
}

// Stack classes

Pop::Pop(LexInfo *li) : Operation(li) { }

bool Pop::action() {
  Object *o;

  o = stack.pop(getLexInfo());
  o->release(getLexInfo());

  return true;
}

Swap::Swap(LexInfo *li) : Operation(li) { }

bool Swap::action() {
  Object *o1;
  Object *o2;

  o1 = stack.pop(getLexInfo());
  o2 = stack.pop(getLexInfo());

  stack.push(o1);
  stack.push(o2);

  return true;
}

Dup::Dup(LexInfo *li) : Operation(li) { }

bool Dup::action() {
  Object *o;

  o = stack.pop(getLexInfo());
  o->hold();
  stack.push(o);
  stack.push(o);

  return true;
}

// Arithmetic classes

Int::Int(LexInfo *li) : Operation(li) { }

bool Int::action() {
  Object *o;
  Number *n;

  o = stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo());
  stack.push(cache.newNumber(n->getInt()));
  n->release(getLexInfo());
  o->release(getLexInfo());

  return true;
}

Double::Double(LexInfo *li) : Operation(li) { }

bool Double::action() {
  Object *o;
  Number *n;

  o = stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo());
  stack.push(cache.newNumber(n->getDouble()));
  n->release(getLexInfo());
  o->release(getLexInfo());

  return true;
}

Plus::Plus(LexInfo *li) : Operation(li) { }

bool Plus::action() {
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

  o2 = stack.pop(getLexInfo());
  o1 = stack.pop(getLexInfo());

  found = false;

  try {
    n1 = o1->getNumber(getLexInfo());
    n2 = o2->getNumber(getLexInfo());

    if(n1->isInt()) {
      if(n2->isInt()) res = cache.newNumber(n1->getInt() + n2->getInt());
      else res = cache.newNumber(n1->getInt() + n2->getDouble());
    } else {
      if(n2->isInt()) res = cache.newNumber(n1->getDouble() + n2->getInt());
      else res = cache.newNumber(n1->getDouble() + n2->getDouble());
    }
    stack.push(res);

    n1->release(getLexInfo());
    n2->release(getLexInfo());

    found = true;
  } catch (Exception *e) { }

  if(! found) {
    try {
      c1 = o1->getCode(getLexInfo());
      c2 = o2->getCode(getLexInfo());

      ol = new OperationList;
      for(oli = c1->getOperationList()->getOperationList(); oli != (OperationListItem *) 0; oli = oli->getNext()) {
        ol->addOperation(oli->getOperation());
      }
      for(oli = c2->getOperationList()->getOperationList(); oli != (OperationListItem *) 0; oli = oli->getNext()) {
        ol->addOperation(oli->getOperation());
      }
      stack.push(new Code(ol));

      c1->release(getLexInfo());
      c2->release(getLexInfo());

      found = true;
    } catch(Exception *e) { }
  }

  if(! found) slexception.chuck("unknown operands", getLexInfo());

  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return true;
}

Minus::Minus(LexInfo *li) : Operation(li) { }

bool Minus::action() {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;
  Number *res;

  o2 = stack.pop(getLexInfo());
  o1 = stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo());
  n2 = o2->getNumber(getLexInfo());

  if(n1->isInt()) {
    if(n2->isInt()) res = cache.newNumber(n1->getInt() - n2->getInt());
    else res = cache.newNumber(n1->getInt() - n2->getDouble());
  } else {
    if(n2->isInt()) res = cache.newNumber(n1->getDouble() - n2->getInt());
    else res = cache.newNumber(n1->getDouble() - n2->getDouble());
  }
  stack.push(res);

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return true;
}

Times::Times(LexInfo *li) : Operation(li) { }

bool Times::action() {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;
  Number *res;

  o2 = stack.pop(getLexInfo());
  o1 = stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo());
  n2 = o2->getNumber(getLexInfo());

  if(n1->isInt()) {
    if(n2->isInt()) res = cache.newNumber(n1->getInt() * n2->getInt());
    else res = cache.newNumber(n1->getInt() * n2->getDouble());
  } else {
    if(n2->isInt()) res = cache.newNumber(n1->getDouble() * n2->getInt());
    else res = cache.newNumber(n1->getDouble() * n2->getDouble());
  }
  stack.push(res);

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return true;
}

Divide::Divide(LexInfo *li) : Operation(li) { }

bool Divide::action() {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;
  Number *res;

  o2 = stack.pop(getLexInfo());
  o1 = stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo());
  n2 = o2->getNumber(getLexInfo());

  if(n1->isInt()) {
    if(n2->isInt()) res = cache.newNumber(n1->getInt() / n2->getInt());
    else res = cache.newNumber(n1->getInt() / n2->getDouble());
  } else {
    if(n2->isInt()) res = cache.newNumber(n1->getDouble() / n2->getInt());
    else res = cache.newNumber(n1->getDouble() / n2->getDouble());
  }
  stack.push(res);

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return true;
}

BitwiseAnd::BitwiseAnd(LexInfo *li) : Operation(li) { }

bool BitwiseAnd::action() {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;

  o2 = stack.pop(getLexInfo());
  o1 = stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo());
  n2 = o2->getNumber(getLexInfo());

  stack.push(cache.newNumber(n1->getInt() & n2->getInt()));

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return true;
}

BitwiseOr::BitwiseOr(LexInfo *li) : Operation(li) { }

bool BitwiseOr::action() {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;

  o2 = stack.pop(getLexInfo());
  o1 = stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo());
  n2 = o2->getNumber(getLexInfo());

  stack.push(cache.newNumber(n1->getInt() | n2->getInt()));

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return true;
}

BitwiseXor::BitwiseXor(LexInfo *li) : Operation(li) { }

bool BitwiseXor::action() {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;

  o2 = stack.pop(getLexInfo());
  o1 = stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo());
  n2 = o2->getNumber(getLexInfo());

  stack.push(cache.newNumber(n1->getInt() ^ n2->getInt()));

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return true;
}

BitwiseNot::BitwiseNot(LexInfo *li) : Operation(li) { }

bool BitwiseNot::action() {
  Object *o1;
  Number *n1;

  o1 = stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo());

  stack.push(cache.newNumber(~n1->getInt()));

  n1->release(getLexInfo());
  o1->release(getLexInfo());

  return true;
}

// Var class

Var::Var(LexInfo *li) : Operation(li) { }

bool Var::action() {
  Object *o;

  o = stack.pop(getLexInfo());
  variableStack.addVariable(o->getName(getLexInfo())->getValue(), getLexInfo());
  o->release(getLexInfo());

  return true;
}

bool Var::isVar() { return true; }

// Assign class

Assign::Assign(LexInfo *li) : Operation(li) { }

bool Assign::action() {
  Object *var;
  Object *val;
  Number *number;
  String *string;
  Code *code;
  Pointer *pointer;
  Object *o;
  Variable *v;
  bool found;

  val = stack.pop(getLexInfo());
  var = stack.pop(getLexInfo());
  found = false;

  // Is this a variable we're assigning?
  try {
    v = variableStack.findVariable(var->getName(getLexInfo())->getValue());
    if(v != (Variable *) 0) {
      try {
        number = val->getNumber(getLexInfo());
        v->setObject(number);
        number->release(getLexInfo());
        found = true;
      } catch(Exception *e) { }

      if(! found) {
        try {
          string = val->getString(getLexInfo());
          v->setObject(string);
          string->release(getLexInfo());
          found = true;
        } catch(Exception *e) { }
      }

      if(! found) {
        try {
          code = val->getCode(getLexInfo());
          v->setObject(code);
          code->release(getLexInfo());
          found = true;
        } catch(Exception *e) { }
      }

      if(! found) {
        try {
          pointer = val->getPointer(getLexInfo());
          v->setObject(pointer);
          pointer->release(getLexInfo());
          found = true;
        } catch(Exception *e) { }
      }
    }
  } catch(Exception *e) { }

  if(! found) slexception.chuck("assignment error", getLexInfo());

  val->release(getLexInfo());
  var->release(getLexInfo());

  return true;
}

// PointerAssign class

PointerAssign::PointerAssign(LexInfo *li) : Operation(li) { }

bool PointerAssign::action() {
  Object *var;
  Object *val;
  Variable *v;
  Pointer *p;
  bool found;

  val = stack.pop(getLexInfo());
  var = stack.pop(getLexInfo());
  found = false;

  try {
    v = variableStack.findVariable(var->getName(getLexInfo())->getValue());
    if(v != (Variable *) 0) {
      p = cache.newPointer(val);
      v->setObject(p);
      p->release(getLexInfo());
      found = true;
    }
  } catch(Exception *e) { }

  if(! found) slexception.chuck("pointer assignment error", getLexInfo());

  val->release(getLexInfo());
  var->release(getLexInfo());

  return true;
}

// PointerDereference class

PointerDereference::PointerDereference(LexInfo *li) : Operation(li) { }

bool PointerDereference::action() {
  Object *var;
  Pointer *p;
  Variable *v;
  Object *o;
  bool found;

  var = stack.pop(getLexInfo());
  found = false;

  try {
    v = variableStack.findVariable(var->getName(getLexInfo())->getValue());
    if(v != (Variable *) 0) {
      p = v->getObject()->getPointer(getLexInfo());
      o = p->getObject();
      o->hold();
      stack.push(o);
      p->release(getLexInfo());
      found = true;
    }
  } catch(Exception *e) { }

  if(! found) slexception.chuck("pointer dereference error", getLexInfo());

  var->release(getLexInfo());

  return true;
}

// Logical operators

LessThan::LessThan(LexInfo *li) : Operation(li) { }

bool LessThan::action() {
  Object *a;
  Object *b;
  Number *an;
  Number *bn;
  bool r;

  b = stack.pop(getLexInfo());
  a = stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo());
  bn = b->getNumber(getLexInfo());

  if(an->isInt()) {
    if(bn->isInt()) r = (an->getInt() < bn->getInt());
    else r = (an->getInt() < bn->getDouble());
  } else {
    if(bn->isInt()) r = (an->getDouble() < bn->getInt());
    else r = (an->getDouble() < bn->getDouble());
  }
  stack.push(cache.newNumber(r ? (INT) 1 : (INT) 0));

  an->release(getLexInfo());
  bn->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return true;
}

LessThanOrEquals::LessThanOrEquals(LexInfo *li) : Operation(li) { }

bool LessThanOrEquals::action() {
  Object *a;
  Object *b;
  Number *an;
  Number *bn;
  bool r;

  b = stack.pop(getLexInfo());
  a = stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo());
  bn = b->getNumber(getLexInfo());

  if(an->isInt()) {
    if(bn->isInt()) r = (an->getInt() <= bn->getInt());
    else r = (an->getInt() <= bn->getDouble());
  } else {
    if(bn->isInt()) r = (an->getDouble() <= bn->getInt());
    else r = (an->getDouble() <= bn->getDouble());
  }
  stack.push(cache.newNumber(r ? (INT) 1 : (INT) 0));

  an->release(getLexInfo());
  bn->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return true;
}

Equals::Equals(LexInfo *li) : Operation(li) { }

bool Equals::action() {
  Object *a;
  Object *b;
  Number *an;
  Number *bn;
  bool r;

  b = stack.pop(getLexInfo());
  a = stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo());
  bn = b->getNumber(getLexInfo());

  if(an->isInt()) {
    if(bn->isInt()) r = (an->getInt() == bn->getInt());
    else r = (an->getInt() == bn->getDouble());
  } else {
    if(bn->isInt()) r = (an->getDouble() == bn->getInt());
    else r = (an->getDouble() == bn->getDouble());
  }
  stack.push(cache.newNumber(r ? (INT) 1 : (INT) 0));

  an->release(getLexInfo());
  bn->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return true;
}

NotEquals::NotEquals(LexInfo *li) : Operation(li) { }

bool NotEquals::action() {
  Object *a;
  Object *b;
  Number *an;
  Number *bn;
  bool r;

  b = stack.pop(getLexInfo());
  a = stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo());
  bn = b->getNumber(getLexInfo());

  if(an->isInt()) {
    if(bn->isInt()) r = (an->getInt() != bn->getInt());
    else r = (an->getInt() != bn->getDouble());
  } else {
    if(bn->isInt()) r = (an->getDouble() != bn->getInt());
    else r = (an->getDouble() != bn->getDouble());
  }
  stack.push(cache.newNumber(r ? (INT) 1 : (INT) 0));

  an->release(getLexInfo());
  bn->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return true;
}

GreaterThanOrEquals::GreaterThanOrEquals(LexInfo *li) : Operation(li) { }

bool GreaterThanOrEquals::action() {
  Object *a;
  Object *b;
  Number *an;
  Number *bn;
  bool r;

  b = stack.pop(getLexInfo());
  a = stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo());
  bn = b->getNumber(getLexInfo());

  if(an->isInt()) {
    if(bn->isInt()) r = (an->getInt() >= bn->getInt());
    else r = (an->getInt() >= bn->getDouble());
  } else {
    if(bn->isInt()) r = (an->getDouble() >= bn->getInt());
    else r = (an->getDouble() >= bn->getDouble());
  }
  stack.push(cache.newNumber(r ? (INT) 1 : (INT) 0));

  an->release(getLexInfo());
  bn->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return true;
}

GreaterThan::GreaterThan(LexInfo *li) : Operation(li) { }

bool GreaterThan::action() {
  Object *a;
  Object *b;
  Number *an;
  Number *bn;
  bool r;

  b = stack.pop(getLexInfo());
  a = stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo());
  bn = b->getNumber(getLexInfo());

  if(an->isInt()) {
    if(bn->isInt()) r = (an->getInt() > bn->getInt());
    else r = (an->getInt() > bn->getDouble());
  } else {
    if(bn->isInt()) r = (an->getDouble() > bn->getInt());
    else r = (an->getDouble() > bn->getDouble());
  }
  stack.push(cache.newNumber(r ? (INT) 1 : (INT) 0));

  an->release(getLexInfo());
  bn->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return true;
}

LogicalAnd::LogicalAnd(LexInfo *li) : Operation(li) { }

bool LogicalAnd::action() {
  Object *a;
  Object *b;
  Object *co;
  Number *an;
  Number *bn;
  Code *c;
  bool found;
  bool r;

  b = stack.pop(getLexInfo());
  a = stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo());

  r = (an->getInt() != 0);
  if(r) {
    try {
      found = false;
      c = b->getCode(getLexInfo());
      c->action();
      co = stack.pop(getLexInfo());
      bn = co->getNumber(getLexInfo());
      co->release(getLexInfo());
      c->release(getLexInfo());
      found = true;
    } catch(Exception *e) { }

    if(! found) {
      bn = b->getNumber(getLexInfo());
    }
    r = (bn->getInt() != 0);
    bn->release(getLexInfo());
  }

  stack.push(cache.newNumber(r ? (INT) 1 : (INT) 0));

  an->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return true;
}

LogicalOr::LogicalOr(LexInfo *li) : Operation(li) { }

bool LogicalOr::action() {
  Object *a;
  Object *b;
  Object *co;
  Number *an;
  Number *bn;
  Code *c;
  bool found;
  bool r;

  b = stack.pop(getLexInfo());
  a = stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo());

  r = (an->getInt() != 0);
  if(! r) {
    try {
      found = false;
      c = b->getCode(getLexInfo());
      c->action();
      co = stack.pop(getLexInfo());
      bn = co->getNumber(getLexInfo());
      co->release(getLexInfo());
      c->release(getLexInfo());
      found = true;
    } catch(Exception *e) { }

    if(! found) {
      bn = b->getNumber(getLexInfo());
    }
    r = (bn->getInt() != 0);
    bn->release(getLexInfo());
  }

  stack.push(cache.newNumber(r ? (INT) 1 : (INT) 0));

  an->release(getLexInfo());
  a->release(getLexInfo());
  b->release(getLexInfo());

  return true;
}

LogicalNot::LogicalNot(LexInfo *li) : Operation(li) { }

bool LogicalNot::action() {
  Object *a;
  Number *an;
  bool r;

  a = stack.pop(getLexInfo());
  an = a->getNumber(getLexInfo());

  if(an->isInt()) r = ! an->getInt();
  else r = ! an->getDouble();
  stack.push(cache.newNumber(r ? (INT) 1 : (INT) 0));

  an->release(getLexInfo());
  a->release(getLexInfo());

  return true;
}

True::True(LexInfo *li) : Operation(li) { }

bool True::action() {
  stack.push(cache.newNumber((INT) 1));

  return true;
}

False::False(LexInfo *li) : Operation(li) { }

bool False::action() {
  stack.push(cache.newNumber((INT) 0));

  return true;
}

// Break class

Break::Break(LexInfo *li) : Operation(li) { }

bool Break::action() {
  return false;
}

// Exit class

Exit::Exit(LexInfo *li) : Operation(li) { }

bool Exit::action() {
  Object *o;
  Number *n;

  o = stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo());
  exit(n->getInt());

  return true;
}

// While class

While::While(LexInfo *li) : Operation(li) { }

bool While::action() {
  Object *cond;
  Object *body;
  Code *condCode;
  Code *bodyCode;
  Object *c;
  Number *n;
  INT num;

  body = stack.pop(getLexInfo());
  cond = stack.pop(getLexInfo());
  condCode = cond->getCode(getLexInfo());
  bodyCode = body->getCode(getLexInfo());

  while(true) {
    condCode->action();
    c = stack.pop(getLexInfo());
    n = c->getNumber(getLexInfo());
    num = n->getInt();
    n->release(getLexInfo());
    c->release(getLexInfo());
    if(num == 0) break;
    if(! bodyCode->action()) break;
  }

  condCode->release(getLexInfo());
  bodyCode->release(getLexInfo());
  cond->release(getLexInfo());
  body->release(getLexInfo());

  return true;
}

// If class

If::If(LexInfo *li) : Operation(li) { }

bool If::action() {
  Object *cond;
  Object *thenPart;
  Object *elsePart;
  Code *thenCode;
  Code *elseCode;
  Number *n;
  bool ret;

  elsePart = stack.pop(getLexInfo());
  thenPart = stack.pop(getLexInfo());
  cond = stack.pop(getLexInfo());
  thenCode = thenPart->getCode(getLexInfo());
  elseCode = elsePart->getCode(getLexInfo());
  n = cond->getNumber(getLexInfo());

  if(n->isInt()) {
    if(n->getInt() == 0) ret = elseCode->action();
    else ret = thenCode->action();
  } else {
    if(n->getDouble() == 0) ret = elseCode->action();
    else ret = thenCode->action();
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

bool IfThen::action() {
  Object *cond;
  Object *thenPart;
  Code *thenCode;
  Number *n;
  bool ret;

  thenPart = stack.pop(getLexInfo());
  cond = stack.pop(getLexInfo());
  thenCode = thenPart->getCode(getLexInfo());
  n = cond->getNumber(getLexInfo());
  ret = true;

  if(n->isInt()) {
    if(n->getInt() != 0) ret = thenCode->action();
  } else {
    if(n->getDouble() != 0) ret = thenCode->action();
  }

  n->release(getLexInfo());
  thenCode->release(getLexInfo());
  cond->release(getLexInfo());
  thenPart->release(getLexInfo());

  return ret;
}

// Value class

Value::Value(LexInfo *li) : Operation(li) { }

bool Value::action() {
  Object *o;

  o = stack.pop(getLexInfo());

  try {
    stack.push(o->getNumber(getLexInfo()));
    o->release(getLexInfo());
    return true;
  } catch (Exception *e) { }

  try {
    stack.push(o->getString(getLexInfo()));
    o->release(getLexInfo());
    return true;
  } catch (Exception *e) { }

  try {
    stack.push(o->getCode(getLexInfo()));
    o->release(getLexInfo());
    return true;
  } catch (Exception *e) { }

  slexception.chuck("value error", getLexInfo());

  return false;
}

// ToName class

ToName::ToName(LexInfo *li) : Operation(li) { }

bool ToName::action() {
  Object *o;
  Number *n;
  String *s;
  char buf[64];

  o = stack.pop(getLexInfo());

  try {
    n = o->getNumber(getLexInfo());
    if(n->isInt()) sprintf(buf, "%ld", n->getInt());
    else sprintf(buf, "%0.3f", n->getDouble());
    stack.push(new Name(buf));
    n->release(getLexInfo());
    o->release(getLexInfo());
    return true;
  } catch (Exception *e) { }

  try {
    s = o->getString(getLexInfo());
    stack.push(new Name(s->getValue()));
    s->release(getLexInfo());
    o->release(getLexInfo());
    return true;
  } catch(Exception *e) { }

  slexception.chuck("to name error", getLexInfo());

  return false;
}

// Namespace class

Namespace::Namespace(LexInfo *li) : Operation(li) { }

bool Namespace::action() {
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

  nsobject = stack.pop(getLexInfo());
  inobject = stack.pop(getLexInfo());

  found = false;
  nsreleasestring = false;

  try {
    nsname = nsobject->getName(getLexInfo());
    nselementp = nsname->getValue();
    found = true;
  } catch(Exception *e) { }

  if(! found) {
    try {
      nsnumber = nsobject->getNumber(getLexInfo());
      if(nsnumber->isInt()) sprintf(nselement, "%ld", nsnumber->getInt());
      else sprintf(nselement, "%0.3f", nsnumber->getDouble());
      nselementp = nselement;
      nsnumber->release(getLexInfo());
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) {
    try {
      nsstring = nsobject->getString(getLexInfo());
      nselementp = nsstring->getValue();
      nsreleasestring = true;
      found =  true;
    } catch(Exception *e) { }
  }

  if(! found) slexception.chuck("unknown namespace name", getLexInfo());

  found = false;
  inreleasestring = false;

  try {
    inname = inobject->getName(getLexInfo());
    inelementp = inname->getValue();
    found = true;
  } catch(Exception *e) { }

  if(! found) {
    try {
      innumber = inobject->getNumber(getLexInfo());
      if(innumber->isInt()) sprintf(inelement, "%ld", innumber->getInt());
      else sprintf(inelement, "%0.3f", innumber->getDouble());
      inelementp = inelement;
      innumber->release(getLexInfo());
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) {
    try {
      instring = inobject->getString(getLexInfo());
      inelementp = instring->getValue();
      inreleasestring = true;
      found =  true;
    } catch(Exception *e) { }
  }

  if(! found) slexception.chuck("unknown index name", getLexInfo());

  i = 0;
  p = inelementp;
  if(*p != '/') namebuf[i++] = '/';
  for(j = 0; (p[j] != 0) && (i < 30); j++, i++) namebuf[i] = p[j];
  if(p[j] != 0) slexception.chuck("name too long", getLexInfo());

  p = nselementp;
  if(*p != '/') namebuf[i++] = '/';
  for(j = 0; (p[j] != 0) && (i < 31); j++, i++) namebuf[i] = p[j];
  if(p[j] != 0) slexception.chuck("name too long", getLexInfo());

  namebuf[i] = 0;
  stack.push(new Name(namebuf));

  if(nsreleasestring) nsstring->release(getLexInfo());
  if(inreleasestring) instring->release(getLexInfo());

  inobject->release(getLexInfo());
  nsobject->release(getLexInfo());

  return true;
}

// Execute class

Execute::Execute(LexInfo *li) : Operation(li) { }

bool Execute::action() {
  Object *o;
  Code *code;
  bool ret;

  o = stack.pop(getLexInfo());
  code = o->getCode(getLexInfo());

  ret = code->action();

  code->release(getLexInfo());
  o->release(getLexInfo());

  return ret;
}

// Print classes

Print::Print(bool nl, LexInfo *li) : Operation(li), newline(nl) { }

bool Print::action() {
  Object *o;
  Number *n;
  String *s;
  bool found;

  o = stack.pop(getLexInfo());
  found = false;

  if(! found) {
    try {
      n = o->getNumber(getLexInfo());
      if(n->isInt()) printf("%ld", n->getInt());
      else printf("%0.3f", n->getDouble());
      n->release(getLexInfo());
      found = true;
    } catch(Exception *e) {
    }
  }

  if(! found) {
    try {
      s = o->getString(getLexInfo());
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

  return true;
}

Printf::Printf(bool o, LexInfo *li) : Operation(li), output(o) { }

bool Printf::action() {
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

  formatObject = stack.pop(getLexInfo());
  format = formatObject->getString(getLexInfo());

  op = res;
  *op = 0;

  for(p = format->getValue(); *p != 0; p++ ) {
    if(*p == '%') {
      q = buf;
      *q++ = *p++;
      while((*p != 0) && (*p != 'd') && (*p != 'f') && (*p != 's') && (*p != 't') && (*p != 'x') && (*p != 'X') && (*p != 'p') && (*p != 'n') && (*p != '%')) *q++ = *p++;
      if(*p == 0) slexception.chuck("format error", getLexInfo());
      *q++ = *p;
      *q = 0;
      if(*p == '%') {
        *op++ = '%';
        *op = 0;
      } else {
        o = stack.pop(getLexInfo());
        switch(*p) {
          case 'd':
          case 'x':
          case 'X':
            q--;
            *q++ = 'l';
            *q++ = *p;
            *q = 0;
            n = o->getNumber(getLexInfo());
            sprintf(op, buf, n->getInt());
            n->release(getLexInfo());
            break;

          case 'f':
            n = o->getNumber(getLexInfo());
            sprintf(op, buf, n->getDouble());
            n->release(getLexInfo());
            break;

          case 's':
            str = o->getString(getLexInfo());
            sprintf(op, buf, str->getValue());
            str->release(getLexInfo());
            break;

          case 'p':
            found = false;
            try {
              n = o->getNumber(getLexInfo());
              if(n->isInt()) sprintf(op, "%ld", n->getInt());
              else sprintf(op, "%03f", n->getDouble());
              n->release(getLexInfo());
              found = true;
            } catch(Exception *e) { }

            if(! found) {
              try {
                str = o->getString(getLexInfo());
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
              name = o->getName(getLexInfo());
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
      else { *op++ = '\\'; *op++ = *p; *op = 0; }
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
    stack.push(cache.newString(q, true));
    format->release(getLexInfo());
    formatObject->release(getLexInfo());
  }

  return true;
}

// Defined and Initialised classes

Defined::Defined(LexInfo *li) : Operation(li) { }

bool Defined::action() {
  Object *o;
  Variable *v;
  INT n;
  Name *name;
  char *value;

  o = stack.pop(getLexInfo());

  n = 1;
  if(o->isName()) {
    v = variableStack.findVariable(o->getName(getLexInfo())->getValue());
    if(v == (Variable *) 0) n = 0;
  }
  stack.push(cache.newNumber(n));

  o->release(getLexInfo());

  return true;
}

Initialised::Initialised(LexInfo *li) : Operation(li) { }

bool Initialised::action() {
  Object *o;
  Variable *v;
  INT n;

  o = stack.pop(getLexInfo());

  n = 1;
  if(o->isName()) {
    v = variableStack.findVariable(o->getName(getLexInfo())->getValue());
    if((v == (Variable *) 0) || (! v->isInitialised())) n = 0;
  }
  stack.push(cache.newNumber(n));

  o->release(getLexInfo());

  return true;
}

// PrintStack class

PrintStack::PrintStack(LexInfo *li) : Operation(li) { }

bool PrintStack::action() {
  StackItem *si;
  Object *o;
  Name *na;
  String *s;
  Code *c;
  Number *no;
  int i;
  bool found;

  for(i = 0, si = stack.getStack(); si != (StackItem *) 0; si = si->getDown(), i++) {
    printf("%d: ", i);

    o = si->getObject();
    found = false;

    try {
      na = o->getName(getLexInfo());
      printf("%s\n", na->getValue());
      found = true;
    } catch(Exception *e) { }

    if(! found) {
      try {
        s = o->getString(getLexInfo());
        printf("\"%s\"\n", s->getValue());
        s->release(getLexInfo());
        found = true;
      } catch(Exception *e) { }
    }

    if(! found) {
      try {
        c = o->getCode(getLexInfo());
        printf("{ ...code... }\n");
        c->release(getLexInfo());
        found = true;
      } catch(Exception *e) { }
    }

    if(! found) {
      try {
        no = o->getNumber(getLexInfo());
        if(no->isInt()) printf("%ld\n", no->getInt());
        else printf("%0.3f\n", no->getDouble());
        no->release(getLexInfo());
        found = true;
      } catch(Exception *e) { }
    }

    if(! found) slexception.chuck("unknown stack entry", getLexInfo());
  }

  return true;
}

// Debug class

Debug::Debug(LexInfo *li) : Operation(li) { }

bool Debug::action() {
  cache.debug();
  stack.debug();
  btree.debug();

  return true;
}

// BTree debug class

BTreeDebug::BTreeDebug(LexInfo *li) : Operation(li) { }

bool BTreeDebug::action() {
  btree.print();

  return true;
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

BTree::BTree() : tree((BTreeNode *) 0), depth(0), nodes(0), entries(0) { }

bool BTree::addVariable(Variable *d) {
  BTreeNode *left;
  BTreeNode *right;
  Variable *data;

  if(tree == (BTreeNode *) 0) {
    tree = new BTreeNode(true, (BTreeNode *) 0, d, (BTreeNode *) 0);
    depth = 1;
    nodes = 1;
    entries = 1;
  } else {
    left = (BTreeNode *) 0;
    if(! tree->addData(d, &left, &data, &right, &nodes)) return false;
    if(left != (BTreeNode *) 0) {
      delete(tree);
      tree = new BTreeNode(false, left, data, right);
      depth++;
      nodes++;
    }
    entries++;
  }

  return true;
}

Variable *BTree::findVariable(const char *v) {
  BTreeNode *p;
  Variable *d;
  int c;
  int n;
  int i;

  if(tree == (BTreeNode *) 0) return (Variable *) 0;

  p = tree;
  while(p != (BTreeNode *) 0) {
    n = p->getNumber();
    for(i = 0; i < n; i++) {
      d = p->getData(i);
      c = strcmp(v, d->getName());
      if(c == 0) return d;
      if(c < 0) {
        if(p->isLeaf()) return (Variable *) 0;
        p = p->getPointer(i);
        break;
      }
    }
    if(i >= n) {
      if(p->isLeaf()) return (Variable *) 0;
      p = p->getPointer(n);
    }
  }

  return (Variable *) 0;
}

void BTree::debug() {
  printf("BTree: depth %d, nodes %d, entries %d\n", depth, nodes, entries);
}

void BTree::print() {
  printTree(tree);
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

  v = t->getData(i);

  printf("%s: ", v->getName());
  o = v->getObject();
  if(o == (Object *) 0) {
    printf("...undefined...");
    return;
  }

  found = false;
  try {
    n = o->getNumber((LexInfo *) 0);
    if(n->isInt()) printf("%ld", n->getInt());
    else printf("%0.3f", n->getDouble());
    n->release((LexInfo *) 0);
    found = true;
  } catch(Exception *e) { }

  if(! found) {
    try {
      s = o->getString((LexInfo *) 0);
      printf("\"%s\"", s->getValue());
      s->release((LexInfo *) 0);
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) {
    try {
      p = o->getPointer((LexInfo *) 0);
      printf("...pointer...");
      p->release((LexInfo *) 0);
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) {
    try {
      c = o->getCode((LexInfo *) 0);
      printf("{ ...code... }");
      c->release((LexInfo *) 0);
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) printf("...unknown");

  printf(" (0x%lx, %d", (unsigned long) t, i);
  if(t->isLeaf()) printf(", leaf %d", t->getNumber());
  if(t == tree) printf(", top");
  printf(")\n");
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
  printf("Stack: size %d,  free %d\n", stackSize, usedSize);
}
