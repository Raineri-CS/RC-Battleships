#include "./include/battleship.h"
#include <stdio.h>

int main(int argc, char const *argv[]) {

  tabuleiro localField;
  FILE *fd;
  unsigned int opc;

  opc = 0;

  argv[0] = "Battleships";
  if (argc == 3) {
    fd = fopen("predef.field", "r");

  } else if (argc == 4) {
    fd = fopen(argv[3], "r");
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
    do {
      printf("Informe contra quem quer jogar\n1 - COM\n2 - PLAYER\n");
      scanf("%ud", &opc);
      getchar();
    } while (opc <= 0 || opc > 2);
    switch (opc) {
    case COM:
      gameLoop((char *)argv[1], (unsigned short)atoi(argv[2]), COM,
               &localField);
      break;
    case PLAYER:
      gameLoop((char *)argv[1], (unsigned short)atoi(argv[2]), PLAYER,
               &localField);
      break;
    default:
      break;
    }
  } else {
    fprintf(stderr, "Arquivo de tabuleiro \"predef.field\" nao encontrado!");
    return -1;
  }

  return 0;
}
