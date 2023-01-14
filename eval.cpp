#include "eval.h"
#include "types.h"
#include "search.h"
#include "pos.h"
#include "bits.h"
#include "movegen.h"
#include "search.h"
#include <iostream>
#include <vector>
#include <functional>
#include <iomanip>

using namespace std;

// const vector<const float> connectivity_victim_weights = {50, 35, 30, 10, 4, 0};

// const vector<const float> connectivity_weights = {};

vector<float> piece_eval = {100, 300, 320, 500, 900, 100};

Eval early_pawn_map[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
     20, 20, 20, 20, 20, 20, 20, 20,
     10, 10, 20, 30, 30, 20, 10, 10,
      5,  5, 10, 20, 20, 10,  5,  5,
      0,  0,  0, 10, 10,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,-10,-10,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0
};

Eval anytime_knight_map[64] = {
    -40,-30,-20,-20,-20,-20,-30, -40,
    -30,-10,  0,  0,  0,  0,-10, -30,
    -15,  0, 10, 15, 15, 10,  0, -15,
    -15,  0, 15, 20, 20, 15,  0, -15,
    -15,  0, 15, 20, 20, 15,  0, -15,
    -15,  0, 10, 15, 15, 10,  0, -15,
    -30,-10,  0,  5,  5,  0,-10, -30,
    -40,-30,-20,-20,-20,-20,-30, -40,
};

Eval early_bishop_map[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      0, 10, 10,  0,  0, 10, 10,  0,
      0, 10, 10, 10, 10, 10, 10,  0,
      0,  0, 10, 10, 10, 10,  0,  0,
      0,  0, 20, 10, 10, 20,  0,  0,
      0, 10, 10, 10, 10, 10, 10,  0,
      0, 20, 10,  0, 10, 10, 20,  0,
      0,  0,  0,  0,  0,  0,  0,  0,
};

Eval early_king_map[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     40, 40,-10,-10,-10,-10, 40, 40,
     40, 40,-10,-10,-10,-10, 40, 40,
};

Eval late_king_map[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10, -5,  0,  0,  0,  0, -5,-10,
    -10,  0, 15, 15, 15, 15,  0,-10,
    -5,   0, 15, 15, 15, 15,  0, -5,
    -5,   0, 15, 15, 15, 15,  0, -5,
    -10,  0, 15, 15, 15, 15,  0,-10,
    -10, -5,  0,  0,  0,  0, -5,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

vector<Factor> factors = {
    Factor("pawn_structure", [](Pos& pos, Color color) {

        const static Eval w1[8] = {0, 10, 20, 30, 70, 90, 120, 900};
        const static Eval w2[8] = {0, 40, 50, 60, 90, 120, 230, 900};
        const static Eval w3[8] = {0, 40, 50, 60, 90, 120, 340, 900};

        Eval res = 0;

        res -= bitcount(pos.isolated_pawns(color)) * 10;
        res -= bitcount(pos.doubled_pawns(color)) * 10;
        res -= bitcount(pos.blocked_pawns(color)) * 10;

        res += bitcount(pos.supported_pawns(color)) * 10;
        
        BB passed_pawns = pos.passed_pawns(color);

        if (passed_pawns) {

            BB supported_passed_pawns = passed_pawns & pos.supported_pawns(color);

            BB double_passed_pawns = passed_pawns & (files_of(passed_pawns) & files_of(shift<WEST>(passed_pawns)));

            while (passed_pawns) {
                Square sq = poplsb(passed_pawns);
                res += w1[rel_rank_of(color, sq)];
            }

            while (supported_passed_pawns) {
                Square sq = poplsb(supported_passed_pawns);
                res += w2[rel_rank_of(color, sq)];
            }

            while (double_passed_pawns) {
                Square sq = poplsb(double_passed_pawns);
                res += w3[rel_rank_of(color, sq)];
            }
        }

        return res;
    }),

    Factor("early_king", [](Pos& pos, Color color) {
        float weight = ((float) pos.ref_mat(opp(color))) / 3940;
        return (Eval)(weight * early_king_map[lsb(pos.ref_piece_mask(color, KING))]);
    }),

    Factor("late_king", [](Pos& pos, Color color) {
        float weight = 1 - ((float) pos.ref_mat(opp(color))) / 3940;
        return (Eval)(weight * late_king_map[lsb(pos.ref_piece_mask(color, KING))]);
    }),

    Factor("early_bishop", [](Pos& pos, Color color) {
        Eval result = 0;
        BB pieces = pos.ref_piece_mask(color, BISHOP);
        while (pieces) {
            Square from = poplsb(pieces);
            result += early_bishop_map[color == WHITE ? sqMapTrans(from) : from];
        }
        float weight = ((float) pos.ref_mat(opp(color))) / 3940;
        return (Eval)(weight * result);
    }),

    Factor("early_pawn", [](Pos& pos, Color color) {
        Eval result = 0;
        BB pieces = pos.ref_piece_mask(color, PAWN);
        while (pieces) {
            Square from = poplsb(pieces);
            result += early_pawn_map[color == WHITE ? sqMapTrans(from) : from];
        }
        float weight = ((float) pos.ref_mat(opp(color))) / 3940;
        return (Eval)(weight * result);
    }),

    Factor("anytime_knight", [](Pos& pos, Color color) {
        Eval result = 0;
        BB pieces = pos.ref_piece_mask(color, KNIGHT);
        while (pieces) {
            Square from = poplsb(pieces);
            result += anytime_knight_map[color == WHITE ? sqMapTrans(from) : from];
        }
        float weight = ((float) pos.ref_mat(opp(color))) / 3940;
        return (Eval)(weight * result);
    }),

    /*
    early game king
    end game king
    early game bishop
    early game pawn
    all game knight
    */

    // Factor("control", [](Pos& pos, Color color) {
        // Eval res = 0;
        // char cum[64] = {};

		// add_bb_to_cum(cum, shift<NORTH>(shift<WEST>(pos.ref_piece_mask(color, PAWN)) | shift<EAST>(pos.ref_piece_mask(color, PAWN))), 50);

        // {
        //     BB knights = pos.ref_piece_mask(color, KNIGHT);
        //     while (knights) {
        //         int from = poplsb(knights);
        //         add_bb_to_cum(cum, get_knight_atk(from), 30);
        //     }
        // }

        // {
        //     BB occ = pos.get_occ() & ~(pos.ref_piece_mask(opp(color), KING) | pos.ref_piece_mask(color, BISHOP) | pos.ref_piece_mask(color, QUEEN));
        //     BB bishops = pos.ref_piece_mask(color, BISHOP) | pos.ref_piece_mask(color, QUEEN);
        //     while (bishops) {
        //         int from = poplsb(bishops);
        //         add_bb_to_cum(cum, get_bishop_atk(from, occ), 25);
        //     }
        // }

        // {
        //     BB occ = pos.get_occ() & ~(pos.ref_piece_mask(opp(color), KING) | pos.ref_piece_mask(color, ROOK) | pos.ref_piece_mask(color, QUEEN));
        //     BB rooks = pos.ref_piece_mask(color, ROOK) | pos.ref_piece_mask(color, QUEEN);
        //     while (rooks) {
        //         int from = poplsb(rooks);
        //         add_bb_to_cum(cum, get_rook_atk(from, occ), 15);
        //     }
        // }

        // for (Square sq = 0; sq < 64; sq++) {
        //     float val = ((float)(cum[sq]) / 10);
        //     if (val >= 0) {
        //         res += 2;
        //     }
        // }

        // return res;
    // }),

    // Factor("mobility", [](Pos& pos, Color color) {
    //     Eval res = 0;
    //     BB us = pos.ref_occ(color);
    //     BB occ = pos.ref_occ(opp(color)) | us;

    //     for (Piece pt = BISHOP; pt <= QUEEN; pt++) {
    //         BB froms = pos.ref_piece_mask(color, pt);
    //         while (froms) {
    //             int from = poplsb(froms);
    //             BB tos = get_piece_atk(pt, from, color, occ) & ~us;
    //             Eval max_gain = -INF;
    //             while (tos) {
    //                 int to = poplsb(tos);
    //                 int gain = sea_gain(pos, make_move(from, to, (get_BB(to) & occ) ? CAPTURE : QUIET));
    //                 max_gain = max(max_gain, gain);
    //                 if (gain >= 0) break;
    //             }

    //             if (max_gain < 0) {
    //                 res += max(-get_piece_eval(pt) / 2, max_gain);
    //             }
    //         }
    //     }

    //     return res;
    // }),

    Factor("king_safety", [](Pos& pos, Color color) {
        Eval res = 0;

        BB ksq_bb = pos.ref_piece_mask(color, KING);
        int ksq = lsb(ksq_bb);

        BB surr = get_king_atk(ksq);
        //res += (8 - bitcount(surr)) * 4;
        res += bitcount(surr & pos.ref_piece_mask(color, PAWN)) * 9;
        res += bitcount(surr & pos.ref_piece_mask(color, KNIGHT)) * 1;
        res += bitcount(surr & pos.ref_piece_mask(color, BISHOP)) * 2;
        
        BB temp = surr;
        while (temp) {
            Square square = poplsb(temp);
            if (pos.get_control_value(color, square) < 0) {
                res -= 10;
            }
        }

        res -= bitcount(pos.ref_atk(opp(color)) & surr) * 8;

        return res;
    }),


    /*Factor("connectivity", [](Pos& pos, Color color) {
        Eval res = 0;

        for (Piece victim_type = PAWN; victim_type <= KNIGHT; victim_type++) {
            
            BB victims = pos.ref_piece_mask(color, victim_type);
            while (victims) {
                Square victim_square = poplsb(victims);
                for (Piece defender_type = PAWN; defender_type < KING; defender_type++) {
                    defender_type 
                }
            }
        }

        return res;
    }),*/

};

Eval eval_pos(Pos& pos, Eval lb, Eval ub, bool debug) {
    Eval eval = pos.turn == WHITE ? pos.ref_mat() : -pos.ref_mat();

    if (debug) print(pos, true);

    for (Factor factor : factors) {
        Eval ours = factor.func(pos, pos.turn);
        Eval theirs = factor.func(pos, pos.notturn);
        eval += ours;
        eval -= theirs;
        if (debug) cout << "factor: " << setw(15) << factor.name << " = (" << setw(8) << to_string(ours) << ") - (" << setw(8) << to_string(theirs) << ") = " << setw(8) << to_string(ours - theirs) << endl;
    }

    return eval;
}








