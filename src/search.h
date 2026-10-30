#ifndef SEARCH_H
#define SEARCH_H
#include "board.h"

typedef struct {
    int max_depth;          /* 0 = sem limite de profundidade */
    long long max_nodes;    /* 0 = sem limite de nos */
    double deadline;        /* now_sec() absoluto; 0 = sem limite de tempo */
} Limits;

typedef struct { Move best; int score, depth, has_move; long long nodes; } Result;

double now_sec(void);
Result search(Pos *p, const Limits *lim);
#endif
