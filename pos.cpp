#include "pos.h"
#include "bits.h"
#include "types.h"
#include "movegen.h"
#include "eval.h"
#include <iostream>
#include <map>
#include <string>
#include <cassert>

using namespace std;

Pos::Pos(string fen) {
	static map<char, Piece> fenmap = {
		{'p', PAWN},
		{'n', KNIGHT},
		{'b', BISHOP},
		{'r', ROOK},
		{'q', QUEEN},
		{'k', KING},
	};

	for (int i = 0; i < 64; i++) {
		mailboxes(WHITE,i) = PIECE_NONE;
		mailboxes(BLACK,i) = PIECE_NONE;
	}

	int r = 7;
	int c = 0;
	int i = 0;
	while (!(r == 0 && c == 8)) {
		char ch = fen[i++];
		if (ch == '/') {
			c = 0;
			r--;
		}
		else if (ch >= '0' && ch <= '9') {
			c += ch - '0';
		}
		else {
			Color color = (ch >= 'a' ? BLACK : WHITE);
			setPiece(color, rc(r, c), fenmap[(color == BLACK ? ch : (ch - 'A' + 'a'))]);
			c++;
		}
	}

	fen = fen.substr(i);

	set_turn(fen.find("w") != string::npos ? WHITE : BLACK);

	if (fen.find("K") != string::npos) switch_cr(WKS_I);
	if (fen.find("Q") != string::npos) switch_cr(WQS_I);
	if (fen.find("k") != string::npos) switch_cr(BKS_I);
	if (fen.find("q") != string::npos) switch_cr(BQS_I);

	for (int i = 0; i < 3; i++) fen = fen.substr(fen.find(" ")+1);

	if (fen[0] != '-') set_ep(string_to_square(fen));

	fen = fen.substr(fen.find(" ")+1);
	if (fen.size() == 0) goto end;

	hm_clock = stoi(fen.substr(0, fen.find(" ")));

	fen = fen.substr(fen.find(" ")+1);
	if (fen.size() == 0) goto end;

	m_clock = stoi(fen);

	end:

	move_log.reserve(RESERVE_SIZE);
	hashkey_log.reserve(RESERVE_SIZE);
	cr_log.reserve(RESERVE_SIZE);
	to_piece_log.reserve(RESERVE_SIZE);
	ep_log.reserve(RESERVE_SIZE);

	return;
}

void Pos::do_move(Move m) {
	assert(m != MOVE_NONE);

	if (m == MOVE_NULL) {
		do_null_move();
		return;
	}

	move_log.push_back(m);
	hashkey_log.push_back(hashkey);
	cr_log.push_back(cr);
	ep_log.push_back(ep);
	hm_clock_log.push_back(hm_clock);
	repetitions_index_log.push_back(repetitions_index);

	
	Square from = get_from(m);
	Square to = get_to(m);
	Piece fromPiece = mailboxes(turn, from);
	Piece toPiece = mailboxes(notturn, to);
	to_piece_log.push_back(toPiece);

	removePiece(turn, from, fromPiece);

	if (is_capture(m) || fromPiece == PAWN) {
		hm_clock = 0;
		repetitions_index = hashkey_log.size();
	}
	else hm_clock++;
	if (turn == BLACK) m_clock++;

	if (!is_promotion(m)) setPiece(turn, to, fromPiece);
	else setPiece(turn, to, get_promotion_type(m));

	set_ep(SQUARE_NONE);

	if (get_flags(m)) {
		if (is_capture(m)) {
			if (!is_ep(m)) removePiece(notturn, to, toPiece);
			else removePiece(notturn, to + (turn == WHITE ? -8 : 8), PAWN);
		}
		else if (is_double_pawn_push(m) && (get_rank_mask(to/8) & getKingAtk(to) & pieces(notturn, PAWN))) {
			set_ep(to + (turn == WHITE ? -8 : 8));
		}
		else if (is_king_castle(m)) {
            if (turn == WHITE) {
                removePiece(WHITE, H1, ROOK);
                setPiece(WHITE, F1, ROOK);
            }
            else {
                removePiece(BLACK, H8, ROOK);
                setPiece(BLACK, F8, ROOK);
            }
		}
		else if (is_queen_castle(m)) {
            if (turn == WHITE) {
                removePiece(WHITE, A1, ROOK);
                setPiece(WHITE, D1, ROOK);
            }
            else {
                removePiece(BLACK, A8, ROOK);
                setPiece(BLACK, D8, ROOK);
            }
        }
	}

	if (cr) {
		if (turn == WHITE) {
			if (fromPiece == ROOK) {
				if (getWK(cr) && from == H1) switch_cr(WKS_I);
				if (getWQ(cr) && from == A1) switch_cr(WQS_I);
			}
			if (fromPiece == KING) {
				if (getWK(cr)) switch_cr(WKS_I);
				if (getWQ(cr)) switch_cr(WQS_I);
			}
			if (toPiece == ROOK) {
				if (getBK(cr) && to == H8) switch_cr(BKS_I);
				if (getBQ(cr) && to == A8) switch_cr(BQS_I);
			}
		}
		else {
			if (fromPiece == ROOK) {
				if (getBK(cr) && from == H8) switch_cr(BKS_I);
				if (getBQ(cr) && from == A8) switch_cr(BQS_I);
			}
			if (fromPiece == KING) {
				if (getBK(cr)) switch_cr(BKS_I);
				if (getBQ(cr)) switch_cr(BQS_I);
			}
			if (toPiece == ROOK) {
				if (getWK(cr) && to == H1) switch_cr(WKS_I);
				if (getWQ(cr) && to == A1) switch_cr(WQS_I);
			}
		}
    }

	switch_turn();
}

void Pos::undo_move() {
	Move m = move_log.back();
	
	assert(m != MOVE_NONE);

	if (m == MOVE_NULL) {
		do_null_move();
		return;
	}

	switch_turn();

	if (turn == BLACK) m_clock--;
	
	Square from = get_from(m);
	Square to = get_to(m);
	Piece fromPiece = mailboxes(turn, to);
	Piece toPiece = to_piece_log.back();

	if (!is_promotion(m)) {
		removePiece(turn, to, fromPiece);
		setPiece(turn, from, fromPiece);
	}
	else {
		removePiece(turn, to, get_promotion_type(m));
		setPiece(turn, from, PAWN);
	}

	if (get_flags(m)) {
		if (is_capture(m)) {
			if (!is_ep(m)) setPiece(notturn, to, toPiece);
			else setPiece(notturn, to + (turn == WHITE ? -8 : 8), PAWN);
		}
        else if (is_king_castle(m)) {
            if (turn == WHITE) {
                removePiece(WHITE, F1, ROOK);
                setPiece(WHITE, H1, ROOK);
            }
            else {
                removePiece(BLACK, F8, ROOK);
                setPiece(BLACK, H8, ROOK);
            }
        }
        else if (is_queen_castle(m)) {
            if (turn == WHITE) {
                removePiece(WHITE, D1, ROOK);
                setPiece(WHITE, A1, ROOK);
            }
            else {
                removePiece(BLACK, D8, ROOK);
                setPiece(BLACK, A8, ROOK);
            }
        }
	}

	hashkey = hashkey_log.back();
	ep = ep_log.back();
	cr = cr_log.back();
	hm_clock = hm_clock_log.back();
	repetitions_index = repetitions_index_log.back();

	move_log.pop_back();
	to_piece_log.pop_back();
	hashkey_log.pop_back();
	ep_log.pop_back();
	cr_log.pop_back();
	hm_clock_log.pop_back();
	repetitions_index_log.pop_back();

}

void Pos::do_null_move() {
	move_log.push_back(MOVE_NULL);
	hashkey_log.push_back(hashkey);
	cr_log.push_back(cr);
	ep_log.push_back(ep);
	hm_clock_log.push_back(hm_clock);
	repetitions_index_log.push_back(repetitions_index);
	to_piece_log.push_back(PIECE_NONE);

	if (turn == BLACK) m_clock++;

	set_ep(SQUARE_NONE);

	switch_turn();
	null_moves_made++;
}

void Pos::undo_null_move() {
	if (turn == BLACK) m_clock--;

	hashkey = hashkey_log.back();
	ep = ep_log.back();
	cr = cr_log.back();
	hm_clock = hm_clock_log.back();
	repetitions_index = repetitions_index_log.back();

	move_log.pop_back();
	to_piece_log.pop_back();
	hashkey_log.pop_back();
	ep_log.pop_back();
	cr_log.pop_back();
	hm_clock_log.pop_back();
	repetitions_index_log.pop_back();

	switch_turn();

	null_moves_made--;
}

void print(Pos& p, bool meta) {
	const static char white_piece_notation[6] = {'P', 'N', 'B', 'R', 'Q', 'K'};
	const static char black_piece_notation[6] = {'p', 'n', 'b', 'r', 'q', 'k'};

	for (int r = 7; r >= 0; r--) {
		cout<<" +---+---+---+---+---+---+---+---+ "<<endl;
		for (int c = 0; c < 8; c++) {
			cout<<" | ";
			if (p.mailboxes(WHITE, rc(r, c)) != PIECE_NONE) {
				cout<<white_piece_notation[p.mailboxes(WHITE, rc(r, c)) - PAWN];
			}
			else if (p.mailboxes(BLACK, rc(r, c)) != PIECE_NONE) {
				cout<<black_piece_notation[p.mailboxes(BLACK, rc(r, c)) - PAWN];
			}
			else {
				cout<<' ';
			}
		}
		cout<<" | "<<endl;
	}
	cout<<" +---+---+---+---+---+---+---+---+ "<<endl;

	if (meta) {
		cout<<"turn: "<<(p.turn == WHITE ? "WHITE" : "BLACK")<<endl;
		cout<<"hashkey: "<< hex << uppercase << p.hashkey << nouppercase << dec << endl;
		cout<<"castle rights: ";
		if (getWK(p.cr)) cout<<"K";
		if (getWQ(p.cr)) cout<<"Q";
		if (getBK(p.cr)) cout<<"k";
		if (getBQ(p.cr)) cout<<"q";
		cout<<endl;
		cout<<"ep: "<<square_to_string(p.ep)<<endl;
		cout<<"halfmove clock: "<<to_string(p.hm_clock)<<endl;
		cout<<"move clock: "<<to_string(p.m_clock)<<endl;
		cout<<"fen: "<<getFen(p)<<endl;
	}
}

string getFen(Pos& p) {
	const static char white_piece_notation[6] = {'P', 'N', 'B', 'R', 'Q', 'K'};
	const static char black_piece_notation[6] = {'p', 'n', 'b', 'r', 'q', 'k'};

	string fen = "";

	for (int r = 7; r >= 0; r--) {
		int space_count = 0;
		for (int c = 0; c < 8; c++) {
			if (p.mailboxes(WHITE, rc(r, c)) != PIECE_NONE) {
				if (space_count != 0) fen += to_string(space_count);
				space_count = 0;
				fen += white_piece_notation[p.mailboxes(WHITE, rc(r, c)) - PAWN];
			}
			else if (p.mailboxes(BLACK, rc(r, c)) != PIECE_NONE) {
				if (space_count != 0) fen += to_string(space_count);
				space_count = 0;
				fen += black_piece_notation[p.mailboxes(BLACK, rc(r, c)) - PAWN];
			}
			else space_count++;
		}
		if (space_count != 0) fen += to_string(space_count);
		if (r != 0) fen += '/';
	}

	fen += (p.turn == WHITE ? " w " : " b ");

	if (getWK(p.cr)) fen+="K";
	if (getWQ(p.cr)) fen+="Q";
	if (getBK(p.cr)) fen+="k";
	if (getBQ(p.cr)) fen+="q";

	fen += " " + square_to_string(p.ep);
	fen += " " + to_string(p.hm_clock);
	fen += " " + to_string(p.m_clock);

	return fen;
}

BB Pos::getAtkMask(Color c) {
	BB mask = 0ULL;

    if (c == WHITE)
        mask |= ((pieces(c, PAWN) & ~get_file_mask(7)) << 9) | ((pieces(c, PAWN) & ~get_file_mask(0)) << 7);
    else
        mask |= ((pieces(c, PAWN) & ~get_file_mask(0)) >> 9) | ((pieces(c, PAWN) & ~get_file_mask(7)) >> 7);

    BB knights = pieces(c, KNIGHT);
    while (knights) {
        int from = poplsb(knights);
        mask |= getKnightAtk(from);
    }

    BB nonking_occ = get_occ() & ~pieces(opp(c), KING);

    BB bishops = pieces(c, BISHOP);
    while (bishops) {
        int from = poplsb(bishops);
        mask |= getBishopAtk(from, nonking_occ);
    }

    BB rooks = pieces(c, ROOK);
    while (rooks) {
        int from = poplsb(rooks);
        mask |= getRookAtk(from, nonking_occ);
    }

    BB queens = pieces(c, QUEEN);
    while (queens) {
        int from = poplsb(queens);
        mask |= getQueenAtk(from, nonking_occ);
    }

    mask |= getKingAtk(lsb(pieces(c, KING)));

    return mask;
}

bool Pos::in_check() {
	Square ksq = lsb(pieces(turn, KING));
	BB rooks = pieces(notturn, ROOK) | pieces(notturn, QUEEN);
	BB bishops = pieces(notturn, BISHOP) | pieces(notturn, QUEEN);

	BB occ = get_occ();

	return  (getRookAtk(ksq, occ) & rooks) || 
			(getBishopAtk(ksq, occ) & bishops) || 
			(getKnightAtk(ksq) & pieces(notturn, KNIGHT)) ||
			(getPawnAtk(turn, ksq) & pieces(notturn, PAWN));
}

bool Pos::three_repetitions() {
	int count = 1;
	for (int i = hashkey_log.size()-4; i >= repetitions_index; i-=2) {
		if (hashkey_log[i] == hashkey) {
			count++;
			if (count == 3) return true;
		}
	}
	return false;
}

bool Pos::do_move(string SAN) {
	vector<Move> moves = getLegalMoves(*this);
	Move move = MOVE_NONE;
	for (Move m : moves) {
		if (getSAN(m) == SAN) move = m;
	}
	if (move == MOVE_NONE) return false;
	do_move(move);
	return true;
}

bool Pos::is_over() {
	if (three_repetitions()) return true;
	if (hm_clock == 50) return true;
	if (!getLegalMoves(*this).size()) return true;
	if (insufficient_material()) return true;
	return false;
}

bool Pos::one_repetition(int root) {
	for (int i = hashkey_log.size()-4; i >= max((int)repetitions_index, root); i-=2) if (hashkey_log[i] == hashkey) return true;
	return false;
}

bool Pos::insufficient_material() {
	if (pieces(WHITE, PAWN) | pieces(BLACK, PAWN) | 
		pieces(WHITE, QUEEN) | pieces(BLACK, QUEEN) |
		pieces(WHITE, ROOK) | pieces(BLACK, ROOK))
		return false;

	BB occ = get_occ();
	int piece_count = bitcount(occ);

	if (piece_count == 2) return true; //KvK
	if (bitcount(pieces(WHITE, BISHOP) | pieces(WHITE, KNIGHT)) > 1) return false;
	if (bitcount(pieces(BLACK, BISHOP) | pieces(BLACK, KNIGHT)) > 1) return false;
	if (piece_count == 3) return true;
	if (piece_count == 4) {
		if (pieces(WHITE, BISHOP) && pieces(BLACK, BISHOP) &&
		   ((lsb(pieces(WHITE, BISHOP)) % 2) == (lsb(pieces(BLACK, BISHOP)) % 2))) 
			return true;
	}

	return false;
}

bool Pos::causes_check(Move move) {

	if (is_ep(move) || is_king_castle(move) || is_queen_castle(move) || is_promotion(move)) {
		do_move(move);
		bool result = in_check();
		undo_move();
		return result;
	}

	Square ksq = lsb(pieces(notturn, KING));
	BB occ = get_occ();
	BB rook_rays = getRookAtk(ksq, occ);
	BB bishop_rays = getBishopAtk(ksq, occ);
	BB from_mask = get_BB(get_from(move));
	BB to_mask = get_BB(get_to(move));
	
	if (rook_rays & from_mask
	 && getRookAtk(ksq, occ & ~from_mask) & (pieces(turn, ROOK) | pieces(turn, QUEEN))) {
		return true;
	}

	if (bishop_rays & from_mask
	 && getBishopAtk(ksq, occ & ~from_mask) & (pieces(turn, BISHOP) | pieces(turn, QUEEN))) {
		return true;
	}

	Piece piece = mailboxes(turn, get_from(move));

	switch (piece) {
		case PAWN:
			break;
			return getPawnAtk(notturn, ksq) & to_mask;
		case KNIGHT:
			break;
			return getKnightAtk(ksq) & to_mask;
		case BISHOP:
			break;
			return getBishopAtk(ksq, occ & ~from_mask) & to_mask;
		case ROOK:
			break;
			return getRookAtk(ksq, occ & ~from_mask) & to_mask;
		case QUEEN:
			break;
			return getQueenAtk(ksq, occ & ~from_mask) & to_mask;
		case KING:
			return false;
			break;
	}
	
	return false;
}
