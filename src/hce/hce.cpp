#include "../pos.h"
#include "hce.h"

const Eval psqt_early[6][64] = {
    {
        0,  0,  0,  0,  0,  0,  0,  0,
       70, 70, 70, 70, 70, 70, 70, 70,
       50, 50, 50, 50, 50, 50, 50, 50,
       10, 10, 30, 30, 30, 10, 10, 10,
        0,  0, 15, 15, 15, 15,  0,  0,
        0,  0,  7,  7,  7,  7,  0,  0,
        0,  0,  0,  0,  0, 20,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
    },
    {
      -10,-10,-10,-10,-10,-10,-10,-10,
       10, 10, 10, 10, 10, 10, 10, 10,
       20, 50, 50, 50, 50, 50, 50, 20,
       10, 10, 20, 30, 30, 20, 10, 10,
      -30,  0, 15, 20, 20, 15,  0,-30,
      -30,  0, 10, 15, 15, 10,  0,-30,
      -40,-20,  0,  0,  0,  0,-20,-40,
      -40,-20,-20,-20,-20,-20,-20,-40,
    },
    {
       20,  0,  0,  0,  0,  0,  0, 20,
        0, 30, 10, 10, 10, 10, 30,  0,
        0, 10, 40, 10, 10, 40, 10,  0,
        0, 10, 10, 50, 50, 10, 10,  0,
        0, 10, 10, 50, 50, 10, 10,  0,
        0, 10, 40, 10, 10, 40, 10,  0,
        0, 30, 10, 10, 10, 10, 30,  0,
       20,  0,  0,  0,  0,  0,  0, 20,
    },
    {
        0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
        0, 10, 10, 10, 10, 10, 10,  0,
    },
    {
        20,  0,  0,  0,  0,  0,  0, 20,
         0, 30, 10, 10, 10, 10, 30,  0,
         0, 10, 40, 10, 10, 40, 10,  0,
         0, 10, 10, 50, 50, 10, 10,  0,
         0, 10, 10, 50, 50, 10, 10,  0,
         0, 10, 40, 10, 10, 40, 10,  0,
         0, 30, 10, 10, 10, 10, 30,  0,
        20,  0,  0,  0,  0,  0,  0, 20,
    },
    {
        -200,-200,-200,-200,-200,-200,-200,-200,
        -200,-200,-200,-200,-200,-200,-200,-200,
        -200,-200,-200,-200,-200,-200,-200,-200,
        -100,-100,-100,-100,-100,-100,-100,-100,
        -50,-50,-50,-50,-50,-50,-50,-50,
        -10,-20,-20,-20,-20,-20,-20,-10,
        20, 20,  0,  0,  0,  0, 20, 20,
        20, 30, 10,  0,  0, 10, 30, 20
    },
};

const Eval psqt_late[6][64] = {
    {
        0,  0,  0,  0,  0,  0,  0,  0,
       70, 70, 70, 70, 70, 70, 70, 70,
       50, 50, 50, 50, 50, 50, 50, 50,
       10, 10, 30, 30, 30, 10, 10, 10,
        0,  0, 15, 15, 15, 15,  0,  0,
        0,  0,  7,  7,  7,  7,  0,  0,
        0,  0,  0,  0,  0, 20,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
    },
    {
      -10,-10,-10,-10,-10,-10,-10,-10,
       10, 10, 10, 10, 10, 10, 10, 10,
       20, 50, 50, 50, 50, 50, 50, 20,
       10, 10, 20, 30, 30, 20, 10, 10,
      -30,  0, 15, 20, 20, 15,  0,-30,
      -30,  0, 10, 15, 15, 10,  0,-30,
      -40,-20,  0,  0,  0,  0,-20,-40,
      -40,-20,-20,-20,-20,-20,-20,-40,
    },
    {
       20,  0,  0,  0,  0,  0,  0, 20,
        0, 30, 10, 10, 10, 10, 30,  0,
        0, 10, 40, 10, 10, 40, 10,  0,
        0, 10, 10, 50, 50, 10, 10,  0,
        0, 10, 10, 50, 50, 10, 10,  0,
        0, 10, 40, 10, 10, 40, 10,  0,
        0, 30, 10, 10, 10, 10, 30,  0,
       20,  0,  0,  0,  0,  0,  0, 20,
    },
    {
        0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
        0, 10, 10, 10, 10, 10, 10,  0,
    },
    {
        20,  0,  0,  0,  0,  0,  0, 20,
         0, 30, 10, 10, 10, 10, 30,  0,
         0, 10, 40, 10, 10, 40, 10,  0,
         0, 10, 10, 50, 50, 10, 10,  0,
         0, 10, 10, 50, 50, 10, 10,  0,
         0, 10, 40, 10, 10, 40, 10,  0,
         0, 30, 10, 10, 10, 10, 30,  0,
        20,  0,  0,  0,  0,  0,  0, 20,
    },
    {
        -10,-10,-10,-10,-10,-10,-10,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,-10,-10,-10,-10,-10,-10,-10,
    },
};
    
Eval evaluate_side(const Pos& pos, const Color side, float phase) {
    Eval eval = bitcount(pos.pieces(side, PAWN))   * 100
              + bitcount(pos.pieces(side, KNIGHT)) * 280
              + bitcount(pos.pieces(side, BISHOP)) * 300
              + bitcount(pos.pieces(side, ROOK))   * 500
              + bitcount(pos.pieces(side, QUEEN))  * 900;

    Eval eval_early = 0;
    Eval eval_late = 0;

    for (Piece piece = PAWN; piece <= KING; piece++) {

        BB squares = pos.pieces(side, piece);

        while (squares) {
            Square square = flip_components(poplsb(squares), side == WHITE, false);
            eval_early += psqt_early[piece][square];
            eval_late  += psqt_late [piece][square];
        }

    }

    eval += Eval(eval_early * (1 - phase) + eval_late * phase);

    return eval;
}

float get_phase(const Pos& pos) {
    return float(bitcount(pos.pieces(KNIGHT)) * 3
               + bitcount(pos.pieces(BISHOP)) * 5
               + bitcount(pos.pieces(ROOK))   * 10
               + bitcount(pos.pieces(QUEEN))  * 20) / 112.0f;
}

Eval Evaluator::evaluate(const Pos& pos) {
    float phase = get_phase(pos);
    return evaluate_side(pos, pos.turn(), phase) - evaluate_side(pos, pos.notturn(), phase);
}