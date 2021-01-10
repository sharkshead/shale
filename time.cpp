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
#include <time.h>

#define MAJOR   (INT) 1
#define MINOR   (INT) 0
#define MICRO   (INT) 0

class TimeHelp : public Operation {
  public:
    TimeHelp(LexInfo *);
    OperatorReturn action();
};

class TimeNow : public Operation {
  public:
    TimeNow(LexInfo *);
    OperatorReturn action();
};

class TimeDate : public Operation {
  public:
    TimeDate(LexInfo *);
    OperatorReturn action();
};

class TimeTime : public Operation {
  public:
    TimeTime(LexInfo *);
    OperatorReturn action();
};

const char *timeHelp[] = {
  "Time library:",
  "  now time::()                          - Returns the time library's epoch (time-epoch) time",
  "  {time-epoch} date time::()            - Returns a string on the stack containing the",
  "                                          day month year of the {time-epoch}. If {time-epoch}",
  "                                          is zero then the current date is returned",
  "  {time-epoch} time time::()            - Returns a string on the stack containing the",
  "                                          hours, minutes and seconds (24h, not 12h) of the {time-epoch}.",
  "                                          If {time-epoch} is zero then the current time is returned",
  "  major version:: time::                - major version number",
  "  minor version:: time::                - minor version number",
  "  micro version:: time::                - micro version number",
  "  help time::()                         - this",
  (const char *) 0
};

extern "C" void slmain() {
  OperationList *ol;
  Variable *v;

  // Are we already loaded?
  if(btree.findVariable("/help/time") != (Variable *) 0) return;

  ol = new OperationList;
  ol->addOperation(new TimeHelp((LexInfo *) 0));
  v = new Variable("/help/time");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  v = new Variable("/major/version/time");
  v->setObject(cache.newNumber(MAJOR));
  btree.addVariable(v);

  v = new Variable("/minor/version/time");
  v->setObject(cache.newNumber(MINOR));
  btree.addVariable(v);

  v = new Variable("/micro/version/time");
  v->setObject(cache.newNumber(MICRO));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new TimeNow((LexInfo *) 0));
  v = new Variable("/now/time");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new TimeDate((LexInfo *) 0));
  v = new Variable("/date/time");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new TimeTime((LexInfo *) 0));
  v = new Variable("/time/time");
  v->setObject(new Code(ol));
  btree.addVariable(v);
}

TimeHelp::TimeHelp(LexInfo *li) : Operation(li) { }

OperatorReturn TimeHelp::action() {
  const char **p;

  for(p = timeHelp; *p != (const char *) 0; p++) {
    printf("%s\n", *p);
  }

  return or_continue;
}

TimeNow::TimeNow(LexInfo *li) : Operation(li) { }

OperatorReturn TimeNow::action() {
  struct timespec tp;

  if(clock_gettime(CLOCK_REALTIME, &tp) == 0) {
    stack.push(cache.newNumber((((INT) tp.tv_sec) * 1000) + (((INT) tp.tv_nsec) / 1000000)));
  } else {
    printf("Can't get the realtime clock. Bailing.\n");
    exit(1);
  }

  return or_continue;
}

TimeDate::TimeDate(LexInfo *li) : Operation(li) { }

OperatorReturn TimeDate::action() {
  static const char *month[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  struct timespec tp;
  tm *tm;
  Object *o;
  Number *n;
  INT epoch;
  time_t t;
  char *output;

  o = stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo());

  epoch = n->getInt();
  if(epoch == (INT) 0) {
    if(clock_gettime(CLOCK_REALTIME, &tp) == 0) {
      epoch = (((INT) tp.tv_sec) * 1000) + (((INT) tp.tv_nsec) / 1000000);
    } else {
      printf("Can't get the realtime clock. Bailing.\n");
      exit(1);
    }
  }
  t = epoch / 1000;
  tm = localtime(&t);
  if((output = (char *) malloc(64)) == (char *) 0) {
    printf("Out of memory in date time::()\n");
    exit(1);
  }
  sprintf(output, "%2d %s %4d", tm->tm_mday, month[tm->tm_mon], tm->tm_year + 1900);
  stack.push(cache.newString(output, true));

  n->release(getLexInfo());
  o->release(getLexInfo());

  return or_continue;
}

TimeTime::TimeTime(LexInfo *li) : Operation(li) { }

OperatorReturn TimeTime::action() {
  struct timespec tp;
  tm *tm;
  Object *o;
  Number *n;
  INT epoch;
  time_t t;
  char *output;

  o = stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo());

  epoch = n->getInt();
  if(epoch == (INT) 0) {
    if(clock_gettime(CLOCK_REALTIME, &tp) == 0) {
      epoch = (((INT) tp.tv_sec) * 1000) + (((INT) tp.tv_nsec) / 1000000);
    } else {
      printf("Can't get the realtime clock. Bailing.\n");
      exit(1);
    }
  }
  t = epoch / 1000;
  tm = localtime(&t);
  if((output = (char *) malloc(64)) == (char *) 0) {
    printf("Out of memory in date time::()\n");
    exit(1);
  }
  sprintf(output, "%2d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
  stack.push(cache.newString(output, true));

  n->release(getLexInfo());
  o->release(getLexInfo());

  return or_continue;
}
