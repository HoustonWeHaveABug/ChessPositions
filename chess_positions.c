#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef struct{
	int symbol;
	int moves_n;
	int moves[8];
	int repeat_move;
}
piece_t;

typedef struct {
	int idx;
	int move_idx;
	int step;
	int count;
}
square_state_t;

typedef struct square_s square_t;

struct square_s {
	int row;
	int column;
	int idx;
	piece_t *piece;
	int others_max;
	square_t *h_mirror;
	square_t *v_mirror;
	square_t *opposite;
	square_state_t states[2];
};

typedef struct {
	int pieces[4];
	piece_t *pawn;
	piece_t *threat_piece;
	square_t *king_square;
	int pawn_states;
	int last_steps[17];
	int in_check;
}
color_t;

typedef struct {
	square_t *square;
	int last_steps[2];
	int in_checks[2];
	int others;
}
threat_t;

typedef struct {
	int m;
	int *p;
}
mp_t;

static void usage(void);
static void set_row(int, piece_t *, int);
static void set_side(int, int, int);
static int square_idx(int, int);
static void set_piece(piece_t *, int, int, int [], int);
static void set_square_state(square_state_t *, int);
static void reset_square_state(square_state_t *);
static void update_state_less(square_state_t *);
static void update_state_more(square_state_t *);
static int potential_check(const square_state_t *);
static int influent_step(const square_state_t *);
static int more_influent_step(const square_state_t *);
static void set_square(square_t *, int, int, int, piece_t *, int);
static void reset_square_states(square_t *);
static void output_square(const square_t *);
static void set_color(color_t *, int [], piece_t *, piece_t *);
static void reset_color(color_t *);
static void set_threat(threat_t *, square_t *);
static void save_threat_piece(threat_t *, square_state_t *);
static void restore_threat_piece(threat_t *, square_state_t *);
static int mp_new(mp_t *, int);
static int *p_new(int);
static void mp_free(mp_t *);

static int columns_n, options, mem_columns_n, all_moves[17], p_shrt_max, p_long_max, p_digits;
static piece_t all_pieces[18];
static square_t *mem_squares;
static color_t colors[2];

int main(int argc, char *argv[]) {
	int rows_n, mem_rows_n, royal_moves[8] = { 1, 2, 3, 4, 5, 6, 7, 8 }, rook_moves[4] = { 1, 3, 5, 7 }, bishop_moves[4] = { 2, 4, 6, 8 }, knight_moves[8] = { 9, 10, 11, 12, 13, 14, 15, 16 }, white_pawn_moves[2] = { 2, 4 }, black_pawn_moves[2] = { 6, 8 }, mem_squares_n, others_max, squares_n, white_pieces[4] = { 2, 4, 6, 8 }, black_pieces[4] = { 3, 5, 7, 9 }, i, j;
	square_t **squares;
	threat_t *threats;
	if (argc != 4) {
		usage();
		return EXIT_FAILURE;
	}
	rows_n = atoi(argv[1]);
	columns_n = atoi(argv[2]);
	options = atoi(argv[3]);
	if (rows_n < 2 || columns_n < 1 || options < 0 || options > 7) {
		usage();
		return EXIT_FAILURE;
	}
	mem_rows_n = rows_n+4;
	mem_columns_n = columns_n+4;
	all_moves[0] = 0;
	all_moves[1] = -1;
	all_moves[2] = -mem_columns_n-1;
	all_moves[3] = -mem_columns_n;
	all_moves[4] = -mem_columns_n+1;
	all_moves[5] = 1;
	all_moves[6] = mem_columns_n+1;
	all_moves[7] = mem_columns_n;
	all_moves[8] = mem_columns_n-1;
	all_moves[9] = -mem_columns_n-2;
	all_moves[10] = -mem_columns_n*2-1;
	all_moves[11] = -mem_columns_n*2+1;
	all_moves[12] = -mem_columns_n+2;
	all_moves[13] = mem_columns_n+2;
	all_moves[14] = mem_columns_n*2+1;
	all_moves[15] = mem_columns_n*2-1;
	all_moves[16] = mem_columns_n-2;
	set_piece(all_pieces, 'K', 8, royal_moves, 0);
	set_piece(all_pieces+1, 'k', 8, royal_moves, 0);
	set_piece(all_pieces+2, 'Q', 8, royal_moves, 1);
	set_piece(all_pieces+3, 'q', 8, royal_moves, 1);
	set_piece(all_pieces+4, 'R', 4, rook_moves, 1);
	set_piece(all_pieces+5, 'r', 4, rook_moves, 1);
	set_piece(all_pieces+6, 'B', 4, bishop_moves, 1);
	set_piece(all_pieces+7, 'b', 4, bishop_moves, 1);
	set_piece(all_pieces+8, 'N', 8, knight_moves, 0);
	set_piece(all_pieces+9, 'n', 8, knight_moves, 0);
	set_piece(all_pieces+10, 'P', 2, white_pawn_moves, 0);
	set_piece(all_pieces+11, 'p', 2, black_pawn_moves, 0);
	set_piece(all_pieces+12, '#', 0, NULL, 0);
	set_piece(all_pieces+13, '?', 0, NULL, 0);
	set_piece(all_pieces+14, '.', 0, NULL, 0);
	set_piece(all_pieces+15, 'T', 0, NULL, 0);
	set_piece(all_pieces+16, 't', 0, NULL, 0);
	set_piece(all_pieces+17, '*', 0, NULL, 0);
	mem_squares_n = mem_rows_n*mem_columns_n;
	mem_squares = malloc(sizeof(square_t)*(size_t)mem_squares_n);
	if (!mem_squares) {
		fputs("Could not allocate memory for mem_squares\n", stderr);
		fflush(stderr);
		return EXIT_FAILURE;
	}
	for (i = 0; i < 2; ++i) {
		set_row(i, all_pieces+12, 0);
	}
	others_max = 9;
	if ((options & 1) == 1) {
		++others_max;
	}
	if ((options & 2) == 2) {
		++others_max;
	}
	set_row(2, all_pieces+13, others_max);
	for (i = 3; i <= rows_n; ++i) {
		set_row(i, all_pieces+13, 11);
	}
	set_row(rows_n+1, all_pieces+13, others_max);
	for (i = rows_n+2; i < mem_rows_n; ++i) {
		set_row(i, all_pieces+12, 0);
	}
	squares_n = rows_n*columns_n;
	squares = malloc(sizeof(square_t *)*(size_t)squares_n);
	if (!squares) {
		fputs("Could not allocate memory for squares\n", stderr);
		fflush(stderr);
		free(mem_squares);
		return EXIT_FAILURE;
	}
	i = 0;
	for (j = 2; j < rows_n+2; ++j) {
		int k;
		for (k = 2; k < columns_n+2; ++k) {
			squares[i] = mem_squares+square_idx(j, k);
			squares[i]->h_mirror = mem_squares+square_idx(j, mem_columns_n-k-1);
			squares[i]->v_mirror = mem_squares+square_idx(mem_rows_n-j-1, k);
			squares[i++]->opposite = mem_squares+square_idx(mem_rows_n-j-1, mem_columns_n-k-1);
		}
	}
	set_color(colors, white_pieces, all_pieces+10, all_pieces+16);
	set_color(colors+1, black_pieces, all_pieces+11, all_pieces+15);
	threats = malloc(sizeof(threat_t)*(size_t)(squares_n-2));
	if (!threats) {
		fputs("Could not allocate memory for threats\n", stderr);
		fflush(stderr);
		free(squares);
		free(mem_squares);
		return EXIT_FAILURE;
	}
	for (p_shrt_max = 1, p_long_max = 1, p_digits = 0; p_long_max <= INT_MAX/100; p_shrt_max *= 10, p_long_max *= 100, ++p_digits);
	free(threats);
	free(squares);
	free(mem_squares);
	return EXIT_SUCCESS;
}

static void usage(void) {
	fputs("Program arguments: <rows> <columns> <options>\n", stderr);
	fputs("<rows> must be greater than 1\n", stderr);
	fputs("<columns> must be greater than 0\n", stderr);
	fputs("<options> is the sum of the below flags:\n", stderr);
	fputs("1 = pawns allowed on first row\n", stderr);
	fputs("2 = pawns allowed on last row (no promotions)\n", stderr);
	fputs("4 = color on move counts (positions where no kings are in chess will be counted twice)\n", stderr);
	fflush(stderr);
}

static void set_row(int row, piece_t *piece, int others_max) {
	int i;
	set_side(row, 0, 2);
	for (i = 2; i < columns_n+2; ++i) {
		set_square(mem_squares+square_idx(row, i), row, i, square_idx(row, i), piece, others_max);
	}
	set_side(row, columns_n+2, mem_columns_n);
}

static void set_side(int row, int column_a, int column_b) {
	int i;
	for (i = column_a; i < column_b; ++i) {
		set_square(mem_squares+square_idx(row, i), row, i, square_idx(row, i), all_pieces+12, 0);
	}
}

static int square_idx(int row, int column) {
	return row*mem_columns_n+column;
}

static void set_piece(piece_t *piece, int symbol, int moves_n, int moves[], int repeat_move) {
	int i;
	piece->symbol = symbol;
	piece->moves_n = moves_n;
	for (i = 0; i < moves_n; ++i) {
		piece->moves[i] = moves[i];
	}
	piece->repeat_move = repeat_move;
}

static void set_square_state(square_state_t *square_state, int idx) {
	square_state->idx = idx;
	reset_square_state(square_state);
}

static void reset_square_state(square_state_t *square_state) {
	square_state->move_idx = 0;
	square_state->step = 0;
	square_state->count = 0;
}

static void update_state_less(square_state_t *square_state) {
	if (!influent_step(square_state)) {
		colors[square_state->idx].last_steps[square_state->move_idx] = square_state->step;
	}
}

static void update_state_more(square_state_t *square_state) {
	if (more_influent_step(square_state)) {
		colors[square_state->idx].last_steps[square_state->move_idx] = square_state->step;
	}
}

static int potential_check(const square_state_t *square_state) {
	return influent_step(square_state) && square_state->count && !colors[square_state->idx].in_check;
}

static int influent_step(const square_state_t *square_state) {
	return square_state->step <= colors[square_state->idx].last_steps[square_state->move_idx];
}

static int more_influent_step(const square_state_t *square_state) {
	return square_state->step < colors[square_state->idx].last_steps[square_state->move_idx];
}

static void set_square(square_t *square, int row, int column, int idx, piece_t *piece, int others_max) {
	square->row = row;
	square->column = column;
	square->idx = idx;
	square->piece = piece;
	square->others_max = others_max;
	square->h_mirror = NULL;
	square->v_mirror = NULL;
	square->opposite = NULL;
	set_square_state(square->states, 0);
	set_square_state(square->states+1, 1);
}

static void reset_square_states(square_t *square) {
	reset_square_state(square->states);
	reset_square_state(square->states+1);
}

static void output_square(const square_t *square) {
	putchar(square->piece->symbol);
}

static void set_color(color_t *color, int pieces[], piece_t *pawn, piece_t *threat_piece) {
	int i;
	for (i = 0; i < 4; ++i) {
		color->pieces[i] = pieces[i];
	}
	color->pawn = pawn;
	color->threat_piece = threat_piece;
	color->king_square = NULL;
}

static void reset_color(color_t *color) {
	int i;
	for (i = 0; i < 17; ++i) {
		color->last_steps[i] = 0;
	}
	color->in_check = 0;
}

static void set_threat(threat_t *threat, square_t *square) {
	threat->square = square;
}

static void save_threat_piece(threat_t *threat, square_state_t *square_state) {
	threat->square->piece = colors[square_state->idx].threat_piece;
	threat->last_steps[square_state->idx] = colors[square_state->idx].last_steps[square_state->move_idx];
	threat->in_checks[square_state->idx] = colors[square_state->idx].in_check;
}

static void restore_threat_piece(threat_t *threat, square_state_t *square_state) {
	colors[square_state->idx].in_check = threat->in_checks[square_state->idx];
	colors[square_state->idx].last_steps[square_state->move_idx] = threat->last_steps[square_state->idx];
	threat->square->piece = all_pieces+13;
}

static int mp_new(mp_t *mp, int val) {
	if (val < 0 || val >= p_long_max) {
		fputs("mp_new: initial value is out of range\n", stderr);
		fflush(stderr);
		return 0;
	}
	mp->p = p_new(1);
	if (mp->p) {
		mp->m = 1;
		mp->p[0] = val;
		return 1;
	}
	mp->m = 0;
	return 0;
}

static int *p_new(int m) {
	int *p;
	p = calloc((size_t)m, sizeof(int));
	if (!p) {
		fputs("p_new: could not allocate memory for p\n", stderr);
		fflush(stderr);
		return NULL;
	}
	return p;
}

static void mp_free(mp_t *mp) {
	free(mp->p);
}
