#include "include/battleship.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h> // FD_SET, FD_ISSET, FD_ZERO, time_t
#include <sys/types.h>
#include <unistd.h>

#define PORT 9030
// Numero maximo de clientes que o programa vai aceitar de uma so vez SEMPRE
// DEVE SER PAR
#define MAX_CLIENTS 2
#define MAX_PER_GAME_SESSION 2

typedef struct gameSessionProto {
  int *clientFd[MAX_PER_GAME_SESSION];
  unsigned char isOngoing, areClientsReady;
  // Usado para guardar mensagens ate que todos os clientes estejam prontos
  char persistentBuffer[MAX_PER_GAME_SESSION][128];
} gameSession;

int main(int argc, char const *argv[]) {
  int opt = 1;

  int masterSocket, addrlen, newSocket, clientSocket[MAX_CLIENTS], activity, i,
      valRead, sd, clientNum, gameStatus[MAX_CLIENTS], tempX, tempY,
      paramAmount;

  // Valor maximo dos descritores de socket
  int maxSd;

  gameSession session[MAX_CLIENTS / 2];

  struct sockaddr_in address;

  char buffer[1025], sendBuffer[128],
      persistentBuffer[MAX_CLIENTS][128]; // Buffer de dados

  // Conjunto de descritores de socket para a multiplexacao
  fd_set readfds;

  // a message
  char *message = "Battleships - Lucas Roberto Raineri Oliveira - 38346\r\n";

  // Os campos do server caso todos os clientes queiram jogar contra a CPU
  tabuleiro serverField[MAX_CLIENTS];

  // TODO ideia futura
  // A ideia inicial era connectedForPlayerGame ser um vetor para escalar em N
  // jogadores, porem, restricao de tempo isWaiting eh gambiarra pra nao ter que
  // refatorar o codigo e mexer com coisas sensiveis
  unsigned int connectedForPlayerGame, isWaiting[MAX_CLIENTS],
      lives[MAX_CLIENTS];

  clientNum = 0;
  connectedForPlayerGame = 0;
  paramAmount = 0;
  // Seed do gerador de numeros aleatorios
  srand((time_t)NULL);

  // Inicializa todas as sockets e auxiliares com 0
  for (i = 0; i < MAX_CLIENTS; i++) {
    init(&serverField[i]);
    lives[i] = 32;
    isWaiting[i] = 0;
    gameStatus[i] = 0;
    clientSocket[i] = 0;
  }

  // Inicializa as salas
  for (i = 0; i < MAX_CLIENTS / 2; i++) {
    session[i].areClientsReady = 0;
    session[i].isOngoing = 0;
    session[i].clientFd[0] = 0;
    session[i].clientFd[1] = 0;
    for (int j = 0; j < MAX_PER_GAME_SESSION; j++) {
      for (int k = 0; k < 128; k++) {
        session[i].persistentBuffer[j][k] = '\0';
      }
    }
  }

  // Cria a socket master
  if ((masterSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    fprintf(stderr, "socket failed");
    return -1;
  }

  // Fala pra socket master poder aceitar multiplas conexoes, dependendo do
  // sistema operacional, se usa o SO_REUSEPORT, mas o SO_REUSEADDR cobre a
  // maior gama de sistemas
  if (setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                 sizeof(opt)) < 0) {
    fprintf(stderr, "setsockopt");
    return -1;
  }

  // Tipo da socket
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // Bind da socket pro localhost com a PORT definida por constante, e atribuida
  // acima
  if (bind(masterSocket, (struct sockaddr *)&address, sizeof(address)) < 0) {
    fprintf(stderr, "bind failed");
    return -1;
  }

  printf("Ouvindo na porta %d... \n", PORT);

  // So se podem ter 2 conexoes pendentes no server...
  if (listen(masterSocket, 2) < 0) {
    fprintf(stderr, "listen");
    return -1;
  }

  // Aceita conexoes
  addrlen = sizeof(address);

  // A ideia eh ter um loop infinito, que passa por todas as sockets por loop, a
  // fluidez do servidor depende inteiramente do processador conseguir executar
  // mais rapido o programa, se for muito ruim eu vou tentar colocar um -O3 na
  // compilacao
  for (;;) {
    // Limpa o conjunto de descritores
    FD_ZERO(&readfds);

    // Adiciona a socket mestre no conjunto
    FD_SET(masterSocket, &readfds);
    maxSd = masterSocket;

    // Adiciona as sockets subsequentes no conjunto
    for (i = 0; i < MAX_CLIENTS; i++) {
      // Descritor de socket...
      sd = clientSocket[i];

      // Se o descritor existe (usa 0 como valor base e negativos nao sao
      // aceitos)
      if (sd > 0)
        FD_SET(sd, &readfds);

      // Numero mais alto do descritor, para ser usado do select
      if (sd > maxSd)
        maxSd = sd;
    }

    // Espera alguma atividade em alguma das sockets com um timeout de NULL, ou
    // seja, indefinido
    activity = select(maxSd + 1, &readfds, NULL, NULL, NULL);

    // Se activity for menor que 0 ha algum erro, pois o select volta o numero
    // de descritores prontos
    if ((activity < 0) && (errno != EINTR)) {
      printf("select error");
    }

    // Se algo mudou na socket mestre, eh uma conexao vindo
    if (FD_ISSET(masterSocket, &readfds)) {
      // Aceita a conexao
      if ((newSocket = accept(masterSocket, (struct sockaddr *)&address,
                              (socklen_t *)&addrlen)) < 0) {
        fprintf(stderr, "accept");
        return -1;
      }

      // Informacoes uteis
      printf("Nova conexao , descritor da socket eh %d , ip eh : %s , port : "
             "%d \n ",
             newSocket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

      printf("%d cliente(s)\n", ++clientNum);

      // Adiciona a socket no vetor de sockets
      for (i = 0; i < MAX_CLIENTS; i++) {
        // Se a posicao estiver vazia, adicionar
        if (clientSocket[i] == 0) {
          clientSocket[i] = newSocket;
          printf("Adicionando a socket em %d\n", i);
          // Break porque nao quero adicionar mais de uma por loop, ja que fazer
          // singlethread eh mais simples
          break;
        }
      }
    }

    // Se nada aconteceu com a socket mestre quer dizer que alguma operacao de
    // Input/Output esta acontecendo com alguma oputra socket
    for (i = 0; i < MAX_CLIENTS; i++) {
      // Para cada descritor de socket...
      sd = clientSocket[i];

      if (FD_ISSET(sd, &readfds)) {
        // Se a operacao que esta vindo eh de fechamento e leitura da mensagem
        // vindo
        if ((valRead = read(sd, buffer, 1024)) == 0) {
          // Alguem disconectou, printa as informacoes da desconexao na tela
          getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
          printf("Hospedeiro desconectou , ip %s , port %d \n",
                 inet_ntoa(address.sin_addr), ntohs(address.sin_port));

          // Fechar o descritor da socket e liberar espaco no vetor de
          // sockets...
          close(sd);
          // Resetar o status do jogo
          for (int j = 0; j < (MAX_CLIENTS / 2); j++) {
            for (int l = 0; l < MAX_CLIENTS; l++) {
              if (*session[j].clientFd[l] == clientSocket[l]) {
                gameStatus[l] = 0;
              }
            }
          }
          // Decrementar a quantidade de clientes
          clientNum--;
          clientSocket[i] = 0;
        }
        // A troca de mensagens vai ser aqui
        else {
          // Usado no PvP
          paramAmount = 0;
          // Como as mensagens que vem nao tem o caractere nulo pra terminar,
          // adicionar
          buffer[valRead] = '\0';
          // TODO
          // Se o status do jogo for 0 quer dizer que nao tem nenhum jogo sendo
          // executado
          if (!gameStatus[i]) {
            // Vai definir o gamemode
            switch (atoi(&buffer[0])) {
            case COM:
              // O jogador quer jogar contra a maquina
              // Inicializar o tabuleiro do servidor
              randomizePieces(&serverField[i]);
              gameStatus[i] = COM;
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
                if (!session[j].isOngoing) {
                  for (int k = 0; k < MAX_PER_GAME_SESSION; k++) {
                    // Se o pointer do cliente estiver vazio, quer dizer que a
                    // "cadeira" dele pode ser ocupada
                    if (session[j].clientFd[k] == 0) {
                      // sd eh o client socket atual, mas ele nao eh persistente
                      session[j].clientFd[k] = &clientSocket[i];
                      // Esse cara vem por fora, detectando as mensagens, entao
                      // para this->socket == IDLE
                      sprintf(sendBuffer, "%c ", IDLE + '0');
                      if (send(clientSocket[i], sendBuffer, strlen(sendBuffer),
                               0) != strlen(sendBuffer)) {
                        fprintf(stderr, "Erro ao enviar a mensagem.\n");
                      } else {
                        printf("A mensagem (%s) foi enviada para o descritor "
                               "%d...\n",
                               sendBuffer, clientSocket[i]);
                      }
                      // Se adicionou, nao tem merito continuar no loop
                      break;
                    } else if (session[j].clientFd[k] != 0 &&
                               k + 2 == MAX_PER_GAME_SESSION) {
                      // Essa parte eh para nao acontecer um deadlock em que os
                      // dois estao no estado IDLE
                      // Simula um bool
                      session[j].isOngoing = 1;
                      session[j].clientFd[k + 1] = &clientSocket[i];
                      sprintf(sendBuffer, "%c ", GAME_START + '0');
                      // Como teoricamente a sala esta cheia, percorrer todos os
                      // clientes, mandandoo a flag GAME_START
                      for (int l = 0; l < MAX_PER_GAME_SESSION; l++) {
                        if (send(*session[j].clientFd[l], sendBuffer,
                                 strlen(sendBuffer), 0) != strlen(sendBuffer)) {
                          fprintf(stderr, "Erro ao enviar a mensagem.\n");
                        } else {
                          printf("A mensagem (%s) foi enviada para o descritor "
                                 "%d...\n",
                                 sendBuffer, *session[j].clientFd[l]);
                        }
                      }
                      printf(
                          "Sessao de jogo iniciando para a gameSession %d...\n",
                          j);

                      // Tem que settar o gameStatus de todos os clientes
                      // participantes para o modo de jogo PLAYER
                      for (int l = 0; l < MAX_CLIENTS; l++) {
                        if (*session[j].clientFd[l] == clientSocket[l]) {
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
          } else {
            switch (gameStatus[i]) {
            case COM:
              if (valRead > 1) {
                sscanf(buffer, "%d %d", &tempX, &tempY);
                // Verifica se acertou alguma estrutura
                if (fireProjectile(tempX, tempY, &serverField[i]) == HIT) {
                  lives[i]--;
                  if (lives[i] <= 0) {
                    // Setta aqui o que vai ser ENVIADO pro cliente
                    sprintf(sendBuffer, "%c ", GAME_WIN + '0');
                    if (send(clientSocket[i], sendBuffer, strlen(sendBuffer),
                             0) != strlen(sendBuffer)) {
                      fprintf(stderr, "Erro ao enviar a mensagem.\n");
                    } else {
                      printf("A mensagem (%s) foi enviada para o descritor "
                             "%d...\n",
                             sendBuffer, clientSocket[i]);
                    }
                  }
                  sprintf(sendBuffer, "%c %d %d ", GAME_HIT + '0', rand() % 15,
                          rand() % 15);
                  if (send(clientSocket[i], sendBuffer, strlen(sendBuffer),
                           0) != strlen(sendBuffer)) {
                    fprintf(stderr, "Erro ao enviar a mensagem.\n");
                  } else {
                    printf("A mensagem (%s) foi enviada para o descritor "
                           "%d...\n",
                           sendBuffer, clientSocket[i]);
                  }

                } else {
                  sprintf(sendBuffer, "%c %d %d ", GAME_MISS + '0', rand() % 15,
                          rand() % 15);
                  if (send(clientSocket[i], sendBuffer, strlen(sendBuffer),
                           0) != strlen(sendBuffer)) {
                    fprintf(stderr, "Erro ao enviar a mensagem.\n");
                  } else {
                    printf("A mensagem (%s) foi enviada para o descritor "
                           "%d...\n",
                           sendBuffer, clientSocket[i]);
                  }
                }
              } else {
                if (buffer[0] == GAME_WIN || buffer[0] == GAME_LOSE) {
                  sprintf(sendBuffer, "%c ", GAME_LOSE + '0');
                  if (send(clientSocket[i], sendBuffer, strlen(sendBuffer),
                           0) != strlen(sendBuffer)) {
                    fprintf(stderr, "Erro ao enviar a mensagem.\n");
                  } else {
                    printf("A mensagem (%s) foi enviada para o descritor "
                           "%d...\n",
                           sendBuffer, clientSocket[i]);
                  }
                  gameStatus[i] = GAME_OVER;
                }
              }
              break;
            case PLAYER:
              for (int j = 0; j < (MAX_CLIENTS / 2); j++) {
                for (int k = 0; k < MAX_PER_GAME_SESSION; k++) {
                  // Se o cara que enviou a mensagem pertence a essa sessao
                  if (*session[j].clientFd[k] == sd) {
                    // Copiar a mensagem para um buffer persistente de indice
                    // igual ao seu no array de clientes
                    strcpy(session[j].persistentBuffer[k], buffer);
                    printf("A mensagem (%s) foi recebida do descritor %d...\n",session[j].persistentBuffer[k],clientSocket[i]);
                    // O custo de fazer as coisas sem ser async eh a
                    // complexidade quadratica Verifica se todos os clientes tem
                    // mensagens para enviar
                    // Assumir com otimismo que todas as mensagens estarao
                    // prontas para envio
                    session[j].areClientsReady = 1;
                    for (int l = 0; l < MAX_PER_GAME_SESSION; l++) {
                      if (session[j].persistentBuffer[l][0] == '\0') {
                        session[j].areClientsReady = 0;
                        break;
                      }
                    }

                    // Se todas as mensagens estao prontas para envio
                    if (session[j].areClientsReady) {
                      for (int l = 0; l < MAX_PER_GAME_SESSION; l++) {
                        strcpy(sendBuffer, session[j].persistentBuffer[l]);
                        // Vai enviar para todos os clientes a nao ser ele mesmo
                        for (int m = 0; m < MAX_PER_GAME_SESSION; m++) {
                          if (l != m) {
                            if (send(*session[j].clientFd[m], sendBuffer,
                                     strlen(sendBuffer),
                                     0) != strlen(sendBuffer)) {
                              fprintf(stderr, "Erro ao enviar a mensagem.\n");
                            } else {
                              printf("A mensagem (%s) foi enviada para o "
                                     "descritor "
                                     "%d...\n",
                                     sendBuffer, *session[j].clientFd[m]);
                            }
                          }
                        }
                        // Depois de enviadas, "zerar" o buffer persistente
                        // atual
                        strcpy(session[j].persistentBuffer[l], "\0");
                      }
                    } else {
                      // Passa a flag IDLE para que o jogador espere o outro
                      // fazer seu movimento
                      sprintf(sendBuffer, "%c ", IDLE + '0');
                      if (send(clientSocket[i], sendBuffer, strlen(sendBuffer),
                               0) != strlen(sendBuffer)) {
                        fprintf(stderr, "Erro ao enviar a mensagem.\n");
                      } else {
                        printf("A mensagem (%s) foi enviada para o "
                               "descritor "
                               "%d...\n",
                               sendBuffer, clientSocket[i]);
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
        }
      }
    }
  }

  return 0;
}
