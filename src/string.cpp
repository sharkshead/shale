/*

MIT License

Copyright (c) 2020-2023 Graeme Elsworthy <github@sharkshead.com>

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
#define MICRO   (INT) 2

class StringHelp : public Operation {
  public:
    StringHelp(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class StringEquals : public Operation {
  public:
    StringEquals(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class StringConcat : public Operation {
  public:
    StringConcat(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class StringToNumber : public Operation {
  public:
    StringToNumber(LexInfo *, bool);
    OperatorReturn action(ExecutionEnvironment *);

  private:
    bool toInt;
};

const char *stringHelp[] = {
  "String library:",
  "  equals string::()     - compare two strings",
  "  concat string::()     - concatenate two strings",
  "  atoi string::()       - string to integer conversion",
  "  atof string::()       - string to floating point conversion",
  "  help string::()       - this",
  (const char *) 0
};

extern "C" void slmain() {
  OperationList *ol;
  Variable *v;

  // Are we already loaded?
  if(btree.findVariable("/help/string") != (Variable *) 0) return;

  ol = new OperationList;
  ol->addOperation(new StringHelp((LexInfo *) 0));
  v = new Variable("/help/string");
  v->setObject(new Code(ol, &mainEE.cache));
  btree.addVariable(v);

  v = new Variable("/major/version/string");
  v->setObject(mainEE.cache.newNumber(MAJOR));
  btree.addVariable(v);

  v = new Variable("/minor/version/string");
  v->setObject(mainEE.cache.newNumber(MINOR));
  btree.addVariable(v);

  v = new Variable("/micro/version/string");
  v->setObject(mainEE.cache.newNumber(MICRO));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new StringEquals((LexInfo *) 0));
  v = new Variable("/equals/string");
  v->setObject(new Code(ol, &mainEE.cache));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new StringConcat((LexInfo *) 0));
  v = new Variable("/concat/string");
  v->setObject(new Code(ol, &mainEE.cache));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new StringToNumber((LexInfo *) 0, true));
  v = new Variable("/atoi/string");
  v->setObject(new Code(ol, &mainEE.cache));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new StringToNumber((LexInfo *) 0, false));
  v = new Variable("/atof/string");
  v->setObject(new Code(ol, &mainEE.cache));
  btree.addVariable(v);
}

StringHelp::StringHelp(LexInfo *li) : Operation(li) { }

OperatorReturn StringHelp::action(ExecutionEnvironment *ee) {
  const char **p;

  for(p = stringHelp; *p != (const char *) 0; p++) {
    printf("%s\n", *p);
  }

  return or_continue;
}

StringEquals::StringEquals(LexInfo *li) : Operation(li) { }

OperatorReturn StringEquals::action(ExecutionEnvironment *ee) {
  Object *o1;
  Object *o2;
  String *s1;
  String *s2;
  INT n;

  o2 = ee->stack.pop(getLexInfo());
  o1 = ee->stack.pop(getLexInfo());

  s1 = o1->getString(getLexInfo(), ee);
  s2 = o2->getString(getLexInfo(), ee);

  n = (strcmp(s1->getValue(), s2->getValue()) == 0 ? 1 : 0);
  ee->stack.push(mainEE.cache.newNumber(n));

  s2->release(getLexInfo());
  s1->release(getLexInfo());

  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return or_continue;
}

StringConcat::StringConcat(LexInfo *li) : Operation(li) { }

OperatorReturn StringConcat::action(ExecutionEnvironment *ee) {
  Object *o1;
  Object *o2;
  String *s1;
  String *s2;
  const char *p1;
  const char *p2;
  char *p;

  o2 = ee->stack.pop(getLexInfo());
  o1 = ee->stack.pop(getLexInfo());

  s1 = o1->getString(getLexInfo(), ee);
  s2 = o2->getString(getLexInfo(), ee);

  p1 = s1->getValue();
  p2 = s2->getValue();
  if((p = (char *) malloc(strlen(p1) + strlen(p2) + 1)) == (char *) 0) slexception.chuck("Out of memory in concat string::()", getLexInfo());
  sprintf(p, "%s%s", p1, p2);
  ee->stack.push(mainEE.cache.newString(p, true));

  s2->release(getLexInfo());
  s1->release(getLexInfo());

  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return or_continue;
}

StringToNumber::StringToNumber(LexInfo *li, bool ti) : Operation(li), toInt(ti) { }

OperatorReturn StringToNumber::action(ExecutionEnvironment *ee) {
  Object *o;
  String *s;
  Number *n;

  o = ee->stack.pop(getLexInfo());
  s = o->getString(getLexInfo(), ee);

  if(toInt) {
    n = mainEE.cache.newNumber((INT) atoi(s->getValue()));
  } else {
    n = mainEE.cache.newNumber((double) atof(s->getValue()));
  }
  ee->stack.push(n);

  s->release(getLexInfo());
  o->release(getLexInfo());

  return or_continue;
}
