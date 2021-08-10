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
// Numero maximo de clientes que o programa vai aceitar de uma so vez
#define MAX_CLIENTS 2

// FIXME aprender a usar o SELECT
int main(int argc, char const *argv[]) {
  int opt = 1;

  int masterSocket, addrlen, newSocket, clientSocket[MAX_CLIENTS], activity, i,
      valRead, sd, clientNum, gameStatus[MAX_CLIENTS], tempX, tempY;

  // Valor maximo dos descritores de socket
  int maxSd;

  struct sockaddr_in address;

  char buffer[1025], sendBuffer[128]; // Buffer de dados

  // Conjunto de descritores de socket para a multiplexacao
  fd_set readfds;

  // a message
  char *message = "Battleships - Lucas Roberto Raineri oliveira \r\n";

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

      // Manda o status de espera
      if (send(newSocket, "1", 1, 0) != strlen(message)) {
        fprintf(stderr, "send");
      }

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
          gameStatus[i] = 0;
          // Decrementar a quantidade de clientes
          clientNum--;
          clientSocket[i] = 0;
        }
        // A troca de mensagens vai ser aqui
        else {
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
              // O jogador quer jogar contra outro jogador, portanto, aguardar 2
              // conexoes
              if (connectedForPlayerGame == 2) {
                gameStatus[i] = PLAYER;
              } else if (!isWaiting[i]) {
                isWaiting[i] = 1;
                connectedForPlayerGame++;
              }
              break;
            default:
              break;
            }
          } else {
            // TODO O jogo vai acontecer aqui
            switch (gameStatus[i]) {
            case COM:
              if (valRead > 1) {
                sscanf(buffer, "%d %d", &tempX, &tempY);
                // Verifica se acertou alguma estrutura
                if (fireProjectile(tempX, tempY, &serverField[i]) == HIT) {
                  lives[i]--;
                  sprintf(sendBuffer, "%d", GAME_HIT + '0');
                  send(clientSocket[i], sendBuffer, strlen(sendBuffer), 0);
                }
                // Se a quantidade de vidas dessa estrutura...
                if (lives[i] <= 0) {
                  // Setta aqui o que vai ser ENVIADO pro cliente
                  sprintf(sendBuffer, "%c", GAME_WIN + '0');
                  send(clientSocket[i], sendBuffer, strlen(sendBuffer), 0);
                } else {
                  // Se as vidas nao acabaram, continua o jogo...
                  sprintf(sendBuffer, "%d %d", rand() % 15, rand() % 15);
                  send(clientSocket[i], sendBuffer, strlen(sendBuffer), 0);
                }
              } else {
                if (buffer[0] == GAME_WIN || buffer[0] == GAME_LOSE) {
                  sprintf(sendBuffer, "%c", GAME_LOSE + '0');
                  send(clientSocket[i], sendBuffer, strlen(sendBuffer), 0);
                  gameStatus[i] = GAME_OVER;
                }
              }
              break;
            case PLAYER:
              break;
            case GAME_LOSE:
            case GAME_WIN:
              // TODO Mandar mensagem e encerrar o jogo no game_over
            case GAME_OVER:
              // TODO Encerrrar o jogo aqui
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
