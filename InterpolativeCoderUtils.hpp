/**
 * @file InterpolativeCoderUtils.hpp
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
 * @section DESCRIPTION
 *
 * Header file for interpolative encoding and decoding utilities
 */


#ifndef INTERPOLATIVECODERUTILS_HPP
#define INTERPOLATIVECODERUTILS_HPP
#include "globaldefs.hpp"
#include<vector>
#include<iostream>
#include<cassert>
using namespace std;
namespace bwtc {
    struct freq {
        vector<uint32> frequencies;
        vector<byte> bytes;
        vector<int> bytemap;
        const int size() const { return frequencies.size();}
        uint32& operator[](unsigned int i){ return frequencies[i];}
        const uint32& operator[](unsigned int i)const{ return frequencies[i];}
        freq() {}
        freq(int s) { 
            frequencies.resize(s,0);
            bytes.resize(s,0);
            bytemap.resize(256);
            if(s==256) for(int i=0;i<s;i++) bytemap[i]=bytes[i]=i;
        }

        freq& operator=(const freq& other) {
            if(this != &other) {
                frequencies.resize(other.size());
                bytes.resize(other.size());
                bytemap.resize(other.bytemap.size());
                for(int i=0;i<bytemap.size();i++) bytemap[i]=other.bytemap[i];
               for(int i=0;i<other.size();i++) frequencies[i]=other[i];
                for(int i=0;i<other.size();i++) bytes[i]=other.bytes[i];
            }
        }

        freq & operator+=(const freq &other) {
            for(int i=0;i<size();i++) frequencies[i]+=other[i];
            return *this;
        }
        freq & operator-=(const freq &other) {
            for(int i=0;i<size();i++) frequencies[i]+=other[i];
            return *this;
        }
        
        void set_bytes(vector<byte>& b,bool clear=false) {
            frequencies.resize(b.size());
            bytemap.resize(256);
            bytes.resize(b.size());
            for(int i=0;i<b.size();i++) {
                bytemap[b[i]]=i;
                bytes[i]=b[i];
                if(clear) frequencies[i]=0;
            }
        }
        const freq operator+(const freq& other) const {
            freq a = freq(size());
            for(int i=0;i<size();i++) {
                a[i]=frequencies[i]+other[i];
                a.bytes[i]=bytes[i];

            }
            a.bytemap.resize(other.bytemap.size());
            for(int i=0;i<bytemap.size();i++) a.bytemap[i]=other.bytemap[i];
            return a;
        }
        const freq operator-(const freq& other) const {
            freq a = freq(size());
            for(int i=0;i<size();i++) {
                a[i]=frequencies[i]-other[i];
                a.bytes[i]=bytes[i];
            }
            a.bytemap.resize(other.bytemap.size());
            for(int i=0;i<bytemap.size();i++) a.bytemap[i]=other.bytemap[i];
            return a;
        }
        void clean() {
            int previous_slot=0;
            int removes=0;
            for(int i=0;i<frequencies.size();i++) {
                if(frequencies[i]==0) removes++;
                else {
                    while(previous_slot<i && frequencies[previous_slot]!=0) previous_slot++;
                    if(frequencies[previous_slot]==0) {
                        frequencies[previous_slot]=frequencies[i];
                        frequencies[i]=0;
                        bytes[previous_slot]=bytes[i];
                        bytemap[bytes[i]]=previous_slot;
                    }   
                }   
            }   
            frequencies.resize(frequencies.size()-removes);
            bytes.resize(size());
        }
    };

    class FreqMem {
        public:
            bool alloc;
            FreqMem(byte* begin, uint size);
            FreqMem() : alloc(false),smallbytemap(256){}
            ~FreqMem();
            freq search(uint a, uint b,vector<byte>& bytes);

            freq small_search(uint a, uint b,vector<byte>& bytes);
            void update_small(uint a, uint b,vector<byte>& bytes);
        private:
            byte* block;
            uint size;
            uint big_interval;
            uint big_size;
            uint* big_mem;

            uint small_interval;
            uint small_begin;
            uint small_end;
            uint* small_mem;
            uint smallmemsize;
            uint smallmemcount;
            vector<byte> smallbytemap;

    };
}
#endif
