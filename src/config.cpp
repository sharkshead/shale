#include <stdlib.h>
#include <stdio.h>

int main(int ac, char **av) {
  const char *INT;
  const char *PCTD;
  FILE *fd;

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

  fd = fopen("config.h", "w");
  if(fd == NULL) {
    perror("ocnfig.h");
    return 1;
  }
  fprintf(fd, "#define INT %s\n#define PCTD \"%s\"\n", INT, PCTD);
  fclose(fd);

  return 0;
}
