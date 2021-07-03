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
#define MICRO   (INT) 0

class NamespaceHelp : public Operation {
  public:
    NamespaceHelp(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class NamespaceStatic : public Operation {
  public:
    NamespaceStatic(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

const char *namespaceHelp[] = {
  "Namespace library",
  "  help namespace::()        - this",
  (const char *) 0
};

extern "C" void slmain() {
  OperationList *ol;
  Variable *v;

  // Are we already loaded?
  if(btree.findVariable("/help/namespace") != (Variable *) 0) return;

  ol = new OperationList;
  ol->addOperation(new NamespaceHelp((LexInfo *) 0));
  v = new Variable("/help/namespace");
  v->setObject(new Code(ol, &mainEE.cache, IS_STATIC));
  btree.addVariable(v);

  v = new Variable("/major/version/namespace");
  v->setObject(mainEE.cache.newNumber(MAJOR));
  btree.addVariable(v);

  v = new Variable("/minor/version/namespace");
  v->setObject(mainEE.cache.newNumber(MINOR));
  btree.addVariable(v);

  v = new Variable("/micro/version/namespace");
  v->setObject(mainEE.cache.newNumber(MICRO));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new NamespaceStatic((LexInfo *) 0));
  v = new Variable("/static/namespace");
  v->setObject(new Code(ol, &mainEE.cache, IS_STATIC));
  btree.addVariable(v);
}

NamespaceHelp::NamespaceHelp(LexInfo *li) : Operation(li) { }

OperatorReturn NamespaceHelp::action(ExecutionEnvironment *ee) {
  const char **p;

  for(p = namespaceHelp; *p != (const char *) 0; p++) {
    printf("%s\n", *p);
  }

  return or_continue;
}

NamespaceStatic::NamespaceStatic(LexInfo *li) : Operation(li) { }

OperatorReturn NamespaceStatic::action(ExecutionEnvironment *ee) {
  Object *o;
  Name *ns;

  o = ee->stack.pop(getLexInfo());
  ns = o->getName(getLexInfo(), ee);

  btree.toStatic(ns->getValue());

  o->release(getLexInfo());

  return or_continue;
}
