/* Avaliacao PROVISORIA (material + posicao simples).
   Quando o nnue.bin ficar pronto, so esta funcao troca. */
#include <stdlib.h>
#include "nnue.h"

static const int VAL[7] = {0, 100, 320, 330, 500, 900, 0};

int evaluate(const Pos *p) {
    int score = 0, npm = 0;
    for (int sq = 0; sq < 64; sq++) {
        int t = abs(p->sq[sq]);
        if (t > PAWN && t < QUEEN + 1) npm += VAL[t];
    }
    for (int sq = 0; sq < 64; sq++) {
        int pc = p->sq[sq];
        if (!pc) continue;
        int t = abs(pc), r = sq / 8, f = sq % 8;
        int rr = pc > 0 ? r : 7 - r;                       /* linha do ponto de vista das brancas */
        int cen = 14 - (abs(2 * r - 7) + abs(2 * f - 7));  /* 0 (canto) .. 12 (centro) */
        int v = VAL[t];
        switch (t) {
            case PAWN:   v += (6 - rr) * 6 + cen / 2; break;
            case KNIGHT: v += cen * 4; break;
            case BISHOP: v += cen * 2; break;
            case ROOK:   v += (rr == 1) ? 10 : 0; break;
            case QUEEN:  v += cen; break;
            case KING:   v += (npm < 1300) ? cen * 4 : -cen * 2 + (rr == 7 ? 15 : 0); break;
        }
        score += pc > 0 ? v : -v;
    }
    return (p->side == WHITE ? score : -score) + 10;
}
