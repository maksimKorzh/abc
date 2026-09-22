#include "abc.h"

// Parse FEN and set up board
void set_board(char *fen) {
    // Clear board
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 16; file++) {
            int square = rank * 16 + file;
            if (!(square & 0x88)) board[square] = EMPTY;
        }
    }
    
    // Clear game state
    side = NONE;
    castle = EMPTY;
    enpassant = NONE;
    
    // Reset repetition index
    repetition_index = 0;
    
    // Reset repetition table
    memset(repetition_table, 0ULL, sizeof(repetition_table));
    
    // Set up pieces
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 16; file++) {
            int square = rank * 16 + file;
            if (!(square & 0x88)) {
                if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')) {
                    if (*fen == 'K') king_square[WHITE] = square;
                    else if (*fen == 'k') king_square[BLACK] = square;
                    board[square] = char_pieces[*fen];
                    *fen++;
                } if (*fen >= '0' && *fen <= '9') {
                    int offset = *fen - '0';
                    if (!(board[square])) file--;
                    file += offset;
                    *fen++;
                } if (*fen == '/') *fen++;
            }
        }
    }
    
    // Set up side to move
    *fen++;
    side = (*fen == 'w') ? WHITE : BLACK;
    fen += 2;
    
    // Set up castling rights
    while (*fen != ' ') {
        switch(*fen) {
            case 'K': castle |= WKC; break;
            case 'Q': castle |= WQC; break;
            case 'k': castle |= BKC; break;
            case 'q': castle |= BQC; break;
            case '-': break;
        } *fen++;
    }
    
    // Set up enpassant square
    *fen++;
    if (*fen != '-') {
        int file = fen[0] - 'a';
        int rank = 8 - (fen[1] - '0');
        enpassant = rank * 16 + file;
    } else enpassant = NONE;   
}

// Print board to console
void print_board() {
    // Print board
    printf("\n");
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 16; file++) {
            int square = rank * 16 + file;
            if (file == 0) printf(" %d  ", 8 - rank);
            if (!(square & 0x88)) printf("%c ", ascii_pieces[board[square]]);
        } printf("\n");
    }
    
    // Get castling rights
    int K = castle & WKC;
    int Q = castle & WQC;
    int k = castle & BKC;
    int q = castle & BQC;

    // Print game state
    printf("\n    a b c d e f g h\n\n");
    printf("    Side:     %s\n", (side == WHITE) ? "white": "black");
    printf("    Castling:  %c%c%c%c\n", K ? 'K' : '-', Q ? 'Q' : '-', k ? 'k' : '-', q ? 'q' : '-');
    printf("    Enpassant:   %s\n", (enpassant == NONE)? "no" : square_to_coords[enpassant]);
    printf("    King square: %s\n", square_to_coords[king_square[side]]);
    printf("    Hash:        %d\n\n", generate_hash_key());
}

// Print move
void print_move(int source, int target, int promoted) {
    char *src = square_to_coords[source];
    char *dst = square_to_coords[target];
    if (promoted) printf("%s%s%c", src, dst, promoted_pieces[promoted]);
    else printf("%s%s", src, dst);
}