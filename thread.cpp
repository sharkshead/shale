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

class ThreadPack {
  public:
    ExecutionEnvironment ee;
    Code *code;
};

class ThreadHelp : public Operation {
  public:
    ThreadHelp(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class ThreadCreate : public Operation {
  public:
    ThreadCreate(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class ThreadMutex : public Operation {
  public:
    ThreadMutex(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class ThreadLock : public Operation {
  public:
    ThreadLock(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class ThreadUnlock : public Operation {
  public:
    ThreadUnlock(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class ThreadSemaphore : public Operation {
  public:
    ThreadSemaphore(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class ThreadWait : public Operation {
  public:
    ThreadWait(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class ThreadPost : public Operation {
  public:
    ThreadPost(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

const char *threadHelp[] = {
  "Thread library",
  "  {code} create thread::()    - create a new thread running the given code",
  "  {m} mutex thread::()        - create and initialise a mutex",
  "  {m} lock thread::()         - lock a mutex",
  "  {m} unlock thread::()       - unlock a mutex",
  "  {s} semaphore thread::()    - create and initialise a semaphore",
  "  {s} wait thread::()         - wait on a semaphore",
  "  {s} post thread::()         - post to a semaphore",
  "  major version:: thread::    - major version number",
  "  minor version:: thread::    - minor version number",
  "  micro version:: number::    - micro version number",
  (const char *) 0
};

extern "C" void slmain() {
  OperationList *ol;
  Variable *v;
  pthread_mutex_t *btreeMutex;
  pthread_mutex_t *cacheMutex;

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

  ol = new OperationList;
  ol->addOperation(new ThreadMutex((LexInfo *) 0));
  v = new Variable("/mutex/thread");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new ThreadCreate((LexInfo *) 0));
  v = new Variable("/create/thread");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new ThreadLock((LexInfo *) 0));
  v = new Variable("/lock/thread");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new ThreadUnlock((LexInfo *) 0));
  v = new Variable("/unlock/thread");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new ThreadSemaphore((LexInfo *) 0));
  v = new Variable("/semaphore/thread");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new ThreadWait((LexInfo *) 0));
  v = new Variable("/wait/thread");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new ThreadPost((LexInfo *) 0));
  v = new Variable("/post/thread");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  cacheMutex = new pthread_mutex_t;
  pthread_mutex_init(cacheMutex, NULL);
  cache.setMutex(cacheMutex);

  btreeMutex = new pthread_mutex_t;
  pthread_mutex_init(btreeMutex, NULL);
  btree.setMutex(btreeMutex);
}

ThreadHelp::ThreadHelp(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadHelp::action(ExecutionEnvironment *ee) {
  const char **p;

  for(p = threadHelp; *p != (const char *) 0; p++) {
    printf("%s\n", *p);
  }

  return or_continue;
}

void *theThread(void *arg) {
  ThreadPack *tp = (ThreadPack *) arg;

  tp->code->action(&tp->ee);
  tp->code->release((LexInfo *) 0);
  delete(tp);

  return (void *) 0;
}

ThreadCreate::ThreadCreate(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadCreate::action(ExecutionEnvironment *ee) {
  Object *o;
  Code *code;
  ThreadPack *tp;
  pthread_t thread;
  pthread_attr_t attr;

  o = ee->stack.pop(getLexInfo());

  tp = new ThreadPack;
  tp->code = o->getCode(getLexInfo(), ee);

  try {
    if(pthread_attr_init(&attr) != 0) slexception.chuck("Can't initialise thread attributes", getLexInfo());
    if(pthread_attr_setstacksize(&attr, 1024*1024) != 0) slexception.chuck("Can't set thread stack size", getLexInfo());
    if(pthread_create(&thread, &attr, theThread, tp) != 0) slexception.chuck("Can't create thread", getLexInfo());
  } catch(Exception *e) {
    e->printError();
  } catch(exception &e) {
    printf("%s\n", e.what());
  }

  o->release(getLexInfo());

  return or_continue;
}

ThreadMutex::ThreadMutex(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadMutex::action(ExecutionEnvironment *ee) {
  Object *o;
  Name *n;
  Number *no;
  String *s;
  Variable *v;
  char *str;
  char fmt[16];
  char name[64];
  char mutexName[64];
  pthread_mutex_t *mutex;

  o = ee->stack.pop(getLexInfo());

  name[0] = 0;

  try {
    no = o->getNumber(getLexInfo(), ee);
    if(no->isInt()) { sprintf(fmt, "%%%sd", PCTD); sprintf(name, fmt, no->getInt()); }
    else sprintf(name, "%0.3f", no->getDouble());
    no->release(getLexInfo());
  } catch(Exception *e) { }

  if(name[0] == 0) {
    try {
      s = o->getString(getLexInfo(), ee);
      strcpy(name, s->getValue());
      s->release(getLexInfo());
    } catch(Exception *e) { }
  }

  if(name[0] == 0) {
    try {
      n = o->getName(getLexInfo(), ee);
      str = n->getValue();
      strcpy(name, str + (str[0] == '/' ? 1 : 0));
    } catch(Exception *e) { }
  }

  if(name[0] == 0) slexception.chuck("Unknown argument type", getLexInfo());

  mutex = new pthread_mutex_t;
  pthread_mutex_init(mutex, NULL);
  sprintf(mutexName, "/%s/mutex/thread", name);
  v = btree.findVariable(mutexName);
  if(v != (Variable *) 0) slexception.chuck("Name already exists", getLexInfo());
  v = new Variable(mutexName);
  v->setObject(cache.newNumber((INT) mutex));
  btree.addVariable(v);

  o->release(getLexInfo());

  return or_continue;
}

ThreadLock::ThreadLock(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadLock::action(ExecutionEnvironment *ee) {
  Object *o;
  Name *n;
  Number *no;
  String *s;
  Variable *v;
  char *str;
  char fmt[16];
  char name[64];
  char mutexName[64];
  pthread_mutex_t *mutex;

  o = ee->stack.pop(getLexInfo());

  name[0] = 0;

  try {
    no = o->getNumber(getLexInfo(), ee);
    if(no->isInt()) { sprintf(fmt, "%%%sd", PCTD); sprintf(name, fmt, no->getInt()); }
    else sprintf(name, "%0.3f", no->getDouble());
    no->release(getLexInfo());
  } catch(Exception *e) { }

  if(name[0] == 0) {
    try {
      s = o->getString(getLexInfo(), ee);
      strcpy(name, s->getValue());
      s->release(getLexInfo());
    } catch(Exception *e) { }
  }

  if(name[0] == 0) {
    try {
      n = o->getName(getLexInfo(), ee);
      str = n->getValue();
      strcpy(name, str + (str[0] == '/' ? 1 : 0));
    } catch(Exception *e) { }
  }

  if(name[0] == 0) slexception.chuck("Unknown argument type", getLexInfo());

  sprintf(mutexName, "/%s/mutex/thread", name);
  v = btree.findVariable(mutexName);
  if(v == (Variable *) 0) slexception.chuck("Mutex not found", getLexInfo());
  no = v->getObject()->getNumber(getLexInfo(), ee);
  pthread_mutex_lock((pthread_mutex_t *) no->getInt());
  no->release(getLexInfo());

  o->release(getLexInfo());

  return or_continue;
}

ThreadUnlock::ThreadUnlock(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadUnlock::action(ExecutionEnvironment *ee) {
  Object *o;
  Name *n;
  Number *no;
  String *s;
  Variable *v;
  char *str;
  char fmt[16];
  char name[64];
  char mutexName[64];
  pthread_mutex_t *mutex;

  o = ee->stack.pop(getLexInfo());

  name[0] = 0;

  try {
    no = o->getNumber(getLexInfo(), ee);
    if(no->isInt()) { sprintf(fmt, "%%%sd", PCTD); sprintf(name, fmt, no->getInt()); }
    else sprintf(name, "%0.3f", no->getDouble());
    no->release(getLexInfo());
  } catch(Exception *e) { }

  if(name[0] == 0) {
    try {
      s = o->getString(getLexInfo(), ee);
      strcpy(name, s->getValue());
      s->release(getLexInfo());
    } catch(Exception *e) { }
  }

  if(name[0] == 0) {
    try {
      n = o->getName(getLexInfo(), ee);
      str = n->getValue();
      strcpy(name, str + (str[0] == '/' ? 1 : 0));
    } catch(Exception *e) { }
  }

  if(name[0] == 0) slexception.chuck("Unknown argument type", getLexInfo());

  sprintf(mutexName, "/%s/mutex/thread", name);
  v = btree.findVariable(mutexName);
  if(v == (Variable *) 0) slexception.chuck("Mutex not found", getLexInfo());
  no = v->getObject()->getNumber(getLexInfo(), ee);
  pthread_mutex_unlock((pthread_mutex_t *) no->getInt());
  no->release(getLexInfo());

  o->release(getLexInfo());

  return or_continue;
}

ThreadSemaphore::ThreadSemaphore(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadSemaphore::action(ExecutionEnvironment *ee) {
  Object *o;
  Name *n;
  Number *no;
  String *s;
  Variable *v;
  char *str;
  char fmt[16];
  char name[64];
  char semName[64];
  char externalName[64];
  sem_t *sem;
  char *p;

  o = ee->stack.pop(getLexInfo());

  name[0] = 0;

  try {
    no = o->getNumber(getLexInfo(), ee);
    if(no->isInt()) { sprintf(fmt, "%%%sd", PCTD); sprintf(name, fmt, no->getInt()); }
    else sprintf(name, "%0.3f", no->getDouble());
    no->release(getLexInfo());
  } catch(Exception *e) { }

  if(name[0] == 0) {
    try {
      s = o->getString(getLexInfo(), ee);
      strcpy(name, s->getValue());
      s->release(getLexInfo());
    } catch(Exception *e) { }
  }

  if(name[0] == 0) {
    try {
      n = o->getName(getLexInfo(), ee);
      str = n->getValue();
      strcpy(name, str + (str[0] == '/' ? 1 : 0));
    } catch(Exception *e) { }
  }

  if(name[0] == 0) slexception.chuck("Unknown argument type", getLexInfo());

  sprintf(semName, "/%s/semaphore/thread", name);
  sprintf(externalName, "/shale.%s", name);
  for(p = externalName; *p != 0; p++) {
    if(*p == '/') *p = '_';
  }
  v = btree.findVariable(semName);
  if(v != (Variable *) 0) slexception.chuck("Name already exists", getLexInfo());
  sem_unlink(externalName);
  if((sem = sem_open(externalName, O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) slexception.chuck("Can't create semaphore", getLexInfo());
  v = new Variable(semName);
  v->setObject(cache.newNumber((INT) sem));
  btree.addVariable(v);

  o->release(getLexInfo());

  return or_continue;
}

ThreadWait::ThreadWait(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadWait::action(ExecutionEnvironment *ee) {
  Object *o;
  Name *n;
  Number *no;
  String *s;
  Variable *v;
  char *str;
  char fmt[16];
  char name[64];
  char semName[64];
  sem_t *sem;

  o = ee->stack.pop(getLexInfo());

  name[0] = 0;

  try {
    no = o->getNumber(getLexInfo(), ee);
    if(no->isInt()) { sprintf(fmt, "%%%sd", PCTD); sprintf(name, fmt, no->getInt()); }
    else sprintf(name, "%0.3f", no->getDouble());
    no->release(getLexInfo());
  } catch(Exception *e) { }

  if(name[0] == 0) {
    try {
      s = o->getString(getLexInfo(), ee);
      strcpy(name, s->getValue());
      s->release(getLexInfo());
    } catch(Exception *e) { }
  }

  if(name[0] == 0) {
    try {
      n = o->getName(getLexInfo(), ee);
      str = n->getValue();
      strcpy(name, str + (str[0] == '/' ? 1 : 0));
    } catch(Exception *e) { }
  }

  if(name[0] == 0) slexception.chuck("Unknown argument type", getLexInfo());

  sprintf(semName, "/%s/semaphore/thread", name);
  v = btree.findVariable(semName);
  if(v == (Variable *) 0) slexception.chuck("Semaphore not found", getLexInfo());
  no = v->getObject()->getNumber(getLexInfo(), ee);
  sem_wait((sem_t *) no->getInt());
  no->release(getLexInfo());

  o->release(getLexInfo());

  return or_continue;
}

ThreadPost::ThreadPost(LexInfo *li) : Operation(li) { }

OperatorReturn ThreadPost::action(ExecutionEnvironment *ee) {
  Object *o;
  Name *n;
  Number *no;
  String *s;
  Variable *v;
  char *str;
  char fmt[16];
  char name[64];
  char semName[64];
  sem_t *sem;

  o = ee->stack.pop(getLexInfo());

  name[0] = 0;

  try {
    no = o->getNumber(getLexInfo(), ee);
    if(no->isInt()) { sprintf(fmt, "%%%sd", PCTD); sprintf(name, fmt, no->getInt()); }
    else sprintf(name, "%0.3f", no->getDouble());
    no->release(getLexInfo());
  } catch(Exception *e) { }

  if(name[0] == 0) {
    try {
      s = o->getString(getLexInfo(), ee);
      strcpy(name, s->getValue());
      s->release(getLexInfo());
    } catch(Exception *e) { }
  }

  if(name[0] == 0) {
    try {
      n = o->getName(getLexInfo(), ee);
      str = n->getValue();
      strcpy(name, str + (str[0] == '/' ? 1 : 0));
    } catch(Exception *e) { }
  }

  if(name[0] == 0) slexception.chuck("Unknown argument type", getLexInfo());

  sprintf(semName, "/%s/semaphore/thread", name);
  v = btree.findVariable(semName);
  if(v == (Variable *) 0) slexception.chuck("Semaphore not found", getLexInfo());
  no = v->getObject()->getNumber(getLexInfo(), ee);
  sem_post((sem_t *) no->getInt());
  no->release(getLexInfo());

  o->release(getLexInfo());

  return or_continue;
}
