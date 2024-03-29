/*

MIT License

Copyright (c) 2021-2023 Graeme Elsworthy <github@sharkshead.com>

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
#define MINOR   (INT) 1
#define MICRO   (INT) 3

class TimeHelp : public Operation {
  public:
    TimeHelp(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class TimeNow : public Operation {
  public:
    TimeNow(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class TimeDate : public Operation {
  public:
    TimeDate(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class TimeTime : public Operation {
  public:
    TimeTime(LexInfo *, bool);
    OperatorReturn action(ExecutionEnvironment *);

  private:
    bool includeMs;
};

class TimeLocaltime : public Operation {
  public:
    TimeLocaltime(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

class TimeSleep : public Operation {
  public:
    TimeSleep(LexInfo *);
    OperatorReturn action(ExecutionEnvironment *);
};

const char *timeHelp[] = {
  "Time library:",
  "  now time::()                      - Returns the time library's tepoch time.",
  "                                      This is the Unix epoch time times 1000 plus milliseconds",
  "  {tepoch} date time::()            - Returns a string on the stack containing the",
  "                                      day month year of the {tepoch}. If {tepoch}",
  "                                      is zero then the current date is returned.",
  "                                      Language aware through the language option:: shale::",
  "                                      option - \"en\" (the default), \"de\" and \"fr\" only",
  "  {tepoch} time time::()            - Returns a string on the stack containing the",
  "                                      hours, minutes and seconds (24h, not 12h) of the {tepoch}.",
  "                                      If {tepoch} is zero then the current time is returned",
  "  {tepoch} timems time::()          - same as time time::() but includes milliseconds",
  "  {tepoch} localtime time::()       - stores the 9 elements of struct tm in the tm:: time:: namespace,",
  "                                      each entry without the tm_ prefix, eg, sec tm:: time::",
  "                                      If {tepoch} is zero then the current time is used",
  "  {ms} sleep time::()               - sleep for the specified milli seconds",
  "  dateformat time:: {fmt} =         - Format of the output of date time::(). Default is \"DMY\"",
  "                                      with options \"MDY\", \"YMD\" or \"Y-M-D\"",
  "  language option:: shale:: {l} =   - Change the language output by date time::()",
  "                                      where {l} can be \"en\" (the default), \"de\" or \"fr\"",
  "  major version:: time::            - major version number",
  "  minor version:: time::            - minor version number",
  "  micro version:: time::            - micro version number",
  "  help time::()                     - this",
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
  v->setObject(new Code(ol, &mainEE.cache));
  btree.addVariable(v);

  v = new Variable("/major/version/time");
  v->setObject(mainEE.cache.newNumber(MAJOR));
  btree.addVariable(v);

  v = new Variable("/minor/version/time");
  v->setObject(mainEE.cache.newNumber(MINOR));
  btree.addVariable(v);

  v = new Variable("/micro/version/time");
  v->setObject(mainEE.cache.newNumber(MICRO));
  btree.addVariable(v);

  v = new Variable("/dateformat/time");
  v->setObject(mainEE.cache.newString("DMY", false));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new TimeNow((LexInfo *) 0));
  v = new Variable("/now/time");
  v->setObject(new Code(ol, &mainEE.cache));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new TimeDate((LexInfo *) 0));
  v = new Variable("/date/time");
  v->setObject(new Code(ol, &mainEE.cache));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new TimeTime((LexInfo *) 0, false));
  v = new Variable("/time/time");
  v->setObject(new Code(ol, &mainEE.cache));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new TimeTime((LexInfo *) 0, true));
  v = new Variable("/timems/time");
  v->setObject(new Code(ol, &mainEE.cache));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new TimeLocaltime((LexInfo *) 0));
  v = new Variable("/localtime/time");
  v->setObject(new Code(ol, &mainEE.cache));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new TimeSleep((LexInfo *) 0));
  v = new Variable("/sleep/time");
  v->setObject(new Code(ol, &mainEE.cache));
  btree.addVariable(v);
}

TimeHelp::TimeHelp(LexInfo *li) : Operation(li) { }

OperatorReturn TimeHelp::action(ExecutionEnvironment *ee) {
  const char **p;

  for(p = timeHelp; *p != (const char *) 0; p++) {
    printf("%s\n", *p);
  }

  return or_continue;
}

TimeNow::TimeNow(LexInfo *li) : Operation(li) { }

OperatorReturn TimeNow::action(ExecutionEnvironment *ee) {
  struct timespec tp;

  if(clock_gettime(CLOCK_REALTIME, &tp) == 0) {
    ee->stack.push(mainEE.cache.newNumber((((INT) tp.tv_sec) * 1000) + (((INT) tp.tv_nsec) / 1000000)));
  } else {
    printf("Can't get the realtime clock. Bailing.\n");
    exit(1);
  }

  return or_continue;
}

TimeDate::TimeDate(LexInfo *li) : Operation(li) { }

OperatorReturn TimeDate::action(ExecutionEnvironment *ee) {
  static const char *en_month[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  static const char *de_month[12] = { "Jan", "Feb", "Mar", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez" };
  static const char *fr_month[12] = { "Janv","Févr","Mars","Avr", "Mai", "Juin","Juil","Août","Sept","Oct", "Nov", "Déc" };
  const char **month;
  struct timespec tp;
  tm *tm;
  Object *o;
  Number *n;
  Variable *v;
  String *s;
  const char *str;
  INT epoch;
  time_t t;
  char *output;
  const char *fmt;

  o = ee->stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo(), ee);

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
  v = btree.findVariable("/language/option/shale");
  if(v != (Variable *) 0) {
    s = v->getObject()->getString(getLexInfo(), ee);
    str = s->getValue();
    if(strcmp(str, "de") == 0) month = de_month;
    else if(strcmp(str, "fr") == 0) month = fr_month;
    else month = en_month;
    s->release(getLexInfo());
  } else {
    month = en_month;
  }

  v = btree.findVariable("/dateformat/time");
  if(v != (Variable *) 0) {
    s = v->getObject()->getString(getLexInfo(), ee);
    fmt = s->getValue();
    s->release(getLexInfo());
  } else {
    fmt = "DMY";
  }

  if((strcmp(fmt, "MDY") == 0) || (strcmp(fmt, "mdy") == 0)) {
    sprintf(output, "%s %2d %4d", month[tm->tm_mon], tm->tm_mday, tm->tm_year + 1900);
  } else if((strcmp(fmt, "YMD") == 0) || (strcmp(fmt, "ymd") == 0)) {
    sprintf(output, "%4d %s %2d", tm->tm_year + 1900, month[tm->tm_mon], tm->tm_mday);
  } else if((strcmp(fmt, "Y-M-D") == 0) || (strcmp(fmt, "y-m-d") == 0)) {
    sprintf(output, "%4d-%02d-%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
  } else {
    sprintf(output, "%2d %s %4d", tm->tm_mday, month[tm->tm_mon], tm->tm_year + 1900);
  }
  ee->stack.push(mainEE.cache.newString(output, true));

  n->release(getLexInfo());
  o->release(getLexInfo());

  return or_continue;
}

TimeTime::TimeTime(LexInfo *li, bool ms) : Operation(li), includeMs(ms) { }

OperatorReturn TimeTime::action(ExecutionEnvironment *ee) {
  struct timespec tp;
  tm *tm;
  Object *o;
  Number *n;
  INT epoch;
  time_t t;
  char *output;
  int ms;

  o = ee->stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo(), ee);

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
  ms = epoch % 1000;
  tm = localtime(&t);
  if((output = (char *) malloc(64)) == (char *) 0) {
    printf("Out of memory in date time::()\n");
    exit(1);
  }
  if(includeMs)
    sprintf(output, "%2d:%02d:%02d.%03d", tm->tm_hour, tm->tm_min, tm->tm_sec, ms);
  else
    sprintf(output, "%2d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
  ee->stack.push(mainEE.cache.newString(output, true));

  n->release(getLexInfo());
  o->release(getLexInfo());

  return or_continue;
}

TimeLocaltime::TimeLocaltime(LexInfo *li) : Operation(li) { }

OperatorReturn TimeLocaltime::action(ExecutionEnvironment *ee) {
  struct timespec tp;
  tm *tm;
  Object *o;
  Number *n;
  INT epoch;
  time_t t;
  char *output;
  int ms;
  Variable *v;

  o = ee->stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo(), ee);

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
  ms = epoch % 1000;
  tm = localtime(&t);

  v = btree.findVariable("/sec/tm/time");
  if(v == (Variable *) 0) {
    v = new Variable("/sec/tm/time");
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_sec));
    btree.addVariable(v);
    
  } else {
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_sec));
  }

  v = btree.findVariable("/min/tm/time");
  if(v == (Variable *) 0) {
    v = new Variable("/min/tm/time");
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_min));
    btree.addVariable(v);
    
  } else {
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_min));
  }

  v = btree.findVariable("/hour/tm/time");
  if(v == (Variable *) 0) {
    v = new Variable("/hour/tm/time");
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_hour));
    btree.addVariable(v);
    
  } else {
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_hour));
  }

  v = btree.findVariable("/mday/tm/time");
  if(v == (Variable *) 0) {
    v = new Variable("/mday/tm/time");
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_mday));
    btree.addVariable(v);
    
  } else {
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_mday));
  }

  v = btree.findVariable("/mon/tm/time");
  if(v == (Variable *) 0) {
    v = new Variable("/mon/tm/time");
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_mon));
    btree.addVariable(v);
    
  } else {
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_mon));
  }

  v = btree.findVariable("/year/tm/time");
  if(v == (Variable *) 0) {
    v = new Variable("/year/tm/time");
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_year));
    btree.addVariable(v);
    
  } else {
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_year));
  }

  v = btree.findVariable("/wday/tm/time");
  if(v == (Variable *) 0) {
    v = new Variable("/wday/tm/time");
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_wday));
    btree.addVariable(v);
    
  } else {
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_wday));
  }

  v = btree.findVariable("/yday/tm/time");
  if(v == (Variable *) 0) {
    v = new Variable("/yday/tm/time");
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_yday));
    btree.addVariable(v);
    
  } else {
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_yday));
  }

  v = btree.findVariable("/isdst/tm/time");
  if(v == (Variable *) 0) {
    v = new Variable("/isdst/tm/time");
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_isdst));
    btree.addVariable(v);
    
  } else {
    v->setObject(mainEE.cache.newNumber((INT) tm->tm_isdst));
  }

  n->release(getLexInfo());
  o->release(getLexInfo());

  return or_continue;
}

TimeSleep::TimeSleep(LexInfo *li) : Operation(li) { }

OperatorReturn TimeSleep::action(ExecutionEnvironment *ee) {
  Object *o;
  Number *n;

  o = ee->stack.pop(getLexInfo());
  n = o->getNumber(getLexInfo(), ee);

  usleep(n->getInt() * 1000);

  n->release(getLexInfo());
  o->release(getLexInfo());

  return or_continue;
}
