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
