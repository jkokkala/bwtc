/**
 * @file HuffmanUtilCoders.hpp
 * @author Dominik Kempa <dominik.kempa@cs.helsinki.fi>
 * @author Pekka Mikkola <pmikkol@gmail.com>
 * @author Jussi Kokkala <jkokkala@gmail.com>
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
 * Header for simple HuffmanUtil coder.
 *
 */

#ifndef BWTC_HUFFMANUTIL_HPP_
#define BWTC_HUFFMANUTIL_HPP_

#include "EntropyCoders.hpp"
#include "globaldefs.hpp"
#include "BitCoders.hpp"
#include "Streams.hpp"
#include <iostream>
#include <string>
#include <vector>


namespace bwtc {

class HuffmanUtilEncoder {
 public:
  HuffmanUtilEncoder();
  ~HuffmanUtilEncoder();
size_t encode(byte* data, uint64 size, OutStream* out);

  void encodeData(const byte* data, const std::vector<uint32>& stats,
                  uint32 blockSize, OutStream* out);
  void writeBlockHeader(std::vector<uint32>& stats, OutStream* out);

  void writePackedInteger(uint64 packed_integer, OutStream* out);
void finishBlock(OutStream* out);


 private:
  long int m_headerPosition;
  uint64 m_compressedBlockLength;

  void serializeShape(uint32 *clen, std::vector<bool> &vec);
  HuffmanUtilEncoder(const HuffmanUtilEncoder&);
  HuffmanUtilEncoder& operator=(const HuffmanUtilEncoder&);
};

class HuffmanUtilDecoder {
 public:
  HuffmanUtilDecoder();
  ~HuffmanUtilDecoder();

  uint64 readPackedInteger(InStream* in);
  void decodeBlock(std::vector<byte>& data,InStream* in);
  uint64 readBlockHeader(std::vector<uint64>* stats, InStream* in);

 private:

  size_t deserializeShape(InStream &input, uint32 *clen);
  HuffmanUtilDecoder(const HuffmanUtilDecoder&);
  HuffmanUtilDecoder& operator=(const HuffmanUtilDecoder&);
};

} // namespace bwtc

#endif
