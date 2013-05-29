/**
 * @file TestCoders.cpp
 * @author Dominik Kempa <dominik.kempa@cs.helsinki.fi>
 * @author Pekka Mikkola <pmikkol@gmail.com>
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
 * Implementations of Test encoder and decoder.
 */

#include <cassert>
#include <cstdio>
#include <ctime>
#include <iterator>
#include <iostream> // For std::streampos
#include <numeric> // for std::accumulate
#include <algorithm> // for sort, reverse, fill
#include <string>
#include <vector>
#include <map> // for entropy profiling

#include "TestCoders.hpp"
#include "globaldefs.hpp"
#include "Utils.hpp"
#include "Profiling.hpp"

namespace bwtc {

TestEncoder::TestEncoder()
    : m_headerPosition(0), m_compressedBlockLength(0) {}

TestEncoder::~TestEncoder() {}

size_t TestEncoder::
transformAndEncode(BWTBlock& block, BWTManager& bwtm, OutStream* out) {
  bwtm.doTransform(block);
  size_t bytes_used=block.writeHeader(out)+6;
  long int pos = out->getPos();
  for(int i=0;i<6;i++) out->writeByte(0);
  bytes_used+=6+block.size();
    out->writeBlock(block.begin(),block.end());
  out->write48bits(block.size(),pos);
  return bytes_used;

}



void TestDecoder::decodeBlock(BWTBlock& block, InStream* in) {
  PROFILE("TestDecoder::decodeBlock");
  if(in->compressedDataEnding()) return;
  block.readHeader(in);
  uint64 length = in->read48bits();
  block.setSize(length);
 in->readBlock(block.begin(),length);
}
TestDecoder::TestDecoder() {}

TestDecoder::~TestDecoder() {}


} // namespace bwtc

