#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define ROYAL_MOVES_N 8
#define ROOK_MOVES_N 4
#define BISHOP_MOVES_N 4
#define KNIGHT_MOVES_N 8
#define PAWN_MOVES_N 2
#define MOVES_N 17
#define OFFICERS_N 4
#define OTHERS_MAX 9
#define PIECES_N 18
#define BLACK_KING 1
#define WHITE_QUEEN 2
#define BLACK_QUEEN 3
#define WHITE_ROOK 4
#define BLACK_ROOK 5
#define WHITE_BISHOP 6
#define BLACK_BISHOP 7
#define WHITE_KNIGHT 8
#define BLACK_KNIGHT 9
#define WHITE_PAWN 10
#define BLACK_PAWN 11
#define PIECE_OUTSIDE 12
#define PIECE_UNDEFINED 13
#define PIECE_EMPTY 14
#define WHITE_THREAT 15
#define BLACK_THREAT 16
#define PIECE_OTHERS 17
#define COLOR_WHITE 0
#define COLOR_BLACK 1
#define COLORS_N 2
#define PAWNS_FIRST_ROW 1
#define PAWNS_LAST_ROW 2
#define COLOR_COUNTS 4
#define ALL_OPTIONS PAWNS_FIRST_ROW+PAWNS_LAST_ROW+COLOR_COUNTS
#define MEM_OFFSET 2
#define P_MUL 10

typedef struct{
	int symbol;
	int moves_n;
	int moves[ROYAL_MOVES_N];
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
	square_state_t states[COLORS_N];
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
	int last_steps[COLORS_N];
	int in_checks[COLORS_N];
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
static void init_square_state(square_state_t *, int);
static void reset_square_state(square_state_t *);
static void set_square_state(square_state_t *, int, int);
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
static void save_empty(threat_t *);
static void restore_empty(threat_t *);
static void save_others(threat_t *);
static void restore_others(threat_t *);
static int mp_new(mp_t *, int);
static int *p_new(int);
static int mp_mul_val(mp_t *, int);
static int mp_mul(mp_t *, mp_t *);
static int mp_positive(const mp_t *);
static void mp_print(const mp_t *);
static void mp_free(mp_t *);
static void set_king_square(square_t *, piece_t *, color_t *);
static int search_white_king(square_t *);
static void count_positions(square_t *, square_t *);
static void set_color_states(square_t *, int, int);
static void set_piece_states(piece_t *, square_t *, int);
static void check_square_threats(square_t *);
static void search_positions(int, mp_t *);
static void set_cache(square_t *, square_t *);
static void free_cache(int);

static int rows_n, columns_n, options, mem_columns_n, all_moves[MOVES_N], mem_squares_n, squares_n, p_max, p_len, threats_n;
static piece_t all_pieces[PIECES_N];
static square_t *mem_squares, **squares;
static color_t colors[COLORS_N];
static threat_t *threats;
static mp_t **cache, positions_all, factor;

int main(int argc, char *argv[]) {
	int mem_rows_n, royal_moves[ROYAL_MOVES_N] = { 1, 2, 3, 4, 5, 6, 7, 8 }, rook_moves[ROOK_MOVES_N] = { 1, 3, 5, 7 }, bishop_moves[BISHOP_MOVES_N] = { 2, 4, 6, 8 }, knight_moves[KNIGHT_MOVES_N] = { 9, 10, 11, 12, 13, 14, 15, 16 }, white_pawn_moves[PAWN_MOVES_N] = { 2, 4 }, black_pawn_moves[PAWN_MOVES_N] = { 6, 8 }, others_max, white_pieces[OFFICERS_N] = { WHITE_QUEEN, WHITE_ROOK, WHITE_BISHOP, WHITE_KNIGHT }, black_pieces[OFFICERS_N] = { BLACK_QUEEN, BLACK_ROOK, BLACK_BISHOP, BLACK_KNIGHT }, i, k;
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
	set_piece(all_pieces, 'K', ROYAL_MOVES_N, royal_moves, 0);
	set_piece(all_pieces+BLACK_KING, 'k', ROYAL_MOVES_N, royal_moves, 0);
	set_piece(all_pieces+WHITE_QUEEN, 'Q', ROYAL_MOVES_N, royal_moves, 1);
	set_piece(all_pieces+BLACK_QUEEN, 'q', ROYAL_MOVES_N, royal_moves, 1);
	set_piece(all_pieces+WHITE_ROOK, 'R', ROOK_MOVES_N, rook_moves, 1);
	set_piece(all_pieces+BLACK_ROOK, 'r', ROOK_MOVES_N, rook_moves, 1);
	set_piece(all_pieces+WHITE_BISHOP, 'B', BISHOP_MOVES_N, bishop_moves, 1);
	set_piece(all_pieces+BLACK_BISHOP, 'b', BISHOP_MOVES_N, bishop_moves, 1);
	set_piece(all_pieces+WHITE_KNIGHT, 'N', KNIGHT_MOVES_N, knight_moves, 0);
	set_piece(all_pieces+BLACK_KNIGHT, 'n', KNIGHT_MOVES_N, knight_moves, 0);
	set_piece(all_pieces+WHITE_PAWN, 'P', PAWN_MOVES_N, white_pawn_moves, 0);
	set_piece(all_pieces+BLACK_PAWN, 'p', PAWN_MOVES_N, black_pawn_moves, 0);
	set_piece(all_pieces+PIECE_OUTSIDE, '#', 0, NULL, 0);
	set_piece(all_pieces+PIECE_UNDEFINED, '?', 0, NULL, 0);
	set_piece(all_pieces+PIECE_EMPTY, '.', 0, NULL, 0);
	set_piece(all_pieces+WHITE_THREAT, 'T', 0, NULL, 0);
	set_piece(all_pieces+BLACK_THREAT, 't', 0, NULL, 0);
	set_piece(all_pieces+PIECE_OTHERS, '*', 0, NULL, 0);
	mem_squares_n = mem_rows_n*mem_columns_n;
	mem_squares = malloc(sizeof(square_t)*(size_t)mem_squares_n);
	if (!mem_squares) {
		fputs("Could not allocate memory for mem_squares\n", stderr);
		fflush(stderr);
		return EXIT_FAILURE;
	}
	for (i = 0; i < MEM_OFFSET; ++i) {
		set_row(i, all_pieces+PIECE_OUTSIDE, 0);
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
		set_row(i, all_pieces+PIECE_OUTSIDE, 0);
	}
	squares_n = rows_n*columns_n;
	squares = malloc(sizeof(square_t *)*(size_t)squares_n);
	if (!squares) {
		fputs("Could not allocate memory for squares\n", stderr);
		fflush(stderr);
		free(mem_squares);
		return EXIT_FAILURE;
	}
	k = 0;
	for (i = MEM_OFFSET; i < MEM_OFFSET+rows_n; ++i) {
		int j;
		for (j = MEM_OFFSET; j < MEM_OFFSET+columns_n; ++j) {
			squares[k] = mem_squares+square_idx(i, j);
			squares[k]->h_mirror = mem_squares+square_idx(i, mem_columns_n-j-1);
			squares[k]->v_mirror = mem_squares+square_idx(mem_rows_n-i-1, j);
			squares[k++]->opposite = mem_squares+square_idx(mem_rows_n-i-1, mem_columns_n-j-1);
		}
	}
	set_color(colors, white_pieces, all_pieces+WHITE_PAWN, all_pieces+BLACK_THREAT);
	set_color(colors+COLOR_BLACK, black_pieces, all_pieces+BLACK_PAWN, all_pieces+WHITE_THREAT);
	threats = malloc(sizeof(threat_t)*(size_t)(squares_n-COLORS_N));
	if (!threats) {
		fputs("Could not allocate memory for threats\n", stderr);
		fflush(stderr);
		free(squares);
		free(mem_squares);
		return EXIT_FAILURE;
	}
	for (p_max = 1, p_len = 0; p_max <= SHRT_MAX/P_MUL; p_max *= P_MUL, ++p_len);
	--p_max;
	cache = malloc(sizeof(mp_t *)*(size_t)mem_squares_n);
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
		cache[i] = calloc((size_t)mem_squares_n, sizeof(mp_t));
		if (!cache[i]) {
			fprintf(stderr, "Could not allocate memory for cache[%d]\n", i);
			fflush(stderr);
			free_cache(i);
			free(threats);
			free(squares);
			free(mem_squares);
			return EXIT_FAILURE;
		}
		for (j = 0; j < mem_squares_n; ++j) {
			if (!mp_new(&cache[i][j], 0)) {
				free_cache(i);
				free(threats);
				free(squares);
				free(mem_squares);
				return EXIT_FAILURE;
			}
		}
	}
	for (i = 0; i < squares_n; ++i) {
		int j;
		set_king_square(squares[i], all_pieces, colors);
		for (j = 0; j < squares_n; ++j) {
			if (mp_positive(&cache[squares[i]->idx][squares[j]->idx]) || squares[j]->piece != all_pieces+PIECE_UNDEFINED || search_white_king(squares[j])) {
				continue;
			}
			set_king_square(squares[j], all_pieces+BLACK_KING, colors+COLOR_BLACK);
			count_positions(squares[i], squares[j]);
			squares[j]->piece = all_pieces+PIECE_UNDEFINED;
		}
		squares[i]->piece = all_pieces+PIECE_UNDEFINED;
	}
	free_cache(mem_squares_n);
	free(threats);
	free(squares);
	free(mem_squares);
	return EXIT_SUCCESS;
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

static void set_row(int row, piece_t *piece, int others_max) {
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
		set_square(mem_squares+square_idx(row, i), row, i, square_idx(row, i), all_pieces+PIECE_OUTSIDE, 0);
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

static void init_square_state(square_state_t *square_state, int idx) {
	square_state->idx = idx;
	reset_square_state(square_state);
}

static void reset_square_state(square_state_t *square_state) {
	square_state->move_idx = 0;
	square_state->step = 0;
	square_state->count = 0;
}

static void set_square_state(square_state_t *square_state, int move_idx, int step) {
	square_state->move_idx = move_idx;
	square_state->step = step;
	++square_state->count;
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
	init_square_state(square->states, COLOR_WHITE);
	init_square_state(square->states+COLOR_BLACK, COLOR_BLACK);
}

static void reset_square_states(square_t *square) {
	reset_square_state(square->states);
	reset_square_state(square->states+COLOR_BLACK);
}

static void output_square(const square_t *square) {
	putchar(square->piece->symbol);
}

static void set_color(color_t *color, int officers[], piece_t *pawn, piece_t *threat_piece) {
	int i;
	for (i = 0; i < OFFICERS_N; ++i) {
		color->officers[i] = officers[i];
	}
	color->pawn = pawn;
	color->threat_piece = threat_piece;
	color->king_square = NULL;
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
}

static void save_threat_piece(threat_t *threat, square_state_t *square_state) {
	threat->square->piece = colors[square_state->idx].threat_piece;
	threat->last_steps[square_state->idx] = colors[square_state->idx].last_steps[square_state->move_idx];
	threat->in_checks[square_state->idx] = colors[square_state->idx].in_check;
}

static void restore_threat_piece(threat_t *threat, square_state_t *square_state) {
	colors[square_state->idx].in_check = threat->in_checks[square_state->idx];
	colors[square_state->idx].last_steps[square_state->move_idx] = threat->last_steps[square_state->idx];
	threat->square->piece = all_pieces+PIECE_UNDEFINED;
	threat->others -= square_state->count;
}

static void save_empty(threat_t *threat) {
	int i;
	threat->square->piece = all_pieces+PIECE_EMPTY;
	for (i = 0; i < COLORS_N; ++i) {
		threat->in_checks[i] = colors[i].in_check;
	}
}

static void restore_empty(threat_t *threat) {
	int i;
	for (i = 0; i < COLORS_N; ++i) {
		colors[i].in_check = threat->in_checks[i];
	}
	threat->square->piece = all_pieces+PIECE_UNDEFINED;
	--threat->others;
}

static void save_others(threat_t *threat) {
	int i;
	threat->square->piece = all_pieces+PIECE_OTHERS;
	for (i = 0; i < COLORS_N; ++i) {
		threat->last_steps[i] = colors[i].last_steps[threat->square->states[i].move_idx];
	}
}

static void restore_others(threat_t *threat) {
	int i;
	for (i = 0; i < COLORS_N; ++i) {
		colors[i].last_steps[threat->square->states[i].move_idx] = threat->last_steps[i];
	}
	threat->square->piece = all_pieces+PIECE_UNDEFINED;
}

static int mp_new(mp_t *mp, int val) {
	if (val < 0 || val > p_max) {
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

static int mp_mul_val(mp_t *mp, int val) {
	return 1;
}

static int mp_mul(mp_t *mp_a, mp_t *mp_b) {
	return 1;
}

static int mp_positive(const mp_t *mp) {
	return mp->m > 1 || mp->p;
}

static void mp_print(const mp_t *mp) {
}

static void mp_free(mp_t *mp) {
	if (mp->p) {
		free(mp->p);
	}
}

static void set_king_square(square_t *square, piece_t *piece, color_t *color) {
	square->piece = piece;
	color->king_square = square;
}

static int search_white_king(square_t *square) {
	int i;
	for (i = 0; i < all_pieces->moves_n; ++i) {
		if (mem_squares[square->idx-all_moves[all_pieces->moves[i]]].piece == all_pieces) {
			return 1;
		}
	}
	return 0;
}

static void count_positions(square_t *w_square, square_t *b_square) {
	int i;
	mp_t one;
	for (i = 0; i < squares_n; ++i) {
		reset_square_states(squares[i]);
	}
	set_color_states(w_square, COLOR_WHITE, MEM_OFFSET+1);
	set_color_states(b_square, COLOR_BLACK, MEM_OFFSET+rows_n-2);
	for (i = 0; i < COLORS_N; ++i) {
		reset_color(colors+i);
	}
	if (!mp_new(&positions_all, 0)) {
		return;
	}
	if (!mp_new(&factor, 1)) {
		mp_free(&positions_all);
		return;
	}
	threats_n = 0;
	for (i = 0; i < squares_n; ++i) {
		if (squares[i]->piece == all_pieces+PIECE_UNDEFINED) {
			check_square_threats(squares[i]);
		}
	}
	if (!mp_new(&one, 1)) {
		mp_free(&factor);
		mp_free(&positions_all);
		return;
	}
	search_positions(0, &one);
	mp_free(&one);
	if (!mp_mul(&positions_all, &factor)) {
		mp_free(&factor);
		mp_free(&positions_all);
		return;
	}
	mp_free(&factor);
	set_cache(w_square, b_square);
	if (colors[COLOR_WHITE].pawn_states == colors[COLOR_BLACK].pawn_states) {
		b_square->h_mirror->column < b_square->column ? set_cache(b_square->opposite, w_square->opposite):set_cache(b_square->v_mirror, w_square->v_mirror);
	}
	for (i = MEM_OFFSET; i < MEM_OFFSET+rows_n; ++i) {
		int j;
		for (j = MEM_OFFSET; j < MEM_OFFSET+columns_n; ++j) {
			output_square(mem_squares+square_idx(i, j));
		}
		puts("");
	}
	mp_print(&positions_all);
	fflush(stdout);
	mp_free(&positions_all);
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
				set_square_state(mem_squares[target_idx].states+color_idx, piece->moves[i], step);
			}
		}
	}
	else {
		for (i = 0; i < piece->moves_n; ++i) {
			int target_idx = square->idx-all_moves[piece->moves[i]];
			if (mem_squares[target_idx].piece == all_pieces+PIECE_UNDEFINED) {
				set_square_state(mem_squares[target_idx].states+color_idx, piece->moves[i], 1);
			}
		}
	}
}

static void check_square_threats(square_t *square) {
	if (square->states[COLOR_WHITE].count || square->states[COLOR_BLACK].count) {
		int i;
		for (i = 0; i < COLORS_N; ++i) {
			update_state_less(square->states+i);
		}
		set_threat(threats+threats_n, square);
		++threats_n;
	}
	else {
		if (!mp_mul_val(&factor, square->others_max)) {
			return;
		}
	}
}

static void search_positions(int thread_idx, mp_t *positions) {
}

static void set_cache(square_t *w_square, square_t *b_square) {
}

static void free_cache(int cache_size) {
	int i;
	for (i = 0; i < cache_size; ++i) {
		int j;
		for (j = 0; j < mem_squares_n; ++j) {
			mp_free(&cache[i][j]);
		}
		free(cache[i]);
	}
	free(cache);
}
