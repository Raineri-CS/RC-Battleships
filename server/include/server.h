#ifndef __SERVER_H__
#define __SERVER_H__

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h> // strlen
#include <sys/socket.h>
#include <unistd.h> // close
#include <signal.h> // Checagem de erros com signal() pra ignorar broken pipe
#include <errno.h>

#include "battleship.h" // Constantes

// Porta padrao caso nao for especificado no argv
#define DEFAULT_PORT 9030
// Numero maximo de clientes que o programa vai aceitar de uma so vez SEMPRE
// DEVE SER PAR
#define MAX_CLIENTS 2
// Numero maximo de jogadores em uma sessao de jogos, usado para a facil
// modularizacao da estrutura gameSession
#define MAX_PER_GAME_SESSION 2

// Estrutura que define uma sessao de jogos, com informacoes suficientes para a
// troca de mensagens entre MAX_PER_GAME_SESSION clientes, o codigo inteiro foi
// pensado nessa generalizacao de n
typedef struct gameSessionProto {
  int *clientFd[MAX_PER_GAME_SESSION];
  unsigned char isOngoing, areClientsReady;
  // Usado para guardar mensagens ate que todos os clientes estejam prontos
  char persistentBuffer[MAX_PER_GAME_SESSION][128];
} gameSession;

// Tenta fechar uma gameSession e todas as suas sockets respectivas, 0 pra
// sucesso, -1 pra erros
int endGameSession(gameSession *this, int *gameStatusArray,
                   int *clientSocketArray, int closedSocket);

// Procura o socketDescriptor dentro de clientSockets, volta o indice se achou,
// -1 se nao
int contains(int socketDescriptor, int *clientSockets);

// Seleciona o gamemode pelo server
// O uso do int index nao pode ser abstraido infelizmente, pois no final da
// funcao eu uso aritimetica de ponteiros pra settar o restante dos status dos
// clietnes da gameSession
void selectGameMode(gameSession *sessionArray, tabuleiro *serverField,
                    int *gameStatusArray, char *buffer, int *clientArray, int index);

void doGameIteration(gameSession *sessionArray, tabuleiro *serverField,
                     int *gameStatus, char *buffer, int *client,
                     unsigned int *lives);

#endif