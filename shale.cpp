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

#include "shale.h"

FILE *lexInput;
int lexChar;
int lexSavedChar;
int lexLineNumber;
char lexLine[10240];
char *lexInfoLine;
int lexLineIndex;
int lexTokenIndex;
LexReplacement *lexReplacements;
char *lexReplacement;
bool lexSendNamespaceToken;    // This is really dodgy.
const char *shaleFileName;
int olStackIndex;
bool interactive;
Exception shaleException;

Keyword keyword[] = {
  { "dup",            LEX_TOKEN_KEYWORD_DUP             },
  { "pop",            LEX_TOKEN_KEYWORD_POP             },
  { "swap",           LEX_TOKEN_KEYWORD_SWAP            },
  { "var",            LEX_TOKEN_KEYWORD_VAR             },
  { "if",             LEX_TOKEN_KEYWORD_IF              },
  { "ifthen",         LEX_TOKEN_KEYWORD_IFTHEN          },
  { "while",          LEX_TOKEN_KEYWORD_WHILE           },
  { "execute",        LEX_TOKEN_KEYWORD_EXECUTE         },
  { "print",          LEX_TOKEN_KEYWORD_PRINT           },
  { "println",        LEX_TOKEN_KEYWORD_PRINTLN         },
  { "printf",         LEX_TOKEN_KEYWORD_PRINTF          },
  { "sprintf",        LEX_TOKEN_KEYWORD_SPRINTF         },
  { "true",           LEX_TOKEN_KEYWORD_TRUE            },
  { "false",          LEX_TOKEN_KEYWORD_FALSE           },
  { "int",            LEX_TOKEN_KEYWORD_INT             },
  { "double",         LEX_TOKEN_KEYWORD_DOUBLE          },
  { "float",          LEX_TOKEN_KEYWORD_DOUBLE          },
  { "break",          LEX_TOKEN_KEYWORD_BREAK           },
  { "exit",           LEX_TOKEN_KEYWORD_EXIT            },
  { "value",          LEX_TOKEN_KEYWORD_VALUE           },
  { "defined",        LEX_TOKEN_KEYWORD_DEFINED         },
  { "initialised",    LEX_TOKEN_KEYWORD_INITIALISED     },
  { "library",        LEX_TOKEN_KEYWORD_LIBRARY         },
  { "stack",          LEX_TOKEN_KEYWORD_STACK           },
  { "debug",          LEX_TOKEN_KEYWORD_DEBUG           },
  { "btree",          LEX_TOKEN_KEYWORD_BTREE           },
  { (const char *) 0, 0                                 }
};

Keyword dotKeyword[] = {
  { ".value",           LEX_TOKEN_KEYWORD_VALUE           },
  { ".defined",         LEX_TOKEN_KEYWORD_DEFINED         },
  { ".initialised",     LEX_TOKEN_KEYWORD_INITIALISED     },
  { (const char *) 0,   0                                 }
};

INT atoINT(const char *p) {
  INT res;

  res = 0;
  while(*p != 0) {
    res *= 10;
    res += *p - '0';
    p++;
  }
  return res;
}

void lexGetChar() {
  char *p;

  if(lexReplacement != (char *) 0) {
    if(*lexReplacement != 0) {
      lexChar = *lexReplacement++;
      return;
    } else {
      lexReplacement = (char *) 0;
    }
  }
  if(lexSavedChar != -2) {
    lexChar = lexSavedChar;
    lexSavedChar = -2;
  } else {
    if(lexLine[lexLineIndex] == 0) {
      if(interactive) {
        if(olStackIndex != 0) printf("%d", olStackIndex);
        printf("> ");
      }
      if(fgets(lexLine, sizeof(lexLine), lexInput) == NULL) {
        if(interactive) printf("\n");
        lexChar = EOF;
        return;
      }
      lexLineNumber++;
      if((lexInfoLine = (char *) malloc(strlen(lexLine) + 1)) == (char *) 0) {
        shaleException.chuck("malloc error", (LexInfo *) 0);
      }
      p = lexInfoLine;
      strcpy(p, lexLine);
      while((*p != 0) && (*p != '\n') && (*p != '\r')) p++;
      *p = 0;
      lexLineIndex = 0;
    }
    lexChar = lexLine[lexLineIndex++]; // fgetc(lexInput);
  }
}

void lexUseReplacement(LexReplacement *r) {
  lexSavedChar = lexChar;
  lexReplacement = r->replacement;
}

void lexShutdown() {
  fclose(lexInput);
}

int hextoi(char *p) {
  int ret;
  int n;

  ret = 0;
  while(*p != 0) {
    if((*p >= '0') && (*p <= '9')) n = *p - '0';
    else if((*p >= 'a') && (*p <= 'f')) n = *p - 'a' + 10;
    else if((*p >= 'A') && (*p <= 'F')) n = *p - 'A' + 10;
    ret = ret * 16 + n;
    p++;
  }

  return ret;
}

void lexNextToken(Lex &lex) {
  LexReplacement *r;
  Keyword *kp;
  char *p;
  char *q;
  int sacSAC;
  int sacNode;
  int sacNumber;
  int sacSubType;
  int timerNumber;
  bool goaround;

  while((lexChar != EOF) && ((lexChar == '#') || (lexChar == ' ') || (lexChar == '\t') || (lexChar == '\n') || (lexChar == '\r'))) {
    if(lexChar == '#') {
      while((lexChar != EOF) && (lexChar != '\n') && (lexChar != '\r')) {
        lexGetChar();
      }
    } else {
      lexGetChar();
    }
  }
  if(lexChar == EOF) {
    lex.token = LEX_TOKEN_EOF;
    return;
  }

  lexTokenIndex = lexLineIndex - 1;
  goaround = true;
  while(goaround) {
    goaround = false;
    switch(lexChar) {
      case '/':
        lexGetChar();
        if(lexChar == '/') {
          lexGetChar();
          lex.token = LEX_TOKEN_NOTHING;
          while((lexChar != EOF) && (lexChar != '\n') && (lexChar != '\r')) {
            lexGetChar();
          }
          if(lexChar == EOF) {
            lex.token = LEX_TOKEN_EOF;
            break;
          }
        } else {
          lex.token = LEX_TOKEN_MATHS_OP;
          lex.mathsOp = LEX_TOKEN_MATHS_OP_DIVIDE;
        }
        break;

      case '$':
        lex.token = LEX_TOKEN_KEYWORD;
        lex.keyword = LEX_TOKEN_KEYWORD_TONAME;
        lexGetChar();
        break;

      case ':':
        lexGetChar();
        if(lexChar == ':') {
          lexGetChar();
          lex.token = LEX_TOKEN_KEYWORD;
          lex.keyword = LEX_TOKEN_KEYWORD_NAMESPACE;
        } else {
          lex.token = LEX_TOKEN_UNKNOWN;
          lex.unknownToken = ':';
        }
        break;

      case '+':
      case '*':
        lex.token = LEX_TOKEN_MATHS_OP;
        lex.mathsOp = (lexChar == '+' ? LEX_TOKEN_MATHS_OP_PLUS : LEX_TOKEN_MATHS_OP_TIMES);
        lexGetChar();
        break;

      case '-':
        lexGetChar();
        if((lexChar >= '0') && (lexChar <= '9')) {
          // This is a number
          lex.token = LEX_TOKEN_NUMBER;
          p = lex.str;
          *p++ = '-';
          *p++ = lexChar;
          lexGetChar();
          while((lexChar >= '0') && (lexChar <= '9')) {
            *p++ = lexChar;
            lexGetChar();
          }
          if(lexChar == '.') {
            *p++ = '.';
            lexGetChar();
            while((lexChar >= '0') && (lexChar <= '9')) {
              *p++ = lexChar;
              lexGetChar();
            }
            *p = 0;
            lex.number.intRepresentation = false;
            lex.number.valueDouble = atof(lex.str);
          } else {
            *p = 0;
            lex.number.intRepresentation = true;
            lex.number.valueInt = atoINT(lex.str);
          }
        } else if(lexChar == '>') {
          // This is a pointer deference.
          lexGetChar();
          lex.token = LEX_TOKEN_MATHS_OP;
          lex.mathsOp = LEX_TOKEN_MATHS_OP_POINTER_DEREF;
        } else {
          lex.token = LEX_TOKEN_MATHS_OP;
          lex.mathsOp = LEX_TOKEN_MATHS_OP_MINUS;
        }
        break;

      case '=':
        lexGetChar();
        if(lexChar == '=') {
          lex.token = LEX_TOKEN_LOGICAL_OP;
          lex.logicalOp = LEX_TOKEN_LOGICAL_OP_EQ;
          lexGetChar();
        } else {
          lex.token = LEX_TOKEN_MATHS_OP;
          lex.mathsOp = LEX_TOKEN_MATHS_OP_ASSIGN;
        }
        break;

      case '<':
        lex.token = LEX_TOKEN_LOGICAL_OP;
        lexGetChar();
        if(lexChar == '=') {
          lex.logicalOp = LEX_TOKEN_LOGICAL_OP_LE;
          lexGetChar();
        } else {
          lex.logicalOp = LEX_TOKEN_LOGICAL_OP_LT;
        }
        break;

      case '>':
        lex.token = LEX_TOKEN_LOGICAL_OP;
        lexGetChar();
        if(lexChar == '=') {
          lex.logicalOp = LEX_TOKEN_LOGICAL_OP_GE;
          lexGetChar();
        } else {
          lex.logicalOp = LEX_TOKEN_LOGICAL_OP_GT;
        }
        break;

      case '!':
        lex.token = LEX_TOKEN_LOGICAL_OP;
        lexGetChar();
        if(lexChar == '=') {
          lex.logicalOp = LEX_TOKEN_LOGICAL_OP_NE;
          lexGetChar();
        } else {
          lex.logicalOp = LEX_TOKEN_LOGICAL_OP_NOT;
        }
        break;

      case '|':
        lexGetChar();
        if(lexChar == '|') {
          lex.token = LEX_TOKEN_LOGICAL_OP;
          lex.logicalOp = LEX_TOKEN_LOGICAL_OP_OR;
          lexGetChar();
        } else {
          lex.token = LEX_TOKEN_MATHS_OP;
          lex.mathsOp = LEX_TOKEN_MATHS_OP_OR;
        }
        break;

      case '&':
        lexGetChar();
        if(lexChar == '&') {
          lex.token = LEX_TOKEN_LOGICAL_OP;
          lex.logicalOp = LEX_TOKEN_LOGICAL_OP_AND;
          lexGetChar();
        } else if(lexChar == '=') {
          lex.token = LEX_TOKEN_MATHS_OP;
          lex.mathsOp = LEX_TOKEN_MATHS_OP_POINTER_ASSIGN;
          lexGetChar();
        } else {
          lex.token = LEX_TOKEN_MATHS_OP;
          lex.mathsOp = LEX_TOKEN_MATHS_OP_AND;
        }
        break;

      case '^':
        lex.token = LEX_TOKEN_MATHS_OP;
        lex.mathsOp = LEX_TOKEN_MATHS_OP_XOR;
        lexGetChar();
        break;

      case '~':
        lex.token = LEX_TOKEN_MATHS_OP;
        lex.mathsOp = LEX_TOKEN_MATHS_OP_NOT;
        lexGetChar();
        break;

      case '(':
        // Special case of () being equivalent to the execute keyword.
        lexGetChar();
        if(lexChar == ')') {
          lex.token = LEX_TOKEN_KEYWORD;
          lex.keyword = LEX_TOKEN_KEYWORD_EXECUTE;
          lexGetChar();
        } else {
          lex.token = LEX_TOKEN_UNKNOWN;
          lex.unknownToken = '(';
        }
        break;

      case '.':
        p = lex.str;
        *p++ = '.';
        lexGetChar();
        if((lexChar >= '0') && (lexChar <= '9')) {
          *p++ = lexChar;
          lexGetChar();
          while((lexChar >= '0') && (lexChar <= '9')) {
            *p++ = lexChar;
            lexGetChar();
          }
          *p = 0;
          lex.token = LEX_TOKEN_NUMBER;
          lex.number.intRepresentation = false;
          lex.number.valueDouble = atof(lex.str);
        } else if(((lexChar >= 'a') && (lexChar <= 'z')) || ((lexChar >= 'A') && (lexChar <= 'Z'))) {
          // dot keyword
          *p++ = lexChar;
          lexGetChar();
          while(((lexChar >= 'a') && (lexChar <= 'z')) || ((lexChar >= 'A') && (lexChar <= 'Z')) || ((lexChar >= '0') && (lexChar <= '9')) || (lexChar == '_')) {
            if(p < &lex.str[sizeof(lex.str) - 2]) *p++ = lexChar;
            lexGetChar();
          }
          *p = 0;
          for(kp = dotKeyword; kp->string != (char *) 0; kp++) if(strcmp(lex.str, kp->string) == 0) break;
          if(kp->string != (char *) 0) {
            // Yes it is.
            lex.token = LEX_TOKEN_KEYWORD;
            lex.keyword = kp->keyword;
            break;
          } else {
            // Put is out as a name.
            lex.token = LEX_TOKEN_NAME;
          }
        } else {
          lex.token = LEX_TOKEN_UNKNOWN;
          lex.unknownToken = '.';
        }
        break;

      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm':
      case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M':
      case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
        p = lex.str;
        *p++ = lexChar;
        lexGetChar();
        while(((lexChar >= 'a') && (lexChar <= 'z')) || ((lexChar >= 'A') && (lexChar <= 'Z')) || ((lexChar >= '0') && (lexChar <= '9')) || (lexChar == '_')) {
          if(p < &lex.str[sizeof(lex.str) - 2]) *p++ = lexChar;
          lexGetChar();
        }
        *p = 0;
        // Is this a synonym for && || and !
        if(strcmp(lex.str, "and") == 0) {
          lex.token = LEX_TOKEN_LOGICAL_OP;
          lex.logicalOp = LEX_TOKEN_LOGICAL_OP_AND;
          break;
        }
        if(strcmp(lex.str, "or") == 0) {
          lex.token = LEX_TOKEN_LOGICAL_OP;
          lex.logicalOp = LEX_TOKEN_LOGICAL_OP_OR;
          break;
        }
        if(strcmp(lex.str, "not") == 0) {
          lex.token = LEX_TOKEN_LOGICAL_OP;
          lex.logicalOp = LEX_TOKEN_LOGICAL_OP_NOT;
          break;
        }
        // Is this a replacement?
        for(r = lexReplacements; r != (LexReplacement *) 0; r = r->next) {
          if(strcmp(lex.str, r->keyword) == 0) {
            lexUseReplacement(r);
            lexGetChar();
            break;
          }
        }
        if(r != (LexReplacement *) 0) {
          goaround = true;
          break;
        }
        // Is this a keyword?
        for(kp = keyword; kp->string != (char *) 0; kp++) if(strcmp(lex.str, kp->string) == 0) break;
        if(kp->string != (char *) 0) {
          // Yes it is.
          lex.token = LEX_TOKEN_KEYWORD;
          lex.keyword = kp->keyword;
          break;
        }
        // Last but not least, it is a name.
        lex.token = LEX_TOKEN_NAME;
        break;

      case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        lex.token = LEX_TOKEN_NUMBER;
        p = lex.str;
        *p++ = lexChar;
        lexGetChar();
        if((lex.str[0] == '0') && (lexChar == 'x')) {
          // A hex number
          lexGetChar();
          if(((lexChar >= '0') && (lexChar <= '9')) || ((lexChar >= 'a') && (lexChar <= 'f')) || ((lexChar >= 'A') && (lexChar <= 'F'))) {
            p = lex.str;
            *p++ = lexChar;
            lexGetChar();
            while(((lexChar >= '0') && (lexChar <= '9')) || ((lexChar >= 'a') && (lexChar <= 'f')) || ((lexChar >= 'A') && (lexChar <= 'F'))) {
              *p++ = lexChar;
              lexGetChar();
            }
            *p = 0;
            lex.number.intRepresentation = true;
            lex.number.valueInt = hextoi(lex.str);
          }
        } else {
          while((lexChar >= '0') && (lexChar <= '9')) {
            *p++ = lexChar;
            lexGetChar();
          }
          if(lexChar == '.') {
            *p++ = '.';
            lexGetChar();
            while((lexChar >= '0') && (lexChar <= '9')) {
              *p++ = lexChar;
              lexGetChar();
            }
            *p = 0;
            lex.number.intRepresentation = false;
            lex.number.valueDouble = atof(lex.str);
          } else {
            *p = 0;
            lex.number.intRepresentation = true;
            lex.number.valueInt = atoINT(lex.str);
          }
        }
        break;

      case '"':
        lex.token = LEX_TOKEN_STRING;
        lexGetChar();
        p = lex.str;
        while(lexChar != '"') {
          if(lexChar == '\\') {
            *p++ = lexChar;
            lexGetChar();
            *p++ = lexChar;
          } else {
            *p++ = lexChar;
          }
          lexGetChar();
        }
        lexGetChar();
        *p = 0;
        break;

      case '{':
        lex.token = LEX_TOKEN_OPEN_CURLY;
        lexGetChar();
        break;

      case '}':
        lex.token = LEX_TOKEN_CLOSE_CURLY;
        lexGetChar();
        break;

      default:
        lex.token = LEX_TOKEN_UNKNOWN;
        lex.unknownToken = lexChar;
        lexGetChar();
        break;
    }
  }
}

OperationList *mainProgram;
OperationList *olStack[OL_STACK_SIZE];

void olInitialise() {
  mainProgram = new OperationList;
  olStack[0] = mainProgram;
  olStackIndex = 0;
}

void olBuild(Lex &lex) {
  Object *o;
  Operation *op;
  char *p;
  LexInfo *li;

  li = new LexInfo(shaleFileName, lexLineNumber, lexInfoLine, lexTokenIndex);

  switch(lex.token) {
    case LEX_TOKEN_UNKNOWN:
      printf("Unknown token '%c' ignored\n", lex.unknownToken);
      break;

    case LEX_TOKEN_MATHS_OP:
      switch(lex.mathsOp) {
        case LEX_TOKEN_MATHS_OP_PLUS: op = new Plus(li); break;
        case LEX_TOKEN_MATHS_OP_MINUS: op = new Minus(li); break;
        case LEX_TOKEN_MATHS_OP_TIMES: op = new Times(li); break;
        case LEX_TOKEN_MATHS_OP_DIVIDE: op = new Divide(li); break;
        case LEX_TOKEN_MATHS_OP_AND: op = new BitwiseAnd(li); break;
        case LEX_TOKEN_MATHS_OP_OR: op = new BitwiseOr(li); break;
        case LEX_TOKEN_MATHS_OP_XOR: op = new BitwiseXor(li); break;
        case LEX_TOKEN_MATHS_OP_NOT: op = new BitwiseNot(li); break;
        case LEX_TOKEN_MATHS_OP_ASSIGN: op = new Assign(li); break;
        case LEX_TOKEN_MATHS_OP_POINTER_ASSIGN: op = new PointerAssign(li); break;
        case LEX_TOKEN_MATHS_OP_POINTER_DEREF: op = new PointerDereference(li); break;
        default: shaleException.chuck("parse error", li); // throw parseException;
      }
      olStack[olStackIndex]->addOperation(op);
      break;

    case LEX_TOKEN_LOGICAL_OP:
      switch(lex.logicalOp) {
        case LEX_TOKEN_LOGICAL_OP_LT: op = new LessThan(li); break;
        case LEX_TOKEN_LOGICAL_OP_LE: op = new LessThanOrEquals(li); break;
        case LEX_TOKEN_LOGICAL_OP_EQ: op = new Equals(li); break;
        case LEX_TOKEN_LOGICAL_OP_NE: op = new NotEquals(li); break;
        case LEX_TOKEN_LOGICAL_OP_GE: op = new GreaterThanOrEquals(li); break;
        case LEX_TOKEN_LOGICAL_OP_GT: op = new GreaterThan(li); break;
        case LEX_TOKEN_LOGICAL_OP_AND: op = new LogicalAnd(li); break;
        case LEX_TOKEN_LOGICAL_OP_OR: op = new LogicalOr(li); break;
        case LEX_TOKEN_LOGICAL_OP_NOT: op = new LogicalNot(li); break;
        default: shaleException.chuck("parse error", li); // throw parseException;
      }
      olStack[olStackIndex]->addOperation(op);
      break;

    case LEX_TOKEN_KEYWORD:
      switch(lex.keyword) {
        case LEX_TOKEN_KEYWORD_DUP: op = new Dup(li); break;
        case LEX_TOKEN_KEYWORD_POP: op = new Pop(li); break;
        case LEX_TOKEN_KEYWORD_SWAP: op = new Swap(li); break;
        case LEX_TOKEN_KEYWORD_VAR: op = new Var(li); break;
        case LEX_TOKEN_KEYWORD_IF: op = new If(li); break;
        case LEX_TOKEN_KEYWORD_IFTHEN: op = new IfThen(li); break;
        case LEX_TOKEN_KEYWORD_WHILE: op = new While(li); break;
        case LEX_TOKEN_KEYWORD_EXECUTE: op = new Execute(li); break;
        case LEX_TOKEN_KEYWORD_PRINT: op = new Print(false, li); break;
        case LEX_TOKEN_KEYWORD_PRINTLN: op = new Print(true, li); break;
        case LEX_TOKEN_KEYWORD_PRINTF: op = new Printf(true, li); break;
        case LEX_TOKEN_KEYWORD_SPRINTF: op = new Printf(false, li); break;
        case LEX_TOKEN_KEYWORD_TRUE: op = new True(li); break;
        case LEX_TOKEN_KEYWORD_FALSE: op = new False(li); break;
        case LEX_TOKEN_KEYWORD_INT: op = new Int(li); break;
        case LEX_TOKEN_KEYWORD_DOUBLE: op = new Double(li); break;
        case LEX_TOKEN_KEYWORD_BREAK: op = new Break(li); break;
        case LEX_TOKEN_KEYWORD_EXIT: op = new Exit(li); break;
        case LEX_TOKEN_KEYWORD_STACK: op = new PrintStack(li); break;
        case LEX_TOKEN_KEYWORD_VALUE: op = new Value(li); break;
        case LEX_TOKEN_KEYWORD_DEFINED: op = new Defined(li); break;
        case LEX_TOKEN_KEYWORD_INITIALISED: op = new Initialised(li); break;
        case LEX_TOKEN_KEYWORD_TONAME: op = new ToName(li); break;
        case LEX_TOKEN_KEYWORD_NAMESPACE: op = new Namespace(li); break;
        case LEX_TOKEN_KEYWORD_LIBRARY: op = new Library(li); break;
        case LEX_TOKEN_KEYWORD_DEBUG: op = new Debug(li); break;
        case LEX_TOKEN_KEYWORD_BTREE: op = new BTreeDebug(li); break;
        default: shaleException.chuck("unexpected token", li); // throw unexpectedTokenException;
      }
      olStack[olStackIndex]->addOperation(op);
      break;

    case LEX_TOKEN_NAME:
      if((p = (char *) malloc(strlen(lex.str) + 1)) != (char *) 0) {
        strcpy(p, lex.str);
        olStack[olStackIndex]->addOperation(new Push(new Name(p), li));
      } else {
        shaleException.chuck("malloc failed", li); // throw mallocFailedException;
      }
      break;

    case LEX_TOKEN_NUMBER:
      if(lex.number.intRepresentation) o = new Number(lex.number.valueInt);
      else o = new Number(lex.number.valueDouble);
      olStack[olStackIndex]->addOperation(new Push(o, li));
      break;

    case LEX_TOKEN_STRING:
      if((p = (char *) malloc(strlen(lex.str) + 1)) != (char *) 0) {
        strcpy(p, lex.str);
        olStack[olStackIndex]->addOperation(new Push(new String(p), li));
      }
      break;

    case LEX_TOKEN_OPEN_CURLY:
      if(olStackIndex < (OL_STACK_SIZE - 2)) {
        olStackIndex++;
        olStack[olStackIndex] = new OperationList;
      } else {
        shaleException.chuck("code stack too big", li); // throw codeStackTooBigException;
      }
      break;

    case LEX_TOKEN_CLOSE_CURLY:
      if(olStackIndex > 0) {
        olStackIndex--;
        olStack[olStackIndex]->addOperation(new Push(new Code(olStack[olStackIndex + 1]), li));
      } else {
        shaleException.chuck("code stack undderrun", li); // throw codeStackUnderrunException;
      }
      break;
  }
}

void olCheck() {
  if(olStackIndex != 0) {
    printf("OL Stack not empty\n");
    exit(1);
  }
}

void usage() {
  printf("Usage: shale [option ...] [script [replacement ...]\n");
  printf("  option\n");
  printf("    -h             - this\n");
  printf("    --h[elp]       - this\n");
  printf("    -v             - print version and exit\n");
  printf("    -s             - give detailed syntax information\n");
  printf("  script\n");
  printf("    specify a shale script. if not given then standard input is read\n");
  printf("  replacement\n");
  printf("    name=text where name in the script is replaced by the given text\n");
  exit(0);
}

void syntax() {
  printf("Maths ops\n");
  printf("  + - * / & | ^ ~ = &= ->                     - &= is pointer assignment, -> is pointer dereference.\n");
  printf("                                                see below for generating shale code from within shale\n"); 
  printf("Logical ops\n");
  printf("  < <= == != >= > && || !                     - see below for 'conditional and' and 'conditional or'\n");
  printf("Keywords\n");
  printf("  and             same as &&\n");
  printf("  or              same as ||\n");
  printf("  not             same as !\n");
  printf("  { ... }                                     - pushes a code block, sometimes called a code fragment, onto the stack.\n");
  printf("                                                see below for generating shale code from within shale\n"); 
  printf("  dup             {value} dup                 - duplicate the top of the stack\n");
  printf("  pop             {value} pop                 - pops the top of the stack off\n");
  printf("  swap            {a} {b} swap                - swap the two top elements of the stack\n");
  printf("  var             {name} var                  - defines a variable\n");
  printf("  $                                           - turns a number or string into a name\n");
  printf("  ::              {index} {name} ::           - create the variable {index} in namespace {name}\n");
  printf("  if              bool { then-code } { else-code } if\n");
  printf("  ifthen          bool { then-code } ifthen\n");
  printf("  while           { bool } { code } while\n");
  printf("  execute         { code } execute\n");
  printf("  ()                                          - same as execute. allows shorthand for function calls, eg plus()\n");
  printf("  {name} library                              - load the library {name}. see below\n");
  printf("  print           {value} print\n");
  printf("  println         {value} println\n");
  printf("  printf          {args ...} {format} printf  - printf-like formatted output. see below\n");
  printf("  sprintf         {args ...} {format} sprintf - sprintf-like formatted output. see below\n");
  printf("  true                                        - pushes 1 on the stack\n");
  printf("  false                                       - pushes 0 on the stack\n");
  printf("  int             {n} int                     - convert to int\n");
  printf("  double          {n} double                  - convert to floating point\n");
  printf("  float                                       - same as double\n");
  printf("  break                                       - exit a repeat or while loop\n");
  printf("  exit            {n} exit                    - terminte the process with the given exit code\n");
  printf("  value           {name} value                - replace a name with its value\n");
  printf("  .value                                      - same as value\n");
  printf("  defined         {name} defined              - pushes true on the stack if the name is defined, false otherwise\n");
  printf("  .defined                                    - same as defined\n");
  printf("  initialised     {name} initialised          - pushes true on the stack if the name is defined and initialised, false otherwise.\n");
  printf("  .initialised                                - same as initialised\n");
  printf("  debug                                       - prints various internal counters\n");
  printf("  stack                                       - prints the stack, top of the stack first. does not change the stack contents\n");
  printf("  btree                                       - prints the namespace contents\n");
  printf("\n");
  printf("Conditional AND and Conditional OR\n");
  printf("  To effect a conditional and or a conditional or, push the second argument on as a code block. For example\n");
  printf("    cond1 { cond2 } and     or\n");
  printf("    cond1 { cond2 } or\n");
  printf("  will have the same result as this code\n");
  printf("    cond1 cond2 and      (likewise for the or operator)\n");
  printf("  except that because the second condition is enclosed within a code block it will only be evaluated if required.\n");
  printf("\n");
  printf("Printf and sprintf operator\n");
  printf("  Takes the following %% specifiers and passes them, complete with any field width and decimal place specs, to printf:\n");
  printf("    %%d %%x %%f %%s %%%%\n");
  printf("  Takes %%p to print any object, ala the print keyword.\n");
  printf("  Takes %%n to print the name of a variable.\n");
  printf("  \\n produces a new line, all others are printed as-is\n");
  printf("  any other character not specified above is printed as-is\n");
  printf("  Note, the args and format string are pushed on the stack in the opposite order to the printf() arguments when called from C\n");
  printf("  The sprintf operator takes the same format specifier as printf and pushes the resulting output on the stack as a string.\n");
  printf("\n");
  printf("Pointer assignment and dereference\n");
  printf("  {var} {anything} &=                         - a pointer to {anything} is assigned to {var}, eg: i var i 123 = p var p i &=\n");
  printf("  {var} ->                                    - dereference the {var} pointer, eg p-> 987 = (will assign 987 to i)\n");
  printf("\n");
  printf("Namespaces and arrays\n");
  printf("  Namespaces store globally accessible variables, separate from 'standard' variables that may be\n");
  printf("  rescoped by subesquent code blocks. C++ namespaces use the :: operator, such as std::syslog, and so shale\n");
  printf("  uses :: as the namespace operator. The equivalent to C++ std::syslog is 'syslog std::'.\n");
  printf("  The :: operator takes two arguments, the namespace name and an index.\n");
  printf("  The index and name can be a name, a number or a string. Numbers can be either an integer or a float (to 3 decimal places).\n");
  printf("  Some examples:\n");
  printf("    i a::         equivalent to a::i in C++, or like a struct or class attribute reference a.i\n");
  printf("    0 a::         an array element, essentially equivalent to a[0]\n");
  printf("    i.value a::   equivalent to a[i]\n");
  printf("    \"i\" a::       same as a::i or a.i\n");
  printf("  And they can be nested\n");
  printf("    a b:: c::     equivalent to c::b::a (not sure this exists in C++), or c.b.a\n");
  printf("    4 8:: a::     multi-dimensional array, equivalent to a[8][4]\n");
  printf("    3.142 a::     example floating point index\n");
  printf("  If you want floating point indices and names to be something other than 3 decimal places, use sprintf to\n");
  printf("  format them to a string with the required number of decimal places.\n");
  printf("\n");
  printf("Special shale:: namespace\n");
  printf("  The shale:: namespace is used by shale to provide interaction between the script and shale.\n");
  printf("  The namespace currently contains the shale script name and version numbers:\n");
  printf("    file arg:: shale::\n");
  printf("    major version:: shale::\n");
  printf("    minor version:: shale::\n");
  printf("    micro version:: shale::\n");
  printf("\n");
  printf("Libraries\n");
  printf("  Libraries are loaded with the library keyword\n");
  printf("    {name} library\n");
  printf("  The library places all of its offerings under the {name}:: namespace. Every library has a help function accessable as\n");
  printf("    help {name}::()\n");
  printf("  once the library is loaded, and will detail all of the functionality the library provides.\n");
  printf("  Current libraries are:\n");
  printf("    array       - support for sparse and fully populated arrays. See help array::() for details.\n");
  printf("    file        - some stdio function. See help file::() for details.\n");
  printf("    maths       - pi, e, log functions, etc. See help maths::() for details.\n");
  printf("\n");
  printf("Generating new shale code from within shale\n");
  printf("  You can generate new shale code by adding code blocks together with the + operator.\n");
  printf("  The + operator will take two code blocks, or variables whose value is a code block,\n");
  printf("  and push on the stack a code block that combines both. For example\n");
  printf("    { 6 7 } { * pl } + execute\n");
  printf("  will output 42\n");
  printf("\n");
  printf("Replacements\n");
  printf("  When passing in the shale script via a file, any arguments following the file can specify replacements, and are specified as\n");
  printf("    name=text\n");
  printf("  Anywhere in the script where 'name' appears will be replaced with the given text. Any number of replacements can follow the file.\n");
  printf("  This introduces command line arguments into your script.\n");
  printf("\n");
  printf("  As an example, say you want to pass different values to the code, the script might look like (paraphrased):\n");
  printf("\n");
  printf("    factorial var\n");
  printf("    factorial { ... } =\n");
  printf("    n factorial() println\n");
  printf("\n");
  printf("  Then when using this script (say, in a file called fact) you can try different values for n with\n");
  printf("\n");
  printf("    shale fact n=5\n");
  printf("    shale fact n=20\n");
  exit(0);
}

void version() {
  printf("%ld.%ld.%ld\n", MAJOR, MINOR, MICRO);
  exit(0);
}

void setupShaleNamespace(const char *arg0) {
  Variable *v;

  v = new Variable("/file/arg/shale");
  v->setObject(new String(arg0));
  btree.addVariable(v);

  v = new Variable("/major/version/shale");
  v->setObject(new Number(MAJOR));
  btree.addVariable(v);

  v = new Variable("/minor/version/shale");
  v->setObject(new Number(MINOR));
  btree.addVariable(v);

  v = new Variable("/micro/version/shale");
  v->setObject(new Number(MICRO));
  btree.addVariable(v);
}

int main(int ac, char **av) {
  LexReplacement *r;
  Lex lex;
  char *p;
  struct stat statb;
  char command[1024];
  int node;
  int number;
  int i;

  interactive = false;
  lexLineNumber = 0;
  lexLine[0] = 0;
  lexLineIndex = 0;
  shaleFileName = (char *) 0;

  while((ac > 1) && (av[1][0] == '-')) {
    if((av[1][1] == 'h') || ((av[1][1] == '-') && (av[1][2] == 'h'))) {
      usage();
    } else if(av[1][1] == 's') {
      syntax();
    } else if(av[1][1] == 'v') {
      version();
    } else {
      usage();
    }
    ac--;
    av++;
  }

  lexReplacements = (LexReplacement *) 0;
  lexSavedChar = -2;
  lexSendNamespaceToken = false;
  if(ac == 1) {
    lexInput = stdin;
    interactive = true;
    shaleFileName = "<stdin>";
  } else {
    shaleFileName = av[1];
    if((lexInput = fopen(shaleFileName, "r")) == NULL) {
      perror(av[1]);
      exit(1);
    }
    for(av++, ac--; ac > 1; ac--, av++) {
      // Does this look like a replacement?
      for(p = av[1]; (*p != 0) && (*p != '='); p++);
      if(*p == '=') {
        // It is.
        *p++ = 0;
        r = new LexReplacement;
        r->keyword = av[1];
        r->replacement = p;
        r->next = lexReplacements;
        lexReplacements = r;
      }
    }
  }

  setupShaleNamespace(shaleFileName);

  do {
    try {
      olInitialise();

      // Parse the input and build the compiled code.
      lexGetChar();
      lexNextToken(lex);
      while(lex.token != LEX_TOKEN_EOF) {
        olBuild(lex);
        if(interactive && (olStackIndex == 0)) olStack[0]->actionLatest();
        lexNextToken(lex);
      }
      lexShutdown();
      olCheck();

      // Execute the code if there are no problems.
      if(! interactive) olStack[0]->action();
    } catch(Exception *e) {
      e->printError();
      if(interactive) lexLine[lexLineIndex] = 0;
    } catch(exception &e) {
      printf("%s\n", e.what());
      if(interactive) lexLine[lexLineIndex] = 0;
    }
  } while((lex.token != LEX_TOKEN_EOF) && interactive);

  return 0;
}
