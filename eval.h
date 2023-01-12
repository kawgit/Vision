#pragma once

#include "types.h"
#include "pos.h"
#include <functional>

using namespace std;

extern vector<float> piece_eval;

inline Eval get_piece_eval(Piece p) {
    return piece_eval[p - PAWN];
}

inline int sqMapTrans(int sq) { return rc(7-(sq/8), sq%8); }

Eval eval_pos(Pos& p, Eval LB, Eval UB, bool debug = false);

struct Factor {
    string name;
    function<Eval(Pos&, Color)> func;

    Factor(string n, function<Eval(Pos&, Color)> f) {
        name = n;
        func = f;
    }
};