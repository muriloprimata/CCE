#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "board.h"

static const int KN[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
/* 0-3 retas (torre), 4-7 diagonais (bispo) */
static const int DIR[8][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{-1,1},{1,-1},{1,1}};

static int inb(int r, int f) { return r >= 0 && r < 8 && f >= 0 && f < 8; }
static int cidx(int side) { return side == WHITE ? 0 : 1; }

void pos_startpos(Pos *p) {
    pos_from_fen(p, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

int pos_from_fen(Pos *p, const char *fen) {
    memset(p, 0, sizeof *p);
    p->ep = -1; p->fullmove = 1; p->ksq[0] = p->ksq[1] = -1;
    int r = 0, f = 0;
    const char *c = fen;
    for (; *c && *c != ' '; c++) {
        if (*c == '/') { r++; f = 0; continue; }
        if (*c >= '1' && *c <= '8') { f += *c - '0'; continue; }
        int t;
        switch (tolower((unsigned char)*c)) {
            case 'p': t = PAWN; break;   case 'n': t = KNIGHT; break;
            case 'b': t = BISHOP; break; case 'r': t = ROOK; break;
            case 'q': t = QUEEN; break;  case 'k': t = KING; break;
            default: return 0;
        }
        if (r > 7 || f > 7) return 0;
        int white = isupper((unsigned char)*c) != 0;
        p->sq[r * 8 + f] = white ? t : -t;
        if (t == KING) p->ksq[white ? 0 : 1] = r * 8 + f;
        f++;
    }
    char s[8] = "", cs[8] = "", eps[8] = "";
    int hm = 0, fm = 1;
    if (sscanf(c, " %7s %7s %7s %d %d", s, cs, eps, &hm, &fm) < 3) return 0;
    p->side = (s[0] == 'b') ? BLACK : WHITE;
    for (char *x = cs; *x; x++) {
        switch (*x) {
            case 'K': p->castle |= CR_WK; break; case 'Q': p->castle |= CR_WQ; break;
            case 'k': p->castle |= CR_BK; break; case 'q': p->castle |= CR_BQ; break;
        }
    }
    if (eps[0] >= 'a' && eps[0] <= 'h' && eps[1] >= '1' && eps[1] <= '8')
        p->ep = (8 - (eps[1] - '0')) * 8 + (eps[0] - 'a');
    p->halfmove = hm; p->fullmove = fm;
    return p->ksq[0] >= 0 && p->ksq[1] >= 0;
}

int sq_attacked(const Pos *p, int sq, int by) {
    int r = sq / 8, f = sq % 8;
    int pr = r + (by == WHITE ? 1 : -1);
    for (int df = -1; df <= 1; df += 2)
        if (inb(pr, f + df) && p->sq[pr * 8 + f + df] == by * PAWN) return 1;
    for (int i = 0; i < 8; i++) {
        int rr = r + KN[i][0], ff = f + KN[i][1];
        if (inb(rr, ff) && p->sq[rr * 8 + ff] == by * KNIGHT) return 1;
    }
    for (int i = 0; i < 8; i++) {
        int rr = r + DIR[i][0], ff = f + DIR[i][1];
        if (inb(rr, ff) && p->sq[rr * 8 + ff] == by * KING) return 1;
        while (inb(rr, ff)) {
            int q = p->sq[rr * 8 + ff];
            if (q) {
                if (q == by * QUEEN) return 1;
                if (i < 4 && q == by * ROOK) return 1;
                if (i >= 4 && q == by * BISHOP) return 1;
                break;
            }
            rr += DIR[i][0]; ff += DIR[i][1];
        }
    }
    return 0;
}

int in_check(const Pos *p) { return sq_attacked(p, p->ksq[cidx(p->side)], -p->side); }

static int add(Move *m, int n, int from, int to, int promo, int flags) {
    m[n].from = from; m[n].to = to; m[n].promo = promo; m[n].flags = flags;
    return n + 1;
}

static int gen_pseudo(const Pos *p, Move *m) {
    int n = 0, side = p->side;
    for (int sq = 0; sq < 64; sq++) {
        int pc = p->sq[sq];
        if (pc * side <= 0) continue;
        int t = abs(pc), r = sq / 8, f = sq % 8;
        if (t == PAWN) {
            int dr = -side, start = (side == WHITE) ? 6 : 1, last = (side == WHITE) ? 0 : 7;
            int rr = r + dr;
            if (inb(rr, f) && !p->sq[rr * 8 + f]) {
                if (rr == last) { for (int q = QUEEN; q >= KNIGHT; q--) n = add(m, n, sq, rr * 8 + f, q, 0); }
                else {
                    n = add(m, n, sq, rr * 8 + f, 0, 0);
                    if (r == start && !p->sq[(r + 2 * dr) * 8 + f])
                        n = add(m, n, sq, (r + 2 * dr) * 8 + f, 0, 0);
                }
            }
            for (int df = -1; df <= 1; df += 2) {
                if (!inb(rr, f + df)) continue;
                int to = rr * 8 + f + df;
                if (p->sq[to] * side < 0) {
                    if (rr == last) { for (int q = QUEEN; q >= KNIGHT; q--) n = add(m, n, sq, to, q, 0); }
                    else n = add(m, n, sq, to, 0, 0);
                } else if (to == p->ep) n = add(m, n, sq, to, 0, F_EP);
            }
        } else if (t == KNIGHT) {
            for (int i = 0; i < 8; i++) {
                int rr = r + KN[i][0], ff = f + KN[i][1];
                if (inb(rr, ff) && p->sq[rr * 8 + ff] * side <= 0) n = add(m, n, sq, rr * 8 + ff, 0, 0);
            }
        } else {
            int a = (t == BISHOP) ? 4 : 0, b = (t == ROOK) ? 4 : 8;
            for (int i = a; i < b; i++) {
                int rr = r + DIR[i][0], ff = f + DIR[i][1];
                while (inb(rr, ff)) {
                    int q = p->sq[rr * 8 + ff];
                    if (q * side > 0) break;
                    n = add(m, n, sq, rr * 8 + ff, 0, 0);
                    if (q || t == KING) break;
                    rr += DIR[i][0]; ff += DIR[i][1];
                }
            }
        }
    }
    /* roque */
    int e = (side == WHITE) ? 60 : 4, opp = -side;
    int ks = (side == WHITE) ? CR_WK : CR_BK, qs = (side == WHITE) ? CR_WQ : CR_BQ;
    if ((p->castle & ks) && !p->sq[e + 1] && !p->sq[e + 2] &&
        !sq_attacked(p, e, opp) && !sq_attacked(p, e + 1, opp) && !sq_attacked(p, e + 2, opp))
        n = add(m, n, e, e + 2, 0, F_CASTLE);
    if ((p->castle & qs) && !p->sq[e - 1] && !p->sq[e - 2] && !p->sq[e - 3] &&
        !sq_attacked(p, e, opp) && !sq_attacked(p, e - 1, opp) && !sq_attacked(p, e - 2, opp))
        n = add(m, n, e, e - 2, 0, F_CASTLE);
    return n;
}

static int crmask(int sq) {
    switch (sq) {
        case 0:  return ~CR_BQ;
        case 4:  return ~(CR_BK | CR_BQ);
        case 7:  return ~CR_BK;
        case 56: return ~CR_WQ;
        case 60: return ~(CR_WK | CR_WQ);
        case 63: return ~CR_WK;
        default: return ~0;
    }
}

void make_move(Pos *p, Move m, Undo *u) {
    int me = p->side, pc = p->sq[m.from], t = abs(pc), cap = p->sq[m.to];
    u->castle = p->castle; u->ep = p->ep; u->halfmove = p->halfmove;
    if (m.flags & F_EP) {
        int cs = m.to + (me == WHITE ? 8 : -8);
        cap = p->sq[cs]; p->sq[cs] = 0;
    }
    u->captured = (int8_t)cap;
    p->sq[m.to] = m.promo ? m.promo * me : pc;
    p->sq[m.from] = 0;
    if (m.flags & F_CASTLE) {
        if (m.to > m.from) { p->sq[m.from + 1] = p->sq[m.from + 3]; p->sq[m.from + 3] = 0; }
        else               { p->sq[m.from - 1] = p->sq[m.from - 4]; p->sq[m.from - 4] = 0; }
    }
    if (t == KING) p->ksq[cidx(me)] = m.to;
    p->ep = (t == PAWN && abs(m.to - m.from) == 16) ? (m.from + m.to) / 2 : -1;
    p->castle &= crmask(m.from) & crmask(m.to);
    p->halfmove = (t == PAWN || cap) ? 0 : p->halfmove + 1;
    if (me == BLACK) p->fullmove++;
    p->side = -me;
}

void undo_move(Pos *p, Move m, const Undo *u) {
    int me = -p->side;
    p->side = me;
    int pc = p->sq[m.to];
    if (m.promo) pc = PAWN * me;
    p->sq[m.from] = pc;
    if (m.flags & F_EP) {
        p->sq[m.to] = 0;
        p->sq[m.to + (me == WHITE ? 8 : -8)] = u->captured;
    } else p->sq[m.to] = u->captured;
    if (m.flags & F_CASTLE) {
        if (m.to > m.from) { p->sq[m.from + 3] = p->sq[m.from + 1]; p->sq[m.from + 1] = 0; }
        else               { p->sq[m.from - 4] = p->sq[m.from - 1]; p->sq[m.from - 1] = 0; }
    }
    if (abs(pc) == KING) p->ksq[cidx(me)] = m.from;
    p->castle = u->castle; p->ep = u->ep; p->halfmove = u->halfmove;
    if (me == BLACK) p->fullmove--;
}

int gen_legal(Pos *p, Move *out) {
    Move tmp[MAX_MOVES];
    int n = gen_pseudo(p, tmp), c = 0, me = p->side;
    for (int i = 0; i < n; i++) {
        Undo u;
        make_move(p, tmp[i], &u);
        if (!sq_attacked(p, p->ksq[cidx(me)], -me)) out[c++] = tmp[i];
        undo_move(p, tmp[i], &u);
    }
    return c;
}

void move_to_str(Move m, char *b) {
    b[0] = 'a' + m.from % 8; b[1] = '8' - m.from / 8;
    b[2] = 'a' + m.to % 8;   b[3] = '8' - m.to / 8;
    if (m.promo) { b[4] = "nbrq"[m.promo - 2]; b[5] = 0; } else b[4] = 0;
}

int move_from_str(Pos *p, const char *s, Move *out) {
    Move m[MAX_MOVES];
    int n = gen_legal(p, m);
    char b[8];
    for (int i = 0; i < n; i++) {
        move_to_str(m[i], b);
        size_t L = strlen(b);
        if (!strncmp(b, s, L) && (s[L] == 0 || isspace((unsigned char)s[L]))) { *out = m[i]; return 1; }
    }
    return 0;
}
