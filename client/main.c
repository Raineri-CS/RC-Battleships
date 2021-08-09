#include "./include/battleship.h"
#include <stdio.h>

int main(int argc, char const *argv[]) {

  tabuleiro localField;
  FILE *fd;

  argv[0] = "Battleships";
  if (argc > 1) {
    fd = fopen(argv[1], "r+");
  } else {
    fd = fopen("predef.field", "r");
  }
  init(&localField);

  if (!verifyFileIntegrity(fd, &localField)) {
    // TODO o jogo acontece aqui
    printField(&localField);
  } else {
    fprintf(stderr, "Arquivo de tabuleiro \"predef.field\" nao encontrado!");
    return -1;
  }

  return 0;
}
