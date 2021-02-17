/*

MIT License

Copyright (c) 2021-2021 Graeme Elsworthy <github@sharkshead.com>

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
#define MINOR   (INT) 1
#define MICRO   (INT) 1

class PrimesHelp : public Operation {
  public:
    PrimesHelp(LexInfo *);
    OperatorReturn action();
};

class PrimesType : public Operation {
  public:
    PrimesType(LexInfo *);
    OperatorReturn action();
};

class PrimesGenerate : public Operation {
  public:
    PrimesGenerate(LexInfo *);
    OperatorReturn action();
};

class PrimesGet : public Operation {
  public:
    PrimesGet(LexInfo *);
    OperatorReturn action();
};

class PrimesIsPrime : public Operation {
  public:
    PrimesIsPrime(LexInfo *);
    OperatorReturn action();
};

class PrimesMap : public Operation {
  public:
    PrimesMap(LexInfo *);
    OperatorReturn action();
};

const char *primesHelp[] = {
  "Primes library:",
  "  {ns} {type} type primes::()             - Set the memory layout used to store the primes, where type",
  "                                            can be array or sieve, eg: p sieve type primes::().",
  "                                            For backward compatibility, array is the default.",
  "  {last} {count} {ns} generate primes::() - Generate primes into namespace {ns}::",
  "                                            When {last} is non-zero, primes will be generated",
  "                                            up to and including this value. When {count} is",
  "                                            non-zero, this many primes will be generated.",
  "                                            When both are non-zero, whichever of the two",
  "                                            produces more primes will terminate the generation.",
  "                                            last {ns}:: is the highest prime generated, and",
  "                                            count {ns}:: is the number generated.",
  "                                            May be called repeatedly. For a sieve memory layout the",
  "                                            count argument cannot be used.",
  "  {index} {ns} get primes::()             - Returns the index-th prime, returning 2 at index 0,",
  "                                            3 at index 1, and so on. For a sieve memory layout this",
  "                                            is optimised for sequential (non-decreasing) access.",
  "  {n} {ns} isprime primes::()             - Returns true on the stack if the given number n is",
  "                                            a prime as listed in the {ns}:: namespace, false otherwise.",
  "  {ns} map primes::()                     - Generate a reverse map of the currently generated",
  "                                            primes in the map:: {ns}:: namespace. Indexed by",
  "                                            an integer, eg i.value map:: {ns}:: is initialised",
  "                                            to the index in the {ns}:: namespace if i.value is prime,",
  "                                            and will not be initialised if i.value is not a prime.",
  "                                            This is only applicable to an array memory layout and is",
  "                                            deprecated in favour of isprime primes::().",
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
  ol->addOperation(new PrimesType((LexInfo *) 0));
  v = new Variable("/type/primes");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new PrimesGenerate((LexInfo *) 0));
  v = new Variable("/generate/primes");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new PrimesGet((LexInfo *) 0));
  v = new Variable("/get/primes");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new PrimesIsPrime((LexInfo *) 0));
  v = new Variable("/isprime/primes");
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

PrimesType::PrimesType(LexInfo *li) : Operation(li) { }

OperatorReturn PrimesType::action() {
  Object *ons;
  Object *otype;
  Name *type;
  Name *ns;
  Variable *v;
  char *typeValue;
  static char buf[128];
  char *p;

  otype = stack.pop(getLexInfo());
  ons = stack.pop(getLexInfo());
  type = otype->getName(getLexInfo());
  typeValue = type->getValue();
  ns = ons->getName(getLexInfo());

  if((strcmp(typeValue, "array") != 0) && (strcmp(typeValue, "sieve") != 0)) {
    sprintf(buf, "Unknown primes memory type: %s", typeValue);
    slexception.chuck(buf, getLexInfo());
    return or_continue;
  }

  sprintf(buf, "/type/%s", ns->getValue());
  v = btree.findVariable(buf);
  if(v == (Variable *) 0) {
    v = new Variable(buf);
    if((p = (char *) malloc(strlen(typeValue) + 1)) != (char *) 0) {
      strcpy(p, typeValue);
      v->setObject(cache.newString(p, true));
      btree.addVariable(v);
    } else {
      slexception.chuck("Out of memory in type primes::()", getLexInfo());
    }
  } else {
    slexception.chuck("Can only set the primes memory type once", getLexInfo());
  }

  otype->release(getLexInfo());
  ons->release(getLexInfo());

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
  char *p;
  INT candidate;
  Variable *v;
  INT lastReq;
  INT numberReq;
  Number *num;
  String *memoryType;
  bool isArrayType;
  INT last;
  INT count;
  INT squareRoot;
  INT index;
  INT lastIndex;
  INT prime;
  INT bit;
  INT word;
  INT sieve;
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

    if(last < (INT) 2) last = (INT) 2;
    if(count < (INT) 1) count = (INT) 1;

    // Check what memory type this is.
    sprintf(buf, "/type/%s", name);
    v = btree.findVariable(buf);
    if(v == (Variable *) 0) {
      if((p = (char *) malloc(8)) == (char *) 0) slexception.chuck("Out of memory in generate primes::()", getLexInfo());
      strcpy(p, "array");
      v = new Variable(buf);
      v->setObject(cache.newString(p, true));
      btree.addVariable(v);
      isArrayType = true;
    } else {
      memoryType = v->getObject()->getString(getLexInfo());
      isArrayType = (strcmp(memoryType->getValue(), "array") == 0);
      memoryType->release(getLexInfo());
    }

    if(isArrayType) {
      // Enter 2 manually.
      sprintf(fmt, "/%%%sd/%%s", PCTD);
      sprintf(buf, fmt, (INT) 0, name);
      v = btree.findVariable(buf);
      if(v == (Variable *) 0) {
        v = new Variable(buf);
        v->setObject(cache.newNumber((INT) 2));
        btree.addVariable(v);
      }

      candidate = (last == (INT) 2 ? (INT) 3 : last + 2);
      while(((lastReq == (INT) 0) || (lastReq >= candidate)) && ((numberReq == (INT) 0) || (numberReq > count))) {
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
    } else {
      // Sieve type.
      // ATM this is really simple in that if generate is called multiple times it starts
      // the sieve process from scratch, if does not try to pick up from where it was last time.

      if(numberReq > (INT) 0) slexception.chuck("Can't specify a count on a sieve primes type", getLexInfo());
      count = (INT) 1;
      if(lastReq > (INT) 2) {
        sprintf(fmt, "/%%%sd/%s", PCTD, name);
        candidate = 3;
        while(candidate <= (lastReq / 2)) {
          index = (candidate - 3) / 128;
          bit = (INT) 1 << (((candidate - 3) / 2) % 64);
          sprintf(buf, fmt, index);
          v = btree.findVariable(buf);
          if(v == (Variable *) 0) {
            v = new Variable(buf);
            v->setObject(cache.newNumber((INT) -1));
            btree.addVariable(v);
          }
          num = v->getObject()->getNumber(getLexInfo());
          word = num->getInt();
          num->release(getLexInfo());
          if(word & bit) {
            // This is a prime so start sieving
            sieve = 3 * candidate;
            while(sieve < lastReq) {
              index = (sieve - 3) / 128;
              bit = (INT) 1 << (((sieve - 3) / 2) % 64);
              sprintf(buf, fmt, index);
              v = btree.findVariable(buf);
              if(v == (Variable *) 0) {
                v = new Variable(buf);
                v->setObject(cache.newNumber((INT) -1));
                btree.addVariable(v);
              }
              num = v->getObject()->getNumber(getLexInfo());
              num->setInt(num->getInt() & (~ bit));
              num->release(getLexInfo());

              sieve += 2 * candidate;
            }
          }

          candidate += 2;
        }

        // Now count the number of primes generated.
        // fixme - this is hopeleessly inefficient. Should be counting bits using a loop around x & (x - 1).
        lastIndex = (INT) -1;
        for(candidate = 3; candidate <= lastReq; candidate += 2) {
          index = (candidate - 3) / 128;
          bit = (INT) 1 << (((candidate - 3) / 2) % 64);
          if(index != lastIndex) {
            sprintf(buf, fmt, index);
            v = btree.findVariable(buf);
            num = v->getObject()->getNumber(getLexInfo());
            word = num->getInt();
            num->release(getLexInfo());
            lastIndex = index;
          }
          if(word & bit) {
            count++;
            last = candidate;
          }
        }
      }
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

PrimesGet::PrimesGet(LexInfo *li) : Operation(li) { }

OperatorReturn PrimesGet::action() {
  Object *n;
  Object *ns;
  Variable *v;
  Name *nsn;
  String *memoryType;
  char *name;
  Number *num;
  INT number;
  char fmt[32];
  char buf[1024];
  bool isArrayType;
  bool isPrime;
  Number *ret;
  INT count;
  INT c;
  INT index;
  INT word;
  INT bit;
  INT last;
  INT i;
  INT j;
  static INT cachedIndex = -1;
  static INT cachedC = -1;
  static INT cachedI = -1;

  ns = stack.pop(getLexInfo());
  n = stack.pop(getLexInfo());
  nsn = ns->getName(getLexInfo());
  name = nsn->getValue();
  num = n->getNumber(getLexInfo());
  index = num->getInt();
  num->release(getLexInfo());

  // Check what memory type this is.
  sprintf(buf, "/type/%s", name);
  v = btree.findVariable(buf);
  if(v == (Variable *) 0) {
    isArrayType = true;
  } else {
    memoryType = v->getObject()->getString(getLexInfo());
    isArrayType = (strcmp(memoryType->getValue(), "array") == 0);
    memoryType->release(getLexInfo());
  }

  sprintf(fmt, "/%%%sd/%s", PCTD, name);

  if(isArrayType) {
    sprintf(buf, fmt, index);
    if((v = btree.findVariable(buf)) == (Variable *) 0) {
      ret = cache.newNumber((INT) 0);
    } else {
      ret = v->getObject()->getNumber(getLexInfo());
    }
  } else {
    // Sieve type
    if(index == (INT) 0) {
      ret = cache.newNumber((INT) 2);
    } else {
      // fixme - this is really crud because it start at the beginning and linearly searches for the
      // index-th prime.
      sprintf(buf, "/count/%s", name);
      v = btree.findVariable(buf);
      if(v == (Variable *) 0) slexception.chuck("This doesn't look like a primes list", getLexInfo());
      num = v->getObject()->getNumber(getLexInfo());
      count = num->getInt();
      num->release(getLexInfo());
      ret = (Number *) 0;
      if((cachedIndex >= 0) && (index >= cachedIndex)) {
        c = cachedC;
        i = cachedI;
      } else {
        c = (INT) 0;
        i = (INT) 0;
      }
      cachedIndex = index;
      while(c < count) {
        cachedC = c;
        cachedI = i;

        // get the word from the btree
        sprintf(buf, fmt, i);
        v = btree.findVariable(buf);
        if(v == (Variable *) 0) break;
        num = v->getObject()->getNumber(getLexInfo());
        word = num->getInt();
        num->release(getLexInfo());
        bit = 0x01;
        for(j = 0; j < 64; j++) {
          // if the bit is set, then increase the count.
          // if we have hit our index then return the result.
          if(word & bit) {
            c++;
            if(c == index) {
              ret = cache.newNumber((i * 64 + j) * 2 + 3);
              break;
            }
          }
          bit <<= 1;
        }
        if(ret != (Number *) 0) break;
        i++;
      }
      if(ret == (Number *) 0) {
        cachedIndex = -1;
        ret = cache.newNumber((INT) 0);
      }
    }
  }

  stack.push(ret);

  n->release(getLexInfo());
  ns->release(getLexInfo());

  return or_continue;
}

PrimesIsPrime::PrimesIsPrime(LexInfo *li) : Operation(li) { }

OperatorReturn PrimesIsPrime::action() {
  Object *n;
  Object *ns;
  Variable *v;
  Name *nsn;
  String *memoryType;
  char *name;
  Number *num;
  INT number;
  char fmt[32];
  char buf[1024];
  bool isArrayType;
  bool isPrime;
  Number *ret;
  INT index;
  INT bit;
  INT count;
  INT upper;
  INT lower;
  INT mid;
  INT lastMid;
  INT i;

  ns = stack.pop(getLexInfo());
  n = stack.pop(getLexInfo());
  nsn = ns->getName(getLexInfo());
  name = nsn->getValue();
  num = n->getNumber(getLexInfo());
  number = num->getInt();
  num->release(getLexInfo());

  // Check what memory type this is.
  sprintf(buf, "/type/%s", name);
  v = btree.findVariable(buf);
  if(v == (Variable *) 0) {
    isArrayType = true;
  } else {
    memoryType = v->getObject()->getString(getLexInfo());
    isArrayType = (strcmp(memoryType->getValue(), "array") == 0);
    memoryType->release(getLexInfo());
  }

  sprintf(fmt, "/%%%sd/%s", PCTD, name);

  if(isArrayType) {
    // Find the number of entries.
    isPrime = false;
    sprintf(buf, "/count/%s", name);
    v = btree.findVariable(buf);
    if(v == (Variable *) 0) slexception.chuck("This doesn't look like a primes array", getLexInfo());
    num = v->getObject()->getNumber(getLexInfo());
    count = num->getInt();
    num->release(getLexInfo());
    lower = (INT) 0;
    upper = count;
    lastMid = (INT) -1;
    while((lastMid != mid) && (lower < upper)) {
      lastMid = mid;
      mid = (lower + upper) / 2;
      sprintf(buf, fmt, mid);
      if((v = btree.findVariable(buf)) == (Variable *) 0) slexception.chuck("Can't find the middle", getLexInfo());
      num = v->getObject()->getNumber(getLexInfo());
      i = num->getInt();
      num->release(getLexInfo());
      if(i == number) {
        isPrime = true;
        break;
      }
      if(i < number) lower = mid;
      else upper = mid;
    }
  } else {
    // Sieve type
    if((number & (INT) 1) == 0) {
      isPrime = (number == (INT) 2);
    } else {
      index = (number - 3) / 128;
      bit = (INT) 1 << (((number - 3) / 2) % 64);
      sprintf(buf, fmt, index);
      v = btree.findVariable(buf);
      num = v->getObject()->getNumber(getLexInfo());
      isPrime = (num->getInt() & bit);
      num->release(getLexInfo());
    }
  }
  stack.push(cache.newNumber(isPrime ? (INT) 1 : (INT) 0));

  n->release(getLexInfo());
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
  String *type;
  char buf[1024];
  char fmt[64];
  INT count;
  INT index;
  INT prime;

  ns = stack.pop(getLexInfo());
  nsn = ns->getName(getLexInfo());
  name = nsn->getValue();

  sprintf(buf, "/type/%s", name);
  v = btree.findVariable(buf);
  if(v != (Variable *) 0) {
    type = v->getObject()->getString(getLexInfo());
    if(strcmp(type->getValue(), "array") != 0) slexception.chuck("map primes::() is only available for array memory type", getLexInfo());
    type->release(getLexInfo());
  }

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
