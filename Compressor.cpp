/**
 * @file Compressor.cpp
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
 * Implementation of Compressor.
 */

#include "Compressor.hpp"
#include "PrecompressorBlock.hpp"
#include "Streams.hpp"
#include "Profiling.hpp"

#include <string>

namespace bwtc {

Compressor::
Compressor(const std::string& in, const std::string& out,
           const std::string& preprocessing, size_t memLimit, char entropyCoder)
    : m_in(new RawInStream(in)), m_out(new RawOutStream(out)),
      m_coder(giveEntropyEncoder(entropyCoder)), m_precompressor(preprocessing),
      m_options(memLimit, entropyCoder) {}

Compressor::
Compressor(InStream* in, OutStream* out,
           const std::string& preprocessing, size_t memLimit, char entropyCoder)
    : m_in(in), m_out(out), m_coder(giveEntropyEncoder(entropyCoder)),
      m_precompressor(preprocessing), m_options(memLimit, entropyCoder) {}

Compressor::~Compressor() {
  delete m_in;
  delete m_out;
  delete m_coder;
}

size_t Compressor::writeGlobalHeader() {
  m_out->writeByte(static_cast<byte>(m_options.entropyCoder));
  return 1;
}

void Compressor::initializeBwtAlgorithm(char choice, uint32 startingPoints) {
  m_bwtmanager.initialize(choice);
  m_bwtmanager.setStartingPoints(startingPoints);
}

size_t Compressor::compress(size_t threads) {
  PROFILE("Compressor::compress");
  if(threads != 1) {
    std::cerr << "Supporting only single thread!" << std::endl;
    return 0;
  }

  size_t compressedSize = writeGlobalHeader();

  /* Precompressor uses (1/3)n bytes of additional memory for the block
   * of size n so we can handle 0.75*memLim sized block at once (0.74 should be
   * enough to be on the safe side). */
  size_t pbBlockSize = static_cast<size_t>(m_options.memLimit*0.74);
  size_t bwtBlockSize = std::min(static_cast<size_t>(m_options.memLimit*0.185),
                                 static_cast<size_t>(0x7fffffff - 1));

  if(m_precompressor.options().size() == 0) pbBlockSize = bwtBlockSize;
  

  size_t preBlocks = 0, bwtBlocks = 0;
  while(true) {
    PrecompressorBlock *pb = m_precompressor.readBlock(pbBlockSize, m_in);
    if(pb->originalSize() == 0) {
      delete pb;
      break;
    }
    /* BWT uses roughly 4n additional bytes of additional memory but
     * we have to also account the memory used for the rest of the data.
     * Based on the experiments the limits used here are somewhat conservative.
     */
    if(pbBlockSize != bwtBlockSize) {
      size_t s = (m_options.memLimit - pb->size())/4.5;
      bwtBlockSize = std::min(s,static_cast<size_t>(0x7fffffff - 1));
    }
    
    pb->sliceIntoBlocks(bwtBlockSize);
    ++preBlocks;
    bwtBlocks += pb->slices();

    compressedSize += pb->writeBlockHeader(m_out);

    for(size_t i = 0; i < pb->slices(); ++i) {
      compressedSize += m_coder->
          transformAndEncode(pb->getSlice(i), m_bwtmanager, m_out);
      //TODO: if optimizing overall memory usage now would be time to
      //delete space allocated for i:th slice. However the worst case
      //stays the same
    }
    delete pb;
  }
  compressedSize += PrecompressorBlock::writeEmptyHeader(m_out);

  return compressedSize;
}

} //namespace bwtc
