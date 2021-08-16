#ifndef __BATTLESHIP_H__
#define __BATTLESHIP_H__
// TODO documentar o que usei de cada
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h> // abs
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h> // close

// Constantes gerais
#define FIELD_SIZE 15
#define SUB 0  // SUBmarino
#define TOR 1  // TORpedeiro
#define TAS 2  // TAnk Ship
#define AIP 3  // AIr Port
#define WAT 10 // WATer
// Parte destruida?
#define HIT 11

// Game modes
#define COM 1
#define PLAYER 2

// Orientations
#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

// Status codes
#define IDLE 1
#define GAME_START 2
#define GAME_WIN 3
#define GAME_LOSE 4
#define GAME_MOVE 5
#define GAME_HIT 6
#define GAME_MISS 7
#define GAME_OVER 8
#define GAME_WAIT 9

typedef struct pieceProto {
  unsigned char isOccupied;
  unsigned char clientShot;
  unsigned char serverShot;
  unsigned char type;
} piece;

typedef struct tabuleiroProto {
  piece field[15][15];
} tabuleiro;

void init(tabuleiro *tab);
/*
ID|Quantidade|Representacao|Nome formal     |Espaco ocupado
0 |5         |SS           |submarinos (S)  |ocupando 2 quadrados adjacentes;
1 |3         |TTT          |torpedeiros (T) |ocupando 3 quadrados adjacentes;
2 |2         |NNNN         |navio-tanque (N)|ocupando 4 quadrados adjacentes;
3 |1         |PPPPP        |porta-aviões (P)|ocupando 5 quadrados adjacentes;
*/
// Adiciona ao campo, sempre inicial, final, tipo
int addToField(unsigned int x1, unsigned int y1, unsigned int x2,
               unsigned int y2, unsigned char type, tabuleiro *tab);

// Tenta acertar uma casa do tabuleiro adversario
int fireProjectile(unsigned int x, unsigned int y, tabuleiro *tab);

// Loop do jogo, interface principal com o main.c
int gameLoop(char *domain, unsigned short int port, unsigned char gameMode,
             tabuleiro *tab);

// Verifica se o arquivo condiz com as regras
int verifyFileIntegrity(FILE *fd, tabuleiro *tab);

// Mostra o tabuleiro DO JOGADOR LOCAL
void printField(tabuleiro *tab);

void randomizePieces(tabuleiro *tab);
/*
  Simbolos utilizados (Extended ASCII)
  │└├─┐┬┴┼┘┌

  Dados usados pra representar
    -> Matriz 15x15 com os simbolos (no meio de cada espaco tem tres espacos, no
printf vai ter (' %c ',mat[i][j]))

-------------------------------------------------------------
   1  2  3  4  5  6  7  8  9  10 11 12 13 14 15
  ┌──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┬──┐
a │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┤
b │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┤
c │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┤
d │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┤
e │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┤
f │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┤
g │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┤
h │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┤
i │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┤
j │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┤
k │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┤
l │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  ├──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┼──┤
m │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  └──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┘
-------------------------------------------------------------

*/
#endif