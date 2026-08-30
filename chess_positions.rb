# frozen_string_literal: true

# Method added to the String standard class to check integers
class String
  def integer?
    to_i.to_s == self
  end
end

# Chess piece management
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
  attr_reader :idx, :move_idx, :count

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

  def update_step_less(colors)
    colors[@idx].last_steps[@move_idx] = @step unless influent_step?(colors)
  end

  def update_step_more(colors)
    colors[@idx].last_steps[@move_idx] = @step if more_influent_step?(colors)
  end

  def influent_step?(colors)
    @step <= colors[@idx].last_steps[@move_idx]
  end

  def more_influent_step?(colors)
    @step < colors[@idx].last_steps[@move_idx]
  end

  def potential_check?(colors)
    influent_step?(colors) && @count.positive? && !colors[@idx].in_check
  end
end

# Chess square management
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

  def reset_states
    @states[0].reset
    @states[1].reset
  end

  def more_influent_step?(colors)
    @states[0].more_influent_step?(colors) || @states[1].more_influent_step?(colors)
  end

  def output
    putc(@piece.symbol)
  end
end

# Chess color management
class ChessColor
  attr_reader :pieces, :pawn, :threat_piece
  attr_accessor :king_square, :pawn_states, :last_steps, :in_check

  def initialize(pieces, pawn, threat_piece)
    @pieces = pieces
    @pawn = pawn
    @threat_piece = threat_piece
    @king_square = nil
    @pawn_states = nil
    @last_steps = Array.new(17)
    @in_check = nil
  end

  def reset
    @last_steps.fill(0)
    @in_check = false
  end
end

# Chess threat management
class ChessThreat
  attr_reader :square
  attr_accessor :others

  def initialize(square)
    @square = square
    @last_steps = []
    @in_checks = []
  end

  def save_threat_piece(state, colors)
    @square.piece = colors[state.idx].threat_piece
    @last_steps[state.idx] = colors[state.idx].last_steps[state.move_idx]
    @in_checks[state.idx] = colors[state.idx].in_check
  end

  def restore_threat_piece(colors, state, piece)
    colors[state.idx].in_check = @in_checks[state.idx]
    colors[state.idx].last_steps[state.move_idx] = @last_steps[state.idx]
    @square.piece = piece
    @others -= state.count
  end

  def save_empty(piece, colors)
    @square.piece = piece
    @square.states.each do |state|
      @in_checks[state.idx] = colors[state.idx].in_check
    end
  end

  def restore_empty(colors, piece)
    @square.states.each do |state|
      colors[state.idx].in_check = @in_checks[state.idx]
    end
    @square.piece = piece
    @others -= 1
  end

  def save_others(piece, colors)
    @square.piece = piece
    @square.states.each do |state|
      @last_steps[state.idx] = colors[state.idx].last_steps[state.move_idx]
    end
  end

  def restore_others(colors, piece)
    @square.states.each do |state|
      colors[state.idx].last_steps[state.move_idx] = @last_steps[state.idx]
    end
    @square.piece = piece
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

def set_row(row, piece, others_max)
  set_side(row, 0, 1)
  (2..@columns + 1).each do |column|
    @mem_squares[square_idx(row, column)] = ChessSquare.new(row, column, square_idx(row, column), piece, others_max)
  end
  set_side(row, @columns + 2, @mem_columns - 1)
end

def set_side(row, column_a, column_b)
  (column_a..column_b).each do |column|
    @mem_squares[square_idx(row, column)] = ChessSquare.new(row, column, square_idx(row, column), @pieces['#'], 0)
  end
end

def square_idx(row, column)
  row * @mem_columns + column
end

def set_king_square(square, piece_idx, color_idx)
  square.piece = @pieces[piece_idx]
  @colors[color_idx].king_square = square
end

def search_threat(square, threat)
  return true if square.piece != @pieces['?']

  return search_threat_repeat(square, threat) if threat.repeat_move

  search_threat_unique(square, threat)
end

def search_threat_repeat(square, threat)
  threat.moves.each do |move_idx|
    target_idx = square.idx - @moves[move_idx]
    target_idx -= @moves[move_idx] while @mem_squares[target_idx].piece == @pieces['?']
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

def set_pieces_states(square, state_idx, row)
  @colors[state_idx].pieces.each do |piece|
    set_piece_states(square, @pieces[piece], state_idx)
  end
  @colors[state_idx].pawn_states = @options & 1 == 1 || square.row != row
  set_piece_states(square, @colors[state_idx].pawn, state_idx) if @colors[state_idx].pawn_states
end

def set_piece_states(square, piece, state_idx)
  if piece.repeat_move
    set_states_repeat(square, piece, state_idx)
  else
    set_states_unique(square, piece, state_idx)
  end
end

def set_states_repeat(square, piece, state_idx)
  piece.moves.each do |move_idx|
    target_idx = square.idx - @moves[move_idx]
    step = 1
    while @mem_squares[target_idx].piece == @pieces['?']
      @mem_squares[target_idx].states[state_idx].set(move_idx, step)
      target_idx -= @moves[move_idx]
      step += 1
    end
  end
end

def set_states_unique(square, piece, state_idx)
  piece.moves.each do |move_idx|
    target_idx = square.idx - @moves[move_idx]
    next unless @mem_squares[target_idx].piece == @pieces['?']

    @mem_squares[target_idx].states[state_idx].set(move_idx, 1)
  end
end

def set_threats
  @colors.each(&:reset)
  @positions = 0
  @factor = 1
  @squares.each do |square|
    push_square_threats(square) if square.piece == @pieces['?']
  end
  @threats_size = @threats.size
end

def push_square_threats(square)
  if square.states[0].count.positive? || square.states[1].count.positive?
    update_steps(square)
    @threats.push(ChessThreat.new(square))
  else
    @factor *= square.others_max
  end
end

def update_steps(square)
  square.states.each do |state|
    state.update_step_less(@colors)
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

def set_choices(threat_idx, positions, threat)
  threat.others = threat.square.others_max
  threat.square.states.each do |state|
    set_choice_threat_piece(threat_idx, positions, threat, state) if state.potential_check?(@colors)
  end
  set_choice_empty(threat_idx, positions, threat) if threat.square.more_influent_step?(@colors)
  set_choice_others(threat_idx, positions, threat)
end

def set_choice_threat_piece(threat_idx, positions, threat, state)
  threat.save_threat_piece(state, @colors)
  state.update_step_more(@colors)
  search_color_threat(@colors[state.idx], state.move_idx)
  count_positions(threat_idx + 1, positions * state.count)
  threat.restore_threat_piece(@colors, state, @pieces['?'])
end

def set_choice_empty(threat_idx, positions, threat)
  threat.save_empty(@pieces['.'], @colors)
  threat.square.states.each do |state|
    next unless state.potential_check?(@colors)

    search_color_threat(@colors[state.idx], state.move_idx)
  end
  count_positions(threat_idx + 1, positions)
  threat.restore_empty(@colors, @pieces['?'])
end

def set_choice_others(threat_idx, positions, threat)
  threat.save_others(@pieces['*'], @colors)
  threat.square.states.each do |state|
    state.update_step_more(@colors)
  end
  count_positions(threat_idx + 1, positions * threat.others)
  threat.restore_others(@colors, @pieces['?'])
end

def search_color_threat(color, move_idx)
  target_idx = color.king_square.idx - @moves[move_idx]
  target_idx -= @moves[move_idx] while @mem_squares[target_idx].piece == @pieces['.']
  color.in_check = @mem_squares[target_idx].piece == color.threat_piece
end

def clear_threats
  @threats.clear
  @positions *= @factor
end

def set_cache(w_square, b_square)
  @cache[w_square.idx][b_square.idx] = @positions
  @cache[w_square.h_mirror.idx][b_square.h_mirror.idx] = @positions
end

def output_chessboard
  (2..@rows + 1).each do |row|
    (2..@columns + 1).each do |column|
      @mem_squares[square_idx(row, column)].output
    end
    puts
  end
  puts @positions
  $stdout.flush
end

def output_positions_sum
  positions_sum = 0
  @cache.each do |white|
    white.each do |black|
      positions_sum += black
    end
  end
  puts "Positions #{positions_sum}"
  $stdout.flush
end

usage unless ARGV.size == 3 && ARGV[0].integer? && ARGV[1].integer? && ARGV[2].integer?
@rows = ARGV[0].to_i
@columns = ARGV[1].to_i
@options = ARGV[2].to_i
usage unless @rows > 1 && @columns.positive? && @options >= 0 && @options < 8
@mem_rows = @rows + 4
@mem_columns = @columns + 4
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
@pieces = {
  'K' => ChessPiece.new('K', [1, 2, 3, 4, 5, 6, 7, 8], false),
  'k' => ChessPiece.new('k', [1, 2, 3, 4, 5, 6, 7, 8], false),
  'Q' => ChessPiece.new('Q', [1, 2, 3, 4, 5, 6, 7, 8], true),
  'q' => ChessPiece.new('q', [1, 2, 3, 4, 5, 6, 7, 8], true),
  'R' => ChessPiece.new('R', [1, 3, 5, 7], true),
  'r' => ChessPiece.new('r', [1, 3, 5, 7], true),
  'B' => ChessPiece.new('B', [2, 4, 6, 8], true),
  'b' => ChessPiece.new('b', [2, 4, 6, 8], true),
  'N' => ChessPiece.new('N', [9, 10, 11, 12, 13, 14, 15, 16], false),
  'n' => ChessPiece.new('n', [9, 10, 11, 12, 13, 14, 15, 16], false),
  'P' => ChessPiece.new('P', [2, 4], false),
  'p' => ChessPiece.new('p', [6, 8], false),
  '#' => ChessPiece.new('#', nil, false),
  '?' => ChessPiece.new('?', nil, false),
  '.' => ChessPiece.new('.', nil, false),
  'T' => ChessPiece.new('T', nil, false),
  't' => ChessPiece.new('t', nil, false),
  '*' => ChessPiece.new('*', nil, false)
}
@mem_squares = []
2.times do |row|
  set_row(row, @pieces['#'], 0)
end
others_max = 9
others_max += 1 if @options & 1 == 1
others_max += 1 if @options & 2 == 2
set_row(2, @pieces['?'], others_max)
(3..@rows).each do |row|
  set_row(row, @pieces['?'], 11)
end
set_row(@rows + 1, @pieces['?'], others_max)
(@rows + 2..@mem_rows - 1).each do |row|
  set_row(row, @pieces['#'], 0)
end
@squares = []
(2..@rows + 1).each do |row|
  (2..@columns + 1).each do |column|
    @squares.push(@mem_squares[square_idx(row, column)])
  end
end
@squares.each do |square|
  square.h_mirror = @mem_squares[square_idx(square.row, @mem_columns - square.column - 1)]
  square.v_mirror = @mem_squares[square_idx(@mem_rows - square.row - 1, square.column)]
  square.opposite = @mem_squares[square_idx(@mem_rows - square.row - 1, @mem_columns - square.column - 1)]
end
@colors = [
  ChessColor.new(%w[Q R B N], @pieces['P'], @pieces['t']),
  ChessColor.new(%w[q r b n], @pieces['p'], @pieces['T'])
]
@threats = []
@cache = Array.new(@mem_rows * @mem_columns) do
  Array.new(@mem_rows * @mem_columns, 0)
end
@squares.each do |w_square|
  set_king_square(w_square, 'K', 0)
  @squares.each do |b_square|
    next if @cache[w_square.idx][b_square.idx].positive? || search_threat(b_square, @pieces['K'])

    set_king_square(b_square, 'k', 1)
    @squares.each(&:reset_states)
    set_pieces_states(w_square, 0, 3)
    set_pieces_states(b_square, 1, @rows)
    set_threats
    count_positions(0, 1)
    clear_threats
    set_cache(w_square, b_square)
    if @colors[0].pawn_states == @colors[1].pawn_states
      if b_square.h_mirror.column < b_square.column
        set_cache(b_square.opposite, w_square.opposite)
      else
        set_cache(b_square.v_mirror, w_square.v_mirror)
      end
    end
    output_chessboard
    b_square.piece = @pieces['?']
  end
  w_square.piece = @pieces['?']
end
output_positions_sum
