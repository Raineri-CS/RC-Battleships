# This is a networking assignment from 2021

## It's a battleships game using TCP as a protocol, the players can choose between playing against a CPU or a PLAYER.

* Rules:
  1. Players take turns trying to hit each other structures.
  1. Inputs are taken as a lowercase letter and a number, separated by a space, I.E (a 3)
  1. Once all the structures are destroyed, the game ends, tracked by the "lives" variable
  1. Structures are as follows:
    * 5|SUBmarines|(length 2)|SUB
    * 3|TORpedos  |(length 3)|TOR
    * 2|TAnk Ships|(length 4)|TAS
    * 1|AIrPort   |(length 5)|AIP

>Clientside modularizing and abstraction needs work, but it will suffice as the delivered code.

* About the .field format:
  1. To place a piece in the field, you need to call them by their code aliases
  1. Every piece alias needs to be declared with 2 XY plane coordinates (x1,y1) (x2,y2)
  1. The piece call should be something like this : PIECE_NAME 0 2 3 2
  1. Note that only ONE axis was modified, no diagonal pieces are allowed
  1. The field is a 15x15 matrix, with 0-14 being valid values
  1.The program will verify the integrity of the file, in case of an error, the program will exit