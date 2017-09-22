# ChessPositions

Search the number of valid positions on a chessboard such that:

- Both kings must be on the board (and there can only be one of each color)
- Not both kings can be in check

The pawns may be placed anywhere on the board (including first and last rows).

The program takes 2 arguments:

- Number of rows (> 0)
- Number of columns (> 0)
    
The product of rows and columns must be greater than 1.

The program loops on all possible placements of both kings on the chessboard and for each placement:

- Determines the number of pieces that can put each king in check from every squares
- Performs a search of all valid positions iterating on the squares that can threat at least one king
