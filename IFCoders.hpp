/**
 * @file IFCoders.hpp
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
 * Header for simple IF coder.
 *
 */

#ifndef BWTC_IF_CODERS_HPP_
#define BWTC_IF_CODERS_HPP_

#include "EntropyCoders.hpp"
#include "globaldefs.hpp"
#include "BitCoders.hpp"
#include "Streams.hpp"
#include "BWTBlock.hpp"

#include <iostream>
#include <string>
#include <vector>


namespace bwtc {
    class IFEncoder : public EntropyEncoder {
        public:
            IFEncoder();
            ~IFEncoder();

            size_t transformAndEncode(BWTBlock& block, BWTManager& bwtm,
                    OutStream* out);

    std::vector<byte> RLE(byte* data, uint32 length, byte maxval, int minrun, OutStream* out, size_t& bytes_used);



        private:
            IFEncoder(const IFEncoder&);
            IFEncoder& operator=(const IFEncoder&);
    };

    class IFDecoder : public EntropyDecoder {
        public:
            IFDecoder();
            ~IFDecoder();

            void decodeBlock(BWTBlock& block, InStream* in);
            std::vector<uint64> readRLE(InStream* in, int& extra);

        private:
            IFDecoder(const IFDecoder&);
            IFDecoder& operator=(const IFDecoder&);
    };

} // namespace bwtc

#endif
