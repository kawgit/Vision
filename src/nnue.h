#pragma once

#include <immintrin.h>

#include "types.h"
#include "pos.h"

#define ALIGN64 alignas(64)

struct AccumulatorSlice {
    bool accurate = false;
    Eval white_ps;

    void add_piece(const Color color, const Piece piece, const Square square);

    void rem_piece(const Color color, const Piece piece, const Square square);
};

struct Accumulator {

    AccumulatorSlice slice_stack[256];
    AccumulatorSlice* slice;

    Accumulator();

    void reset(const Pos& pos);

    void push();
    
    void pop();

    void recursively_update(const Pos& pos, size_t offset = 0);

};

namespace nnue {

    extern Eval ps_weights[N_PIECES][N_SQUARES];

/*
    const size_t simd_size = 256 / (8 * sizeof(float));

    const size_t IN_LEN = N_COLORS * N_PIECES * N_SQUARES;
    const size_t L1_LEN = 1024;
    const size_t L2_LEN = 16;
    const size_t L3_LEN = 16;
    const size_t OP_LEN = 1;

    extern float accumulator[N_COLORS][L1_LEN];

    extern float in_l1_weights[IN_LEN * L1_LEN];
    extern float l1_l2_weights[L1_LEN * L2_LEN];
    extern float l2_l3_weights[L2_LEN * L3_LEN];
    extern float l3_op_weights[L3_LEN * OP_LEN];

    extern float l1_biases[L1_LEN];
    extern float l2_biases[L2_LEN];
    extern float l3_biases[L3_LEN];
    extern float op_biases[OP_LEN];

    void read_network(std::string path);

	void add_piece(const Color color, const Piece piece, const Square square);

	void rem_piece(const Color color, const Piece piece, const Square square);

    template <size_t SRC_LEN, size_t DST_LEN>
    void forward_pass(const float* src, float* dst, const float* weights, const float* biases);
*/

    Eval evaluate(Accumulator& accumulator, Pos& pos);
}