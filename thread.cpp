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

class ThreadHelp : public Operation {
  public:
    ThreadHelp(LexInfo *);
    OperatorReturn action();
};

class ThreadCreate : public Operation {
  public:
    ThreadCreate(LexInfo *);
    OperatorReturn action();
};

class ThreadJoin : public Operation {
  public:
    ThreadJoin(LexInfo *);
    OperatorReturn action();
};

class ThreadMutex : public Operation {
  public:
    ThreadMutex(LexInfo *);
    OperatorReturn action();
};

class ThreadLock : public Operation {
  public:
    ThreadLock(LexInfo *);
    OperatorReturn action();
};

class ThreadUnlock : public Operation {
  public:
    ThreadUnlock(LexInfo *);
    OperatorReturn action();
};

class ThreadSemaphore : public Operation {
  public:
    ThreadSemaphore(LexInfo *);
    OperatorReturn action();
};

class ThreadWait : public Operation {
  public:
    ThreadWait(LexInfo *);
    OperatorReturn action();
};

class ThreadPost : public Operation {
  public:
    ThreadPost(LexInfo *);
    OperatorReturn action();
};

class ThreadLockNamespaces : public Operation {
  public:
    ThreadLockNamespaces(LexInfo *);
    OperatorReturn action();
};

class ThreadUnlockNamespaces : public Operation {
  public:
    ThreadUnlockNamespaces(LexInfo *);
    OperatorReturn action();
};

const char *threadHelp[] = {
  "this is the help"
};

extern "C" void slmain() {
  OperationList *ol;
  Variable *v;

  // Are we already loaded?
  if(btree.findVariable("/help/thread") != (Variable *) 0) return;

  ol = new OperationList;
  ol->addOperation(new ThreadHelp((LexInfo *) 0));
  v = new Variable("/help/thread");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  v = new Variable("/major/version/thread");
  v->setObject(cache.newNumber(MAJOR));
  btree.addVariable(v);

  v = new Variable("/minor/version/thread");
  v->setObject(cache.newNumber(MINOR));
  btree.addVariable(v);

  v = new Variable("/micro/version/thread");
  v->setObject(cache.newNumber(MICRO));
  btree.addVariable(v);
}

ThreadHelp::ThreadHelp(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadHelp::action() {
  const char **p;

  for(p = threadHelp; *p != (const char *) 0; p++) {
    printf("%s\n", *p);
  }

  return or_continue;
}

ThreadCreate::ThreadCreate(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadCreate::action() {
  return or_continue;
}

ThreadJoin::ThreadJoin(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadJoin::action() {
  return or_continue;
}

ThreadMutex::ThreadMutex(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadMutex::action() {
  return or_continue;
}

ThreadLock::ThreadLock(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadLock::action() {
  return or_continue;
}

ThreadUnlock::ThreadUnlock(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadUnlock::action() {
  return or_continue;
}

ThreadSemaphore::ThreadSemaphore(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadSemaphore::action() {
  return or_continue;
}

ThreadWait::ThreadWait(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadWait::action() {
  return or_continue;
}

ThreadPost::ThreadPost(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadPost::action() {
  return or_continue;
}

ThreadLockNamespaces::ThreadLockNamespaces(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadLockNamespaces::action() {
  return or_continue;
}

ThreadUnlockNamespaces::ThreadUnlockNamespaces(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadUnlockNamespaces::action() {
  return or_continue;
}
