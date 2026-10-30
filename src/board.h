#ifndef BOARD_H
#define BOARD_H
#include <stdint.h>

enum { EMPTY = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6 };
#define WHITE 1
#define BLACK (-1)
enum { CR_WK = 1, CR_WQ = 2, CR_BK = 4, CR_BQ = 8 };
enum { F_NONE = 0, F_EP = 1, F_CASTLE = 2 };

typedef struct { int8_t from, to, promo, flags; } Move;

typedef struct {
    int8_t sq[64];          /* a8 = 0 ... h1 = 63; + brancas, - pretas */
    int side, castle, ep, halfmove, fullmove;
    int ksq[2];             /* [0] rei branco, [1] rei preto */
} Pos;

typedef struct { int castle, ep, halfmove; int8_t captured; } Undo;

#define MAX_MOVES 256

void pos_startpos(Pos *p);
int  pos_from_fen(Pos *p, const char *fen);
int  sq_attacked(const Pos *p, int sq, int by);
int  in_check(const Pos *p);
int  gen_legal(Pos *p, Move *out);
void make_move(Pos *p, Move m, Undo *u);
void undo_move(Pos *p, Move m, const Undo *u);
void move_to_str(Move m, char *buf);          /* buf com >= 6 bytes */
int  move_from_str(Pos *p, const char *s, Move *out);

#endif
