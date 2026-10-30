#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "board.h"
#include "search.h"

static long long next_num(void) {
    char *t = strtok(NULL, " \t\r\n");
    return t ? atoll(t) : 0;
}

static void cmd_position(Pos *p, char *line) {
    char *t = line + 8;
    while (*t == ' ') t++;
    char *rest = NULL, *mv = strstr(t, " moves");
    if (mv) { *mv = 0; rest = mv + 6; }
    if (!strncmp(t, "startpos", 8)) pos_startpos(p);
    else if (!strncmp(t, "fen", 3)) { t += 3; while (*t == ' ') t++; pos_from_fen(p, t); }
    if (rest)
        for (char *tok = strtok(rest, " \t\r\n"); tok; tok = strtok(NULL, " \t\r\n")) {
            Move m; Undo u;
            if (move_from_str(p, tok, &m)) make_move(p, m, &u);
        }
}

static void cmd_go(Pos *p, char *line) {
    long long wtime = -1, btime = -1, winc = 0, binc = 0, movetime = -1, nodes = 0, budget = 0;
    int movestogo = 0, depth = 0, infinite = 0;
    strtok(line, " \t\r\n");                       /* "go" */
    for (char *tok; (tok = strtok(NULL, " \t\r\n")); ) {
        if (!strcmp(tok, "wtime")) wtime = next_num();
        else if (!strcmp(tok, "btime")) btime = next_num();
        else if (!strcmp(tok, "winc")) winc = next_num();
        else if (!strcmp(tok, "binc")) binc = next_num();
        else if (!strcmp(tok, "movestogo")) movestogo = (int)next_num();
        else if (!strcmp(tok, "movetime")) movetime = next_num();
        else if (!strcmp(tok, "depth")) depth = (int)next_num();
        else if (!strcmp(tok, "nodes")) nodes = next_num();
        else if (!strcmp(tok, "infinite")) infinite = 1;
    }
    double start = now_sec();
    if (movetime >= 0) { budget = movetime - 30; if (budget < 1) budget = 1; }
    else if (wtime >= 0 || btime >= 0) {          /* controle de tempo fica AQUI, nao na busca */
        long long left = p->side == WHITE ? wtime : btime;
        long long inc = p->side == WHITE ? winc : binc;
        if (left < 0) left = 0;
        budget = left / (movestogo > 0 ? movestogo : 30) + inc * 3 / 4;
        if (budget > left - 50) budget = left - 50;
        if (budget < 5) budget = 5;
    }
    Limits lim; memset(&lim, 0, sizeof lim);
    lim.max_depth = depth > 0 ? depth : (budget > 0 || nodes > 0 ? 0 : (infinite ? 20 : 6));
    lim.max_nodes = nodes;
    lim.deadline = budget > 0 ? start + budget / 1000.0 : 0;
    Result r = search(p, &lim);
    if (!r.has_move) { puts("bestmove 0000"); }
    else { char b[8]; move_to_str(r.best, b); printf("bestmove %s\n", b); }
    fflush(stdout);
}

int main(void) {
    char line[4096];
    Pos pos; pos_startpos(&pos);
    while (fgets(line, sizeof line, stdin)) {
        size_t n = strlen(line);
        while (n && isspace((unsigned char)line[n - 1])) line[--n] = 0;
        if (!strcmp(line, "uci")) {
            puts("id name Cartman 0.1"); puts("id author Cartman team"); puts("uciok");
        } else if (!strcmp(line, "isready")) puts("readyok");
        else if (!strcmp(line, "ucinewgame")) pos_startpos(&pos);
        else if (!strncmp(line, "position", 8)) cmd_position(&pos, line);
        else if (!strncmp(line, "go", 2) && (line[2] == 0 || line[2] == ' ')) cmd_go(&pos, line);
        else if (!strcmp(line, "quit")) break;
        fflush(stdout);
    }
    return 0;
}
