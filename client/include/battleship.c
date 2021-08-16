#include "battleship.h"

void clear() {
#ifdef __linux__
  system("clear");
#elif _WIN32
  system("cls");
#else
#error "Sistema operacional nao suportado!"
#endif
}

void clearStdin(void) {
  char c;
  c = 0;
  while (c != '\n' && c != EOF) {
    c = getchar();
  }
}

int gameLoop(char *domain, unsigned short int port, unsigned char gameMode,
             tabuleiro *tab) {
  int clientSockfd, tempX, tempY, lives, valRead;
  struct sockaddr_in serverAddr;
  struct hostent *host;
  char recvBuffer[256], sendBuffer[256];
  unsigned char didChoose, playerMove1, showMap;
  unsigned int playerMove2, paramAmount;

  /* Inicializacoes */
  clientSockfd = 0;
  recvBuffer[0] = '\0';
  tempX = 0;
  tempY = 0;
  didChoose = 0;
  lives = 32;
  paramAmount = 0;
  showMap = 0;
  host = gethostbyname(domain);

  /* Protocolo TCP */
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);

  /* Abre a socket do cliente no modo tcp do tipo STREAM */
  if ((clientSockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "\n Erro ao criar a socket! \n");
    return -1;
  }
  memcpy(&serverAddr.sin_addr, host->h_addr_list[0], host->h_length);

  if (connect(clientSockfd, (struct sockaddr *)&serverAddr,
              sizeof(serverAddr)) < 0) {
    fprintf(stderr, "\n Conexao falhou! \n");
    return -1;
  }

  /* Finalmente, o loop do jogo */
  switch (gameMode) {
  case COM:
    // Envia a mensagem que se deseja jogar no modo COM
    sendBuffer[0] = COM + '0';
    send(clientSockfd, &sendBuffer[0], 1, 0);
    for (;;) {
      paramAmount = 0;
      if ((valRead = recv(clientSockfd, recvBuffer, 128, 0)) < 0) {
        fprintf(stderr, "Erro em receber inforrmacoes do servidor\n");
        return -1;
      }
      recvBuffer[valRead] = '\0';
      // Conta a quantidade de parametros da mensagem
      for (int j = 0; j < (int)strlen(recvBuffer); j++) {
        if (recvBuffer[j] == ' ') {
          paramAmount++;
        }
      }

      // Quer dizer que ele leu um ataque
      if (paramAmount == 2) {
        sscanf(recvBuffer, "%d %d", &tempX, &tempY);
        if (fireProjectile(tempX, tempY, tab) == HIT) {
          lives--;
          printf("O adversario te acertou!\n");
        } else {
          printf("O adversario errou!\n");
        }
        // Se a quantidade de vidas dessa estrutura...
        if (lives <= 0) {
          // Setta aqui o que vai ser ENVIADO pro cliente
          sprintf(sendBuffer, "%c ", GAME_WIN + '0');
          send(clientSockfd, sendBuffer, strlen(sendBuffer), 0);
        }
        // Se as vidas nao acabaram, continua o jogo...
      } else if (paramAmount == 3) {
        // Quer dizer que as mensagens foram concatenadas
        // A primeira sempre vai ser o STATUS, e a segunda as COORDENADAS
        sscanf(recvBuffer, "%c %d %d", &recvBuffer[0], &tempX, &tempY);
        switch (atoi(&recvBuffer[0])) {
        case GAME_HIT:
          printf("Voce acertou o adversario!\n");
          break;
        case GAME_MISS:
          printf("Voce errou o adversario!\n");
          break;
        case GAME_WIN:
          printf("Parabens, voce ganhou o jogo!\n");
          close(clientSockfd);
          return 0;
          break;
        case GAME_OVER:
        case GAME_LOSE:
          printf("Que pena! Voce perdeu o jogo.\n");
          close(clientSockfd);
          return 0;
          break;
        default:
          break;
        }
        if (fireProjectile(tempX, tempY, tab) == HIT) {
          lives--;
          printf("O adversario te acertou!\n");
        } else {
          printf("O adversario errou!\n");
        }
        // Se a quantidade de vidas dessa estrutura...
        if (lives <= 0) {
          // Setta aqui o que vai ser ENVIADO pro cliente
          sprintf(sendBuffer, "%c ", GAME_WIN + '0');
          send(clientSockfd, sendBuffer, strlen(sendBuffer), 0);
        }
        // Se as vidas nao acabaram, continua o jogo...
      } else {
        switch (atoi(&recvBuffer[0])) {
        case GAME_HIT:
          printf("Voce acertou o adversario!\n");
          break;
        case GAME_MISS:
          printf("Voce errou o adversario!\n");
          break;
        case GAME_WIN:
          printf("Parabens, voce ganhou o jogo!\n");
          close(clientSockfd);
          return 0;
          break;
        case GAME_OVER:
        case GAME_LOSE:
          printf("Que pena! Voce perdeu o jogo.\n");
          close(clientSockfd);
          return 0;
          break;
        default:
          break;
        }
      }

      do {
        // Mostra o tabuleiro local
        if (showMap) {
          printField(tab);
        }
        // Le o movimento do jogador
        printf(
            "Informe sua jogada no formato (com espaco) LETRA NUMERO ou um de "
            "cada vez\n\"M\" sozinho para mapa.\n");
        fflush(stdin);
        scanf("%c", &playerMove1);
        if (playerMove1 == 'M') {
          showMap = !showMap;
          clearStdin();
        }
      } while (playerMove1 == 'M');

      scanf("%u", &playerMove2);
      clear();
      // ESSE CARA LIMPA A PORRA DO BUFFER DE INPUT QUE VEM COM O \n, FIQUEI 2
      // HORAS PRA ACHAR ESSE BUG, NAO MEXER
      clearStdin();

      // strncpy(sendBuffer, "\0", 256);
      sprintf(sendBuffer, "%d %d ", (playerMove1 - 97), playerMove2 - 1);
      send(clientSockfd, sendBuffer, strlen(sendBuffer), 0);
    }
    break;
  case PLAYER:
    sendBuffer[0] = PLAYER + '0';
    send(clientSockfd, &sendBuffer[0], 1, 0);
    while (!didChoose) {
      if ((valRead = recv(clientSockfd, recvBuffer, 128, 0)) < 0) {
        fprintf(stderr, "Erro em receber inforrmacoes do servidor\n");
        return -1;
      }
      recvBuffer[valRead] = '\0';
      clear();
      switch (atoi(&recvBuffer[0])) {
      case IDLE:
        printf("Esperando por outro jogador...\n");
        break;
      case GAME_START:
        printf("O jogo esta prestes a comecar!\n");
        didChoose = !didChoose;
        break;
      default:
        break;
      }
    }

    // Mostra o tabuleiro local
    do {
      do {
        // Mostra o tabuleiro local
        if (showMap) {
          clear();
          printField(tab);
        }
        // Le o movimento do jogador
        printf(
            "Informe sua jogada no formato (com espaco) LETRA NUMERO ou um de "
            "cada vez\n\"M\" sozinho para mapa.\n");
        scanf("%c", &playerMove1);
        if (playerMove1 == 'M') {
          showMap = !showMap;
          clearStdin();
        }
      } while (playerMove1 == 'M');
      scanf("%u", &playerMove2);
      clear();
      // ESSE CARA LIMPA A PORRA DO BUFFER DE INPUT QUE VEM COM O \n,
      // FIQUEI 2 HORAS PRA ACHAR ESSE BUG, NAO MEXER
      clearStdin();
    } while ((playerMove1 - 97) > 14 || playerMove2 - 1 > 14);
    // Representa no tabuleiro local onde foi tentado o tiro
    tab->field[(int)(playerMove1 - 97)][(playerMove2 - 1)].clientShot = 1;
    sprintf(sendBuffer, "%d %d ", (playerMove1 - 97), playerMove2 - 1);
    send(clientSockfd, sendBuffer, strlen(sendBuffer), 0);

    for (;;) {
      paramAmount = 0;
      if ((valRead = recv(clientSockfd, recvBuffer, 128, 0)) < 0) {
        fprintf(stderr, "Erro em receber inforrmacoes do servidor\n");
        return -1;
      }
      recvBuffer[valRead] = '\0';

      // Conta a quantidade de parametros da mensagem
      for (int j = 0; j < (int)strlen(recvBuffer); j++) {
        if (recvBuffer[j] == ' ') {
          paramAmount++;
        }
      }

      // Quer dizer que ele leu um ataque do primeiro TURNO

      switch (paramAmount) {
      case 1:
        switch (atoi(&recvBuffer[0])) {
        case IDLE:
          printf("Esperando adversario...\n");
          break;
        case GAME_WAIT:
        case GAME_START:
          break;
        case GAME_HIT:
          printf("Voce acertou o adversario!\n");
          // Mostra o tabuleiro local
          do {
            do {
              // Mostra o tabuleiro local
              if (showMap) {
                clear();
                printField(tab);
              }
              // Le o movimento do jogador
              printf("Informe sua jogada no formato (com espaco) LETRA NUMERO "
                     "ou um de "
                     "cada vez\n\"M\" sozinho para mapa.\n");
              scanf("%c", &playerMove1);
              if (playerMove1 == 'M') {
                showMap = !showMap;
                clearStdin();
              }
            } while (playerMove1 == 'M');
            scanf("%u", &playerMove2);
            clear();
            // ESSE CARA LIMPA A PORRA DO BUFFER DE INPUT QUE VEM COM O \n,
            // FIQUEI 2 HORAS PRA ACHAR ESSE BUG, NAO MEXER
            clearStdin();
          } while ((playerMove1 - 97) > 14 || playerMove2 - 1 > 14);
          // Representa no tabuleiro local onde foi tentado o tiro
          tab->field[(int)(playerMove1 - 97)][(playerMove2 - 1)].clientShot = 1;
          // Envia o tiro
          sprintf(sendBuffer, "%d %d ", (playerMove1 - 97), playerMove2 - 1);
          send(clientSockfd, sendBuffer, strlen(sendBuffer), 0);
          break;
        case GAME_MISS:
          printf("Voce errou o adversario!\n");
          // Mostra o tabuleiro local
          do {
            do {
              // Mostra o tabuleiro local
              if (showMap) {
                clear();
                printField(tab);
              }
              // Le o movimento do jogador
              printf("Informe sua jogada no formato (com espaco) LETRA NUMERO "
                     "ou um de "
                     "cada vez\n\"M\" sozinho para mapa.\n");
              scanf("%c", &playerMove1);
              if (playerMove1 == 'M') {
                showMap = !showMap;
                clearStdin();
              }
            } while (playerMove1 == 'M');
            scanf("%u", &playerMove2);
            clear();
            // ESSE CARA LIMPA A PORRA DO BUFFER DE INPUT QUE VEM COM O \n,
            // FIQUEI 2 HORAS PRA ACHAR ESSE BUG, NAO MEXER
            clearStdin();
          } while ((playerMove1 - 97) > 14 || playerMove2 - 1 > 14);
          // Representa no tabuleiro local onde foi tentado o tiro
          tab->field[(playerMove1 - 97)][playerMove2 - 1].clientShot = 1;
          // Envia o tiro
          sprintf(sendBuffer, "%d %d ", (playerMove1 - 97), playerMove2 - 1);
          send(clientSockfd, sendBuffer, strlen(sendBuffer), 0);
          break;
        case GAME_WIN:
          printf("Parabens, voce ganhou o jogo!\n");
          close(clientSockfd);
          return 0;
          break;
        case GAME_OVER:
        case GAME_LOSE:
          printf("Que pena! Voce perdeu o jogo.\n");
          close(clientSockfd);
          return 0;
          break;
        default:
          break;
        }
        break;
      case 2:
        sscanf(recvBuffer, "%d %d", &tempX, &tempY);

        if (fireProjectile(tempX, tempY, tab) == HIT) {
          lives--;
          sprintf(sendBuffer, "%c ", GAME_HIT + '0');
          send(clientSockfd, sendBuffer, strlen(sendBuffer), 0);
          printf("O adversario te acertou!\n");
        } else {
          sprintf(sendBuffer, "%c ", GAME_MISS + '0');
          send(clientSockfd, sendBuffer, strlen(sendBuffer), 0);
          printf("O adversario errou!\n");
        }
        // Se a quantidade de vidas dessa estrutura...
        if (lives <= 0) {
          // Setta aqui o que vai ser ENVIADO pro cliente
          sprintf(sendBuffer, "%c ", GAME_WIN + '0');
          send(clientSockfd, sendBuffer, strlen(sendBuffer), 0);
          recvBuffer[0] = GAME_OVER + '0';
        }
        // Se as vidas nao acabaram, continua o jogo...

        break;
      default:
        break;
      }
    }
    break;
  default:
    break;
  }

  /* Se por algum motivo ele chegar aqui, deu ruim em algum lugar */
  return -1;
}

void init(tabuleiro *tab) {
  for (unsigned int i = 0; i < FIELD_SIZE; i++)
    for (int j = 0; j < FIELD_SIZE; j++) {
      tab->field[i][j].isOccupied = 0;
      tab->field[i][j].type = WAT;
      tab->field[i][j].clientShot = 0;
      tab->field[i][j].serverShot = 0;
    }
}

int addToField(unsigned int x1, unsigned int y1, unsigned int x2,
               unsigned int y2, unsigned char type, tabuleiro *tab) {
  // Tratando erros de preprocessamento
  if ((x1 > 14 || y1 > 14) || (x2 > 14 || y2 > 14)) {
    fprintf(stderr, "Erro ao inserir a peca, fora dos indices do tabuleiro!\n");
    return -1;
  }

  switch (type) {
  case SUB:
    // Verifica qual dos eixos se diferenciou
    if (x1 == x2) {
      // Verifica se eh do tamanho correto
      // abs te retorna o valor sem o bit de sinal da subtracao
      // esta sendo castado pra int dentro da funcao pra sumir com um warning
      // de que os parametros sao unsigned int, logo nao tem efeito a chamada
      if (abs((int)(y2 - y1)) + 1 != 2) {
        fprintf(
            stderr,
            "Erro ao inserir a peca, o tamanho nao condiz com a categoria\n");
        return -1;
      }
      for (unsigned int i = y1; i <= y2; i++) {
        // Verifica se o espaco ja esta ocupado antes de tentar inserir
        if (tab->field[x1][i].isOccupied) {
          fprintf(stderr, "Erro ao inserir peca, espaco (%d,%d) ja ocupado!",
                  x1, i);
          return -1;
        }
      }
      for (unsigned int i = y1; i <= y2; i++) {
        tab->field[x1][i].isOccupied = 1;
        tab->field[x1][i].type = SUB;
      }
    } else if (y1 == y2) {
      // Tem o caso de estar erradas as coordenadas, nao se pode ter pecas na
      // diagonal
      if (abs((int)(x2 - x1)) + 1 != 2) {
        fprintf(
            stderr,
            "Erro ao inserir a peca, o tamanho nao condiz com a categoria\n");
        return -1;
      }
      // Antes de inserir, se verifica todas as casas a serem ocupadas para
      // ver se ha algo nelas
      for (unsigned int i = x1; i <= x2; i++) {
        if (tab->field[i][y1].isOccupied) {
          fprintf(stderr, "Erro ao inserir peca, espaco (%d,%d) ja ocupado!", i,
                  y1);
          return -1;
        }
      }
      for (unsigned int i = x1; i <= x2; i++) {
        tab->field[i][y1].isOccupied = 1;
        tab->field[i][y1].type = SUB;
      }
    }
    break;
  case TOR:
    if (x1 == x2) {
      if (abs((int)(y2 - y1)) + 1 != 3) {
        fprintf(
            stderr,
            "Erro ao inserir a peca, o tamanho nao condiz com a categoria\n");
        return -1;
      }
      for (unsigned int i = y1; i <= y2; i++) {
        if (tab->field[x1][i].isOccupied) {
          fprintf(stderr, "Erro ao inserir peca, espaco (%d,%d) ja ocupado!",
                  x1, i);
          return -1;
        }
      }
      for (unsigned int i = y1; i <= y2; i++) {
        tab->field[x1][i].isOccupied = 1;
        tab->field[x1][i].type = TOR;
      }

    } else if (y1 == y2) {
      if (abs((int)(x2 - x1)) + 1 != 3) {
        fprintf(
            stderr,
            "Erro ao inserir a peca, o tamanho nao condiz com a categoria\n");
        return -1;
      }
      for (unsigned int i = x1; i <= x2; i++) {
        if (tab->field[i][y1].isOccupied) {
          fprintf(stderr, "Erro ao inserir peca, espaco (%d,%d) ja ocupado!", i,
                  y1);
          return -1;
        }
      }
      for (unsigned int i = x1; i <= x2; i++) {
        tab->field[i][y1].isOccupied = 1;
        tab->field[i][y1].type = TOR;
      }
    }
    break;
  case TAS:
    if (x1 == x2) {
      if (abs((int)(y2 - y1)) + 1 != 4) {
        fprintf(
            stderr,
            "Erro ao inserir a peca, o tamanho nao condiz com a categoria\n");
        return -1;
      }
      for (unsigned int i = y1; i <= y2; i++) {
        if (tab->field[x1][i].isOccupied) {
          fprintf(stderr, "Erro ao inserir peca, espaco (%d,%d) ja ocupado!",
                  x1, i);
          return -1;
        }
      }
      for (unsigned int i = y1; i <= y2; i++) {
        tab->field[x1][i].isOccupied = 1;
        tab->field[x1][i].type = TAS;
      }

    } else if (y1 == y2) {
      if (abs((int)(x2 - x1)) + 1 != 4) {
        fprintf(
            stderr,
            "Erro ao inserir a peca, o tamanho nao condiz com a categoria\n");
        return -1;
      }
      for (unsigned int i = x1; i <= x2; i++) {
        if (tab->field[i][y1].isOccupied) {
          fprintf(stderr, "Erro ao inserir peca, espaco (%d,%d) ja ocupado!", i,
                  y1);
          return -1;
        }
      }
      for (unsigned int i = x1; i <= x2; i++) {
        tab->field[i][y1].isOccupied = 1;
        tab->field[i][y1].type = TAS;
      }
    }
    break;
  case AIP:
    if (x1 == x2) {
      if (abs((int)(y2 - y1)) + 1 != 5) {
        fprintf(
            stderr,
            "Erro ao inserir a peca, o tamanho nao condiz com a categoria\n");
        return -1;
      }
      for (unsigned int i = y1; i <= y2; i++) {
        if (tab->field[x1][i].isOccupied) {
          fprintf(stderr, "Erro ao inserir peca, espaco (%d,%d) ja ocupado!",
                  x1, i);
          return -1;
        }
      }
      for (unsigned int i = y1; i <= y2; i++) {
        tab->field[x1][i].isOccupied = 1;
        tab->field[x1][i].type = AIP;
      }

    } else if (y1 == y2) {
      if (abs((int)(x2 - x1)) + 1 != 5) {
        fprintf(
            stderr,
            "Erro ao inserir a peca, o tamanho nao condiz com a categoria\n");
        return -1;
      }
      for (unsigned int i = x1; i <= x2; i++) {
        if (tab->field[i][y1].isOccupied) {
          fprintf(stderr, "Erro ao inserir peca, espaco (%d,%d) ja ocupado!", i,
                  y1);
          return -1;
        }
      }
      for (unsigned int i = x1; i <= x2; i++) {
        tab->field[i][y1].isOccupied = 1;
        tab->field[i][y1].type = AIP;
      }
    }
    break;
  default:
    break;
  }
  return 0;
}

// Retorna HIT se acertou, 0 se errou
int fireProjectile(unsigned int x, unsigned int y, tabuleiro *tab) {
  if (x > 14 || y > 14)
    return 0;
  // A mensagem vinda do servidor tentou atirar no espaco x y
  tab->field[x][y].serverShot = 1;
  if (tab->field[x][y].isOccupied) {
    tab->field[x][y].type = HIT;
    tab->field[x][y].isOccupied = 0;
    return HIT;
  }
  return 0;
}

int verifyFileIntegrity(FILE *fd, tabuleiro *tab) {
  int tempX1, tempY1, tempX2, tempY2, subQty, torQty, tasQty, aipQty;
  char pieceBuffer[32];

  // Temporary coordinates
  tempX1 = 0;
  tempY1 = 0;
  tempX2 = 0;
  tempY2 = 0;

  // Used to check if the correct number of units is inputtted by the end of
  // the file
  subQty = 0;
  torQty = 0;
  tasQty = 0;
  aipQty = 0;

  while (fscanf(fd, "%s %d %d %d %d", pieceBuffer, &tempX1, &tempY1, &tempX2,
                &tempY2) != EOF) {
    /* Tem que ser nested ifs porque o switch-case nao gosta de multiplos
     * caracteres */
    if (strncmp(pieceBuffer, "SUB", 3) == 0) {
      if (addToField(tempX1, tempY1, tempX2, tempY2, SUB, tab) == -1)
        return -1;
      subQty++;
      if (subQty > 5) {
        fprintf(stderr, "A quantidade de SUBMARINOS excedeu 5!\n");
        return -1;
      }
    } else if (strncmp(pieceBuffer, "TOR", 3) == 0) {
      if (addToField(tempX1, tempY1, tempX2, tempY2, TOR, tab))
        return -1;
      torQty++;
      if (torQty > 3) {
        fprintf(stderr, "A quantidade de TORPEDEIROS excedeu 3!\n");
        return -1;
      }
    } else if (strncmp(pieceBuffer, "TAS", 3) == 0) {
      if (addToField(tempX1, tempY1, tempX2, tempY2, TAS, tab))
        return -1;
      tasQty++;
      if (tasQty > 2) {
        fprintf(stderr, "A quantidade de NAVIO-TANQUE excedeu 2!\n");
        return -1;
      }
    } else if (strncmp(pieceBuffer, "AIP", 3) == 0) {
      if (addToField(tempX1, tempY1, tempX2, tempY2, AIP, tab))
        return -1;
      aipQty++;
      if (aipQty > 1) {
        fprintf(stderr, "A quantidade de PORTA-AVIOES excedeu 1!\n");
        return -1;
      }
    }
  }

  // Verify if all the pieces are present
  if (subQty != 5 || torQty != 3 || tasQty != 2 || aipQty != 1) {
    fprintf(stderr, "Pecas insuficientes para completar um tabuleiro!");
    return -1;
  }

  return 0;
}

void printField(tabuleiro *tab) {
  printf("\t1  2  3  4  5  6  7  8  9  10 11 12 13 14 "
         "15\n\t┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐\n");
  for (unsigned int i = 0; i < 15; i++) {
    // Letras do lado do tabuleiro
    printf("%c \t│", 97 + i);
    for (unsigned int j = 0; j < 15; j++) {
      switch (tab->field[i][j].type) {
      case SUB:
        // TODO guardar tiros do adversario assim como os seus, e mostra-los por
        // simbolos diferentes
        if (tab->field[i][j].clientShot && tab->field[i][j].serverShot) {
          printf("S@│");
        } else {
          // Bit do 32 (o 5o bit) esta levantado nas tres comparacoes
          printf("S%c│", '*' * tab->field[i][j].clientShot |
                             'o' * tab->field[i][j].serverShot | ' ');
        }
        break;
      case TOR:
        if (tab->field[i][j].clientShot && tab->field[i][j].serverShot) {
          printf("T@│");
        } else {
          // Bit do 32 (o 5o bit) esta levantado nas tres comparacoes
          printf("T%c│", '*' * tab->field[i][j].clientShot |
                             'o' * tab->field[i][j].serverShot | ' ');
        }
        break;
      case TAS:
        if (tab->field[i][j].clientShot && tab->field[i][j].serverShot) {
          printf("N@│");
        } else {
          // Bit do 32 (o 5o bit) esta levantado nas tres comparacoes
          printf("N%c│", '*' * tab->field[i][j].clientShot |
                             'o' * tab->field[i][j].serverShot | ' ');
        }
        break;
      case AIP:
        if (tab->field[i][j].clientShot && tab->field[i][j].serverShot) {
          printf("P@│");
        } else {
          // Bit do 32 (o 5o bit) esta levantado nas tres comparacoes
          printf("P%c│", '*' * tab->field[i][j].clientShot |
                             'o' * tab->field[i][j].serverShot | ' ');
        }
        break;
      case HIT:
        if (tab->field[i][j].clientShot && tab->field[i][j].serverShot) {
          printf("X@│");
        } else {
          // Bit do 32 (o 5o bit) esta levantado nas tres comparacoes
          printf("X%c│", '*' * tab->field[i][j].clientShot |
                             'o' * tab->field[i][j].serverShot | ' ');
        }
        break;
      default:
        if (tab->field[i][j].clientShot && tab->field[i][j].serverShot) {
          printf(" @│");
        } else {
          // Bit do 32 (o 5o bit) esta levantado nas tres comparacoes
          printf(" %c│", '*' * tab->field[i][j].clientShot |
                             'o' * tab->field[i][j].serverShot | ' ');
        }
        break;
      }
    }
    if (i + 1 == 15) {
      printf("\n");
      break;
    }
    printf("\n\t├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┤\n");
  }
  printf("\t└──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘\n\n");
}

void randomizePieces(tabuleiro *tab) {
  unsigned int tempX1, tempY1, tempX2, tempY2, subQty, torQty, tasQty, aipQty,
      pieceDir, swapAux;

  // Seed do numero aleatorio
  srand((time_t)NULL);

  tempX1 = 0;
  tempY1 = 0;
  tempX2 = 0;
  tempY2 = 0;
  swapAux = 0;

  subQty = 0;
  torQty = 0;
  tasQty = 0;
  aipQty = 0;

  // Um loop por tipo de peca

  while (subQty != 5) {
    // Como separei os loops, consigo saber qual o tamanho esperado de cada um
    // Posicoes iniciais
    tempX1 = rand() % 15;
    tempY1 = rand() % 15;
    tempX2 = 0;
    tempY2 = 0;

    pieceDir = rand() % 4;
    switch (pieceDir) {
    case UP:
      tempX2 = tempX1;
      // O final SEMPRE tem que ser maior que o inicial
      swapAux = tempY1;
      tempY2 = tempY1;
      tempY1 = swapAux - 1;
      break;
    case DOWN:
      tempX2 = tempX1;
      tempY2 = tempY1 + 1;
      break;
    case LEFT:
      swapAux = tempX1;
      tempX2 = tempX1;
      tempX1 = swapAux - 1;
      tempY2 = tempY1;
      break;
    case RIGHT:
      tempX2 = tempX1 + 1;
      tempY2 = tempY1;
      break;
    default:
      break;
    }

    if (addToField(tempX1, tempY1, tempX2, tempY2, SUB, tab) != -1) {
      subQty++;
    }

    clear();
  }

  while (torQty != 3) {
    // Posicoes iniciais
    tempX1 = rand() % 15;
    tempY1 = rand() % 15;
    tempX2 = 0;
    tempY2 = 0;

    pieceDir = rand() % 4;
    switch (pieceDir) {
    case UP:
      tempX2 = tempX1;
      swapAux = tempY1;
      tempY2 = tempY1;
      tempY1 = swapAux - 2;

      break;
    case DOWN:
      tempX2 = tempX1;
      tempY2 = tempY1 + 2;
      break;
    case LEFT:
      swapAux = tempX1;
      tempX2 = tempX1;
      tempX1 = swapAux - 2;
      tempY2 = tempY1;
      break;
    case RIGHT:
      tempX2 = tempX1 + 2;
      tempY2 = tempY1;
      break;
    default:
      break;
    }

    if (addToField(tempX1, tempY1, tempX2, tempY2, TOR, tab) != -1) {
      torQty++;
    }
    clear();
  }

  while (tasQty != 2) {
    // Posicoes iniciais
    tempX1 = rand() % 15;
    tempY1 = rand() % 15;
    tempX2 = 0;
    tempY2 = 0;

    pieceDir = rand() % 4;
    switch (pieceDir) {
    case UP:
      tempX2 = tempX1;
      swapAux = tempY1;
      tempY2 = tempY1;
      tempY1 = swapAux - 3;

      break;
    case DOWN:
      tempX2 = tempX1;
      tempY2 = tempY1 + 3;
      break;
    case LEFT:
      swapAux = tempX1;
      tempX2 = tempX1;
      tempX1 = swapAux - 3;
      tempY2 = tempY1;
      break;
    case RIGHT:
      tempX2 = tempX1 + 3;
      tempY2 = tempY1;
      break;
    default:
      break;
    }

    if (addToField(tempX1, tempY1, tempX2, tempY2, TAS, tab) != -1) {
      tasQty++;
    }
    clear();
  }

  while (aipQty != 1) {
    // Posicoes iniciais
    tempX1 = rand() % 15;
    tempY1 = rand() % 15;
    tempX2 = 0;
    tempY2 = 0;

    pieceDir = rand() % 4;
    switch (pieceDir) {
    case UP:
      tempX2 = tempX1;
      swapAux = tempY1;
      tempY2 = tempY1;
      tempY1 = swapAux - 4;

      break;
    case DOWN:
      tempX2 = tempX1;
      tempY2 = tempY1 + 4;
      break;
    case LEFT:
      swapAux = tempX1;
      tempX2 = tempX1;
      tempX1 = swapAux - 4;
      tempY2 = tempY1;
      break;
    case RIGHT:
      tempX2 = tempX1 + 4;
      tempY2 = tempY1;
      break;
    default:
      break;
    }

    if (addToField(tempX1, tempY1, tempX2, tempY2, AIP, tab) != -1) {
      aipQty++;
    }
    clear();
  }
}
