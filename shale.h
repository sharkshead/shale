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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "shalelib.h"

#define MAJOR ((INT)  1)
#define MINOR ((INT)  2)
#define MICRO ((INT)  6)

// Lexical analyser stuff.

// The tokens returned.
#define LEX_TOKEN_EOF                       0
#define LEX_TOKEN_UNKNOWN                   1
#define LEX_TOKEN_NOTHING                   2
#define LEX_TOKEN_MATHS_OP                  3
#define LEX_TOKEN_LOGICAL_OP                4
#define LEX_TOKEN_KEYWORD                   5
#define LEX_TOKEN_NAME                      6
#define LEX_TOKEN_NUMBER                    7
#define LEX_TOKEN_STRING                    8
#define LEX_TOKEN_OPEN_CURLY                9
#define LEX_TOKEN_CLOSE_CURLY              10

#define LEX_TOKEN_MATHS_OP_PLUS             0
#define LEX_TOKEN_MATHS_OP_MINUS            1
#define LEX_TOKEN_MATHS_OP_TIMES            2
#define LEX_TOKEN_MATHS_OP_DIVIDE           3
#define LEX_TOKEN_MATHS_OP_MOD              4
#define LEX_TOKEN_MATHS_OP_AND              5
#define LEX_TOKEN_MATHS_OP_OR               6
#define LEX_TOKEN_MATHS_OP_XOR              7
#define LEX_TOKEN_MATHS_OP_NOT              8
#define LEX_TOKEN_MATHS_OP_LEFT_SHIFT       9
#define LEX_TOKEN_MATHS_OP_RIGHT_SHIFT     10
#define LEX_TOKEN_MATHS_OP_ASSIGN          11
#define LEX_TOKEN_MATHS_OP_POINTER_ASSIGN  12
#define LEX_TOKEN_MATHS_OP_POINTER_DEREF   13
#define LEX_TOKEN_MATHS_OP_PLUSPLUS        14
#define LEX_TOKEN_MATHS_OP_MINUSMINUS      15

#define LEX_TOKEN_LOGICAL_OP_LT             0
#define LEX_TOKEN_LOGICAL_OP_LE             1
#define LEX_TOKEN_LOGICAL_OP_EQ             2
#define LEX_TOKEN_LOGICAL_OP_NE             3
#define LEX_TOKEN_LOGICAL_OP_GE             4
#define LEX_TOKEN_LOGICAL_OP_GT             5
#define LEX_TOKEN_LOGICAL_OP_AND            6
#define LEX_TOKEN_LOGICAL_OP_OR             7
#define LEX_TOKEN_LOGICAL_OP_NOT            8

#define LEX_TOKEN_KEYWORD_DUP               0
#define LEX_TOKEN_KEYWORD_POP               1
#define LEX_TOKEN_KEYWORD_SWAP              2
#define LEX_TOKEN_KEYWORD_VAR               3
#define LEX_TOKEN_KEYWORD_IF                4
#define LEX_TOKEN_KEYWORD_IFTHEN            5
#define LEX_TOKEN_KEYWORD_WHILE             6
#define LEX_TOKEN_KEYWORD_EXECUTE           7
#define LEX_TOKEN_KEYWORD_PRINT             8
#define LEX_TOKEN_KEYWORD_PRINTLN           9
#define LEX_TOKEN_KEYWORD_PRINTF           10
#define LEX_TOKEN_KEYWORD_SPRINTF          11
#define LEX_TOKEN_KEYWORD_TRUE             12
#define LEX_TOKEN_KEYWORD_FALSE            13
#define LEX_TOKEN_KEYWORD_INT              14
#define LEX_TOKEN_KEYWORD_DOUBLE           15
#define LEX_TOKEN_KEYWORD_BREAK            16
#define LEX_TOKEN_KEYWORD_EXIT             17
#define LEX_TOKEN_KEYWORD_VALUE            18
#define LEX_TOKEN_KEYWORD_DEFINED          19
#define LEX_TOKEN_KEYWORD_INITIALISED      20
#define LEX_TOKEN_KEYWORD_TONAME           21
#define LEX_TOKEN_KEYWORD_NAMESPACE        22
#define LEX_TOKEN_KEYWORD_LIBRARY          23
#define LEX_TOKEN_KEYWORD_FUNCTION         24
#define LEX_TOKEN_KEYWORD_RETURN           25
#define LEX_TOKEN_KEYWORD_DEBUG            26
#define LEX_TOKEN_KEYWORD_STACK            27
#define LEX_TOKEN_KEYWORD_BTREE            28

// How the lexical analyser stores things.

// Replacements.
struct LexReplacement {
  char *keyword;
  char *replacement;
  LexReplacement *next;
};

// Holds a number.
struct LexNumber {
  bool intRepresentation;
  INT valueInt;
  double valueDouble;
};

struct Lex {
  int token;
  char unknownToken;
  int mathsOp;
  int logicalOp;
  int keyword;
  LexNumber number;
  char str[128];
};

struct Keyword {
  const char *string;
  int keyword;
};

// Size of the stack to keep track of nested code segments.
#define OL_STACK_SIZE 256
