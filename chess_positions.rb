# frozen_string_literal: true

# Method added to the String standard class to check integers
class String
  def integer?
    to_i.to_s == self
  end
end

# Chess piece setup
class ChessPiece
  attr_reader :symbol, :moves, :repeat_move

  def initialize(symbol, moves, repeat_move)
    @symbol = symbol
    @moves = moves
    @repeat_move = repeat_move
  end
end

# Chess square state management
class ChessSquareState
  attr_reader :idx, :move_idx, :step, :count

  def initialize(idx)
    @idx = idx
    reset
  end

  def reset
    @move_idx = 0
    @step = 0
    @count = 0
  end

  def set(move_idx, step)
    @move_idx = move_idx
    @step = step
    @count += 1
  end
end

# Chess square setup
class ChessSquare
  attr_reader :row, :column, :idx, :others_max, :states
  attr_accessor :piece, :h_mirror, :v_mirror, :opposite

  def initialize(row, column, idx, piece, others_max)
    @row = row
    @column = column
    @idx = idx
    @piece = piece
    @others_max = others_max
    @h_mirror = nil
    @v_mirror = nil
    @opposite = nil
    @states = [ChessSquareState.new(0), ChessSquareState.new(1)]
  end
end

# Chess color setup
class ChessColor
  attr_accessor :threat_piece, :king_square, :last_steps, :in_check

  def initialize(threat_piece)
    @threat_piece = threat_piece
    @king_square = nil
    @last_steps = Array.new(17)
    @in_check = false
  end

  def reset
    @last_steps.fill(0)
    @in_check = false
  end
end

def usage
  warn 'Program arguments: <rows> <columns> <options>'
  warn '<rows> must be greater than 1'
  warn '<columns> must be greater than 0'
  warn '<options> is the sum of none, some or all of the below flags:'
  warn '1 = pawns allowed on first row'
  warn '2 = pawns allowed on last row (no promotions)'
  warn '4 = color on move counts (positions where no kings are in chess will be counted twice)'
  $stderr.flush
  exit false
end

def set_side(row, column_a, column_b)
  (column_a..column_b).each do |column|
    @mem_squares[square_idx(row, column)] = ChessSquare.new(row, column, square_idx(row, column), @pieces['Side'], 0)
  end
end

def set_row(row, piece, others_max)
  set_side(row, 0, MEM_OFFSET - 1)
  (MEM_OFFSET..MEM_OFFSET + @columns - 1).each do |column|
    @mem_squares[square_idx(row, column)] = ChessSquare.new(row, column, square_idx(row, column), piece, others_max)
  end
  set_side(row, MEM_OFFSET + @columns, @mem_columns - 1)
end

def square_idx(row, column)
  row * @mem_columns + column
end

def search_threat(square, threat)
  return true if square.piece != @pieces['Undefined']

  return search_threat_repeat(square, threat) if threat.repeat_move

  search_threat_unique(square, threat)
end

def search_threat_repeat(square, threat)
  threat.moves.each do |move_idx|
    target_idx = square.idx - @moves[move_idx]
    target_idx -= @moves[move_idx] while @mem_squares[target_idx].piece == @pieces['Undefined']
    return true if @mem_squares[target_idx].piece == threat
  end
  false
end

def search_threat_unique(square, threat)
  threat.moves.each do |move_idx|
    return true if @mem_squares[square.idx - @moves[move_idx]].piece == threat
  end
  false
end

def set_piece_states(square, piece, state_idx)
  piece.repeat_move ? set_states_repeat(square, piece, state_idx) : set_states_unique(square, piece, state_idx)
end

def set_states_repeat(square, piece, state_idx)
  piece.moves.each do |move_idx|
    target_idx = square.idx - @moves[move_idx]
    step = 1
    while @mem_squares[target_idx].piece == @pieces['Undefined']
      @mem_squares[target_idx].states[state_idx].set(move_idx, step)
      target_idx -= @moves[move_idx]
      step += 1
    end
  end
end

def set_states_unique(square, piece, state_idx)
  piece.moves.each do |move_idx|
    target_idx = square.idx - @moves[move_idx]
    next unless @mem_squares[target_idx].piece == @pieces['Undefined']

    @mem_squares[target_idx].states[state_idx].set(move_idx, 1)
  end
end

def set_threats
  @colors.each(&:reset)
  @positions = 0
  @factor = 1
  @squares.each do |square|
    push_square_threats(square)
  end
  @threats_size = @threats.size
end

def push_square_threats(square)
  return unless square.piece == @pieces['Undefined']

  if square.states[0].count.positive? || square.states[1].count.positive?
    update_steps(square)
    @threats.push(square)
  else
    @factor *= square.others_max
  end
end

def update_steps(square)
  square.states.each do |state|
    next unless state.step > @colors[state.idx].last_steps[state.move_idx]

    @colors[state.idx].last_steps[state.move_idx] = state.step
  end
end

def count_positions(threat_idx, positions)
  return if @colors[0].in_check && @colors[1].in_check

  if threat_idx < @threats_size
    set_choices(threat_idx, positions, @threats[threat_idx])
  else
    @positions += positions
    @positions += positions if @options & 4 == 4 && !@colors[0].in_check && !@colors[1].in_check
  end
end

def set_choices(threat_idx, positions, square)
  others = square.others_max
  square.states.each do |state|
    next unless potential_check(state)

    square.piece = @colors[state.idx].threat_piece
    last_step = @colors[state.idx].last_steps[state.move_idx]
    @colors[state.idx].last_steps[state.move_idx] = state.step if more_influent_step(state)
    in_check = @colors[state.idx].in_check
    @colors[state.idx].in_check = search_color_threat(@colors[state.idx], state.move_idx)
    count_positions(threat_idx + 1, positions * state.count)
    @colors[state.idx].in_check = in_check
    @colors[state.idx].last_steps[state.move_idx] = last_step
    square.piece = @pieces['Undefined']
    others -= state.count
  end
  if more_influent_step(square.states[0]) || more_influent_step(square.states[1])
    square.piece = @pieces['Empty']
    in_checks = []
    square.states.each do |state|
      in_checks[state.idx] = @colors[state.idx].in_check
      next unless potential_check(state)

      @colors[state.idx].in_check = search_color_threat(@colors[state.idx], state.move_idx)
    end
    count_positions(threat_idx + 1, positions)
    square.states.each do |state|
      @colors[state.idx].in_check = in_checks[state.idx]
    end
    square.piece = @pieces['Undefined']
    others -= 1
  end
  square.piece = @pieces['Others']
  last_steps = []
  square.states.each do |state|
    last_steps[state.idx] = @colors[state.idx].last_steps[state.move_idx]
    @colors[state.idx].last_steps[state.move_idx] = state.step if more_influent_step(state)
  end
  count_positions(threat_idx + 1, positions * others)
  square.states.each do |state|
    @colors[state.idx].last_steps[state.move_idx] = last_steps[state.idx]
  end
  square.piece = @pieces['Undefined']
end

def potential_check(state)
  influent_step(state) && state.count.positive? && !@colors[state.idx].in_check
end

def influent_step(state)
  state.step <= @colors[state.idx].last_steps[state.move_idx]
end

def more_influent_step(state)
  state.step < @colors[state.idx].last_steps[state.move_idx]
end

def search_color_threat(color, move_idx)
  target_idx = color.king_square.idx - @moves[move_idx]
  target_idx -= @moves[move_idx] while @mem_squares[target_idx].piece == @pieces['Empty']
  @mem_squares[target_idx].piece == color.threat_piece
end

def output_chessboard
  (MEM_OFFSET..MEM_OFFSET + @rows - 1).each do |row|
    (MEM_OFFSET..MEM_OFFSET + @columns - 1).each do |column|
      putc(@mem_squares[square_idx(row, column)].piece.symbol)
    end
    puts
  end
  puts @positions * @factor
  $stdout.flush
end

def output_positions_sum
  puts "Positions #{@positions_sum}"
  $stdout.flush
end

usage unless ARGV.size == 3 && ARGV[0].integer? && ARGV[1].integer? && ARGV[2].integer?
@rows = ARGV[0].to_i
@columns = ARGV[1].to_i
@options = ARGV[2].to_i
usage unless @rows > 1 && @columns.positive? && @options >= 0 && @options < 8
MEM_OFFSET = 2
@mem_rows = MEM_OFFSET + @rows + MEM_OFFSET
@mem_columns = MEM_OFFSET + @columns + MEM_OFFSET
@moves = [
  0,
  -1,
  -@mem_columns - 1,
  -@mem_columns,
  -@mem_columns + 1,
  1,
  @mem_columns + 1,
  @mem_columns,
  @mem_columns - 1,
  -@mem_columns - 2,
  -@mem_columns * 2 - 1,
  -@mem_columns * 2 + 1,
  -@mem_columns + 2,
  @mem_columns + 2,
  @mem_columns * 2 + 1,
  @mem_columns * 2 - 1,
  @mem_columns - 2
]
royal_moves = [1, 2, 3, 4, 5, 6, 7, 8]
rook_moves = [1, 3, 5, 7]
bishop_moves = [2, 4, 6, 8]
knight_moves = [9, 10, 11, 12, 13, 14, 15, 16]
w_pawn_moves = [2, 4]
b_pawn_moves = [6, 8]
@pieces = {
  'WhiteKing' => ChessPiece.new('K', royal_moves, false),
  'BlackKing' => ChessPiece.new('k', royal_moves, false),
  'WhiteQueen' => ChessPiece.new('Q', royal_moves, true),
  'BlackQueen' => ChessPiece.new('q', royal_moves, true),
  'WhiteRook' => ChessPiece.new('R', rook_moves, true),
  'BlackRook' => ChessPiece.new('r', rook_moves, true),
  'WhiteBishop' => ChessPiece.new('B', bishop_moves, true),
  'BlackBishop' => ChessPiece.new('b', bishop_moves, true),
  'WhiteKnight' => ChessPiece.new('N', knight_moves, false),
  'BlackKnight' => ChessPiece.new('n', knight_moves, false),
  'WhitePawn' => ChessPiece.new('P', w_pawn_moves, false),
  'BlackPawn' => ChessPiece.new('p', b_pawn_moves, false),
  'Side' => ChessPiece.new('#', nil, false),
  'Undefined' => ChessPiece.new('?', nil, false),
  'Empty' => ChessPiece.new('.', nil, false),
  'WhiteThreat' => ChessPiece.new('T', nil, false),
  'BlackThreat' => ChessPiece.new('t', nil, false),
  'Others' => ChessPiece.new('*', nil, false)
}
@mem_squares = []
(0..MEM_OFFSET - 1).each do |row|
  set_row(row, @pieces['Side'], 0)
end
others_max = 9
others_max += 1 if @options & 1 == 1
others_max += 1 if @options & 2 == 2
set_row(MEM_OFFSET, @pieces['Undefined'], others_max)
(MEM_OFFSET + 1..MEM_OFFSET + @rows - 2).each do |row|
  set_row(row, @pieces['Undefined'], 11)
end
set_row(MEM_OFFSET + @rows - 1, @pieces['Undefined'], others_max)
(MEM_OFFSET + @rows..@mem_rows - 1).each do |row|
  set_row(row, @pieces['Side'], 0)
end
@squares = []
(MEM_OFFSET..MEM_OFFSET + @rows - 1).each do |row|
  (MEM_OFFSET..MEM_OFFSET + @columns - 1).each do |column|
    @squares.push(@mem_squares[square_idx(row, column)])
  end
end
@squares.each do |square|
  square.h_mirror = @mem_squares[square_idx(square.row, @mem_columns - square.column - 1)]
  square.v_mirror = @mem_squares[square_idx(@mem_rows - square.row - 1, square.column)]
  square.opposite = @mem_squares[square_idx(@mem_rows - square.row - 1, @mem_columns - square.column - 1)]
end
@colors = [
  ChessColor.new(@pieces['BlackThreat']),
  ChessColor.new(@pieces['WhiteThreat'])
]
@threats_size = 0
@threats = []
@positions_all = Array.new(@mem_rows * @mem_columns) do
  Array.new(@mem_rows * @mem_columns) do
    0
  end
end
@positions = 0
@factor = 1
@positions_sum = 0
@squares.each do |w_square|
  w_square.piece = @pieces['WhiteKing']
  @colors[0].king_square = w_square
  @squares.each do |b_square|
    next unless @positions_all[w_square.idx][b_square.idx].zero? && !search_threat(b_square, @pieces['WhiteKing'])

    b_square.piece = @pieces['BlackKing']
    @colors[1].king_square = b_square
    @squares.each do |square|
      square.states[0].reset
      square.states[1].reset
    end
    set_piece_states(w_square, @pieces['BlackQueen'], 0)
    set_piece_states(w_square, @pieces['BlackRook'], 0)
    set_piece_states(w_square, @pieces['BlackBishop'], 0)
    set_piece_states(w_square, @pieces['BlackKnight'], 0)
    b_pawn_states = @options & 1 == 1 || w_square.row != MEM_OFFSET + 1
    set_piece_states(w_square, @pieces['BlackPawn'], 0) if b_pawn_states == true
    set_piece_states(b_square, @pieces['WhiteQueen'], 1)
    set_piece_states(b_square, @pieces['WhiteRook'], 1)
    set_piece_states(b_square, @pieces['WhiteBishop'], 1)
    set_piece_states(b_square, @pieces['WhiteKnight'], 1)
    w_pawn_states = @options & 1 == 1 || b_square.row != MEM_OFFSET + @rows - 2
    set_piece_states(b_square, @pieces['WhitePawn'], 1) if w_pawn_states == true
    set_threats
    count_positions(0, 1)
    @threats.clear
    @positions_all[w_square.idx][b_square.idx] = @positions * @factor
    if b_pawn_states == w_pawn_states
      w_h_mirror = w_square.h_mirror
      b_h_mirror = b_square.h_mirror
      @positions_all[w_h_mirror.idx][b_h_mirror.idx] = @positions * @factor
      if b_h_mirror.column < b_square.column
        w_opposite = w_square.opposite
        b_opposite = b_square.opposite
        @positions_all[b_opposite.idx][w_opposite.idx] = @positions * @factor
        w_opposite_h_mirror = w_opposite.h_mirror
        b_opposite_h_mirror = b_opposite.h_mirror
        @positions_all[b_opposite_h_mirror.idx][w_opposite_h_mirror.idx] = @positions * @factor
      else
        w_v_mirror = w_square.v_mirror
        b_v_mirror = b_square.v_mirror
        @positions_all[b_v_mirror.idx][w_v_mirror.idx] = @positions * @factor
        w_v_mirror_h_mirror = w_v_mirror.h_mirror
        b_v_mirror_h_mirror = b_v_mirror.h_mirror
        @positions_all[b_v_mirror_h_mirror.idx][w_v_mirror_h_mirror.idx] = @positions * @factor
      end
    end
    output_chessboard
    b_square.piece = @pieces['Undefined']
  end
  w_square.piece = @pieces['Undefined']
end
@positions_sum = 0
@positions_all.each do |white|
  white.each do |black|
    @positions_sum += black
  end
end
output_positions_sum
