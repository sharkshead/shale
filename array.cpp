#include "shalelib.h"

#define MAJOR   (INT) 1
#define MINOR   (INT) 0
#define MICRO   (INT) 0

const char *arrayHelp[] = {
  "Array library:",
  "  {initial} {count} {name} create array::()  - create an array called {name} with {count} elements,",
  "                                               starting from index 0, with {initial} value",
  "  {value} {name} scooch array::()            - add {value} to the beginning of the array and shuffle all",
  "                                               the other values along. only useable with an array",
  "                                               created with create array::()",
  "  {var} {index} {name} get array::()         - get the array element {index} and store it in {var},",
  "                                               if the element exists. Returns false if the element",
  "                                               does not exist, true otherwise",
  "  {index} {name} {value} set array::()       - set the element {index} to {value}",
  "  major version:: array::                    - major version number",
  "  minor version:: array::                    - minor version number",
  "  micro version:: array::                    - micro version number",
  "  help array::()                             - this",
  (const char *) 0
};

class ArrayHelp : public Operation {
  public:
    ArrayHelp(LexInfo *);
    bool action();
};

class ArrayCreate : public Operation {
  public:
    ArrayCreate(LexInfo *);
    bool action();
};

class ArrayScooch : public Operation {
  public:
    ArrayScooch(LexInfo *);
    bool action();
};

class ArrayGet : public Operation {
  public:
    ArrayGet(LexInfo *);
    bool action();
};

class ArraySet : public Operation {
  public:
    ArraySet(LexInfo *);
    bool action();
};

extern "C" void slmain() {
  OperationList *ol;
  OperationList *ol2;
  Variable *v;

  // Are we already loaded?
  if(btree.findVariable("/help/array") != (Variable *) 0) return;

  // help array::
  ol = new OperationList;
  ol->addOperation(new ArrayHelp((LexInfo *) 0));
  v = new Variable("/help/array");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  v = new Variable("/major/version/array");
  v->setObject(new Number(MAJOR));
  btree.addVariable(v);

  v = new Variable("/minor/version/array");
  v->setObject(new Number(MINOR));
  btree.addVariable(v);

  v = new Variable("/micro/version/array");
  v->setObject(new Number(MICRO));
  btree.addVariable(v);

  // create array::
  ol = new OperationList;
  ol->addOperation(new ArrayCreate((LexInfo *) 0));
  v = new Variable("/create/array");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  // scooch array::
  ol = new OperationList;
  ol->addOperation(new ArrayScooch((LexInfo *) 0));
  v = new Variable("/scooch/array");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  // get array::
  ol = new OperationList;
  ol->addOperation(new ArrayGet((LexInfo *) 0));
  v = new Variable("/get/array");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  // set array::
  ol = new OperationList;
  ol->addOperation(new ArraySet((LexInfo *) 0));
  v = new Variable("/set/array");
  v->setObject(new Code(ol));
  btree.addVariable(v);
}

ArrayHelp::ArrayHelp(LexInfo *li) : Operation(li) { }

bool ArrayHelp::action() {
  const char **p;

  for(p = arrayHelp; *p != (const char *) 0; p++) {
    printf("%s\n", *p);
  }

  return true;
}

ArrayCreate::ArrayCreate(LexInfo *li) : Operation(li) { }

bool ArrayCreate::action() {
  Object *oname;
  Object *size;
  Object *value;
  Number *number;
  String *string;
  Number *s;
  Variable *v;
  char buf[1024];
  char *p;
  int i;
  INT j;
  static char element[1024];
  bool found;

  oname = stack.pop(getLexInfo());
  size = stack.pop(getLexInfo());
  value = stack.pop(getLexInfo());
  s = size->getNumber(getLexInfo());

  found = false;
  try {
    p = oname->getName(getLexInfo())->getValue();
    sprintf(buf, "%s", p);
    found = true;
  } catch(Exception *e) { }

  if(! found) {
    try {
      number = oname->getNumber(getLexInfo());
      if(number->isInt()) sprintf(buf, "%ld", number->getInt());
      else sprintf(buf, "%0.3f", number->getDouble());
      number->release(getLexInfo());
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) {
    try {
    string = oname->getString(getLexInfo());
    sprintf(buf, "%s", string->getValue());
    string->release(getLexInfo());
    found = true;
    } catch(Exception *e) { }
  }

  if(! found) slexception.chuck("unrecognised name", getLexInfo());

  j = s->getInt();
  if(j > 0) {
    sprintf(element, "/_$/%s", buf);
    v = new Variable(element);
    v->setObject(cache.newNumber(j));
    btree.addVariable(v);
    for(i = 0; i < j; i++) {
      sprintf(element, "/%d/%s", i, buf);
      v = new Variable(element);
      v->setObject(value);
      if(! btree.addVariable(v)) {
        sprintf(element, "Name /%d/%s already exists", i, buf);
        slexception.chuck(element, getLexInfo());
      }
    }
  }

  s->release(getLexInfo());
  oname->release(getLexInfo());
  size->release(getLexInfo());
  value->release(getLexInfo());

  return true;
}

ArrayScooch::ArrayScooch(LexInfo *li) : Operation(li) { }

bool ArrayScooch::action() {
  Object *name;
  Object *value;
  Name *arrayName;
  const char *n;
  Variable *vc;
  Number *c;
  INT count;
  Variable *src;
  Variable *dst;
  INT i;
  INT j;
  char element[1024];

  name = stack.pop(getLexInfo());
  value = stack.pop(getLexInfo());
  arrayName = name->getName(getLexInfo());
  n = arrayName->getValue();

  sprintf(element, "/_$/%s", n);
  if((vc = btree.findVariable(element)) == (Variable *) 0) slexception.chuck("Internal count variable not found", getLexInfo());
  c = vc->getObject()->getNumber(getLexInfo());
  count = c->getInt();
  c->release(getLexInfo());

  if(count > 0) {
    j = count - 1;
    sprintf(element, "/%ld/%s", j, n);
    if((dst = btree.findVariable(element)) == (Variable *) 0) slexception.chuck(element, getLexInfo());
    if(j > 0) {
      for(i = j; i > 0; i--) {
        sprintf(element, "/%ld/%s", i - 1, n);
        if((src = btree.findVariable(element)) == (Variable *) 0) slexception.chuck(element, getLexInfo());
        dst->setObject(src->getObject());
        dst = src;
      }
    }
    dst->setObject(value);
  }

  name->release(getLexInfo());
  value->release(getLexInfo());

  return true;
}

ArrayGet::ArrayGet(LexInfo *li) : Operation(li) { }

bool ArrayGet::action() {
  Object *array;
  Object *index;
  Object *variable;
  Name *arrayName;
  Number *indexNumber;
  String *indexString;
  Variable *v;
  Variable *dst;
  char buf[1024];
  char element[1024];
  bool found;

  array = stack.pop(getLexInfo());
  index = stack.pop(getLexInfo());
  variable = stack.pop(getLexInfo());

  arrayName = array->getName(getLexInfo());

  found = false;

  try {
    indexNumber = index->getNumber(getLexInfo());
    if(indexNumber->isInt()) {
      sprintf(buf, "%ld", indexNumber->getInt());
    } else {
      sprintf(buf, "%0.3f", indexNumber->getDouble());
    }
    indexNumber->release(getLexInfo());
    found = true;
  } catch(Exception *e) { }

  if(! found) {
    try {
      indexString = index->getString(getLexInfo());
      sprintf(buf, "%s", indexString->getValue());
      indexString->release(getLexInfo());
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) slexception.chuck("unknown index type", getLexInfo());

  sprintf(element, "/%s/%s", buf, arrayName->getValue());
  v = btree.findVariable(element);
  if(v != (Variable *) 0) {
    dst = variableStack.findVariable(variable->getName(getLexInfo())->getValue());
    dst->setObject(v->getObject());
    stack.push(cache.newNumber((INT) 1));
  } else {
    stack.push(cache.newNumber((INT) 0));
  }

  array->release(getLexInfo());
  index->release(getLexInfo());
  variable->release(getLexInfo());

  return true;
}

ArraySet::ArraySet(LexInfo *li) : Operation(li) { }

bool ArraySet::action() {
  Object *value;
  Object *array;
  Object *index;
  Name *arrayName;
  Number *indexNumber;
  String *indexString;
  Variable *v;
  char buf[1024];
  char element[1024];
  bool found;

  value = stack.pop(getLexInfo());
  array = stack.pop(getLexInfo());
  index = stack.pop(getLexInfo());

  arrayName = array->getName(getLexInfo());

  found = false;

  try {
    indexNumber = index->getNumber(getLexInfo());
    if(indexNumber->isInt()) {
      sprintf(buf, "%ld", indexNumber->getInt());
    } else {
      sprintf(buf, "%0.3f", indexNumber->getDouble());
    }
    indexNumber->release(getLexInfo());
    found = true;
  } catch(Exception *e) { }

  if(! found) {
    try {
      indexString = index->getString(getLexInfo());
      sprintf(buf, "%s", indexString->getValue());
      indexString->release(getLexInfo());
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) slexception.chuck("unknown index type", getLexInfo());

  sprintf(element, "/%s/%s", buf, arrayName->getValue());
  v = btree.findVariable(element);
  if(v == (Variable *) 0) {
    v = new Variable(element);
    btree.addVariable(v);
  }
  v->setObject(value);

  value->release(getLexInfo());
  array->release(getLexInfo());
  index->release(getLexInfo());

  return true;
}
