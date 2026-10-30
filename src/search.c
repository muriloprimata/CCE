#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "search.h"
#include "nnue.h"

#define MAXPLY 64
#define INF 32000
#define MATE 30000
#define SAME(a, b) ((a).from == (b).from && (a).to == (b).to && (a).promo == (b).promo)

static long long nodes;
static int stopped, have_prev;
static const Limits *L;
static double t0;
static Move killers[MAXPLY][2], root_prev, root_best;

double now_sec(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void check_stop(void) {
    if (L->max_nodes && nodes >= L->max_nodes) stopped = 1;
    if (L->deadline > 0 && now_sec() >= L->deadline) stopped = 1;
}

static int is_cap(const Pos *p, Move m) { return p->sq[m.to] != 0 || (m.flags & F_EP); }

static int move_score(const Pos *p, Move m, int ply) {
    if (ply == 0 && have_prev && SAME(m, root_prev)) return 1000000;
    int s = 0;
    if (is_cap(p, m)) {
        int v = (m.flags & F_EP) ? PAWN : abs(p->sq[m.to]);
        s = 100000 + v * 16 - abs(p->sq[m.from]);
    } else if (SAME(m, killers[ply][0])) s = 90000;
    else if (SAME(m, killers[ply][1])) s = 80000;
    if (m.promo) s += 95000 + m.promo * 10;
    return s;
}

static void sort_moves(const Pos *p, Move *m, int n, int ply) {
    int sc[MAX_MOVES];
    for (int i = 0; i < n; i++) sc[i] = move_score(p, m[i], ply);
    for (int i = 0; i < n - 1; i++) {
        int b = i;
        for (int j = i + 1; j < n; j++) if (sc[j] > sc[b]) b = j;
        if (b != i) { int ts = sc[i]; sc[i] = sc[b]; sc[b] = ts; Move tm = m[i]; m[i] = m[b]; m[b] = tm; }
    }
}

static int qsearch(Pos *p, int alpha, int beta, int ply) {
    if ((++nodes & 2047) == 0) check_stop();
    if (stopped) return 0;
    if (ply >= MAXPLY - 1) return evaluate(p);
    int stand = evaluate(p);
    if (stand >= beta) return stand;
    if (stand > alpha) alpha = stand;
    Move m[MAX_MOVES];
    int n = gen_legal(p, m), k = 0;
    for (int i = 0; i < n; i++) if (is_cap(p, m[i]) || m[i].promo) m[k++] = m[i];
    sort_moves(p, m, k, ply);
    for (int i = 0; i < k; i++) {
        Undo u;
        make_move(p, m[i], &u);
        int s = -qsearch(p, -beta, -alpha, ply + 1);
        undo_move(p, m[i], &u);
        if (stopped) return 0;
        if (s >= beta) return s;
        if (s > alpha) alpha = s;
    }
    return alpha;
}

static int negamax(Pos *p, int depth, int alpha, int beta, int ply) {
    if (stopped) return 0;
    if ((++nodes & 2047) == 0) check_stop();
    if (ply > 0 && p->halfmove >= 100) return 0;
    if (ply >= MAXPLY - 1) return evaluate(p);
    int incheck = in_check(p);
    if (incheck) depth++;
    if (depth <= 0) return qsearch(p, alpha, beta, ply);
    Move m[MAX_MOVES];
    int n = gen_legal(p, m);
    if (!n) return incheck ? -MATE + ply : 0;
    sort_moves(p, m, n, ply);
    int best = -INF;
    for (int i = 0; i < n; i++) {
        int quiet = !is_cap(p, m[i]) && !m[i].promo;
        Undo u;
        make_move(p, m[i], &u);
        int s = -negamax(p, depth - 1, -beta, -alpha, ply + 1);
        undo_move(p, m[i], &u);
        if (stopped) return 0;
        if (s > best) { best = s; if (ply == 0) root_best = m[i]; }
        if (s > alpha) {
            alpha = s;
            if (alpha >= beta) {
                if (quiet && !SAME(m[i], killers[ply][0])) { killers[ply][1] = killers[ply][0]; killers[ply][0] = m[i]; }
                break;
            }
        }
    }
    return best;
}

Result search(Pos *p, const Limits *lim) {
    Result r; memset(&r, 0, sizeof r);
    L = lim; nodes = 0; stopped = 0; have_prev = 0; t0 = now_sec();
    memset(killers, 0, sizeof killers);
    Move m[MAX_MOVES];
    int n = gen_legal(p, m);
    if (!n) return r;
    r.best = m[0]; r.has_move = 1;
    if (n == 1) return r;
    int maxd = lim->max_depth > 0 ? lim->max_depth : MAXPLY - 2;
    if (maxd > MAXPLY - 2) maxd = MAXPLY - 2;
    for (int d = 1; d <= maxd; d++) {
        int s = negamax(p, d, -INF, INF, 0);
        if (stopped) break;
        r.best = root_best; r.score = s; r.depth = d; r.nodes = nodes;
        root_prev = root_best; have_prev = 1;
        double el = now_sec() - t0;
        char b[8]; move_to_str(root_best, b);
        printf("info depth %d ", d);
        if (abs(s) > MATE - MAXPLY) { int mv = (MATE - abs(s) + 1) / 2; printf("score mate %d ", s > 0 ? mv : -mv); }
        else printf("score cp %d ", s);
        printf("nodes %lld time %d nps %lld pv %s\n", nodes, (int)(el * 1000),
               el > 0 ? (long long)(nodes / el) : 0, b);
        fflush(stdout);
        if (abs(s) > MATE - MAXPLY) break;
        if (lim->deadline > 0 && el > 0.4 * (lim->deadline - t0)) break;
    }
    r.nodes = nodes;
    return r;
}
