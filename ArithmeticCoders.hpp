/**
 * @file ArithmeticCoders.hpp
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
 * Header for simple Arithmetic coder.
 *
 */

#ifndef BWTC_TEST_CODERS_HPP_
#define BWTC_TEST_CODERS_HPP_

#include "EntropyCoders.hpp"
#include "globaldefs.hpp"
#include "BitCoders.hpp"
#include "Streams.hpp"
#include "BWTBlock.hpp"

#include <iostream>
#include <string>
#include <vector>


namespace bwtc {

class ArithmeticEncoder : public EntropyEncoder {
 public:
  ArithmeticEncoder();
  ~ArithmeticEncoder();

  size_t transformAndEncode(BWTBlock& block, BWTManager& bwtm,
                            OutStream* out);
  
  void encodeData(const byte* data, const std::vector<uint32>& stats,
                  uint32 blockSize, OutStream* out);


 private:
  long int m_headerPosition;
  uint64 m_compressedBlockLength;

  ArithmeticEncoder(const ArithmeticEncoder&);
  ArithmeticEncoder& operator=(const ArithmeticEncoder&);
    std::vector<byte> RLE(byte* orig, uint32 length, byte maxval, int minrun, OutStream* out, size_t& bytes_used);
};

class ArithmeticDecoder : public EntropyDecoder {
 public:
  ArithmeticDecoder();
  ~ArithmeticDecoder();

  void decodeBlock(BWTBlock& block, InStream* in);

 private:

  ArithmeticDecoder(const ArithmeticDecoder&);
  ArithmeticDecoder& operator=(const ArithmeticDecoder&);
    std::vector<uint64> readRLE(InStream* in, int& extra);
};

} // namespace bwtc

#endif
