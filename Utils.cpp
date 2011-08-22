/**
 * @file Utils.cpp
 * @author Pekka Mikkola <pjmikkol@cs.helsinki.fi>
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
 * Implementation of utility functions.
 */

#include <cassert>
#include <algorithm>
#include <utility>
#include <vector>

#include "globaldefs.hpp"
#include "Utils.hpp"

namespace utils {

uint32 mostSignificantBit16(uint32 n) {
  assert(n < (1 << 16));
  n |= (n >> 1);
  n |= (n >> 2);
  n |= (n >> 4);
  n |= (n >> 8);
  return n & (~(n >> 1));
}

uint32 mostSignificantBit(uint32 n) {
  assert(sizeof(uint32) == 4);
  n |= (n >> 1);
  n |= (n >> 2);
  n |= (n >> 4);
  n |= (n >> 8);
  n |= (n >> 16);
  return n & (~(n >> 1));
}

static const uint64 kEightBit = 0x80;

unsigned packAndWriteInteger(uint64 integer, byte *to) {
  unsigned bytes_used = 0;
  while(1) {
    if(integer) {
      byte to_written = static_cast<byte>(integer & 0x7F);
      integer >>= 7;
      if(integer) to_written |= 0x80;
      *to++ = to_written;
      ++bytes_used;
    } else {
      return bytes_used;
    }
  }
}

/* Returns the bytes used for the integer */
unsigned readAndUnpackInteger(byte *from, uint64 *to) {
  unsigned bytes_read = 0;
  uint64 result = 0;
  while(1) {
    uint64 read = *from;
    result |= ((read & 0x7F) << 7*bytes_read);
    ++bytes_read;
    if (((*from) & 0x80) == 0) break;
    else ++from;
  }
  *to = result;
  return bytes_read;
}

uint64 packInteger(uint64 integer, int* bytes_needed) {
  uint64 result = 0; int i;
  // For optimization (if needed) two OR-operations could be merged
  for(i = 0; integer; ++i) {
    result |= ((integer & 0x7F) << i*8);
    integer >>= 7;
    assert(i < 8);
    if (integer) result |= (kEightBit << i*8);
  }
  *bytes_needed = i;
  return result;
}

uint64 unpackInteger(uint64 packed_integer) {
  uint64 result = 0; int bits_handled = 0;
  bool bits_left;
  do {
    bits_left = (packed_integer & 0x80) != 0;
    result |= ((packed_integer & 0x7F) << bits_handled);
    packed_integer >>= 8;
    bits_handled += 7;
    assert(bits_handled <= 56);
  } while(bits_left);
  return result;
}

void calculateRunFrequencies(uint64 *runFreqs, const byte *src, size_t length)
{
  const byte *prev = src;
  const byte *curr = src+1;
  do {
    while(curr < src + length && *prev == *curr) ++curr;
    ++runFreqs[*prev];
    prev = curr++;
  } while(curr < src + length);
  if(prev < src + length) ++runFreqs[*prev];
}

void calculateHuffmanLengths(std::vector<std::pair<uint64, byte> >& codeLengths,
                             uint64 *freqs)
{
  assert(codeLengths.size() == 0);
  for(size_t i = 0; i < 256; ++i) {
    if(freqs[i])
      codeLengths.push_back(std::make_pair(freqs[i], static_cast<byte>(i)));
  }
  if(codeLengths.size() == 1) {
    codeLengths[0].first = 1;
    return;
  }
  
  std::sort(codeLengths.begin(), codeLengths.end());
  const size_t n = codeLengths.size();
  for(size_t i = 0; i < n; ++i) {
    freqs[i] = codeLengths[i].first;
  }

  size_t s = 0, r = 0;
  for(size_t t = 0; t < n-1; ++t) {
    if(s >= n || (r < t && freqs[r] < freqs[s])) {
      freqs[t] = freqs[r];
      freqs[r++] = t;
    } else {
      freqs[t] = freqs[s++];
    }
    if(s >= n || (r < t && freqs[r] < freqs[s])) {
      freqs[t] += freqs[r];
      freqs[r++] = t;
    } else {
      freqs[t] += freqs[s++];
    }
  }

  freqs[n-2] = 0;
  for(int t = n-3; t >= 0; --t) {
    freqs[t] = freqs[freqs[t]] + 1;
  }
  int a = 1, u = 0, depth = 0;
  int x = n-1, t = n-2;
  while(a > 0) {
    while(t >= 0 && freqs[t] == depth) { ++u; --t; }
    while(a > u) { freqs[x] = depth; --x; --a; }
    a = 2*u;
    ++depth;
    u = 0;
  }
  for(size_t i = 0; i < n; ++i) {
    codeLengths[i].first = freqs[i];    
  }
}

void writePackedInteger(uint64 packed_integer, byte *to) {
  do {
    byte to_written = static_cast<byte>(packed_integer & 0xFF);
    packed_integer >>= 8;
    *to++ = to_written;
  } while (packed_integer);
}

uint64 readPackedInteger(byte *to) {
  static const uint64 kEndSymbol = static_cast<uint64>(1) << 63;
  static const uint64 kEndMask = static_cast<uint64>(1) << 7;

  uint64 packed_integer = 0;
  bool bits_left = true;
  int i;
  for(i = 0; bits_left; ++i) {
    uint64 read = *to++;
    bits_left = (read & kEndMask) != 0;
    packed_integer |= (read << i*8);
  }
  if (packed_integer == 0x80) return kEndSymbol;
  return packed_integer;

}

} //namespace utils
