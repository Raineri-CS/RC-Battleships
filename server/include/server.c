#include "server.h"

int endGameSession(gameSession *this, int *status, int *clients,
                   int closedSocket) {
  int found, addrlen;
  char sendBuffer[32];
  struct sockaddr_in address;

  found = 0;
  addrlen = sizeof(address);

  // Fecha os descritores de todas as sockets
  for (int index = 0; index < MAX_PER_GAME_SESSION; index++) {
    // Ve o erro

    if ((found = contains(*this->clientFd[index], clients)) != -1) {
      // Operacoes diferentes para o descritor que ja fechou
      // FIXME a socket "fechada" entra aqui de qualquer jeito por algum motivo
      if (closedSocket != *this->clientFd[index]) {
        if (getpeername(clients[found], (struct sockaddr *)&address,
                        (socklen_t *)&addrlen) == -1) {
          if (ENOTCONN) {
            fprintf(stderr, "O peer nao esta conectado \n");
          }
          fprintf(stderr, "Erro ao resolver nome do peer, com o erro %d.\n",
                  errno);
        }
        printf(
            "Cliente encontrado em uma gameSession fechando, desconectando %s "
            "de porta %d...\n",
            inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        sprintf(sendBuffer, "%c ", SERVER_FORCE_DISCONNECT + '0');
        if (send(clients[found], sendBuffer, strlen(sendBuffer), 0) !=
            (long int)strlen(sendBuffer)) {
          return -1;
        } else {
          printf("Mensagem (SERVER_FORCE_DISCONNECT) enviada...\n");
          status[found] = 0;
          this->clientFd[index] = 0;
          // TODO verificar se essa eh a melhor opcao
          // close(clients[found]);
          // clients[found] = 0;
        }
      } else {
        clients[found] = 0;
        status[found] = 0;
        this->clientFd[index] = 0;
      }
    } else {
      return -1;
    }
  }
  // Setta os parametros default da gameSession
  this->areClientsReady = 0;
  this->isOngoing = 0;
  // O persistentBuffer fica ocupado porque o server que eh o encarregado de
  // lidar com essa informacao em gametime
  return 0;
}

int contains(int socketDescriptor, int *clientSockets) {
  for (int index = 0; index < MAX_CLIENTS; index++)
    if (socketDescriptor == clientSockets[index])
      return index;
  return -1;
}

void selectGameMode(gameSession *sessionList, tabuleiro *serverField,
                    int *gameStatus, char *buffer, int *clientArray,
                    int index) {
  char sendBuffer[32];
  switch (atoi(&buffer[0])) {
  case COM:
    // O jogador quer jogar contra a maquina
    // Inicializar o tabuleiro do servidor
    randomizePieces(serverField);
    *(gameStatus + index) = COM;
    break;
  case PLAYER:
    // Caso escolha jogar contra outro jogador, usar a gameSession
    // Para fazer isso eu decidi criar uma estrutura gameSession, que
    // guarda as informacoes principais dos clientes conectados e
    // jogando, facilitando a organizacao futura do escalamento do
    // software, o drawback sendo a checagem de pertencimento a cada
    // mensagem recebida, encontrando a sessao que o this jogador
    // pertence

    for (int j = 0; j < (MAX_CLIENTS / 2); j++) {
      // Se a sessao nao estiver cheia, adicionar this cliente na
      // sessao
      if (!(sessionList + j)->isOngoing) {
        for (int k = 0; k < MAX_PER_GAME_SESSION; k++) {
          // Se o pointer do cliente estiver vazio, quer dizer que a
          // "cadeira" dele pode ser ocupada
          if ((sessionList + j)->clientFd[k] == 0) {
            // *client eh o client socket atual, mas ele nao eh persistente
            (sessionList + j)->clientFd[k] = (clientArray + index);
            // Esse cara vem por fora, detectando as mensagens, entao
            // para this->socket == IDLE
            sprintf(sendBuffer, "%c ", IDLE + '0');
            if (send(*(clientArray + index), sendBuffer, strlen(sendBuffer),
                     0) != (long int)strlen(sendBuffer)) {
              if (errno == EPIPE) {
                fprintf(stderr,
                        "Broken pipe no envio em selectGameMode, "
                        "desconexao iminente, ignorando a mensagem...\n");
              } else {
                fprintf(stderr,
                        "Erro ao enviar a mensagem em selectGameMode.\n");
              }
            } else {
              printf("A mensagem (IDLE) foi enviada para o descritor "
                     "%d...\n",
                     *(clientArray + index));
            }
            // Se adicionou, nao tem merito continuar no loop
            break;
          } else if ((sessionList + j)->clientFd[k] != 0 &&
                     k + 2 == MAX_PER_GAME_SESSION) {
            // Essa parte eh para nao acontecer um deadlock em que os
            // dois estao no estado IDLE
            // Simula um bool
            (sessionList + j)->isOngoing = 1;
            (sessionList + j)->clientFd[k + 1] = (clientArray + index);
            sprintf(sendBuffer, "%c ", GAME_START + '0');
            // Como teoricamente a sala esta cheia, percorrer todos os
            // clientes, mandandoo a flag GAME_START
            for (int l = 0; l < MAX_PER_GAME_SESSION; l++) {
              if (send(*(sessionList + j)->clientFd[l], sendBuffer,
                       strlen(sendBuffer), 0) != (long int)strlen(sendBuffer)) {
                fprintf(stderr,
                        "Erro ao enviar a mensagem em selectGameMode.\n");
              } else {
                printf("A mensagem (GAME_START) foi enviada para o "
                       "descritor "
                       "%d...\n",
                       *(sessionList + j)->clientFd[l]);
              }
            }
            printf("Sessao de jogo iniciando para a gameSession %d...\n", j);

            // Tem que settar o gameStatus de todos os clientes
            // participantes para o modo de jogo PLAYER
            for (int d = 0; d < MAX_PER_GAME_SESSION; d++) {
              for (int l = 0; l < MAX_CLIENTS; l++) {
                if (*(sessionList + j)->clientFd[d] == *(clientArray + l)) {
                  *(gameStatus + l) = PLAYER;
                }
              }
            }

            // Economiza um loop
            break;
          }
        }
      }
    }
    break;
  default:
    break;
  }
}

void doGameIteration(gameSession *sessionList, tabuleiro *serverField,
                     int *gameStatus, char *buffer, int *client,
                     unsigned int *lives) {
  char sendBuffer[32];
  int paramAmount, valRead, tempX, tempY, addrlen;
  struct sockaddr_in address;

  paramAmount = 0;
  valRead = strlen(buffer);
  tempX = 0;
  tempY = 0;
  addrlen = sizeof(address);

  switch (*gameStatus) {
  case COM:
    paramAmount = 0;
    for (int c = 0; c < valRead; c++) {
      if (buffer[c] == ' ') {
        paramAmount++;
      }
    }
    switch (paramAmount) {
    case 3:
      sscanf(buffer, "%c %d %d", &buffer[0], &tempX, &tempY);

      if (fireProjectile(tempX, tempY, serverField) == HIT) {
        *lives -= 1;
        // Se a quantidade de vidas dessa estrutura...
        if (*lives <= 0) {
          // Setta aqui o que vai ser ENVIADO pro cliente
          sprintf(sendBuffer, "%c %d %d ", GAME_WIN + '0', 0, 0);
          if (send(*client, sendBuffer, strlen(sendBuffer), 0) !=
              (long int)strlen(sendBuffer)) {
            fprintf(stderr, "Erro ao enviar a mensagem.\n");
          } else {
            printf("A mensagem (%s) foi enviada para o descritor "
                   "%d...\n",
                   sendBuffer, *client);
          }
          break;
        }
        // Se as vidas nao acabaram, continua o jogo...
        switch (atoi(&buffer[0])) {
        case GAME_START:
          sprintf(sendBuffer, "%c %d %d ", GAME_HIT + '0', rand() % 15,
                  rand() % 15);
          if (send(*client, sendBuffer, strlen(sendBuffer), 0) !=
              (long int)strlen(sendBuffer)) {
            fprintf(stderr, "Erro ao enviar a mensagem.\n");
          } else {
            printf("A mensagem (%s) foi enviada para o descritor "
                   "%d...\n",
                   sendBuffer, *client);
          }
          break;
        case GAME_HIT:
          sprintf(sendBuffer, "%c %d %d ", GAME_HIT + '0', rand() % 15,
                  rand() % 15);
          if (send(*client, sendBuffer, strlen(sendBuffer), 0) !=
              (long int)strlen(sendBuffer)) {
            fprintf(stderr, "Erro ao enviar a mensagem.\n");
          } else {
            printf("A mensagem (%s) foi enviada para o descritor "
                   "%d...\n",
                   sendBuffer, *client);
          }
          break;
        case GAME_MISS:
          sprintf(sendBuffer, "%c %d %d ", GAME_HIT + '0', rand() % 15,
                  rand() % 15);
          if (send(*client, sendBuffer, strlen(sendBuffer), 0) !=
              (long int)strlen(sendBuffer)) {
            fprintf(stderr, "Erro ao enviar a mensagem.\n");
          } else {
            printf("A mensagem (%s) foi enviada para o descritor "
                   "%d...\n",
                   sendBuffer, *client);
          }
          break;
        case GAME_WIN:
        case GAME_LOSE:
          // Desconecta o peer mesmo antes do peer se desconectar,
          // pois o jogo acabou
          printf("O jogo contra o servidor do cliente %d acabou, "
                 "desconectando...\n",
                 *client);
          if (getpeername(*client, (struct sockaddr *)&address,
                          (socklen_t *)&addrlen) == -1) {
            if (ENOTCONN) {
              fprintf(stderr, "O peer nao esta conectado \n");
            }
            fprintf(stderr, "Erro ao resolver nome do peer, com o erro %d.\n",
                    errno);
          }
          printf("Hospedeiro desconectou , ip %s , porta %d \n",
                 inet_ntoa(address.sin_addr), ntohs(address.sin_port));

          sprintf(sendBuffer, "%c 0 0 ", SERVER_FORCE_DISCONNECT + '0');
          send(*client, sendBuffer, strlen(sendBuffer), 0);

          break;
        default:
          break;
        }
      } else {
        // Aqui o cliente errou
        switch (atoi(&buffer[0])) {
        case GAME_START:
          sprintf(sendBuffer, "%c %d %d ", GAME_HIT + '0', rand() % 15,
                  rand() % 15);
          if (send(*client, sendBuffer, strlen(sendBuffer), 0) !=
              (long int)strlen(sendBuffer)) {
            fprintf(stderr, "Erro ao enviar a mensagem.\n");
          } else {
            printf("A mensagem (%s) foi enviada para o descritor "
                   "%d...\n",
                   sendBuffer, *client);
          }
          break;
        case GAME_HIT:
          sprintf(sendBuffer, "%c %d %d ", GAME_MISS + '0', rand() % 15,
                  rand() % 15);
          if (send(*client, sendBuffer, strlen(sendBuffer), 0) !=
              (long int)strlen(sendBuffer)) {
            fprintf(stderr, "Erro ao enviar a mensagem.\n");
          } else {
            printf("A mensagem (%s) foi enviada para o descritor "
                   "%d...\n",
                   sendBuffer, *client);
          }
          break;
        case GAME_MISS:
          sprintf(sendBuffer, "%c %d %d ", GAME_MISS + '0', rand() % 15,
                  rand() % 15);
          if (send(*client, sendBuffer, strlen(sendBuffer), 0) !=
              (long int)strlen(sendBuffer)) {
            fprintf(stderr, "Erro ao enviar a mensagem.\n");
          } else {
            printf("A mensagem (%s) foi enviada para o descritor "
                   "%d...\n",
                   sendBuffer, *client);
          }
          break;
        case GAME_WIN:
        case GAME_LOSE:
          // Desconecta o peer mesmo antes do peer se desconectar,
          // pois o jogo acabou
          printf("O jogo contra o servidor do cliente %d acabou, "
                 "desconectando...\n",
                 *client);
          if (getpeername(*client, (struct sockaddr *)&address,
                          (socklen_t *)&addrlen) == -1) {
            if (ENOTCONN) {
              fprintf(stderr, "O peer nao esta conectado \n");
            }
            fprintf(stderr, "Erro ao resolver nome do peer, com o erro %d.\n",
                    errno);
          }
          printf("Hospedeiro desconectou , ip %s , porta %d \n",
                 inet_ntoa(address.sin_addr), ntohs(address.sin_port));

          sprintf(sendBuffer, "%c 0 0 ", SERVER_FORCE_DISCONNECT + '0');
          send(*client, sendBuffer, strlen(sendBuffer), 0);
          break;
        default:
          break;
        }
      }

      break;
    default:
      break;
    }
    break;
  case PLAYER:
    for (int j = 0; j < (MAX_CLIENTS / 2); j++) {
      for (int k = 0; k < MAX_PER_GAME_SESSION; k++) {
        // Se o cara que enviou a mensagem pertence a essa sessao
        if (*(sessionList + j)->clientFd[k] == *client) {
          // Copiar a mensagem para um buffer persistente de indice
          // igual ao seu no array de clientes
          strcpy((sessionList + j)->persistentBuffer[k], buffer);
          printf("A mensagem (%s) foi recebida do descritor %d...\n",
                 (sessionList + j)->persistentBuffer[k], *client);
          // O custo de fazer as coisas sem ser async eh a
          // complexidade quadratica Verifica se todos os clientes
          // tem mensagens para enviar Assumir com otimismo que
          // todas as mensagens estarao prontas para envio
          (sessionList + j)->areClientsReady = 1;
          for (int l = 0; l < MAX_PER_GAME_SESSION; l++) {
            if ((sessionList + j)->persistentBuffer[l][0] == '\0') {
              (sessionList + j)->areClientsReady = 0;
              break;
            }
          }

          // Se todas as mensagens estao prontas para envio
          if ((sessionList + j)->areClientsReady) {
            for (int l = 0; l < MAX_PER_GAME_SESSION; l++) {
              strcpy(sendBuffer, (sessionList + j)->persistentBuffer[l]);
              // Vai enviar para todos os clientes a nao ser ele
              // mesmo
              for (int m = 0; m < MAX_PER_GAME_SESSION; m++) {
                if (l != m) {
                  if (send(*(sessionList + j)->clientFd[m], sendBuffer,
                           strlen(sendBuffer),
                           0) != (long int)strlen(sendBuffer)) {
                    // Se o erro for broken pipe...
                    if (errno == EPIPE) {
                      fprintf(stderr,
                              "Broken pipe no envio em doGameIteration, "
                              "desconexao iminente, ignorando a mensagem...\n");
                    } else {
                      // Erro generico
                      fprintf(stderr, "Erro ao enviar a mensagem.\n");
                    }
                  } else {
                    printf("A mensagem (%s) foi enviada para o "
                           "descritor "
                           "%d...\n",
                           sendBuffer, *(sessionList + j)->clientFd[m]);
                  }
                }
              }
              // Depois de enviadas, "zerar" o buffer persistente
              // atual
              memset((sessionList + j)->persistentBuffer[l], '\0', 128);
            }
          } else {
            // Passa a flag IDLE para que o jogador espere o outro
            // fazer seu movimento
            sprintf(sendBuffer, "%c ", IDLE + '0');
            if (send(*client, sendBuffer, strlen(sendBuffer), 0) !=
                (long int)strlen(sendBuffer)) {
              fprintf(stderr, "Erro ao enviar a mensagem.\n");
            } else {
              printf("A mensagem (IDLE) foi enviada para o "
                     "descritor "
                     "%d...\n",
                     *client);
            }
          }
        }
      }
    }
    break;
  default:
    break;
  }
}
