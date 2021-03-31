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
#define MICRO   (INT) 3

#define FUNCTION_LN      0
#define FUNCTION_LOG     1
#define FUNCTION_SQRT    3
#define FUNCTION_GCD     4

class MathsHelp : public Operation {
  public:
    MathsHelp(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class MathsFunction : public Operation {
  public:
    MathsFunction(int, LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);

  private:
    int function;
};

class MathsToThePower : public Operation {
  public:
    MathsToThePower(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class MathsRandom : public Operation {
  public:
    MathsRandom(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class MathsSrandom : public Operation {
  public:
    MathsSrandom(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

const char *mathsHelp[] = {
  "Maths library:",
  "  pi maths::               - 3.141592653589793",
  "  e maths::                - 2.718281828459045",
  "  {n} ln maths::()         - natural log",
  "  {n} log maths::()        - log base 10",
  "  {n} sqrt maths::()       - square root",
  "  {a} {b} power maths::()  - a to the power b",
  "  {a} {b} gcd maths::()    - greatest common divisor of a and b",
  "  random maths::()         - random number between 0 and 2^32-1",
  "  sramdon maths::()        - seed random number generator. RNG is seeded with pseudo-random number by default.",
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
  ol->addOperation(new MathsFunction(FUNCTION_GCD, (LexInfo *) 0));
  v = new Variable("/gcd/maths");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new MathsToThePower((LexInfo *) 0));
  v = new Variable("/power/maths");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new MathsRandom((LexInfo *) 0));
  v = new Variable("/random/maths");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new MathsSrandom((LexInfo *) 0));
  v = new Variable("/srandom/maths");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  srandom(time(0) + getpid());
}

MathsHelp::MathsHelp(LexInfo *li) : Operation(li) { }

OperatorReturn MathsHelp::action(ExecutionEnvironment *ee) {
  const char **p;

  for(p = mathsHelp; *p != (const char *) 0; p++) {
    printf("%s\n", *p);
  }

  return or_continue;
}

MathsFunction::MathsFunction(int f, LexInfo *li) : Operation(li), function(f) { }

OperatorReturn MathsFunction::action(ExecutionEnvironment *ee) {
  Object *o;
  Object *o2;
  Number *n;
  Number *n2;
  Number *res;
  INT a;
  INT b;
  INT t;

  o = ee->stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo(), ee);

  switch(function) {
    case FUNCTION_LN:
      ee->stack.push(cache.newNumber(log(n->getDouble())));
      break;

    case FUNCTION_LOG:
      ee->stack.push(cache.newNumber(log10(n->getDouble())));
      break;

    case FUNCTION_SQRT:
      ee->stack.push(cache.newNumber(sqrt(n->getDouble())));
      break;

    case FUNCTION_GCD:
      o2 = ee->stack.pop(getLexInfo());
      n2 = o2->getNumber(getLexInfo(), ee);
      a = n->getInt();
      b = n2->getInt();

      if(a < b) {
        t = a;
        a = b;
        b = t;
      }
      while(b != 0) {
        t = b;
        b = a % b;
        a = t;
      }
      ee->stack.push(cache.newNumber(a));

      n2->release(getLexInfo());
      o2->release(getLexInfo());
      break;
  }

  n->release(getLexInfo());
  o->release(getLexInfo());

  return or_continue;
}

MathsToThePower::MathsToThePower(LexInfo *li) : Operation(li) { }

OperatorReturn MathsToThePower::action(ExecutionEnvironment *ee) {
  Object *o1;
  Object *o2;
  Number *n1;
  Number *n2;

  o2 = ee->stack.pop(getLexInfo());
  o1 = ee->stack.pop(getLexInfo());
  n1 = o1->getNumber(getLexInfo(), ee);
  n2 = o2->getNumber(getLexInfo(), ee);
    
  ee->stack.push(cache.newNumber(exp(n2->getDouble() * log(n1->getDouble()))));

  n1->release(getLexInfo());
  n2->release(getLexInfo());
  o1->release(getLexInfo());
  o2->release(getLexInfo());

  return or_continue;
}

MathsRandom::MathsRandom(LexInfo *li) : Operation(li) { }

OperatorReturn MathsRandom::action(ExecutionEnvironment *ee) {
  ee->stack.push(cache.newNumber((INT) random()));

  return or_continue;
}

MathsSrandom::MathsSrandom(LexInfo *li) : Operation(li) { }

OperatorReturn MathsSrandom::action(ExecutionEnvironment *ee) {
  Object *o;
  Number *n;

  o = ee->stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo(), ee);

  srandom(n->getInt());

  n->release(getLexInfo());
  o->release(getLexInfo());

  return or_continue;
}
