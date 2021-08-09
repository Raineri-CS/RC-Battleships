#include "./include/battleship.h"
#include <stdio.h>

int main(int argc, char const *argv[]) {

  tabuleiro localField;
  FILE *fd;

  argv[0] = "Battleships";
  if (argc == 3) {
    fd = fopen("predef.field", "r");

  } else if (argc == 4) {
    fd = fopen(argv[1], "r");
  } else {
    fprintf(stderr, "Erro na quantidade de parametros, devem seguir ./client "
                    "DOMINIO PORTA ARQUIVO[opcional]\n");
    return -1;
  }

  init(&localField);

  // Se o arquivo nao existir...
  if (fd == 0) {
    // Insere no brute-force as pecas
    randomizePieces(&localField);
  }

  if (fd == 0 || !verifyFileIntegrity(fd, &localField)) {
    // TODO o jogo acontece aqui
    printField(&localField);
  } else {
    fprintf(stderr, "Arquivo de tabuleiro \"predef.field\" nao encontrado!");
    return -1;
  }

  return 0;
}
