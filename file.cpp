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

class FileHelp : public Operation {
  public:
    FileHelp(LexInfo *);
    bool action();
};

class FileOpen : public Operation {
  public:
    FileOpen(LexInfo *);
    bool action();
};

class FileClose : public Operation {
  public:
    FileClose(LexInfo *);
    bool action();
};

class FileFgets : public Operation {
  public:
    FileFgets(LexInfo *);
    bool action();
};

class FileFprintf : public Operation {
  public:
    FileFprintf(LexInfo *);
    bool action();
};

const char *fileHelp[] = {
  "File library",
  "  {filename} {mode} open file::()      - open filename according to fopen's mode. if the file cannot",
  "                                         be opened it pushes false on the stack, otherwise it",
  "                                         pushes the file handle then true on the stack.",
  "                                         file and mode must both be strings.",
  "  {handle} close file::()              - close the given file handle",
  "  {handle} fgets file::()              - get one line from the handle with newline stripped",
  "  ... {fmt} {handle} fprintf file::()  - fprintf to the given handle",
  "  stdin file::                         - stdin",
  "  stdout file::                        - stdout",
  "  stderr file::                        - stderr",
  "  major version:: file::               - major version number",
  "  minor version:: file::               - minor version number",
  "  micro version:: file::               - micro version number",
  "  help file::()                        - this",
  (const char *) 0
};

#define HANDLES 64
FILE *handle[HANDLES];

extern "C" void slmain() {
  OperationList *ol;
  Variable *v;
  int i;

  // Are we already loaded?
  if(btree.findVariable("/help/file") != (Variable *) 0) return;

  for(i = 0; i < HANDLES; i++) handle[i] = (FILE *) 0;
  handle[0] = stdin;
  handle[1] = stdout;
  handle[2] = stderr;

  ol = new OperationList;
  ol->addOperation(new FileHelp((LexInfo *) 0));
  v = new Variable("/help/file");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  v = new Variable("/major/version/file");
  v->setObject(new Number(MAJOR));
  btree.addVariable(v);

  v = new Variable("/minor/version/file");
  v->setObject(new Number(MINOR));
  btree.addVariable(v);

  v = new Variable("/micro/version/file");
  v->setObject(new Number(MICRO));
  btree.addVariable(v);

  v = new Variable("/stdin/file");
  v->setObject(new Number((INT) 0));
  btree.addVariable(v);

  v = new Variable("/stdout/file");
  v->setObject(new Number((INT) 1));
  btree.addVariable(v);

  v = new Variable("/stderr/file");
  v->setObject(new Number((INT) 2));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new FileOpen((LexInfo *) 0));
  v = new Variable("/open/file");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new FileClose((LexInfo *) 0));
  v = new Variable("/close/file");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new FileFgets((LexInfo *) 0));
  v = new Variable("/fgets/file");
  v->setObject(new Code(ol));
  btree.addVariable(v);

  ol = new OperationList;
  ol->addOperation(new FileFprintf((LexInfo *) 0));
  v = new Variable("/fprintf/file");
  v->setObject(new Code(ol));
  btree.addVariable(v);
}

FileHelp::FileHelp(LexInfo *li) : Operation(li) { }

bool FileHelp::action() {
  const char **p;

  for(p = fileHelp; *p != (const char *) 0; p++) {
    printf("%s\n", *p);
  }

  return true;
}

FileOpen::FileOpen(LexInfo *li) : Operation(li) { }

bool FileOpen::action() {
  Object *of;
  Object *om;
  String *filename;
  String *mode;
  int i;
  FILE *f;

  om = stack.pop(getLexInfo());
  of = stack.pop(getLexInfo());
  mode = om->getString(getLexInfo());
  filename = of->getString(getLexInfo());

  for(i = 0; i < HANDLES; i++) {
    if(handle[i] == (FILE *) 0) break;
  }
  if((i >= 0) && (i < HANDLES)) {
    if((f = fopen(filename->getValue(), mode->getValue())) != (FILE *) 0) {
      handle[i] = f;
      stack.push(cache.newNumber((INT) i));
      stack.push(cache.newNumber((INT) 1));
    } else {
      stack.push(cache.newNumber((INT) 0));
    }
  } else {
    stack.push(cache.newNumber((INT) 0));
  }

  mode->release(getLexInfo());
  filename->release(getLexInfo());
  om->release(getLexInfo());
  of->release(getLexInfo());

  return true;
}

FileClose::FileClose(LexInfo *li) : Operation(li) { }

bool FileClose::action() {
  Object *oh;
  Number *h;
  int i;

  oh = stack.pop(getLexInfo());
  h = oh->getNumber(getLexInfo());

  i = h->getInt();
  if((i >= 0) && (i < HANDLES)) {
    fclose(handle[i]);
    handle[i] = (FILE *) 0;
  } else {
    slexception.chuck("Close of unknown file handle", getLexInfo());
  }

  h->release(getLexInfo());
  oh->release(getLexInfo());

  return true;
}

FileFgets::FileFgets(LexInfo *li) : Operation(li) { }

bool FileFgets::action() {
  Object *oh;
  Number *h;
  int i;
  int fd;
  char line[10240];
  char *p;

  oh = stack.pop(getLexInfo());
  h = oh->getNumber(getLexInfo());

  i = h->getInt();
  if((i >= 0) && (i < HANDLES)) {
    if(fgets(line, sizeof(line), handle[i]) != NULL) {
      for(p = line; *p != 0; p++) {
        if((*p == '\n') || (*p == '\r')) {
          *p = 0;
          break;
        }
      }
      if((p = (char *) malloc(strlen(line) + 1)) == (char *) 0) {
        slexception.chuck("malloc error", getLexInfo());
      } else {
        strcpy(p, line);
        stack.push(cache.newString(p, true));
        stack.push(cache.newNumber((INT) 1));
      }
    } else {
      stack.push(cache.newNumber((INT) 0));
    }
  } else {
    stack.push(cache.newNumber((INT) 0));
  }

  h->release(getLexInfo());
  oh->release(getLexInfo());

  return true;
}

FileFprintf::FileFprintf(LexInfo *li) : Operation(li) { }

bool FileFprintf::action() {
  Object *handleObject;
  Object *formatObject;
  Object *o;
  Number *h;
  int i;
  String *format;
  Number *n;
  String *str;
  Name *name;
  const char *p;
  char *q;
  char buf[128];
  char res[10240];
  char *op;
  bool found;

  handleObject = stack.pop(getLexInfo());
  h = handleObject->getNumber(getLexInfo());
  i = h->getInt();
  formatObject = stack.pop(getLexInfo());
  format = formatObject->getString(getLexInfo());

  if((i < 0) || (i > HANDLES)) slexception.chuck("Bad handle", getLexInfo());

  op = res;
  *op = 0;

  for(p = format->getValue(); *p != 0; p++ ) {
    if(*p == '%') {
      q = buf;
      *q++ = *p++;
      while((*p != 0) && (*p != 'd') && (*p != 'f') && (*p != 's') && (*p != 't') && (*p != 'x') && (*p != 'X') && (*p != 'p') && (*p != 'n') && (*p != '%')) *q++ = *p++;
      if(*p == 0) slexception.chuck("format error", getLexInfo());
      *q++ = *p;
      *q = 0;
      if(*p == '%') {
        *op++ = '%';
        *op = 0;
      } else {
        o = stack.pop(getLexInfo());
        switch(*p) {
          case 'd':
          case 'x':
          case 'X':
            q--;
            *q++ = 'l';
            *q++ = *p;
            *q = 0;
            n = o->getNumber(getLexInfo());
            sprintf(op, buf, n->getInt());
            n->release(getLexInfo());
            break;

          case 'f':
            n = o->getNumber(getLexInfo());
            sprintf(op, buf, n->getDouble());
            n->release(getLexInfo());
            break;

          case 's':
            str = o->getString(getLexInfo());
            sprintf(op, buf, str->getValue());
            str->release(getLexInfo());
            break;

          case 'p':
            found = false;
            try {
              n = o->getNumber(getLexInfo());
              if(n->isInt()) sprintf(op, "%ld", n->getInt());
              else sprintf(op, "%0.3f", n->getDouble());
              n->release(getLexInfo());
              found = true;
            } catch(Exception *e) { }

            if(! found) {
              try {
                str = o->getString(getLexInfo());
                sprintf(op, "%s", str->getValue());
                str->release(getLexInfo());
                found = true;
              } catch(Exception *e) { }
            }
            if(! found) slexception.chuck("print error", getLexInfo());
            break;

          case 'n':
            found = false;
            try {
              name = o->getName(getLexInfo());
              sprintf(op, "%s", name->getValue());
              found = true;
            } catch(Exception *e) { }

            if(! found) slexception.chuck("unknown %n type", getLexInfo());

            break;
        }
        while(*op != 0) op++;
        o->release(getLexInfo());
      }
    } else if(*p == '\\') {
      p++;
      if(*p == 'n') { *op++ = '\n'; *op = 0; }
      else { *op++ = '\\'; *op++ = *p; *op = 0; }
    } else {
      *op++ = *p;
      *op = 0;
    }
  }

  fprintf(handle[i], "%s", res);

  format->release(getLexInfo());
  formatObject->release(getLexInfo());
  h->release(getLexInfo());
  handleObject->release(getLexInfo());

  return true;
}
