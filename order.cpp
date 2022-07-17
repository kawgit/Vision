#include "types.h"
#include "pos.h"
#include "search.h"
#include "bits.h"
#include "eval.h"
#include "tuner.h"
#include <vector>


inline unsigned int mvvlva(Piece attacker, Piece victim) {
    static const unsigned int table[6][6] = { //attacker, victim
        {6000, 20220, 20250, 20400, 20800, 26900},
        {4770,  6000, 20020, 20170, 20570, 26670},
        {4750,  4970,  6000, 20150, 20550, 26650},
        {4600,  4820,  4850,  6000, 20400, 26500},
        {4200,  4420,  4450,  4600,  6010, 26100},
        {3100,  3320,  3350,  3500,  3900, 26000},
    };
    assert(attacker - PAWN >= 0);
    assert(attacker - PAWN < 6);
    assert(victim - PAWN >= 0);
    assert(victim - PAWN < 6);
    return table[attacker - PAWN][victim - PAWN];
}

bool keeps_tempo(Move& move, Pos& pos, ThreadInfo& ti) {
    Eval cur_eval = qsearch(pos, -INF, INF, nullptr, nullptr);
    
    pos.do_move(move);

    if (pos.in_check()) { 
        pos.undo_move();
        return true;
    }

    pos.do_null_move();
    
    Eval threatened_eval = qsearch(pos, -INF, INF, nullptr, nullptr);
    
    pos.undo_null_move();

    pos.undo_move();

    return threatened_eval - cur_eval > TEMPO_MARGIN;
}

vector<Move> order(vector<Move>& unsorted_moves, Pos& pos, ThreadInfo* ti, SearchInfo* si, int& interesting) {
    interesting = 0;
    Move counter_move = si ? si->get_cm(pos) : MOVE_NONE;

    bool found = false;
	TTEntry* entry = si ? si->tt.probe(pos.hashkey, found) : nullptr;
	Move entry_move = found ? entry->get_move() : MOVE_NONE;

    vector<Score> unsorted_scores;
    unsorted_scores.reserve(unsorted_moves.size());

    bool found_huer_response = false;
    for (Move& move : unsorted_moves) {
        Score score = 0;
        if (move == entry_move) { score = SCORE_MAX; found_huer_response = true; }
		else if (move == counter_move) { score += SCORE_MAX - 100; found_huer_response = true; }
        else {
            if (is_capture(move)) score += mvvlva(pos.mailboxes(pos.turn, get_from(move)), is_ep(move) ? PAWN : pos.mailboxes(pos.notturn, get_to(move)));
            if (is_promotion(move)) score += get_piece_eval(get_promotion_type(move))*20;
            if (pos.causes_check(move)) score += 100000;
        }

        if (score != 0) interesting++;
        unsorted_scores.push_back(score);
    }

    vector<Move> sorted_moves;
    vector<Score> sorted_scores;
    sorted_moves.reserve(unsorted_moves.size());
    sorted_scores.reserve(unsorted_moves.size());

    vector<Move> uninteresting;

    for (int j = 0; j < unsorted_moves.size(); j++) {
        Move& move = unsorted_moves[j];
        Score& score = unsorted_scores[j];

        if (score != 0) {
            sorted_moves.push_back(move);
            sorted_scores.push_back(score);
            int i = sorted_moves.size()-1;
            while (i != -1) {
                i--;
                if (sorted_scores[i] < score) {
                    sorted_moves[i+1] = sorted_moves[i];
                    sorted_scores[i+1] = sorted_scores[i];
                }
                else break;
            }
            sorted_moves[i+1] = move;
            sorted_scores[i+1] = score;
        }
        else {
            uninteresting.push_back(move);
        }
    }
    
    if (interesting == 0 && si) {
        int sorted_length = sorted_moves.size();
        
        for (Move& move : uninteresting) {
            Score score = si->get_hist(pos, move);

            sorted_moves.push_back(move);
            sorted_scores.push_back(score);

            int i = sorted_moves.size()-1;
            while (i != sorted_length-1) {
                i--;
                if (sorted_scores[i] < score) {
                    sorted_moves[i+1] = sorted_moves[i];
                    sorted_scores[i+1] = sorted_scores[i];
                }
                else break;
            }
            sorted_moves[i+1] = move;
            sorted_scores[i+1] = score;
        }
    }
    else {
        for (Move& move : uninteresting) {
            sorted_moves.push_back(move);
            sorted_scores.push_back(0);
        }
    }

    return sorted_moves;
}