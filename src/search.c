#include "abc.h"

// Position evaluation
int evaluate_position()
{   
    // Init params
    int game_phase = -1;
    int piece_scores = 0;
    int game_phase_score = 0;
    int score = 0;
    int score_opening = 0;
    int score_endgame = 0;
    int knights = 0;
    int bishops = 0;
    int rooks = 0;
    int queens = 0;
    
    // Evaluate material & positional scores
    for (int square = 0; square < 128; square++) {
        if (!(square & 0x88)) {
            int piece = board[square];
            score_opening += material_score[OPENING][piece];
            score_endgame += material_score[ENDGAME][piece];
			switch(piece) {
				case WK:
                    score_opening += positional_score[OPENING][KING][square];
                    score_endgame += positional_score[ENDGAME][KING][square];
                    break;
                case WP:
                    score_opening += positional_score[OPENING][PAWN][square];
                    score_endgame += positional_score[ENDGAME][PAWN][square];
                    break;
				case WN:
                    score_opening += positional_score[OPENING][KNIGHT][square];
                    score_endgame += positional_score[ENDGAME][KNIGHT][square];
                    knights++;
                    break;
				case WB:
                    score_opening += positional_score[OPENING][BISHOP][square];
                    score_endgame += positional_score[ENDGAME][BISHOP][square];
                    bishops++;
                    break;
				case WR:
                    score_opening += positional_score[OPENING][ROOK][square];
                    score_endgame += positional_score[ENDGAME][ROOK][square];
                    rooks++;
                    break;
                case WQ:
                    score_opening += positional_score[OPENING][QUEEN][square];
                    score_endgame += positional_score[ENDGAME][QUEEN][square];
                    queens++;
                    break;
                case BK:
                    score_opening -= positional_score[OPENING][KING][mirror_score[square]];
                    score_endgame -= positional_score[ENDGAME][KING][mirror_score[square]];
                    break;
                case BP:
                    score_opening -= positional_score[OPENING][PAWN][mirror_score[square]];
                    score_endgame -= positional_score[ENDGAME][PAWN][mirror_score[square]];
                    break;
				case BN:
                    score_opening -= positional_score[OPENING][KNIGHT][mirror_score[square]];
                    score_endgame -= positional_score[ENDGAME][KNIGHT][mirror_score[square]];
                    knights++;
                    break;
				case BB:
                    score_opening -= positional_score[OPENING][BISHOP][mirror_score[square]];
                    score_endgame -= positional_score[ENDGAME][BISHOP][mirror_score[square]];
                    bishops++;
                    break;
				case BR:
                    score_opening -= positional_score[OPENING][ROOK][mirror_score[square]];
                    score_endgame -= positional_score[ENDGAME][ROOK][mirror_score[square]];
                    rooks++;
                    
                    break;
                case BQ:
                    score_opening -= positional_score[OPENING][QUEEN][mirror_score[square]];
                    score_endgame -= positional_score[ENDGAME][QUEEN][mirror_score[square]];
                    queens++;
                    break;
			}
        }
    }
    
    // Calculate game phase
    game_phase_score = knights * material_score[OPENING][KNIGHT];
    game_phase_score += bishops * material_score[OPENING][BISHOP];
    game_phase_score += rooks * material_score[OPENING][ROOK];
    game_phase_score += queens * material_score[OPENING][QUEEN];
    if (game_phase_score > opening_phase_score) game_phase = OPENING;
    else if (game_phase_score < endgame_phase_score) game_phase = ENDGAME;
    else game_phase = MIDDLEGAME;
    
    // Calculate final score
    if (game_phase == MIDDLEGAME)
        score = (
            score_opening * game_phase_score +
            score_endgame * (opening_phase_score - game_phase_score)
        ) / opening_phase_score;
    else if (game_phase == OPENING) score = score_opening;
    else if (game_phase == ENDGAME) score = score_endgame;
    return (side == WHITE) ? score : -score;
}

// Score urgency for move ordering
int score_move(int move) {
    if (pv_table[0][ply] == move) return 20000;
    int score = mvv_lva[board[get_move_source(move)]][board[get_move_target(move)]];         
    if (get_move_capture(move)) score += 10000;
    else {
        if (killer_moves[0][ply] == move) score = 9000;
        else if (killer_moves[1][ply] == move) score = 8000;
        else score = history_moves[board[get_move_source(move)]][get_move_target(move)] + 7000;
    } return score;
}

// Sort moves based on urgency
void sort_moves(Movelist *moves) {
    int move_scores[moves->count];
    for (int count = 0; count < moves->count; count++)
        move_scores[count] = score_move(moves->moves[count]);
    for (int current = 0; current < moves->count; current++) {
        for (int next = current + 1; next < moves->count; next++) {
            if (move_scores[current] < move_scores[next]) {
                int temp_score = move_scores[current];
                move_scores[current] = move_scores[next];
                move_scores[next] = temp_score;
                int temp_move = moves->moves[current];
                moves->moves[current] = moves->moves[next];
                moves->moves[next] = temp_move;
            }
        }
    }    
}

// Position repetition detection
int is_repetition() {
    for (int index = 0; index < repetition_index; index++)
        if (repetition_table[index] == generate_hash_key()) return 1;
    return 0;
}

// Quiescence search
int quiescence_search(int alpha, int beta) {
    // Listen to UCI "stop" command
    if ((nodes & 2047 ) == 0) communicate();
    
    // Count nodes
    nodes++;
    
    // Static evaluation
    int eval = evaluate_position();
    if (eval >= beta) return beta;
    if (eval > alpha) alpha = eval;
    
    // Generate moves
    Movelist moves[1];
    generate_moves(moves);
    
    // Move ordering
    sort_moves(moves);
    
    // Search best move
    for (int count = 0; count < moves->count; count++) {      
        Position position;
        save_position(&position); ply++;
        if (!make_move(moves->moves[count], ONLY_CAPTURES)) { ply--; continue; }
        int score = -quiescence_search(-beta, -alpha);
        restore_position(&position); ply--;
        if (stopped == 1) break;
        if (score > alpha) {
            alpha = score;
            if (score >= beta) return beta;
        }
    } return alpha;
}

// Negamax search
int negamax_search(int alpha, int beta, int depth) {
    // Init params
    int legal_moves = 0;
    int old_alpha = alpha;
    pv_length[ply] = ply;
    
    // 3 fold repetition detection
    if (ply && is_repetition()) return 0;
    
    // Listen to UCI "stop" command
    if ((nodes & 2047 ) == 0) communicate();
    
    // Search until no captures left
    if (!depth) return quiescence_search(alpha, beta);
    
    // Count nodes
    nodes++;
    
    // Search deeper if in check
    int in_check = is_square_attacked(king_square[side], side ^ 1);
    if (in_check) depth++;
    
    // Generate moves
    Movelist moves[1];
    generate_moves(moves);
    
    // Move ordering
    sort_moves(moves);
    
    // Search best move
    for (int count = 0; count < moves->count; count++) {
        int move = moves->moves[count];
        Position position;
        save_position(&position); ply++;
        repetition_index++;
        repetition_table[repetition_index] = generate_hash_key();
        if (!make_move(move, ALL_MOVES)) { ply--; repetition_index--; continue; }
        legal_moves++; int score = 0;
        score = -negamax_search(-beta, -alpha, depth - 1);
        restore_position(&position); ply--; repetition_index--;
        if (stopped == 1) break;
        if (score > alpha) {
            history_moves[board[get_move_source(move)]][get_move_target(move)] += depth;
            alpha = score;
			pv_table[ply][ply] = move;
			for (int i = ply + 1; i < pv_length[ply + 1]; i++) pv_table[ply][i] = pv_table[ply + 1][i];
			pv_length[ply] = pv_length[ply + 1];
            if (score >= beta) {
                killer_moves[1][ply] = killer_moves[0][ply];
                killer_moves[0][ply] = move;
                return beta;
            }
        }      
    }
    
    // Checkmate / Stalemate detection
    if (!legal_moves) {
        if (in_check) return -49000 + ply;
        else return 0;
    } return alpha;
}

// search position
int search_position(int depth)
{
    int start = get_time_ms();
    // Clear search
    nodes = 0;
    stopped = 0;
    ply = 0;
    memset(pv_table, 0, sizeof(pv_table));
    memset(pv_length, 0, sizeof(pv_length));
    memset(killer_moves, 0, sizeof(killer_moves));
    memset(history_moves, 0, sizeof(history_moves));
    
    // Iterative deepening
    for (int current_depth = 1; current_depth <= depth; current_depth++) {    
        // Search position with current depth
	    int score = negamax_search(-50000, 50000, current_depth);
        if (stopped == 1) break;
        
        // Output UCI info
        if (score > -49000 && score < -48000)
            printf("info score mate %d depth %d nodes %ld time %d pv ", -(score + 49000) / 2 - 1, current_depth, nodes, get_time_ms() - start);
        else if (score > 48000 && score < 49000)
            printf("info score mate %d depth %d nodes %ld time %d pv ", (49000 - score) / 2 + 1, current_depth, nodes, get_time_ms() - start);   
        else printf("info score cp %d depth %d nodes %ld time %d pv ", score, current_depth, nodes, get_time_ms() - start);
        
        // Print PV line
        for (int i = 0; i < pv_length[0]; i++) {
            int move = pv_table[0][i];
            print_move(get_move_source(move), get_move_target(move), get_move_promoted(move));
            if (i < pv_length[0]-1) printf(" ");
        } printf("\n"); fflush(stdout);
    }

	// print best move
    int move = pv_table[0][0];
    printf("bestmove ");
    print_move(get_move_source(move), get_move_target(move), get_move_promoted(move));
    printf("\n");
}