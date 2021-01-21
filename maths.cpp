/*

MIT License

Copyright (c) 2020 Graeme Elsworthy <github@sharkshead.com>

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

#define FUNCTION_LN      0
#define FUNCTION_LOG     1
#define FUNCTION_SQRT    2

class MathsHelp : public Operation {
  public:
    MathsHelp(LexInfo *);
    OperatorReturn action();
};

class MathsFunction : public Operation {
  public:
    MathsFunction(int, LexInfo *);
    OperatorReturn action();

  private:
    int function;
};

class MathsToThePower : public Operation {
  public:
    MathsToThePower(LexInfo *);
    OperatorReturn action();
};

const char *mathsHelp[] = {
  "Maths library:",
  "  pi maths::               - 3.141592653589793",
  "  e maths::                - 2.718281828459045",
  "  {n} ln maths::()         - natural log",
  "  {n} log maths::()        - log base 10",
  "  {n} sqrt maths::()       - square root",
  "  {a} {b} power maths::()  - a to the power b",
  "  major version:: maths::  - major version number",
  "  minor version:: maths::  - minor version number",
  "  micro version:: maths::  - micro version number",
  "  help maths::()           - this",
  (const char *) 0
};

extern "C" void slmain() {
  OperationList *ol;
  Variable *v;

  // Are we already loaded?
  if(btree.findVariable("/help/maths") != (Variable *) 0) return;

  ol = new OperationList;
  ol->addOperation(new MathsHelp((LexInfo *) 0));
  v = new Variable("/help/maths");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  v = new Variable("/major/version/maths");
  v->setObject(cache.newNumber(MAJOR));
  btree.addVariable(v);

  v = new Variable("/minor/version/maths");
  v->setObject(cache.newNumber(MINOR));
  btree.addVariable(v);

  v = new Variable("/micro/version/maths");
  v->setObject(cache.newNumber(MICRO));
  btree.addVariable(v);

  v = new Variable("/e/maths");
  v->setObject(cache.newNumber(2.718281828459045));
  btree.addVariable(v);

  v = new Variable("/pi/maths");
  v->setObject(cache.newNumber(3.141592653589793));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new MathsFunction(FUNCTION_LN, (LexInfo *) 0));
  v = new Variable("/ln/maths");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new MathsFunction(FUNCTION_LOG, (LexInfo *) 0));
  v = new Variable("/log/maths");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new MathsFunction(FUNCTION_SQRT, (LexInfo *) 0));
  v = new Variable("/sqrt/maths");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new MathsToThePower((LexInfo *) 0));
  v = new Variable("/power/maths");
  v->setObject(new Code(ol));
  btree.addVariable(v);
}

MathsHelp::MathsHelp(LexInfo *li) : Operation(li) { }

OperatorReturn MathsHelp::action() {
  const char **p;

  for(p = mathsHelp; *p != (const char *) 0; p++) {
    printf("%s\n", *p);
  }

  return or_continue;
}

MathsFunction::MathsFunction(int f, LexInfo *li) : Operation(li), function(f) { }

OperatorReturn MathsFunction::action() {
  Object *o;
  Number *n;
  Number *res;
  double val;

  o = stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo());

  switch(function) {
    case FUNCTION_LN:
      val = log(n->getDouble());
      break;

    case FUNCTION_LOG:
      val = log10(n->getDouble());
      break;

    case FUNCTION_SQRT:
      val = sqrt(n->getDouble());
      break;

  }
  stack.push(cache.newNumber(val));

  n->release(getLexInfo());
  o->release(getLexInfo());

  return or_continue;
}

MathsToThePower::MathsToThePower(LexInfo *li) : Operation(li) { }

OperatorReturn MathsToThePower::action() {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;

  o2 = stack.pop(getLexInfo());
  o1 = stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo());
  n2 = o2->getNumber(getLexInfo());
    
  stack.push(cache.newNumber(exp(n2->getDouble() * log(n1->getDouble()))));

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return or_continue;
}
