/**
 * @file MTFCoders.hpp
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
 * @section DESCRIPTION
 *
 * Header for simple MTF coder.
 *
 */

#ifndef BWTC_MTF_CODERS_HPP_
#define BWTC_MTF_CODERS_HPP_

#include "EntropyCoders.hpp"
#include "globaldefs.hpp"
#include "BitCoders.hpp"
#include "Streams.hpp"
#include "BWTBlock.hpp"

#include <iostream>
#include <string>
#include <vector>


namespace bwtc {
    struct Node {
        Node* next;
        byte val;
        Node(byte v) : val(v) {}
    };
    class MTFEncoder : public EntropyEncoder {
        public:
            MTFEncoder(char encoder);
            ~MTFEncoder();

            size_t transformAndEncode(BWTBlock& block, BWTManager& bwtm,
                    OutStream* out);

            void encodeData(const byte* data, const std::vector<uint32>& stats,
                    uint32 blockSize, OutStream* out);
    std::vector<byte> RLE(byte* data, uint32 length, byte maxval, int minrun, OutStream* out, size_t& bytes_used, char encoder);



        private:
            long int m_headerPosition;
            uint64 m_compressedBlockLength;
            std::vector<byte> m_rankList;
            MTFEncoder(const MTFEncoder&);
            MTFEncoder& operator=(const MTFEncoder&);
            char m_encoder;
    };

    class MTFDecoder : public EntropyDecoder {
        public:
            MTFDecoder(char decoder);
            ~MTFDecoder();

            void decodeBlock(BWTBlock& block, InStream* in);
            std::vector<uint64> readRLE(InStream* in, int& extra, char decoder);

        private:
            std::vector<byte> m_rankList;
            char m_decoder;
            MTFDecoder(const MTFDecoder&);
            MTFDecoder& operator=(const MTFDecoder&);
    };

} // namespace bwtc

#endif
