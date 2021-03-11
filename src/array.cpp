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

#define MAJOR   (INT) 1
#define MINOR   (INT) 0
#define MICRO   (INT) 1

const char *arrayHelp[] = {
  "Array library:",
  "  {initial} {count} {name} create array::()  - create an array called {name} with {count} elements,",
  "                                               starting from index 0, with {initial} value",
  "  {value} {name} scooch array::()            - add {value} to the beginning of the array and shuffle all",
  "                                               the other values along. only useable with an array",
  "                                               created with create array::()",
  "  {index} {name} get array::()               - get the array element {index}",
  "                                               If the element exists it pushes the value then true,",
  "                                               otherwise it pushes false.",
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
    OperatorReturn action(ExecutionEnvironment *);
};

class ArrayCreate : public Operation {
  public:
    ArrayCreate(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class ArrayScooch : public Operation {
  public:
    ArrayScooch(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class ArrayGet : public Operation {
  public:
    ArrayGet(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class ArraySet : public Operation {
  public:
    ArraySet(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
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
  v->setObject(cache.newNumber(MAJOR));
  btree.addVariable(v);

  v = new Variable("/minor/version/array");
  v->setObject(cache.newNumber(MINOR));
  btree.addVariable(v);

  v = new Variable("/micro/version/array");
  v->setObject(cache.newNumber(MICRO));
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

OperatorReturn ArrayHelp::action(ExecutionEnvironment *ee) {
  const char **p;

  for(p = arrayHelp; *p != (const char *) 0; p++) {
    printf("%s\n", *p);
  }

  return or_continue;
}

ArrayCreate::ArrayCreate(LexInfo *li) : Operation(li) { }

OperatorReturn ArrayCreate::action(ExecutionEnvironment *ee) {
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
  char fmt[32];

  oname = ee->stack.pop(getLexInfo());
  size = ee->stack.pop(getLexInfo());
  value = ee->stack.pop(getLexInfo());
  s = size->getNumber(getLexInfo(), ee);

  found = false;
  try {
    p = oname->getName(getLexInfo(), ee)->getValue();
    sprintf(buf, "%s", p);
    found = true;
  } catch(Exception *e) { }

  if(! found) {
    try {
      number = oname->getNumber(getLexInfo(), ee);
      if(number->isInt()) {
        sprintf(fmt, "%%%sd",PCTD);
        sprintf(buf, fmt, number->getInt());
      } else sprintf(buf, "%0.3f", number->getDouble());
      number->release(getLexInfo());
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) {
    try {
    string = oname->getString(getLexInfo(), ee);
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

  return or_continue;
}

ArrayScooch::ArrayScooch(LexInfo *li) : Operation(li) { }

OperatorReturn ArrayScooch::action(ExecutionEnvironment *ee) {
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
  char fmt[32];

  name = ee->stack.pop(getLexInfo());
  value = ee->stack.pop(getLexInfo());
  arrayName = name->getName(getLexInfo(), ee);
  n = arrayName->getValue();

  sprintf(element, "/_$/%s", n);
  if((vc = btree.findVariable(element)) == (Variable *) 0) slexception.chuck("Internal count variable not found", getLexInfo());
  c = vc->getObject()->getNumber(getLexInfo(), ee);
  count = c->getInt();
  c->release(getLexInfo());

  if(count > 0) {
    j = count - 1;
    sprintf(fmt, "/%%%sd/%%s", PCTD);
    sprintf(element, fmt, j, n);
    if((dst = btree.findVariable(element)) == (Variable *) 0) slexception.chuck(element, getLexInfo());
    if(j > 0) {
      for(i = j; i > 0; i--) {
        sprintf(element, fmt, i - 1, n);
        if((src = btree.findVariable(element)) == (Variable *) 0) slexception.chuck(element, getLexInfo());
        dst->setObject(src->getObject());
        dst = src;
      }
    }
    dst->setObject(value);
  }

  name->release(getLexInfo());
  value->release(getLexInfo());

  return or_continue;
}

ArrayGet::ArrayGet(LexInfo *li) : Operation(li) { }

OperatorReturn ArrayGet::action(ExecutionEnvironment *ee) {
  Object *array;
  Object *index;
  Name *arrayName;
  Number *indexNumber;
  String *indexString;
  Variable *v;
  Object *val;
  char buf[1024];
  char element[1024];
  bool found;
  char fmt[32];

  array = ee->stack.pop(getLexInfo());
  index = ee->stack.pop(getLexInfo());

  arrayName = array->getName(getLexInfo(), ee);

  found = false;

  try {
    indexNumber = index->getNumber(getLexInfo(), ee);
    if(indexNumber->isInt()) {
      sprintf(fmt, "%%%sd", PCTD);
      sprintf(buf, fmt, indexNumber->getInt());
    } else {
      sprintf(buf, "%0.3f", indexNumber->getDouble());
    }
    indexNumber->release(getLexInfo());
    found = true;
  } catch(Exception *e) { }

  if(! found) {
    try {
      indexString = index->getString(getLexInfo(), ee);
      sprintf(buf, "%s", indexString->getValue());
      indexString->release(getLexInfo());
      found = true;
    } catch(Exception *e) { }
  }

  if(! found) slexception.chuck("unknown index type", getLexInfo());

  sprintf(element, "/%s/%s", buf, arrayName->getValue());
  v = btree.findVariable(element);
  if(v != (Variable *) 0) {
    val = v->getObject();
    if(val != (Object *) 0) {
      ee->stack.push(val);
      ee->stack.push(cache.newNumber((INT) 1));
    } else {
      ee->stack.push(cache.newNumber((INT) 0));
    }
  } else {
    ee->stack.push(cache.newNumber((INT) 0));
  }

  array->release(getLexInfo());
  index->release(getLexInfo());

  return or_continue;
}

ArraySet::ArraySet(LexInfo *li) : Operation(li) { }

OperatorReturn ArraySet::action(ExecutionEnvironment *ee) {
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
  char fmt[32];

  value = ee->stack.pop(getLexInfo());
  array = ee->stack.pop(getLexInfo());
  index = ee->stack.pop(getLexInfo());

  arrayName = array->getName(getLexInfo(), ee);

  found = false;

  try {
    indexNumber = index->getNumber(getLexInfo(), ee);
    if(indexNumber->isInt()) {
      sprintf(fmt, "%%%sd", PCTD);
      sprintf(buf, fmt, indexNumber->getInt());
    } else {
      sprintf(buf, "%0.3f", indexNumber->getDouble());
    }
    indexNumber->release(getLexInfo());
    found = true;
  } catch(Exception *e) { }

  if(! found) {
    try {
      indexString = index->getString(getLexInfo(), ee);
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

  return or_continue;
}
