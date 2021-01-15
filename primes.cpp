/*

MIT License

Copyright (c) 2021 Graeme Elsworthy <github@sharkshead.com>

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

class PrimesHelp : public Operation {
  public:
    PrimesHelp(LexInfo *);
    OperatorReturn action();
};

class PrimesMap : public Operation {
  public:
    PrimesMap(LexInfo *);
    OperatorReturn action();
};

class PrimesGenerate : public Operation {
  public:
    PrimesGenerate(LexInfo *);
    OperatorReturn action();
};

const char *primesHelp[] = {
  "Primes library:",
  "  {last} {count} {ns} generate primes::() - Generate primes into namespace {ns}::",
  "                                            When {last} is non-zero, primes will be generated",
  "                                            up to and including this value. When {count} is",
  "                                            non-zero, this many primes will be generated.",
  "                                            When both are non-zero, whichever of the two",
  "                                            produces more primes will terminate the generation.",
  "                                            last {ns}:: is the highest prime generated, and",
  "                                            count {ns}:: is the number generated.",
  "                                            Primes are indexed by an integer in {ns}::,",
  "                                            starting from 0 up to one less than count {ns}::",
  "                                            eg. 3 {ns}:: or perhaps i.value {ns}::",
  "                                            May be called repeatedly.",
  "  {ns} map primes::()                     - Generate a reverse map of the currently generated",
  "                                            primes in the map:: {ns}:: namespace. Indexed by",
  "                                            an integer, eg i.value map:: {ns}:: is initialised",
  "                                            to the index in the {ns}:: namespace if i.value is prime,",
  "                                            and will not be initialised if i.value is not a prime.",
  "  major version:: primes::                - major version number",
  "  minor version:: primes::                - minor version number",
  "  micro version:: primes::                - micro version number",
  "  help primes::()                         - this",
  (const char *) 0
};

extern "C" void slmain() {
  OperationList *ol;
  Variable *v;

  // Are we already loaded?
  if(btree.findVariable("/help/primes") != (Variable *) 0) return;

  ol = new OperationList;
  ol->addOperation(new PrimesHelp((LexInfo *) 0));
  v = new Variable("/help/primes");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  v = new Variable("/major/version/primes");
  v->setObject(cache.newNumber(MAJOR));
  btree.addVariable(v);

  v = new Variable("/minor/version/primes");
  v->setObject(cache.newNumber(MINOR));
  btree.addVariable(v);

  v = new Variable("/micro/version/primes");
  v->setObject(cache.newNumber(MICRO));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new PrimesGenerate((LexInfo *) 0));
  v = new Variable("/generate/primes");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new PrimesMap((LexInfo *) 0));
  v = new Variable("/map/primes");
  v->setObject(new Code(ol));
  btree.addVariable(v);
}

PrimesHelp::PrimesHelp(LexInfo *li) : Operation(li) { }

OperatorReturn PrimesHelp::action() {
  const char **p;

  for(p = primesHelp; *p != (const char *) 0; p++) {
    printf("%s\n", *p);
  }

  return or_continue;
}

PrimesGenerate::PrimesGenerate(LexInfo *li) : Operation(li) { }

OperatorReturn PrimesGenerate::action() {
  Object *ns;
  Object *n;
  Object *l;
  Number *nn;
  Number *nl;
  Name *nsn;
  char *name;
  char buf[1024];
  char fmt[64];
  INT candidate;
  Variable *v;
  INT lastReq;
  INT numberReq;
  Number *num;
  INT last;
  INT count;
  INT squareRoot;
  INT index;
  INT prime;
  bool found;

  ns = stack.pop(getLexInfo());
  n = stack.pop(getLexInfo());
  l = stack.pop(getLexInfo());
  nn = n->getNumber(getLexInfo());
  nl = l->getNumber(getLexInfo());
  nsn = ns->getName(getLexInfo());
  name = nsn->getValue();

  lastReq = nl->getInt();
  numberReq = nn->getInt();

  if((lastReq > (INT) 0) || (numberReq > (INT) 0)) {
    // Have we generated any primes into this namespace before?
    sprintf(buf, "/count/%s", name);
    v = btree.findVariable(buf);
    if(v == (Variable *) 0) {
      v = new Variable(buf);
      v->setObject(cache.newNumber((INT) 0));
      btree.addVariable(v);
    }
    num = v->getObject()->getNumber(getLexInfo());
    count = num->getInt();
    num->release(getLexInfo());

    sprintf(buf, "/last/%s", name);
    v = btree.findVariable(buf);
    if(v == (Variable *) 0) {
      v = new Variable(buf);
      v->setObject(cache.newNumber((INT) 0));
      btree.addVariable(v);
    }
    num = v->getObject()->getNumber(getLexInfo());
    last = num->getInt();
    num->release(getLexInfo());

    // Enter 2 manually.
    sprintf(fmt, "/%%%sd/%%s", PCTD);
    sprintf(buf, fmt, (INT) 0, name);
    v = btree.findVariable(buf);
    if(v == (Variable *) 0) {
      v = new Variable(buf);
      v->setObject(cache.newNumber((INT) 2));
      btree.addVariable(v);
    }
    if(last < (INT) 2) last = (INT) 2;
    if(count < (INT) 1) count = (INT) 1;

    candidate = (last == (INT) 2 ? (INT) 3 : last + 2);
    while(((lastReq == (INT) 0) || (lastReq >= candidate)) || ((numberReq == (INT) 0) || (numberReq > count))) {
      squareRoot = ((INT) sqrt(candidate)) + 1;
      index = (INT) 0;
      found = true;
      while(true) {
        sprintf(fmt, "/%%%sd/%%s", PCTD);
        sprintf(buf, fmt, index, name);
        v = btree.findVariable(buf);
        if(v == (Variable *) 0) break;
        num = v->getObject()->getNumber(getLexInfo());
        prime = num->getInt();
        num->release(getLexInfo());
        if(prime > squareRoot) break;
        if((candidate % prime) == 0) {
          found = false;
          break;
        }
        index++;
      }
      if(found) {
        // candidate is a prime
        sprintf(fmt, "/%%%sd/%%s", PCTD);
        sprintf(buf, fmt, count, name);
        v = btree.findVariable(buf);
        if(v == (Variable *) 0) {
          v = new Variable(buf);
          v->setObject(cache.newNumber(candidate));
          btree.addVariable(v);
        } else {
          v->setObject(cache.newNumber(candidate));
        }
        last = candidate;
        count++;
      }

      candidate += 2;
    }

    // Update the last and count entries.
    sprintf(buf, "/count/%s", name);
    v = btree.findVariable(buf);
    if(v != (Variable *) 0) {
      v->setObject(cache.newNumber(count));
    }
    num = v->getObject()->getNumber(getLexInfo());
    count = num->getInt();
    num->release(getLexInfo());

    sprintf(buf, "/last/%s", name);
    v = btree.findVariable(buf);
    if(v != (Variable *) 0) {
      v->setObject(cache.newNumber(last));
    }
    num = v->getObject()->getNumber(getLexInfo());
    last = num->getInt();
    num->release(getLexInfo());
  }

  nn->release(getLexInfo());
  nl->release(getLexInfo());
  n->release(getLexInfo());
  l->release(getLexInfo());
  ns->release(getLexInfo());

  return or_continue;
}

PrimesMap::PrimesMap(LexInfo *li) : Operation(li) { }

OperatorReturn PrimesMap::action() {
  Object *ns;
  Name *nsn;
  char *name;
  Variable *v;
  Number *num;
  char buf[1024];
  char fmt[64];
  INT count;
  INT index;
  INT prime;

  ns = stack.pop(getLexInfo());
  nsn = ns->getName(getLexInfo());
  name = nsn->getValue();

  sprintf(buf, "/count/%s", name);
  v = btree.findVariable(buf);
  if(v == (Variable *) 0) return or_continue;

  num = v->getObject()->getNumber(getLexInfo());
  count = num->getInt();
  num->release(getLexInfo());

  for(index = (INT) 0; index < count; index++) {
    sprintf(fmt, "/%%%sd/%%s", PCTD);
    sprintf(buf, fmt, index, name);
    v = btree.findVariable(buf);
    if(v == (Variable *) 0) return or_continue;
    num = v->getObject()->getNumber(getLexInfo());
    prime = num->getInt();
    num->release(getLexInfo());

    sprintf(fmt, "/%%%sd/map/%%s", PCTD);
    sprintf(buf, fmt, prime, name);
    v = btree.findVariable(buf);
    if(v == (Variable *) 0) {
      v = new Variable(buf);
      v->setObject(cache.newNumber(index));
      btree.addVariable(v);
    } else {
      v->setObject(cache.newNumber(index));
    }
  }

  ns->release(getLexInfo());

  return or_continue;
}
