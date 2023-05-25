/*
  
MIT License

Copyright (c) 2020-2023 Graeme Elsworthy <github@sharkshead.com>

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int ac, char **av) {
  const char *INT;
  const char *PCTD;
  FILE *fdin;
  FILE *fdout;
  char line[1024];

  if(sizeof(int) == 8) {
    INT = "int";
    PCTD = "";
  } else if(sizeof(long int) == 8) {
    INT = "long int";
    PCTD = "l";
  } else if(sizeof(long long int) == 8) {
    INT = "long long int";
    PCTD = "ll";
  } else {
    INT = "int";
    PCTD = "";
  }

  if((fdin = fopen("shalelib.h.template", "r")) == (FILE *) 0) {
    perror("shalelib.h.template");
    exit(1);
  }
  if((fdout = fopen("shalelib.h", "w")) == (FILE *) 0) {
    perror(".sshalelib.h");
    exit(1);
  }

  while(fgets(line, sizeof(line), fdin) != NULL) {
    if(strncmp(line, "#define INT", 11) == 0) {
      fprintf(fdout, "#define INT %s\n", INT);
    } else if(strncmp(line, "#define PCTD",  12) == 0) {
      fprintf(fdout, "#define PCTD \"%s\"\n", PCTD);
    } else {
      fputs(line, fdout);
    }
  }

  fclose(fdin);
  fclose(fdout);

  return 0;
}
