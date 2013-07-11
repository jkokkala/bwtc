/**
 * @file EntropyCoders.cpp
 * @author Dominik Kempa <dominik.kempa@cs.helsinki.fi>
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
 * Implementation of base classes for entropy coders.
 *
 */

#include <iostream>
#include <string>
#include <vector>
#include "IFCoders.hpp"
#include "EntropyCoders.hpp"
#include "WaveletCoders.hpp"
#include "HuffmanCoders.hpp"
#include "ArithmeticCoders.hpp"
#include "MTFCoders.hpp"
#include "InterpolativeCoders.hpp"
namespace bwtc {

EntropyEncoder*
giveEntropyEncoder(char encoder) {
  if(encoder == 'H') {
    if(verbosity > 1) {
      std::clog << "Using Huffman encoder\n";
    }
    return new HuffmanEncoder();
   

  } else if(encoder=='W') {
  
    if(verbosity > 1) {
      std::clog << "Using Wavelet tree encoder\n";
    }
    return new WaveletEncoder(encoder);

  } else if(encoder=='m') {
    if(verbosity > 1) {
      std::clog << "Using Arithmetic encoder\n";
    }
    return new ArithmeticEncoder();
  } else if(encoder=='i') {
    return new InterpolativeEncoder();
  } else if(encoder=='G') {
    return new IFEncoder();
  } 
  else {
    if(verbosity > 1) {
      std::clog << "Using MTF encoder\n";
    }
    return new MTFEncoder(encoder);
  }
}

EntropyDecoder* giveEntropyDecoder(char decoder) {
  if(decoder == 'H') {
    if(verbosity > 1) {
      std::clog << "Using Huffman decoder\n";
    }
    return new HuffmanDecoder();
  }
  else if(decoder=='W') {
    if(verbosity > 1) {
      std::clog << "Using Wavelet tree decoder\n";
    }
    return new WaveletDecoder(decoder);
  } else if(decoder=='m') {
    if(verbosity > 1) {
      std::clog << "Using Arithmetic decoder\n";
    }
    return new ArithmeticDecoder();
  }
   else if(decoder=='i') {
    return new InterpolativeDecoder();
  }
   else if(decoder=='G') {
    return new IFDecoder();
  } 

  else {
    if(verbosity > 1) {
      std::clog << "Using MTF decoder\n";
    }
    return new MTFDecoder(decoder);
  }
}

} // namespace bwtc
