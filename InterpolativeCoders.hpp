/**
 * @file HuffmanCoders.cpp
 * @author Jussi Kokkala <jussi.kokkala@helsinki.fi>
 *
 * @section LICENSE
 *
 * This file is part of bwtc.
 *
 * bwtc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bwtc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bwtc.  If not, see <http://www.gnu.org/licenses/>.
 *
 * 
 * @section DESCRIPTION
 *
 * Header file for interpolative encoder and decoder
 */
#ifndef INTERPOLATIVECODERS_HPP
#define INTERPOLATIVECODERS_HPP
#include<iostream>
#include<set>
#include<map>
#include<cmath>
#include<fstream>
#include<vector>
#include<string>
#include "BWTBlock.hpp"
#include "BitCoders.hpp"
#include "Streams.hpp"
#include "globaldefs.hpp"
#include "EntropyCoders.hpp"
#include "Utils.hpp"
#include "InterpolativeCoderUtils.hpp"
using namespace std;
namespace bwtc {
    
    class InterpolativeEncoder : public EntropyEncoder {

        public:
            InterpolativeEncoder() : bytes_used(0) {} 
            size_t transformAndEncode(BWTBlock& block, BWTManager& bwtm,OutStream* out);
            void encode(vector<byte>& block);
            freq ranged_freq(uint32 a, uint32 b,vector<byte>& bytes);
            void output(freq& values, freq& shape, int sum);
            void rec_output(vector<int>& vec, vector<int>& shape, int a, int b);
            std::vector<byte> RLE(byte* orig, uint32 length, byte maxval, int minrun, OutStream* out, size_t& bytes_used);
            void encode_recursive(int index, uint32 size, freq& freqs);
            OutStream* out;
            size_t bytes_used;

            byte* block_begin;
            vector<vector<int> > index;
            vector<vector<int> > dynamic_mem_big;
            vector<vector<int> > dynamic_mem_small;
            int dynamic_mem_small_start,dynamic_mem_small_end;

            void output_bit(int bit);
            void output_bits(uint32 bits,int n);
            void output_num(int num, uint32 mx);
            int bitsInBuffer;
            uint64 buffer;
            int tmpp;

            FreqMem* mem;

    };

    class InterpolativeDecoder : public EntropyDecoder {
        public:
            InterpolativeDecoder() {
            }
            void decodeBlock(BWTBlock& block, InStream* in);
            void decode_recursive(int index, uint32 size, freq& freqs);
            void input(freq& f, freq& shape, int sum);
            uint32 input_num(uint32 mx);
            void rec_input(freq& f, vector<int>& shape, int a, int b, int sum);
            vector<uint64> readRLE(InStream* in, int& extra);
            InStream* in;
            byte* output;
            uint32 input_bits(int n);

    };
    const bool IP_RLE=true;
    const int MIN_RLE_RUN=1;
    const int MAX_DYN=16;
    static int F_dyn[MAX_DYN][MAX_DYN];
    static void F_init() {
        for(int i=0;i<MAX_DYN;i++) for(int j=0;j<MAX_DYN;j++) {
            if(i==0 || j==0) F_dyn[i][j]=1;
            else F_dyn[i][j]=F_dyn[i-1][j]+F_dyn[i][j-1];
        }
    }
    static int F(int a, int b) {
        return F_dyn[a][b];
    }

    static int split(uint size) {
        uint half = 1<<utils::logFloor(size);
        if(size-half==0) return half>>1;
        return half;
    }
}
#endif
