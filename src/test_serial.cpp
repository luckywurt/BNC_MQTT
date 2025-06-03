
#ifndef WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

  FILE* fp = fopen("/dev/ttyS0", "r");

  if (!fp) {
    printf("Cannot open file.\n");
    exit(9);
  }

  char msg[100];

  while (true) {
    int nb = fread(msg, sizeof(msg), 1, fp);
    printf("read %d\n", nb);
#ifndef WIN32
  sleep(1);
#else
  Sleep(1000);
#endif
  }

  return 0;
}
