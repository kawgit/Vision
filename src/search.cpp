#include "search.h"
#include "pos.h"
#include "bits.h"
#include "timer.h"
#include "tt.h"
#include "movegen.h"
#include "hash.h"
#include "eval.h"
#include "order.h"
#include "uci.h"
#include "move.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <mutex>

using namespace std;

#define REDUCTION(d, i, f) ((d)-1)


ThreadInfo::ThreadInfo(Pos& pos, string id_) {
	root_ply = pos.move_log.size();
	id = id_;
}

BB perft(Pos& pos, Depth depth, bool divide) {

	if (depth == 0) {
		if (divide) cout << "divide on depth zero, total: zero" << endl;
		return 1;
	}

	Timestamp start;
	if (divide) start = get_current_ms();

	BB count = 0;
	vector<Move> moves = get_legal_moves(pos);

	if (depth == 1 && !divide) return moves.size();

	for (Move move : moves) {
		pos.do_move(move);
		BB n = perft(pos, depth-1, false);
		pos.undo_move();

		if (divide) cout << to_san(move) << " " << to_string(n) << endl;
		count += n;
	}

	if (divide) cout << "total: " << to_string(count) << endl;
	if (divide) cout << "time: " << to_string(get_time_diff(start)) <<" ms" << endl;
	if (divide) cout << "nps: " << to_string(count * 1000 / get_time_diff(start) + 1) << endl;

	return count;
}

Eval search(Pos& pos, Depth depth, Eval alpha, Eval beta, ThreadInfo& ti, SearchInfo& si) {
	if (!ti.searching) return alpha;
	ti.nodes++;

	if (beta <= -MINMATE && beta != -INF) {
		beta--;
		if (alpha >= beta) return beta;
	}

	if (pos.insufficient_material()) return 0;
	if (pos.ref_halfmove_clock() >= 4) {
		if (pos.ref_halfmove_clock() >= 100) return 0;
		if (pos.one_repetition(ti.root_ply)) return 0;
		if (pos.three_repetitions()) return 0;
	}

	if (depth <= 0) return qsearch(pos, alpha, beta, ti, si);

	bool found = false;
	TTEntry* entry = si.tt.probe(pos.ref_hashkey(), found);

	if (found && entry->get_depth() >= depth && entry->get_gen() == si.tt.gen) {
		if (entry->get_bound() == EXACT) return entry->get_eval();
		else if (entry->get_bound() == UB && entry->get_eval() < beta) beta = entry->get_eval();
		else if (entry->get_bound() == LB && entry->get_eval() > alpha) alpha = entry->get_eval();
		if (alpha >= beta) return beta;
	}

	vector<Move> moves = get_legal_moves(pos);

	if (moves.size() == 0) {
		Eval eval = pos.in_check() ? -INF : 0;
		entry->save(pos.ref_hashkey(), eval, EXACT, DEPTH_MAX, MOVE_NONE, si.tt.gen);
		return eval;
	}
	
	int num_good;
	int num_boring;
	int num_bad;
	moves = order(moves, pos, ti, si, num_good, num_boring, num_bad, false);

	assert(moves.size());

	Eval besteval = -INF;
	Move bestmove = moves[0];

	for (int i = 0; i < moves.size(); i++) {
		Move& move = moves[i];

		pos.do_move(move);

		Eval eval;
		if (i < num_good) {
			eval = -search(pos, depth - 1, -beta, -max(alpha, besteval), ti, si);
		}
		else if (i < num_good + num_boring) {
			eval = -search(pos, 3 * (depth - 1) / 4, -beta, -max(alpha, besteval), ti, si);
		}
		else {
			eval = -search(pos, (depth - 1) / 4, -beta, -max(alpha, besteval), ti, si);
		}

		pos.undo_move();

		if (!ti.searching && found && i != 0) {
			break;
		}

		if (eval > MINMATE) eval--;

		if (eval > besteval) {
			besteval = eval;
			bestmove = move;
			
			if (besteval >= beta) {
				si.add_failhigh(pos, bestmove);
				break;
			}
		}
	}

	if (besteval <= alpha)		entry->save(pos.ref_hashkey(), besteval, UB   , depth, bestmove, si.tt.gen);
	else if (besteval < beta)	entry->save(pos.ref_hashkey(), besteval, EXACT, depth, bestmove, si.tt.gen);
	else						entry->save(pos.ref_hashkey(), besteval, LB   , depth, bestmove, si.tt.gen);

	return besteval;
}

Eval qsearch(Pos& pos, Eval alpha, Eval beta, ThreadInfo& ti, SearchInfo& si) {
	ti.nodes++;
	Depth depth = pos.move_log.size() - ti.root_ply;
	if (depth > ti.seldepth) {
		ti.seldepth = depth;
	}

	if (beta <= -MINMATE && beta != -INF) {
		beta--;
		if (alpha >= beta) return beta;
	}

    Eval stand_pat = eval_pos(pos, alpha, beta);
    alpha = max(alpha, stand_pat);
    if (alpha >= beta) return beta;

	if (pos.insufficient_material()) return 0;
	// ONLY IF MOVES INCLUDE CHECKS
	if (pos.ref_halfmove_clock() >= 4) {
		if (pos.ref_halfmove_clock() >= 100) return 0;
		if (pos.one_repetition(ti.root_ply)) return 0;
		if (pos.three_repetitions()) return 0;
	}

	// king attacked evasion extension
	if (pos.in_check()) {
		return search(pos, 1, alpha, beta, ti, si);
	}

    vector<Move> moves = get_legal_moves(pos);

	if (moves.size() == 0) {
		if (pos.in_check()) return -INF;
		else return 0;
	}

	int num_good;
	int num_boring;
	int num_bad;
	moves = order(moves, pos, ti, si, num_good, num_boring, num_bad, true);

	assert(moves.size() == num_good);
    
    for (int i = 0; i < num_good; i++) {
		Move move = moves[i];

		pos.do_move(move);
		
		Eval eval = -qsearch(pos, -beta, -alpha, ti, si);
		
		pos.undo_move();
		
		if (eval > MINMATE) eval--;

		alpha = max(alpha, eval);

		if (alpha >= beta) {
			alpha = beta;
			break;
		}
    }
	
    return alpha;
}

mutex print_mutex;
void SearchInfo::launch(bool verbose) {
	stop();
	tis.clear();

    is_active = true;
    start_time = get_current_ms();
	last_depth_searched = 0;
    
    for (int i = 0; i < num_threads; i++) {
        tis.emplace_back(root_pos, "threadname_" + to_string(i));
    }

	vector<thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(&SearchInfo::worker, this, ref(tis[i]), ref(verbose));
    }

    while (true) {
        if (!is_active
            || (!ponder && get_time_diff(start_time) > max_time)) {
            break;
        }

        sleep_ms(10);
    }
    
    stop();
	print_uci_info();

    vector<Move> pv = tt.getPV(root_pos);
    vector<Move> moves = get_legal_moves(root_pos);
    print_mutex.lock();
    cout << "bestmove " + (pv.size() > 0 ? to_san(pv[0]) : (moves.size() ? to_san(moves[0]) : "(none)")) + (pv.size() > 1 ? " ponder " + to_san(pv[1]) : "") << endl;
    print_mutex.unlock();
    
    for (thread& thr : threads) {
        thr.join();
    }
}

void SearchInfo::stop() {
	for (ThreadInfo& ti : tis) {
        ti.searching = false;
    }
    is_active = false;
    last_depth_searched = 0;
}

void SearchInfo::clear() {
	for (int i = 0; i < 6; i++) {
		for (int sq = 0; sq < 64; sq++) {
			hist_hueristic[i][sq] = 0;
			cm_hueristic[i][sq] = MOVE_NONE;
		}
	}
	hist_score_max = 0;
	tt.clear();
}

void SearchInfo::worker(ThreadInfo& ti, bool verbose) {
	Pos root_copy = root_pos;

    while (ti.searching) {
        depth_increment_mutex.lock();
        Depth depth = ++last_depth_searched;
        depth_increment_mutex.unlock();

        if (depth > max_depth || depth < 0) break;
        
        search(root_copy, depth, -INF, INF, ti, *this);

		if (verbose && ti.searching) {
        	// cout << "thread " << ti.id << ":";
        	print_uci_info();
		}
    }
}

void SearchInfo::print_uci_info() {
    bool found = false;
    TTEntry* entry = tt.probe(root_pos.ref_hashkey(), found);
    
    if (!found) return;

    BB nodes = get_nodes();
    Timestamp time_elapsed = get_time_diff(start_time);

    string msg = "";
    msg += "info";
    msg += " depth " + to_string(entry->get_depth());
	msg += " seldepth " + to_string(get_seldepth());
    msg += " score " + eval_to_string(entry->get_eval());
    msg += " nodes " + to_string(nodes);
    msg += " time " + to_string(time_elapsed);
    msg += " nps " + to_string(nodes * 1000 / (time_elapsed + 1));
    msg += " hashfull " + to_string(tt.hashfull());
    msg += " pv " + to_string(tt.getPV(root_pos)) + "\n";

    print_mutex.lock();
    cout << msg;
    print_mutex.unlock();
}

void timer(bool& target, Timestamp time) {
	Timestamp start = get_current_ms();
	while (target && get_time_diff(start) < time) {
		sleep_ms(10);
	}
	target = false;
}

Eval sea_gain(Pos& pos, Move move, Eval alpha) {
	pos.update_atks();

	Eval target_square = get_to(move);
	Eval target_piece_eval = get_piece_eval(pos.ref_mailbox(pos.notturn, target_square));
	if (!(pos.ref_atk(pos.notturn) & get_BB(target_square))) { // hanging
		return get_piece_eval(pos.ref_mailbox(pos.notturn, target_square));
	}
	Eval result = -static_exchange_search(
		pos, 
		target_square, 
		pos.notturn, 
		-(target_piece_eval), 
		pos.ref_occ() & ~get_BB(get_from(move)), 
		get_piece_eval(pos.ref_mailbox(pos.turn, get_from(move))), 
		-INF, 
		-alpha);
	return result;
}

Eval static_exchange_search(Pos& pos, Square target_square, Color turn, Eval curr_mat, BB occ, Eval target_piece_eval, Eval alpha, Eval beta) {
	alpha = max(curr_mat, alpha);
	if (alpha >= beta) return beta;

	Square from = SQUARE_NONE;
	Piece from_piece = PIECE_NONE;
	for (Piece pt = PAWN; pt <= KING; pt++) {
		BB attackers = get_piece_atk(pt, target_square, opp(turn), occ)
			& pos.ref_piece_mask(turn, pt)
			& occ;
		if (attackers) {
			from = lsb(attackers);
			from_piece = pt;
			break;
		}
	}

	if (from == SQUARE_NONE) return curr_mat;

	Eval result_after_move = -static_exchange_search(
		pos, 
		target_square, 
		opp(turn), 
		-(curr_mat + target_piece_eval), 
		occ & ~get_BB(from), 
		get_piece_eval(pos.ref_mailbox(turn, from)), 
		-beta, 
		-alpha
		);
	return max(curr_mat, result_after_move);
}

Move get_best_move(Pos& pos, Depth depth) {
	SearchInfo si;
	ThreadInfo ti(pos);

	for (int d = 0; d < depth; d++) {
		search(pos, depth, -INF, INF, ti, si);
	}

	return si.tt.getPV(pos)[0];
}