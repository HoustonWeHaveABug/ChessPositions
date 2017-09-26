class String
	def is_integer?
		begin
			if Integer(self)
			end
			true
		rescue
			false
		end
	end
end

class ChessPiece
	attr_reader(:symbol)
	attr_reader(:moves)
	attr_reader(:repeat_move)

	def initialize(symbol, moves, repeat_move)
		@symbol = symbol
		@moves = moves
		@repeat_move = repeat_move
	end
end

class ChessSquareState
	attr_reader(:index)
	attr_reader(:move_index)
	attr_reader(:step)
	attr_reader(:count)

	def initialize(index)
		@index = index
		reset
	end

	def reset
		@move_index = 0
		@step = 0
		@count = 0
	end

	def set(move_index, step)
		@move_index = move_index
		@step = step
		@count += 1
	end
end

class ChessSquare
	attr_reader(:row)
	attr_reader(:column)
	attr_reader(:index)
	attr_accessor(:piece)
	attr_accessor(:mirror)
	attr_reader(:states)
	attr_accessor(:positions_sum)
	attr_accessor(:white_square)
	attr_accessor(:positions)

	def initialize(row, column, index, piece)
		@row = row
		@column = column
		@index = index
		@piece = piece
		@mirror = nil
		@states = [
			ChessSquareState.new(0),
			ChessSquareState.new(1)
		]
		@positions_sum = 0
		@white_square = nil
		@positions = 0
	end

	def reflect_positions_sum
		if mirror != self
			mirror.positions_sum = self.positions_sum
		end
	end

	def reflect_positions(white_square)
		if mirror != self
			mirror.white_square = white_square
			mirror.positions = self.positions
		end
	end
end

class ChessColor
	attr_accessor(:threat_piece)
	attr_accessor(:king_square)
	attr_accessor(:last_steps)
	attr_accessor(:in_check)

	def initialize(threat_piece)
		@threat_piece = threat_piece
		@king_square = nil
		@last_steps = []
		@in_check = false
	end
end

class ChessPositions
	@@mem_offset = 2
	@@others_max = 11

	def initialize(rows, columns)
		@rows = rows
		@columns = columns
		@mem_rows = @@mem_offset+@rows+@@mem_offset
		@mem_columns = @@mem_offset+@columns+@@mem_offset
		@moves = [
			0,
			-1,
			-@mem_columns-1,
			-@mem_columns,
			-@mem_columns+1,
			1,
			@mem_columns+1,
			@mem_columns,
			@mem_columns-1,
			-@mem_columns-2,
			-@mem_columns*2-1,
			-@mem_columns*2+1,
			-@mem_columns+2,
			@mem_columns+2,
			@mem_columns*2+1,
			@mem_columns*2-1,
			@mem_columns-2
		]
		royal_moves = [ 1, 2, 3, 4, 5, 6, 7, 8 ]
		rook_moves = [ 1, 3, 5, 7 ]
		bishop_moves = [ 2, 4, 6, 8 ]
		knight_moves = [ 9, 10, 11, 12, 13, 14, 15, 16 ]
		white_pawn_moves = [ 2, 4 ]
		black_pawn_moves = [ 6, 8 ]
		@pieces = {
			"WhiteKing" => ChessPiece.new('K', royal_moves, false),
			"BlackKing" => ChessPiece.new('k', royal_moves, false),
			"WhiteQueen" => ChessPiece.new('Q', royal_moves, true),
			"BlackQueen" => ChessPiece.new('q', royal_moves, true),
			"WhiteRook" => ChessPiece.new('R', rook_moves, true),
			"BlackRook" => ChessPiece.new('r', rook_moves, true),
			"WhiteBishop" => ChessPiece.new('B', bishop_moves, true),
			"BlackBishop" => ChessPiece.new('b', bishop_moves, true),
			"WhiteKnight" => ChessPiece.new('N', knight_moves, false),
			"BlackKnight" => ChessPiece.new('n', knight_moves, false),
			"WhitePawn" => ChessPiece.new('P', white_pawn_moves, false),
			"BlackPawn" => ChessPiece.new('p', black_pawn_moves, false),
			"Outside" => ChessPiece.new('#', nil, false),
			"Undefined" => ChessPiece.new('?', nil, false),
			"Empty" => ChessPiece.new('.', nil, false),
			"WhiteThreat" => ChessPiece.new('T', nil, false),
			"BlackThreat" => ChessPiece.new('t', nil, false),
			"Others" => ChessPiece.new('*', nil, false)
		}
		@mem_squares = []
		for row in 0..@@mem_offset-1
			set_row(row, @pieces["Outside"])
		end
		for row in @@mem_offset..@@mem_offset+@rows-1
			set_row(row, @pieces["Undefined"])
		end
		for row in @@mem_offset+@rows..@mem_rows-1
			set_row(row, @pieces["Outside"])
		end
		@squares = []
		for row in @@mem_offset..@@mem_offset+@rows-1
			for column in @@mem_offset..@@mem_offset+@columns-1
				@squares.push(@mem_squares[square_index(row, column)])
			end
		end
		@squares.each do |square|
			square.mirror = @mem_squares[square_index(square.row, @mem_columns-square.column-1)]
		end
		@positions_sum = 0
		@colors = [
			ChessColor.new(@pieces["BlackThreat"]),
			ChessColor.new(@pieces["WhiteThreat"])
		]
		@positions = 0
		@factor = 1
		@threats_size = 0
		@threats = []
	end

	def set_row(row, piece)
		for column in 0..@@mem_offset-1
			@mem_squares[square_index(row, column)] = ChessSquare.new(row, column, square_index(row, column), @pieces["Outside"])
		end
		for column in @@mem_offset..@@mem_offset+@columns-1
			@mem_squares[square_index(row, column)] = ChessSquare.new(row, column, square_index(row, column), piece)
		end
		for column in @@mem_offset+@columns..@mem_columns-1
			@mem_squares[square_index(row, column)] = ChessSquare.new(row, column, square_index(row, column), @pieces["Outside"])
		end
	end

	def square_index(row, column)
		row*@mem_columns+column
	end

	def run
		@positions_sum = 0
		@squares.each do |white_square|
			if white_square.positions_sum == 0
				white_square.piece = @pieces["WhiteKing"]
				@colors[0].king_square = white_square
				@squares.each do |black_square|
					if black_square.piece == @pieces["Undefined"] && !search_threat(black_square, @pieces["WhiteKing"])
						if white_square.mirror != white_square || black_square.white_square != white_square
							black_square.piece = @pieces["BlackKing"]
							@colors[1].king_square = black_square
							@squares.each do |square|
								square.states[0].reset
								square.states[1].reset
							end
							set_piece_states(white_square, @pieces["BlackQueen"], 0)
							set_piece_states(white_square, @pieces["BlackRook"], 0)
							set_piece_states(white_square, @pieces["BlackBishop"], 0)
							set_piece_states(white_square, @pieces["BlackKnight"], 0)
							set_piece_states(white_square, @pieces["BlackPawn"], 0)
							set_piece_states(black_square, @pieces["WhiteQueen"], 1)
							set_piece_states(black_square, @pieces["WhiteRook"], 1)
							set_piece_states(black_square, @pieces["WhiteBishop"], 1)
							set_piece_states(black_square, @pieces["WhiteKnight"], 1)
							set_piece_states(black_square, @pieces["WhitePawn"], 1)
							set_threats
							count_positions(0, 1)
							@threats.clear
							black_square.white_square = white_square
							black_square.positions = @positions*@factor
							output_chessboard
							puts("#{black_square.positions}")
							black_square.piece = @pieces["Undefined"]
							black_square.reflect_positions(white_square)
						end
						white_square.positions_sum += black_square.positions
					end
				end
				white_square.piece = @pieces["Undefined"]
				white_square.reflect_positions_sum
			end
			@positions_sum += white_square.positions_sum
		end
	end

	def search_threat(square, threat)
		threat.moves.each do |move_index|
			target_index = square.index-@moves[move_index]
			if threat.repeat_move
				while @mem_squares[target_index].piece == @pieces["Undefined"]
					target_index -= @moves[move_index]
				end
			end
			if @mem_squares[target_index].piece == threat
				return true
			end
		end
		false
	end

	def set_piece_states(square, piece, state_index)
		piece.moves.each do |move_index|
			step = 1
			target_index = square.index-@moves[move_index]
			if @mem_squares[target_index].piece == @pieces["Undefined"]
				@mem_squares[target_index].states[state_index].set(move_index, step)
				if piece.repeat_move
					step += 1
					target_index -= @moves[move_index]
					while @mem_squares[target_index].piece == @pieces["Undefined"]
						@mem_squares[target_index].states[state_index].set(move_index, step)
						step += 1
						target_index -= @moves[move_index]
					end
				end
			end
		end
	end

	def set_threats
		@colors.each do |color|
			color.last_steps = @moves.map do |move|
				0
			end
			color.in_check = false
		end
		@positions = 0
		@factor = 1
		@squares.each do |square|
			if square.piece == @pieces["Undefined"]
				if square.states[0].count > 0 || square.states[1].count > 0
					square.states.each do |state|
						if state.step > @colors[state.index].last_steps[state.move_index]
							@colors[state.index].last_steps[state.move_index] = state.step
						end
					end
					@threats.push(square)
				else
					@factor *= @@others_max
				end
			end
		end
		@threats_size = @threats.size
	end

	def count_positions(threat_index, positions)
		if !@colors[0].in_check || !@colors[1].in_check
			if threat_index < @threats_size
				set_choices(threat_index, positions, @threats[threat_index])
			else
				@positions += positions
			end
		end
	end

	def set_choices(threat_index, positions, square)
		others = @@others_max
		square.states.each do |state|
			if potential_check(state)
				square.piece = @colors[state.index].threat_piece
				last_step = @colors[state.index].last_steps[state.move_index]
				if more_influent_step(state)
					@colors[state.index].last_steps[state.move_index] = state.step
				end
				in_check = @colors[state.index].in_check
				@colors[state.index].in_check = search_color_threat(@colors[state.index], state.move_index)
				count_positions(threat_index+1, positions*state.count)
				@colors[state.index].in_check = in_check
				@colors[state.index].last_steps[state.move_index] = last_step
				square.piece = @pieces["Undefined"]
				others -= state.count
			end
		end
		if more_influent_step(square.states[0]) || more_influent_step(square.states[1])
			square.piece = @pieces["Empty"]
			in_checks = []
			square.states.each do |state|
				in_checks[state.index] = @colors[state.index].in_check
				if potential_check(state)
					@colors[state.index].in_check = search_color_threat(@colors[state.index], state.move_index)
				end
			end
			count_positions(threat_index+1, positions)
			square.states.each do |state|
				@colors[state.index].in_check = in_checks[state.index]
			end
			square.piece = @pieces["Undefined"]
			others -= 1
		end
		square.piece = @pieces["Others"]
		last_steps = []
		square.states.each do |state|
			last_steps[state.index] = @colors[state.index].last_steps[state.move_index]
			if more_influent_step(state)
				@colors[state.index].last_steps[state.move_index] = state.step
			end
		end
		count_positions(threat_index+1, positions*others)
		square.states.each do |state|
			@colors[state.index].last_steps[state.move_index] = last_steps[state.index]
		end
		square.piece = @pieces["Undefined"]
	end

	def potential_check(state)
		influent_step(state) && state.count > 0 && !@colors[state.index].in_check
	end

	def influent_step(state)
		state.step <= @colors[state.index].last_steps[state.move_index]
	end

	def more_influent_step(state)
		state.step < @colors[state.index].last_steps[state.move_index]
	end

	def search_color_threat(color, move_index)
		target_index = color.king_square.index-@moves[move_index]
		while @mem_squares[target_index].piece == @pieces["Empty"]
			target_index -= @moves[move_index]
		end
		@mem_squares[target_index].piece == color.threat_piece
	end

	def output_chessboard
		for row in @@mem_offset..@@mem_offset+@rows-1
			for column in @@mem_offset..@@mem_offset+@columns-1
				putc(@mem_squares[square_index(row, column)].piece.symbol)
			end
			puts
		end
	end

	def output_positions_sum
		puts("Positions #{@positions_sum}")
	end
end

if ARGV.size != 2 || !ARGV[0].is_integer? || !ARGV[1].is_integer? || ARGV[0].to_i < 1 || ARGV[1].to_i < 1 || ARGV[0].to_i*ARGV[1].to_i < 2
	exit false
end
chess_positions = ChessPositions.new(ARGV[0].to_i, ARGV[1].to_i)
chess_positions.run
chess_positions.output_positions_sum
