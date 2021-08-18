#include "server.h"

int endGameSession(gameSession *this, int *status, int *clients) {
  int found, addrlen;
  char sendBuffer[32];
  struct sockaddr_in address;

  found = 0;
  addrlen = 0;

  // Fecha os descritores de todas as sockets
  for (int i = 0; i < MAX_PER_GAME_SESSION; i++) {
    if ((found = contains(*this->clientFd[i], clients)) != -1) {
      getpeername(clients[found], (struct sockaddr *)&address,
                  (socklen_t *)&addrlen);
      printf("Cliente encontrado em uma gameSession fechando, desconectando %s "
             "de porta %d...\n",
             inet_ntoa(address.sin_addr), ntohs(address.sin_port));
      sprintf(sendBuffer, "%c ", SERVER_FORCE_DISCONNECT + '0');
      if (send(clients[found], sendBuffer, strlen(sendBuffer), 0) !=
          (long int)strlen(sendBuffer)) {
        return -1;
      } else {
        printf(
            "Mensagem (SERVER_FORCE_DISCONNECT) enviada, fechando socket...\n");
        status[found] = 0;
        this->clientFd[i] = 0;
        // TODO verificar se essa eh a melhor opcao
        // close(clients[found]);
        clients[found] = 0;
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
  for (int i = 0; i < MAX_CLIENTS; i++)
    if (socketDescriptor == clientSockets[i])
      return i;
  return -1;
}

void selectGameMode(gameSession *sessionList, tabuleiro *serverField,
                    int *gameStatus, char *buffer, int *client, int index) {
  char sendBuffer[32];
  switch (atoi(&buffer[0])) {
  case COM:
    // O jogador quer jogar contra a maquina
    // Inicializar o tabuleiro do servidor
    randomizePieces(serverField);
    gameStatus[index] = COM;
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
      if (!sessionList[j].isOngoing) {
        for (int k = 0; k < MAX_PER_GAME_SESSION; k++) {
          // Se o pointer do cliente estiver vazio, quer dizer que a
          // "cadeira" dele pode ser ocupada
          if (sessionList[j].clientFd[k] == 0) {
            // sd eh o client socket atual, mas ele nao eh persistente
            sessionList[j].clientFd[k] = client;
            // Esse cara vem por fora, detectando as mensagens, entao
            // para this->socket == IDLE
            sprintf(sendBuffer, "%c ", IDLE + '0');
            if (send(*client, sendBuffer, strlen(sendBuffer), 0) !=
                (long int)strlen(sendBuffer)) {
              fprintf(stderr, "Erro ao enviar a mensagem em selectGameMode.\n");
            } else {
              printf("A mensagem (IDLE) foi enviada para o descritor "
                     "%d...\n",
                     *client);
            }
            // Se adicionou, nao tem merito continuar no loop
            break;
          } else if (sessionList[j].clientFd[k] != 0 &&
                     k + 2 == MAX_PER_GAME_SESSION) {
            // Essa parte eh para nao acontecer um deadlock em que os
            // dois estao no estado IDLE
            // Simula um bool
            sessionList[j].isOngoing = 1;
            sessionList[j].clientFd[k + 1] = client;
            sprintf(sendBuffer, "%c ", GAME_START + '0');
            // Como teoricamente a sala esta cheia, percorrer todos os
            // clientes, mandandoo a flag GAME_START
            for (int l = 0; l < MAX_PER_GAME_SESSION; l++) {
              if (send(*sessionList[j].clientFd[l], sendBuffer,
                       strlen(sendBuffer), 0) != (long int)strlen(sendBuffer)) {
                fprintf(stderr,
                        "Erro ao enviar a mensagem em selectGameMode.\n");
              } else {
                printf("A mensagem (GAME_START) foi enviada para o "
                       "descritor "
                       "%d...\n",
                       *sessionList[j].clientFd[l]);
              }
            }
            printf("Sessao de jogo iniciando para a gameSession %d...\n", j);

            // Tem que settar o gameStatus de todos os clientes
            // participantes para o modo de jogo PLAYER
            for (int l = 0; l < MAX_CLIENTS; l++) {
              if (*sessionList[j].clientFd[l] == client[l]) {
                gameStatus[l] = PLAYER;
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


