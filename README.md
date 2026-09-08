# ChessPositions

Search the number of valid positions on a chessboard such that:

- Both kings must be on the board (and there can only be one of each color)
- Not both kings can be in check

Depending on the options selected, the pawns may be placed on the first row for each color or not, and also promotion may be enabled or not.

Outside of kings, there may be any number of any kind/color of pieces on the board.

There is no initial starting position and it is not required that the board positions be obtainable by starting from some designated position and making legal moves.

The program takes 3 arguments on the command line: \<rows\> \<columns\> \<options\>
- \<rows\> must be greater than 1
- \<columns\> must be greater than 0
- \<options\> is the sum of none, some or all of the below flags:
	- 1 = pawns allowed on first row
	- 2 = pawns allowed on last row (no promotions)
	- 4 = color on move counts (positions where no kings are in chess will be counted twice)

The program loops on all possible placements of both kings on the chessboard and for each placement:

- Determines the number of pieces that can put each king in check from every squares
- Performs a search of all valid positions iterating on the squares that can threat at least one king
- There is a maximum of 4 choices for each threatening square:
    - Select the black pieces that can put the white king in check
    - Select the white pieces that can put the black king in check
    - Leave the square empty
    - Select the other pieces
- The number of choices (hence the branching factor) can be further reduced in many cases. For example when one king is already in check, the pieces that can put this king in check can be merged with the "other" pieces.
- The order in which the squares are processed also impacts the performance dramatically.

The development was initially done in Ruby, and now a version written in C is also available. Both versions implement the same algorithm. The C program is much faster as shown in [chess_positions_results.txt](https://github.com/HoustonWeHaveABug/ChessPositions/blob/master/chess_positions_results.txt).
