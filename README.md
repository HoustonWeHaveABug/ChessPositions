# ChessPositions

## Problem solved by the program

Count the number of valid positions on a chessboard of given size (including the symmetric ones) such that:

- Both kings must be on the board (and there can only be one of each color)
- Not both kings can be in check

Depending on the options selected, the pawns may be placed on the first row for each color or not, and also promotion may be enabled or not.

Outside of kings, there may be any number of any kind/color of pieces on the board.

There is no initial starting position and it is not required that the board positions be obtainable by starting from some designated position and making legal moves.

## Program usage

The program takes 3 arguments on the command line: \<rows\> \<columns\> \<options\>
- \<rows\> must be greater than 1
- \<columns\> must be greater than 0
- \<options\> is the sum of none, some or all of the below flags:
	- 1 = pawns allowed on first row
	- 2 = pawns allowed on last row (no promotions)
	- 4 = color on move counts (positions where no kings are in chess will be counted twice)

## How does it work ?

The program loops on all the valid placement of both kings on the board and for each placement:
- Determines the number of pieces that can put each king in check from every squares
- Performs a search of all valid positions iterating recursively on the squares that can threat at least one king

There is a maximum of 4 choices for each of those squares:
- Select the black pieces that can put the white king in check
- Select the white pieces that can put the black king in check
- Leave the square empty
- Select the other pieces

The number of choices (hence the branching factor) can be further reduced in case one king is already in check:
- The pieces that can put this king in check can be merged with the "other pieces" choice
- The "empty square" choice can also be merged with the "other pieces" choice

A threatening square has a number of pieces associated with each choice that may change dynamically in case of a merge. This number is multiplied by the number of positions found so far before processing the next square. When all the squares are processed the current number of positions is added to the total number of solutions for the current search.

Before starting the search, the threatening squares are sorted by their proximity with the kings (this improves the search performance dramatically).

The number of valid positions for the non-threatening squares is computed once per search (as only one constant choice is associated with each of them). It is multiplied by the total number of valid positions found at the end of the search.

Each search result is stored in memory and replicated to all the similar placements (by symmetry). A search will be skipped if the number of solutions was already computed for a similar one. The final count is the sum of all stored search results.

## History & Results

The development was initially done in Ruby for a [Reddit Daily Programmer challenge](https://www.reddit.com/r/dailyprogrammer/comments/6yu31a/20170908_challenge_330_hard_minichess_positions), and now a version written in C is also available. Both versions implement the same algorithm. The C program is much faster as shown in [chess_positions_results.txt](https://github.com/HoustonWeHaveABug/ChessPositions/blob/master/chess_positions_results.txt).
