/**
 * @file Utils.hpp
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
 * Header for utility-functions which aren't related to any class that much.
 */

#ifndef BWTC_UTILS_HPP_
#define BWTC_UTILS_HPP_

#include "globaldefs.hpp"

#include <cassert>
#include <iostream>
#include <map>
#include <stack>
#include <utility>
#include <vector>
#include "Streams.hpp"

using bwtc::uint64;
using bwtc::byte;
using bwtc::uint32;

/* Useful functions for debugging activites etc. */
namespace utils {

template <typename T>
class CircularBuffer {
 public:
  explicit CircularBuffer(size_t len)
      : m_length(len), m_data((T*)malloc(sizeof(T)*len)), m_head(0),
        m_start(0), m_inBuffer(0) {}

  ~CircularBuffer() { free(m_data); }

  /** Returns the elements consumed (<= len)*/
  size_t consume(T* to, size_t len) {
    len = std::min(len, m_inBuffer);
    assert(len <= m_length);
    size_t newStart;
    if(m_start + len >= m_length) {
      std::copy(m_data + m_start, m_data + m_length, to);
      newStart = len - (m_length - m_start);
      std::copy(m_data, m_data + newStart, to + (m_length - m_start));
    } else {
      newStart = m_start + len;
      std::copy(m_data + m_start, m_data + newStart, to);
    }
    m_start = newStart;
    m_inBuffer -= len;
    return len;
  }

  inline void push(const T& val) {
    if(m_head >= m_length) m_head = 0;
    m_data[m_head++] = val;
    ++m_inBuffer;
  }

  inline bool full() const {
    return m_inBuffer >= m_length;
  }

 private:
  const size_t m_length;
  T* const m_data;
  size_t m_head; 
  size_t m_start;
  size_t m_inBuffer;
};


static byte logFloor(unsigned n) {
  assert(n > 0);
#ifdef __GNUC__
  return  sizeof(n)*__CHAR_BIT__ - __builtin_clz(n) - 1;
#else
  byte log = 0;
  while(n > 1) {
    n >>= 1;
    ++log;
  }
  return log;
#endif
}

static byte logFloor(unsigned long n) {
  assert(n > 0);
#ifdef __GNUC__
  return  sizeof(n)*__CHAR_BIT__ - __builtin_clzl(n) - 1;
#else
  byte log = 0;
  while(n > 1) {
    n >>= 1;
    ++log;
  }
  return log;
#endif
}

/** Ceiling of logarithm of base two */
template <typename Unsigned>
byte logCeiling(Unsigned n) {
  byte log = logFloor(n);
  return static_cast<Unsigned>(1 << log) < n ? log+1 : log;
}

uint32 mostSignificantBit16(uint32 n);

uint32 mostSignificantBit(uint32 n);

/*************************************************************************
 * PackInteger and UnpackInteger                                         *
 * Packs integer to bytefields, so that the first bit of the byte        *
 * tells if the number has succeeding bytes. Few examples:               *
 * 0xF0   -> 0x01F0                                                      *
 *   -- the last byte is F0, because of the continuation-bit             *
 * 0x2    -> 0x2                                                         *
     -- no overhead here                                                 *
 * 0x142A -> 0x28AA                                                      *
 *   -- no overhead here because the most significant byte had leading   *
 *      zeroes                                                           *
 *************************************************************************/
uint64 packInteger(uint64 integer, int* bytes_needed);

uint64 unpackInteger(uint64 packed_integer);

template <typename Input>
size_t readPackedIntegerRev(Input& input, size_t& bytesRead) {
  size_t read = 0xff, result = 0, j = 0;
  bytesRead = 0;
  while(read & 0x80) {
    read = 0;
    for(size_t i = 0; i < 8; ++i) {
      read |= ((input.readBit()?1:0) << i);
    }
    result |= ((read & 0x7f) << j);
    j += 7;
    ++bytesRead;
  }
  return result;
}

template <typename Input>
size_t readPackedInteger(Input& input, size_t& bytesRead) {
  size_t read = 0xff, result = 0, j = 0;
  bytesRead = 0;
  while(read & 0x80) {
    read = 0;
    for(int i = 7; i >= 0; --i) {
      read |= ((input.readBit()?1:0) << i);
    }
    result |= ((read & 0x7f) << j);
    j += 7;
    ++bytesRead;
  }
  return result;
}

void writePackedInteger(uint64 packed_integer, byte *to);

unsigned packAndWriteInteger(uint64 integer, byte *to);

unsigned readAndUnpackInteger(byte *from, uint64 *to);

void calculateRunFrequencies(uint64 *runFreqs, const byte *src, size_t length);

size_t calculateRunsAndCharacters(uint64 *runFreqs, const byte *src,
                                  size_t length, std::map<uint32, uint32>& runs);

uint64 calculateRunFrequenciesAndStoreRuns(uint64 *runFreqs, byte *runseq,
  uint32 *runlen,  const byte *src, size_t length);
  
bool isPrefix(const std::string &a, const std::string &b);
  
void computeHuffmanCodes(uint32 *clen, uint32 *code);

/**Calculates the code lengths in Huffman coding. Implementation is an
 * algorithm presented in a paper "In-Place Calculation of Minimum-Redundancy
 * codes" by Alistair Moffat and Jyrki Katajainen.
 *
 * @param codeLengths Answer is returned in vector consisting of
 *                    <codelength, symbol> pairs.
 * @param freqs Array of size alphabetSize.
 *              Array IS modified during the calculation.
 * @param alphabetSize Size of the freqs-array.
 */
void
calculateHuffmanLengths(std::vector<std::pair<uint64, uint32> >& codeLengths,
                        uint64 *freqs, uint32 alphabetSize=256);

void
calculateHuffmanLengths(std::vector<std::pair<uint64, uint32> >& codeLengths,
                        uint64 *freqs, const std::vector<uint32>& names);

void
calculateHuTuckerLengths(std::vector<std::pair<uint64, uint32> >& codeLengths,
                         uint64 *freqs, const std::vector<uint32>& names);

void calculateCodeLengths(std::vector<std::pair<uint64, uint32> >& codeLengths,
                          uint64 *freqs, bool sorted=false);
 
template <typename Unsigned, typename BitVector>
void pushBits(Unsigned n, byte bits, BitVector& bitVector) {
  for(size_t i = 1; i <= bits; ++i) {
    bitVector.push_back((n >> (bits-i))&1);
  }
}

/**Represents given integer in bits when the lower and upper bounds for integer
 * are known.
 *
 * @param n Integer to be coded.
 * @param lo The lower bound for n (ie. n >= lo).
 * @param hi The upper bound for n (ie. n <= hi).
 * @param bits Code bits are appended into this bitvector.
 */
template <typename BitVector>
void binaryCode(size_t n, size_t lo, size_t hi, BitVector& bits) {
  size_t rangeLen = hi - lo + 1;
  if(rangeLen == 1) return;
  byte codeLength = logCeiling(rangeLen);
  size_t shortCodewords = (1 << codeLength) - rangeLen;
  size_t longCodewords2 = (rangeLen - shortCodewords)/2;
  if(n - lo < longCodewords2) {
    pushBits(n - lo, codeLength, bits);
  } else if(n - lo < longCodewords2 + shortCodewords) {
    pushBits(n - lo, codeLength - 1, bits);
  } else {
    pushBits(n - lo - shortCodewords, codeLength, bits);
  }
}

/**Calculates binary interpolative code for the sorted list of integers.
 *
 * @param list Sorted list of integers to code.
 * @param begin The first index of list to be coded.
 * @param end The last index of list to be coded.
 * @param lo Minimum possible value.
 * @param hi Maximum possible value.
 * @param bitVector The code is returned in this vector.
 */
template<typename Integer, typename BitVector>
void binaryInterpolativeCode(const std::vector<Integer>& list, size_t begin,
                             size_t end, size_t lo, size_t hi,
                             BitVector& bitVector)
{
  if(begin > end) return;
  if(end - begin == hi - lo) return;
  if(begin == end) {
    binaryCode(list[begin], lo, hi, bitVector);
    return;
  }
  size_t h = (end - begin) / 2;
  size_t half = begin + h;
  binaryCode(list[half], lo + h, hi + half - end, bitVector);
  if(half > begin)
    binaryInterpolativeCode(list, begin, half-1, lo, list[half] - 1, bitVector);
  binaryInterpolativeCode(list, half+1, end, list[half] + 1, hi, bitVector);
}

/**Calculates binary interpolative code for the sorted list of integers
 * according the paper "Binary Interpolative Coding for Effective Index
 * Compression" by Alistair Moffat and Lang Stuiver. It is assumed that
 * the coded values are from range [0..maxValue].
 *
 * @param list Sorted list of integers to code.
 * @param maxValue Maximum possible value for integer to be coded.
 * @param bitVector The code is returned in this vector.
 */
template<typename Integer, typename BitVector>
void binaryInterpolativeCode(const std::vector<Integer>& list, size_t maxValue,
                             BitVector& bitVector)
{
  assert(!list.empty());
  binaryInterpolativeCode(list, 0, list.size() - 1, 0, maxValue, bitVector);
}

template <typename Input>
size_t binaryDecode(Input& input, size_t lo, size_t hi) {
  size_t rangeLen = hi - lo + 1;
  if(rangeLen == 1) return lo;
  byte codeLength = logCeiling(rangeLen);
  size_t shortCodewords = (1 << codeLength) - rangeLen;
  size_t longCodewords2 = (rangeLen - shortCodewords)/2;
  size_t result = 0;
  for(int i = 0; i < codeLength - 1; ++i) {
    result <<= 1;
    result |= (input.readBit()) ? 1 : 0;
  }
  if(result >= longCodewords2) {
    return result + lo;
  } 
  result <<= 1;
  result |= (input.readBit()) ? 1 : 0;
  if (result < longCodewords2) return result + lo;
  else return result + lo + shortCodewords;
}

template <typename Input>
size_t binaryDecode(Input& input, size_t lo, size_t hi, size_t *bitsRead) {
  size_t rangeLen = hi - lo + 1;
  if(rangeLen == 1) return lo;
  byte codeLength = logCeiling(rangeLen);
  size_t shortCodewords = (1 << codeLength) - rangeLen;
  size_t longCodewords2 = (rangeLen - shortCodewords)/2;
  size_t result = 0;
  *bitsRead += codeLength;
  for(int i = 0; i < codeLength - 1; ++i) {
    result <<= 1;
    result |= (input.readBit()) ? 1 : 0;
  }
  if(result >= longCodewords2) {
    --*bitsRead;
    return result + lo;
  } 
  result <<= 1;
  result |= (input.readBit()) ? 1 : 0;
  if (result < longCodewords2) return result + lo;
  else return result + lo + shortCodewords;
}

template <typename Integer, typename Input>
size_t binaryInterpolativeDecode(std::vector<Integer>& list, Input& input,
                                 size_t lo, size_t hi, size_t elements)
{
  if(elements == 0) return 0;
  if(elements == hi - lo + 1) {
    size_t i = lo;
    for(Integer b = lo; i <= hi; ++i, ++b) list.push_back(b);
    return 0;
  }
  size_t bitsRead = 0;
  size_t h = (elements-1)/2;
  size_t r = elements/2 - h;
  Integer mid = binaryDecode(input, lo + h, hi - h - r, &bitsRead);

  bitsRead += binaryInterpolativeDecode(list, input, lo, mid-1, h);
  list.push_back(mid);
  bitsRead +=  binaryInterpolativeDecode(list, input, mid+1, hi, elements-h-1);
  return bitsRead;
}


/**Decodes binary interpolative code. It is assumed that the decoded values
 * are from range [0..maxValue].
 *
 * @param list List for the result integers.
 * @param input Source for the bits. Type must have readBit()-function
 *              returning bool.
 * @param maxValue Maximum possible value for integer to be decoded.
 * @param elements Integers to be decoded.
 * @return Bits read.
 */
template <typename Integer, typename Input>
size_t binaryInterpolativeDecode(std::vector<Integer>& list, Input& input,
                                 size_t maxValue, size_t elements)
{
  return binaryInterpolativeDecode(list, input, 0, maxValue, elements);
}

template <typename BitVector, typename Integer>
inline void pushBits(BitVector& bv, Integer n, size_t bits) {
  for(size_t i = 1; i <= bits; ++i)
    bv.push_back( (n >> (bits-i)) & 1 );
}

template <typename BitVector, typename Integer>
inline void pushBitsRev(BitVector& bv, Integer n, size_t bits) {
  for(size_t i = 0; i < bits; ++i) {
    bv.push_back( n & 1 );
    n >>= 1;
  }
}

/**Forms unary code for given integer and pushes it into the back of
 * given bitvector. Few examples of unary code:
 * 1 -> 1
 * 2 -> 01
 * 7 -> 0000001
 *
 * @param to Bitvector where the result is pushed.
 * @param n Number to encode.
 */
template <typename BitVector>
inline void unaryCode(BitVector& to, size_t n) {
  while(n-- > 1) to.push_back(false);
  to.push_back(true);
}

template <typename Input>
inline size_t unaryDecode(Input& in) {
  size_t n = 1;
  while(!in.readBit()) ++n;
  return n;
}

template <typename Integer>
void printBitRepresentation(Integer word) {
  std::stack<Integer> bits;
  int bytes = sizeof(Integer);
  for(int j = 0; j < bytes; ++j) {
    for(int i = 0; i < 8; ++i) {
      int num = (word & 0x01) ? 1 : 0;
      bits.push(num);
      word >>= 1;
    }
  }
  int i = 0;
  while(!bits.empty()) {
    if(i % 8 == 0 && i) std::cout << " ";
    std::cout << bits.top();
    bits.pop();
    ++i;
  }
  std::cout << "\n";
} 

template<typename Integer>
inline size_t gammaEncode(std::vector<Integer>& ints, bwtc::OutStream* out,int offset=0) {
    size_t bytes_used=0;
    out->flush();

    bwtc::uint64 buffer = 0;
    bwtc::int32 bitsInBuffer = 0;
    for (bwtc::uint64 k = 0; k < ints.size(); ++k) {
        bwtc::uint64 num=ints[k]+offset;
        int gammaCodeLen = utils::logFloor(num) * 2 + 1;
        while (bitsInBuffer + gammaCodeLen > 64) {
            bitsInBuffer -= 8;
            out->writeByte((buffer >> bitsInBuffer) & 0xff);
            bytes_used++;
        }
        buffer <<= gammaCodeLen;
        buffer |= num;
        bitsInBuffer += gammaCodeLen;
    }
    while (bitsInBuffer >= 8) {
        bitsInBuffer -= 8;
        out->writeByte((buffer >> bitsInBuffer) & 0xff);
        ++bytes_used;
    }
    if (bitsInBuffer > 0) {
        buffer <<= (8 - bitsInBuffer);
        out->writeByte(buffer & 0xff);
        ++bytes_used;
    }
    out->flush();
    return bytes_used;
}
template<typename Integer>
inline void gammaDecode(std::vector<Integer>& ints, bwtc::InStream* in, int offset=0) {
        for (bwtc::uint64 k = 0; k < ints.size(); ++k) {

            int zeros = 0;

            while (!in->readBit())
                ++zeros;
            bwtc::uint64 value = 0;        
            for (bwtc::int32 t = 0; t < zeros; ++t) {
                bwtc::int32 bit = in->readBit();
                value = (value << 1) | bit;
            }
            value |= (1 << zeros);
            ints[k] = (Integer)(value-offset);
        }
        in->flushBuffer();
}
template<typename Integer>
inline size_t deltaEncode(std::vector<Integer>& ints, bwtc::OutStream* out, int offset=0) {
    size_t bytes_used=0;
    out->flush();

    bwtc::uint64 buffer = 0;
    bwtc::int32 bitsInBuffer = 0;
    for (bwtc::uint64 k = 0; k < ints.size(); ++k) {
        bwtc::uint64 n=ints[k]+offset;
        int codelength = logFloor(n)+2*logFloor(logFloor(n)+1UL)+1;
        uint64 len = logFloor(n)+1;
        uint64 lenoflen = logFloor(len);

        while (bitsInBuffer + codelength > 64) {
            bitsInBuffer -= 8;

            out->writeByte((buffer >> bitsInBuffer) & 0xff);
            bytes_used++;
        }
        buffer <<= 2*lenoflen+1;
        buffer |= len;
        buffer <<=len-1;
        buffer |= ((~(1<<(len-1))) & n);
        bitsInBuffer += codelength;

    }
    while (bitsInBuffer >= 8) {
        bitsInBuffer -= 8;
        out->writeByte((buffer >> bitsInBuffer) & 0xff);
        ++bytes_used;
    }
    if (bitsInBuffer > 0) {
        buffer <<= (8 - bitsInBuffer);
        out->writeByte(buffer & 0xff);
        ++bytes_used;
    }
    out->flush();
    return bytes_used;
}
template<typename Integer>
inline void deltaDecode(std::vector<Integer>& ints, bwtc::InStream* in, int offset=0) {
        for (bwtc::uint64 k = 0; k < ints.size(); ++k) {

            int zeros = 0;

            while (!in->readBit())
                ++zeros;
            bwtc::uint64 value = 0;        
            for (bwtc::int32 t = 0; t < zeros; ++t) {
                bwtc::int32 bit = in->readBit();
                value = (value << 1) | bit;
            }
            value |= (1 << zeros);
            //gamma code: value
            
            // read value-1 bits
            
            bwtc::uint64 val2=1;
            for(bwtc::int32 t=0;t<value-1;t++) {
                bwtc::int32 bit = in->readBit();
                val2=(val2<<1)|bit;
                
            }
            ints[k]=(Integer)(val2-offset);
        }
}


} //namespace utils

#endif
