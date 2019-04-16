# ChessPositions

Search the number of valid positions on a chessboard such that:

- Both kings must be on the board (and there can only be one of each color)
- Not both kings can be in check

The pawns may be placed on the first row for each color, and also on the last row if promotion is not enabled for them.

Outside of kings, there may be any number of any kind/color of pieces on the board.

There is no initial starting position and it is not required that the board positions be obtainable by starting from some designated position and making legal moves.

The program takes 3 arguments on the command line:

- Number of rows (> 1)
- Number of columns (> 0)
- Promote pawns (1: Yes, 2: No)

The program loops on all possible placements of both kings on the chessboard and for each placement:

- Determines the number of pieces that can put each king in check from every squares
- Performs a search of all valid positions iterating on the squares that can threat at least one king

Valid positions where no king is in check are counted twice, since either white or black can be on move for each one.
