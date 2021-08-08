#include "./include/battleship.h"
#include <stdio.h>

int main(int argc, char const *argv[]) {
  printf("Test\n");

  tabuleiro localField;
  FILE *fd;
  printf("Test\n");

  argv[0] = "Battleships";
  if (argc > 0) {
    fd = fopen(argv[1], "r+");
  } else {
    fd = fopen("predef.field", "r");
  }
  printf("Test\n");
  init(&localField);

  if (!verifyFileIntegrity(fd, &localField)) {
    // TODO o jogo acontece aqui
  } else {
    return -1;
  }

  return 0;
}
