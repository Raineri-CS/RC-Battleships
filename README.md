This is a networking assignment from 2021

It's a battleships game using TCP as a protocol, the players can choose between playing against a CPU or a PLAYER.

About the .field format:
  To build a piece in the field, you need to call them by their code aliases

  Every piece alias needs to be declared with 2 XY plane coordinates (x1,y1) (x2,y2)
  
  The piece call should be something like this : PIECE_NAME 0 2 3 2
  
  Note that only ONE axis was modified, no diagonal pieces are allowed
  
  The field is a 15x15 matrix, with 0-14 being valid values
  
  The program will verify the integrity of the file, in case of an error, the program will exit