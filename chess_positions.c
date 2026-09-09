#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <gmp.h>

#define ROYAL_MOVES_N 8
#define ROOK_MOVES_N 4
#define BISHOP_MOVES_N 4
#define KNIGHT_MOVES_N 8
#define PAWN_MOVES_N 2
#define MOVES_N 17
#define OFFICERS_N 4
#define OTHERS_MAX 9UL
#define PIECES_N 18
#define B_KING 1
#define W_QUEEN 2
#define B_QUEEN 3
#define W_ROOK 4
#define B_ROOK 5
#define W_BISHOP 6
#define B_BISHOP 7
#define W_KNIGHT 8
#define B_KNIGHT 9
#define W_PAWN 10
#define B_PAWN 11
#define PIECE_OUTSIDE 12
#define PIECE_UNDEFINED 13
#define PIECE_EMPTY 14
#define W_THREAT 15
#define B_THREAT 16
#define PIECE_OTHERS 17
#define COLOR_W 0
#define COLOR_B 1
#define COLORS_N 2
#define PAWNS_FIRST_ROW 1
#define PAWNS_LAST_ROW 2
#define COLOR_COUNTS 4
#define ALL_OPTIONS PAWNS_FIRST_ROW+PAWNS_LAST_ROW+COLOR_COUNTS
#define MEM_OFFSET 2

typedef struct{
	int moves_n;
	int moves[ROYAL_MOVES_N];
	int repeat_move;
}
piece_t;

typedef struct {
	int idx;
	int move_idx;
	int step;
	unsigned long count;
}
state_t;

typedef struct square_s square_t;

struct square_s {
	int row;
	int column;
	int idx;
	piece_t *piece;
	unsigned long others_max;
	state_t states[COLORS_N];
	square_t *h_mirror;
	square_t *v_mirror;
	square_t *opposite;
};

typedef struct {
	int officers[OFFICERS_N];
	piece_t *pawn;
	piece_t *threat_piece;
	square_t *king_square;
	int pawn_states;
	int last_steps[MOVES_N];
	int in_check;
}
color_t;

typedef struct {
	square_t *square;
	int step_min;
	int step_max;
	int move_idx_min;
	int move_idx_max;
	int last_steps[COLORS_N];
	int in_checks[COLORS_N];
	unsigned long others;
	mpz_t positions;
}
threat_t;

static void usage(void);
static void set_row(int, piece_t *, unsigned long);
static void set_side(int, int, int);
static int square_idx(int, int);
static void set_piece(piece_t *, int, int [], int);
static void init_state(state_t *, int);
static void reset_state(state_t *);
static void set_state(state_t *, int, int);
static void update_state_less(state_t *);
static void update_state_more(state_t *);
static int potential_check(const state_t *);
static int influent_step(const state_t *);
static int more_influent_step(const state_t *);
static void set_square(square_t *, int, int, int, piece_t *, unsigned long);
static void reset_states(square_t *);
static void set_color(color_t *, int [], piece_t *, piece_t *);
static void reset_color(color_t *);
static void set_threat(threat_t *, square_t *);
static void set_min_max(int, int, int *, int *);
static int compare_threats(const void *, const void *);
static void set_king_square(square_t *, piece_t *, color_t *);
static int search_w_king(square_t *);
static void count_positions(square_t *, square_t *);
static void set_color_states(square_t *, int, int);
static void set_piece_states(piece_t *, square_t *, int);
static void check_square_threats(square_t *);
static void search_positions(threat_t *);
static void choose_threat_piece(threat_t *, state_t *);
static void search_color_threat(color_t *, int);
static void set_cache(square_t *, square_t *);
static void output_result(mpz_t);
static void free_data(int);

static int rows_n, columns_n, options, mem_columns_n, all_moves[MOVES_N], mem_squares_n, squares_n, threats_n;
static piece_t all_pieces[PIECES_N];
static square_t *mem_squares, **squares;
static color_t colors[COLORS_N];
static threat_t *threats;
static mpz_t **cache, all_positions, factor;

int main(int argc, char *argv[]) {
	int mem_rows_n, royal_moves[ROYAL_MOVES_N] = { 1, 2, 3, 4, 5, 6, 7, 8 }, rook_moves[ROOK_MOVES_N] = { 1, 3, 5, 7 }, bishop_moves[BISHOP_MOVES_N] = { 2, 4, 6, 8 }, knight_moves[KNIGHT_MOVES_N] = { 9, 10, 11, 12, 13, 14, 15, 16 }, w_pawn_moves[PAWN_MOVES_N] = { 2, 4 }, b_pawn_moves[PAWN_MOVES_N] = { 6, 8 }, w_pieces[OFFICERS_N] = { W_QUEEN, W_ROOK, W_BISHOP, W_KNIGHT }, b_pieces[OFFICERS_N] = { B_QUEEN, B_ROOK, B_BISHOP, B_KNIGHT }, i;
	unsigned long others_max;
	mpz_t positions_sum;
	if (argc != 4) {
		usage();
		return EXIT_FAILURE;
	}
	rows_n = atoi(argv[1]);
	columns_n = atoi(argv[2]);
	options = atoi(argv[3]);
	if (rows_n < COLORS_N || columns_n < 1 || options < 0 || options > ALL_OPTIONS) {
		usage();
		return EXIT_FAILURE;
	}
	mem_rows_n = MEM_OFFSET+rows_n+MEM_OFFSET;
	mem_columns_n = MEM_OFFSET+columns_n+MEM_OFFSET;
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
	set_piece(all_pieces, ROYAL_MOVES_N, royal_moves, 0);
	set_piece(all_pieces+B_KING, ROYAL_MOVES_N, royal_moves, 0);
	set_piece(all_pieces+W_QUEEN, ROYAL_MOVES_N, royal_moves, 1);
	set_piece(all_pieces+B_QUEEN, ROYAL_MOVES_N, royal_moves, 1);
	set_piece(all_pieces+W_ROOK, ROOK_MOVES_N, rook_moves, 1);
	set_piece(all_pieces+B_ROOK, ROOK_MOVES_N, rook_moves, 1);
	set_piece(all_pieces+W_BISHOP, BISHOP_MOVES_N, bishop_moves, 1);
	set_piece(all_pieces+B_BISHOP, BISHOP_MOVES_N, bishop_moves, 1);
	set_piece(all_pieces+W_KNIGHT, KNIGHT_MOVES_N, knight_moves, 0);
	set_piece(all_pieces+B_KNIGHT, KNIGHT_MOVES_N, knight_moves, 0);
	set_piece(all_pieces+W_PAWN, PAWN_MOVES_N, w_pawn_moves, 0);
	set_piece(all_pieces+B_PAWN, PAWN_MOVES_N, b_pawn_moves, 0);
	set_piece(all_pieces+PIECE_OUTSIDE, 0, NULL, 0);
	set_piece(all_pieces+PIECE_UNDEFINED, 0, NULL, 0);
	set_piece(all_pieces+PIECE_EMPTY, 0, NULL, 0);
	set_piece(all_pieces+W_THREAT, 0, NULL, 0);
	set_piece(all_pieces+B_THREAT, 0, NULL, 0);
	set_piece(all_pieces+PIECE_OTHERS, 0, NULL, 0);
	mem_squares_n = mem_rows_n*mem_columns_n;
	mem_squares = malloc(sizeof(square_t)*(size_t)mem_squares_n);
	if (!mem_squares) {
		fputs("Could not allocate memory for mem_squares\n", stderr);
		fflush(stderr);
		return EXIT_FAILURE;
	}
	for (i = 0; i < MEM_OFFSET; ++i) {
		set_row(i, all_pieces+PIECE_OUTSIDE, 0UL);
	}
	others_max = OTHERS_MAX;
	if ((options & PAWNS_FIRST_ROW) == PAWNS_FIRST_ROW) {
		++others_max;
	}
	if ((options & PAWNS_LAST_ROW) == PAWNS_LAST_ROW) {
		++others_max;
	}
	set_row(MEM_OFFSET, all_pieces+PIECE_UNDEFINED, others_max);
	for (i = MEM_OFFSET+1; i < MEM_OFFSET+rows_n-1; ++i) {
		set_row(i, all_pieces+PIECE_UNDEFINED, OTHERS_MAX+COLORS_N);
	}
	set_row(MEM_OFFSET+rows_n-1, all_pieces+PIECE_UNDEFINED, others_max);
	for (i = MEM_OFFSET+rows_n; i < mem_rows_n; ++i) {
		set_row(i, all_pieces+PIECE_OUTSIDE, 0UL);
	}
	squares_n = rows_n*columns_n;
	squares = malloc(sizeof(square_t *)*(size_t)squares_n);
	if (!squares) {
		fputs("Could not allocate memory for squares\n", stderr);
		fflush(stderr);
		free(mem_squares);
		return EXIT_FAILURE;
	}
	squares_n = 0;
	for (i = MEM_OFFSET; i < MEM_OFFSET+rows_n; ++i) {
		int j;
		for (j = MEM_OFFSET; j < MEM_OFFSET+columns_n; ++j) {
			squares[squares_n] = mem_squares+square_idx(i, j);
			squares[squares_n]->h_mirror = mem_squares+square_idx(i, mem_columns_n-j-1);
			squares[squares_n]->v_mirror = mem_squares+square_idx(mem_rows_n-i-1, j);
			squares[squares_n++]->opposite = mem_squares+square_idx(mem_rows_n-i-1, mem_columns_n-j-1);
		}
	}
	set_color(colors, w_pieces, all_pieces+W_PAWN, all_pieces+B_THREAT);
	set_color(colors+COLOR_B, b_pieces, all_pieces+B_PAWN, all_pieces+W_THREAT);
	threats = malloc(sizeof(threat_t)*(size_t)squares_n);
	if (!threats) {
		fputs("Could not allocate memory for threats\n", stderr);
		fflush(stderr);
		free(squares);
		free(mem_squares);
		return EXIT_FAILURE;
	}
	cache = malloc(sizeof(mpz_t *)*(size_t)mem_squares_n);
	if (!cache) {
		fputs("Could not allocate memory for cache\n", stderr);
		fflush(stderr);
		free(threats);
		free(squares);
		free(mem_squares);
		return EXIT_FAILURE;
	}
	for (i = 0; i < mem_squares_n; ++i) {
		int j;
		cache[i] = calloc((size_t)mem_squares_n, sizeof(mpz_t));
		if (!cache[i]) {
			fprintf(stderr, "Could not allocate memory for cache[%d]\n", i);
			fflush(stderr);
			free_data(i);
			return EXIT_FAILURE;
		}
		for (j = 0; j < mem_squares_n; ++j) {
			mpz_init(cache[i][j]);
		}
	}
	for (i = 0; i < squares_n; ++i) {
		int j;
		set_king_square(squares[i], all_pieces, colors);
		for (j = 0; j < squares_n; ++j) {
			if (mpz_cmp_ui(cache[squares[i]->idx][squares[j]->idx], 0UL) > 0 || squares[j]->piece != all_pieces+PIECE_UNDEFINED || search_w_king(squares[j])) {
				continue;
			}
			set_king_square(squares[j], all_pieces+B_KING, colors+COLOR_B);
			count_positions(squares[i], squares[j]);
			squares[j]->piece = all_pieces+PIECE_UNDEFINED;
		}
		squares[i]->piece = all_pieces+PIECE_UNDEFINED;
	}
	mpz_init(positions_sum);
	for (i = 0; i < mem_squares_n; ++i) {
		int j;
		for (j = 0; j < mem_squares_n; ++j) {
			mpz_add(positions_sum, positions_sum, cache[i][j]);
		}
	}
	printf("Total ");
	output_result(positions_sum);
	free_data(mem_squares_n);
	return EXIT_SUCCESS;
}

static void set_piece(piece_t *piece, int moves_n, int moves[], int repeat_move) {
	int i;
	piece->moves_n = moves_n;
	for (i = 0; i < moves_n; ++i) {
		piece->moves[i] = moves[i];
	}
	piece->repeat_move = repeat_move;
}

static void init_state(state_t *state, int idx) {
	state->idx = idx;
	reset_state(state);
}

static void reset_state(state_t *state) {
	state->move_idx = 0;
	state->step = 0;
	state->count = 0UL;
}

static void set_state(state_t *state, int move_idx, int step) {
	state->move_idx = move_idx;
	state->step = step;
	++state->count;
}

static void update_state_less(state_t *state) {
	if (!influent_step(state)) {
		colors[state->idx].last_steps[state->move_idx] = state->step;
	}
}

static void update_state_more(state_t *state) {
	if (more_influent_step(state)) {
		colors[state->idx].last_steps[state->move_idx] = state->step;
	}
}

static int potential_check(const state_t *state) {
	return influent_step(state) && state->count && !colors[state->idx].in_check;
}

static int influent_step(const state_t *state) {
	return state->step <= colors[state->idx].last_steps[state->move_idx];
}

static int more_influent_step(const state_t *state) {
	return state->step < colors[state->idx].last_steps[state->move_idx];
}

static void set_square(square_t *square, int row, int column, int idx, piece_t *piece, unsigned long others_max) {
	square->row = row;
	square->column = column;
	square->idx = idx;
	square->piece = piece;
	square->others_max = others_max;
	init_state(square->states, COLOR_W);
	init_state(square->states+COLOR_B, COLOR_B);
}

static void reset_states(square_t *square) {
	reset_state(square->states);
	reset_state(square->states+COLOR_B);
}

static void set_color(color_t *color, int officers[], piece_t *pawn, piece_t *threat_piece) {
	int i;
	for (i = 0; i < OFFICERS_N; ++i) {
		color->officers[i] = officers[i];
	}
	color->pawn = pawn;
	color->threat_piece = threat_piece;
}

static void reset_color(color_t *color) {
	int i;
	for (i = 0; i < MOVES_N; ++i) {
		color->last_steps[i] = 0;
	}
	color->in_check = 0;
}

static void set_threat(threat_t *threat, square_t *square) {
	threat->square = square;
	if (square) {
		set_min_max(threat->square->states[COLOR_W].step, threat->square->states[COLOR_B].step, &threat->step_min, &threat->step_max);
		set_min_max(threat->square->states[COLOR_W].move_idx, threat->square->states[COLOR_B].move_idx, &threat->move_idx_min, &threat->move_idx_max);
	}
	mpz_init(threat->positions);
}

static void set_min_max(int w_val, int b_val, int *min, int *max) {
	if (w_val < b_val) {
		*min = w_val;
		*max = b_val;
	}
	else {
		*min = b_val;
		*max = w_val;
	}
	if (*min == 0) {
		*min = *max;
	}
}

static int compare_threats(const void *a, const void *b) {
	const threat_t *threat_a = (const threat_t *)a, *threat_b = (const threat_t *)b;
	if (threat_a->step_min != threat_b->step_min) {
		return threat_a->step_min-threat_b->step_min;
	}
	if (threat_a->step_max != threat_b->step_max) {
		return threat_a->step_max-threat_b->step_max;
	}
	if (threat_a->move_idx_min != threat_b->move_idx_min) {
		return threat_a->move_idx_min-threat_b->move_idx_min;
	}
	return threat_a->move_idx_max-threat_b->move_idx_max;
}

static void usage(void) {
	fputs("Program arguments: <rows> <columns> <options>\n", stderr);
	fprintf(stderr, "<rows> must be greater than or equal to %d\n", COLORS_N);
	fputs("<columns> must be greater than 0\n", stderr);
	fputs("<options> is the sum of the below flags:\n", stderr);
	fprintf(stderr, "%d = pawns allowed on first row\n", PAWNS_FIRST_ROW);
	fprintf(stderr, "%d = pawns allowed on last row (no promotions)\n", PAWNS_LAST_ROW);
	fprintf(stderr, "%d = color on move counts (positions where no kings are in chess will be counted twice)\n", COLOR_COUNTS);
	fflush(stderr);
}

static void set_row(int row, piece_t *piece, unsigned long others_max) {
	int i;
	set_side(row, 0, MEM_OFFSET);
	for (i = MEM_OFFSET; i < MEM_OFFSET+columns_n; ++i) {
		set_square(mem_squares+square_idx(row, i), row, i, square_idx(row, i), piece, others_max);
	}
	set_side(row, MEM_OFFSET+columns_n, mem_columns_n);
}

static void set_side(int row, int column_a, int column_b) {
	int i;
	for (i = column_a; i < column_b; ++i) {
		set_square(mem_squares+square_idx(row, i), row, i, square_idx(row, i), all_pieces+PIECE_OUTSIDE, 0UL);
	}
}

static int square_idx(int row, int column) {
	return row*mem_columns_n+column;
}

static void set_king_square(square_t *square, piece_t *piece, color_t *color) {
	square->piece = piece;
	color->king_square = square;
}

static int search_w_king(square_t *square) {
	int i;
	for (i = 0; i < all_pieces[B_KING].moves_n; ++i) {
		if (mem_squares[square->idx-all_moves[all_pieces[B_KING].moves[i]]].piece == all_pieces) {
			return 1;
		}
	}
	return 0;
}

static void count_positions(square_t *w_square, square_t *b_square) {
	int i;
	for (i = 0; i < squares_n; ++i) {
		reset_states(squares[i]);
	}
	set_color_states(w_square, COLOR_W, MEM_OFFSET+1);
	set_color_states(b_square, COLOR_B, MEM_OFFSET+rows_n-2);
	for (i = 0; i < COLORS_N; ++i) {
		reset_color(colors+i);
	}
	mpz_init(all_positions);
	mpz_init(factor);
	mpz_add_ui(factor, factor, 1UL);
	set_threat(threats, NULL);
	mpz_add_ui(threats->positions, threats->positions, 1UL);
	threats_n = 1;
	for (i = 0; i < squares_n; ++i) {
		if (squares[i]->piece == all_pieces+PIECE_UNDEFINED) {
			check_square_threats(squares[i]);
		}
	}
	qsort(threats+1, (size_t)(threats_n-1), sizeof(threat_t), compare_threats);
	search_positions(threats+1);
	for (i = 0; i < threats_n; ++i) {
		mpz_clear(threats[i].positions);
	}
	mpz_mul(all_positions, all_positions, factor);
	mpz_clear(factor);
	set_cache(w_square, b_square);
	if (colors[COLOR_W].pawn_states == colors[COLOR_B].pawn_states) {
		if (b_square->h_mirror->column < b_square->column) {
			set_cache(b_square->opposite, w_square->opposite);
		}
		else {
			set_cache(b_square->v_mirror, w_square->v_mirror);
		}
	}
	printf("Position %d/%d ", w_square->idx, b_square->idx);
	output_result(all_positions);
}

static void set_color_states(square_t *square, int color_idx, int row) {
	int i;
	for (i = 0; i < OFFICERS_N; ++i) {
		set_piece_states(all_pieces+colors[color_idx].officers[i], square, color_idx);
	}
	colors[color_idx].pawn_states = (options & PAWNS_FIRST_ROW) == PAWNS_FIRST_ROW || square->row != row;
	if (colors[color_idx].pawn_states) {
		set_piece_states(colors[color_idx].pawn, square, color_idx);
	}
}

static void set_piece_states(piece_t *piece, square_t *square, int color_idx) {
	int i;
	if (piece->repeat_move) {
		for (i = 0; i < piece->moves_n; ++i) {
			int target_idx, step;
			for (target_idx = square->idx-all_moves[piece->moves[i]], step = 1; mem_squares[target_idx].piece == all_pieces+PIECE_UNDEFINED; target_idx -= all_moves[piece->moves[i]], ++step) {
				set_state(mem_squares[target_idx].states+color_idx, piece->moves[i], step);
			}
		}
	}
	else {
		for (i = 0; i < piece->moves_n; ++i) {
			int target_idx = square->idx-all_moves[piece->moves[i]];
			if (mem_squares[target_idx].piece == all_pieces+PIECE_UNDEFINED) {
				set_state(mem_squares[target_idx].states+color_idx, piece->moves[i], 1);
			}
		}
	}
}

static void check_square_threats(square_t *square) {
	if (square->states[COLOR_W].count || square->states[COLOR_B].count) {
		int i;
		for (i = 0; i < COLORS_N; ++i) {
			update_state_less(square->states+i);
		}
		set_threat(threats+threats_n, square);
		++threats_n;
	}
	else {
		mpz_mul_ui(factor, factor, square->others_max);
	}
}

static void search_positions(threat_t *threat) {
	if (colors[COLOR_W].in_check && colors[COLOR_B].in_check) {
		return;
	}
	if (threat < threats+threats_n) {
		int i;
		threat->others = threat->square->others_max;
		for (i = 0; i < COLORS_N; ++i) {
			if (potential_check(threat->square->states+i)) {
				choose_threat_piece(threat, threat->square->states+i);
			}
		}
		if (more_influent_step(threat->square->states) || more_influent_step(threat->square->states+COLOR_B)) {
			threat->square->piece = all_pieces+PIECE_EMPTY;
			for (i = 0; i < COLORS_N; ++i) {
				threat->in_checks[i] = colors[i].in_check;
			}
			for (i = 0; i < COLORS_N; ++i) {
				if (potential_check(threat->square->states+i)) {
					search_color_threat(colors+i, threat->square->states[i].move_idx);
				}
			}
			mpz_set(threat->positions, (threat-1)->positions);
			search_positions(threat+1);
			for (i = 0; i < COLORS_N; ++i) {
				colors[i].in_check = threat->in_checks[i];
			}
			threat->square->piece = all_pieces+PIECE_UNDEFINED;
			--threat->others;
		}
		threat->square->piece = all_pieces+PIECE_OTHERS;
		for (i = 0; i < COLORS_N; ++i) {
			threat->last_steps[i] = colors[i].last_steps[threat->square->states[i].move_idx];
		}
		for (i = 0; i < COLORS_N; ++i) {
			update_state_more(threat->square->states+i);
		}
		mpz_mul_ui(threat->positions, (threat-1)->positions, threat->others);
		search_positions(threat+1);
		for (i = 0; i < COLORS_N; ++i) {
			colors[i].last_steps[threat->square->states[i].move_idx] = threat->last_steps[i];
		}
		threat->square->piece = all_pieces+PIECE_UNDEFINED;
	}
	else {
		mpz_add(all_positions, all_positions, (threat-1)->positions);
		if ((options & COLOR_COUNTS) == COLOR_COUNTS && !colors[COLOR_W].in_check && !colors[COLOR_B].in_check) {
			mpz_add(all_positions, all_positions, (threat-1)->positions);
		}
	}
}

static void choose_threat_piece(threat_t *threat, state_t *state) {
	threat->square->piece = colors[state->idx].threat_piece;
	threat->last_steps[state->idx] = colors[state->idx].last_steps[state->move_idx];
	threat->in_checks[state->idx] = colors[state->idx].in_check;
	update_state_more(state);
	search_color_threat(colors+state->idx, state->move_idx);
	mpz_mul_ui(threat->positions, (threat-1)->positions, state->count);
	search_positions(threat+1);
	colors[state->idx].in_check = threat->in_checks[state->idx];
	colors[state->idx].last_steps[state->move_idx] = threat->last_steps[state->idx];
	threat->square->piece = all_pieces+PIECE_UNDEFINED;
	threat->others -= state->count;
}

static void search_color_threat(color_t *color, int move_idx) {
	int target_idx;
	for (target_idx = color->king_square->idx-all_moves[move_idx]; mem_squares[target_idx].piece == all_pieces+PIECE_EMPTY; target_idx -= all_moves[move_idx]);
	color->in_check = mem_squares[target_idx].piece == color->threat_piece;
}

static void set_cache(square_t *w_square, square_t *b_square) {
	mpz_set(cache[w_square->idx][b_square->idx], all_positions);
	mpz_set(cache[w_square->h_mirror->idx][b_square->h_mirror->idx], all_positions);
}

static void output_result(mpz_t result) {
	mpz_out_str(stdout, 10, result);
	puts("");
	fflush(stdout);
	mpz_clear(result);
}

static void free_data(int cache_size) {
	int i;
	for (i = 0; i < cache_size; ++i) {
		int j;
		for (j = 0; j < mem_squares_n; ++j) {
			mpz_clear(cache[i][j]);
		}
		free(cache[i]);
	}
	free(cache);
	free(threats);
	free(squares);
	free(mem_squares);
}
