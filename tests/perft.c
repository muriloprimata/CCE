#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "board.h"

static long long perft(Pos *p, int d) {
    Move m[MAX_MOVES];
    int n = gen_legal(p, m);
    if (d == 1) return n;
    long long c = 0; Undo u;
    for (int i = 0; i < n; i++) {
        make_move(p, m[i], &u); c += perft(p, d - 1); undo_move(p, m[i], &u);
    }
    return c;
}

typedef struct { const char *nome, *fen; int depth; long long exp[6]; } T;
static const T tests[] = {
  {"inicial", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, {0,20,400,8902,197281,4865609}},
  {"kiwipete", "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", 4, {0,48,2039,97862,4085603}},
  {"pos3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 5, {0,14,191,2812,43238,674624}},
  {"pos4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, {0,6,264,9467,422333}},
  {"pos5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, {0,44,1486,62379,2103487}},
};

int main(int argc, char **argv) {
    Pos p;
    if (argc >= 4 && !strcmp(argv[1], "div")) {   /* ./perft div "<fen>" <depth> */
        if (!pos_from_fen(&p, argv[2])) { puts("FEN invalido"); return 1; }
        int d = atoi(argv[3]); Move m[MAX_MOVES]; Undo u; long long tot = 0;
        int n = gen_legal(&p, m);
        for (int i = 0; i < n; i++) {
            char b[8]; move_to_str(m[i], b);
            make_move(&p, m[i], &u);
            long long c = d > 1 ? perft(&p, d - 1) : 1;
            undo_move(&p, m[i], &u);
            printf("%s: %lld\n", b, c); tot += c;
        }
        printf("total: %lld\n", tot);
        return 0;
    }
    int cap = argc > 1 ? atoi(argv[1]) : 99, fail = 0;
    for (unsigned i = 0; i < sizeof tests / sizeof *tests; i++) {
        pos_from_fen(&p, tests[i].fen);
        for (int d = 1; d <= tests[i].depth && d <= cap; d++) {
            clock_t t0 = clock();
            long long got = perft(&p, d);
            int ok = got == tests[i].exp[d]; if (!ok) fail = 1;
            printf("%-9s d%d: %lld %s (%.2fs)\n", tests[i].nome, d, got,
                   ok ? "OK" : "FALHOU", (double)(clock() - t0) / CLOCKS_PER_SEC);
        }
    }
    puts(fail ? "ALGO ERRADO" : "TUDO CERTO");
    return fail;
}
