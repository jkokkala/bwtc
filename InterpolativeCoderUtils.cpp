/**
 * @file InterpolativeCoderUtils.cpp
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
 * Implementation of interpolative encoding and decoding utilities
 */
#include "InterpolativeCoderUtils.hpp"
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include "Utils.hpp"
using namespace std;
namespace bwtc {
    FreqMem::FreqMem(byte* block, uint size) {
        alloc=true;
        this->block=block;
        this->size=size;
        big_interval=1;
        while((size/big_interval)*256 > 2000000) {
            big_interval <<=1;
        }
        big_size = size/big_interval;
        big_mem=(uint*) calloc((big_size+1)*256,sizeof(uint));
        small_mem=(uint*) calloc(big_interval*256,sizeof(uint));
        for(int i=0;i<=size;i++) {
            if(i>0 && (i%big_interval)==0) {
                for(int j=0;j<256;j++) big_mem[(i/big_interval)*256+j]=big_mem[(i/big_interval-1)*256 +j];
            }
            if(i<size) big_mem[(i/big_interval)*256+block[i]]++;
        }
        small_begin=0,small_end=0;
        smallbytemap.resize(256);
        small_interval=16;
    }
    freq FreqMem::search(uint a, uint b,vector<byte>& bytes) {
        uint len = b - a + 1;
        if(len < big_interval) {
            return small_search(a,b,bytes);
        }
        b++;
        int left = a / big_interval-1;
        int right = b/big_interval-1;
        if(b== size && b%big_interval != 0) right++;
        freq f(bytes.size());
        for(int i=0;i<bytes.size();i++) {
            int byte = bytes[i];
            f[i] = big_mem[right*256 + byte];
            if(left >=0) f[i] -= big_mem[left*256 +byte];
            f.bytes[i]=byte;
            f.bytemap[byte]=i;
        }
        return f;
    }
    void FreqMem::update_small(uint a, uint b, vector<byte>& bytes) {
        b=a+big_interval-1;
        if(b>=size) b=size-1;

        memset(small_mem,0,(big_interval/small_interval+2)*bytes.size());
        smallmemsize=bytes.size();
        for(int i=0;i<bytes.size();i++) smallbytemap[bytes[i]]=i;
        small_begin=a;
        small_end=b;
        for(int i=0;i<b-a+1;i++) {
            if(i>0 && (i%small_interval)==0) {
                int ii = i/small_interval;
               for(int j=0;j<smallmemsize;j++) small_mem[ii*smallmemsize+j]=small_mem[(ii-1)*smallmemsize+j];
            }
            small_mem[(i/small_interval)*smallmemsize+smallbytemap[block[i+a]]]++;
        }
        for(int i=0;i<big_interval/small_interval;i++) {
            int sum=0;
            for(int j=0;j<smallmemsize;j++) { 
                int t = small_mem[i*smallmemsize+j];
                sum+=t;
            }
        }
    }

    freq FreqMem::small_search(uint a, uint b,vector<byte>& bytes) {
        if(b-a+1 <small_interval) {
            freq f2;
            f2.set_bytes(bytes,true);
            for(int i=a;i<=b;i++) f2[f2.bytemap[block[i]]]++;
            return f2;
        }
        if(!(a >=small_begin && b <= small_end)) {
            update_small(a,b,bytes);            
        }
        b++;
        int32 left = (a-small_begin)/small_interval-1;
        int32 right = (b-small_begin)/small_interval-1;
        if(b==size&&(b-small_begin)%small_interval !=0) right++;
        freq f;
        f.set_bytes(bytes);
        int sum=0;
        for(int i=0;i<bytes.size();i++) {
            f[i]=small_mem[right*smallmemsize+smallbytemap[bytes[i]]];
            if(left>=0) f[i]-=small_mem[left*smallmemsize+smallbytemap[bytes[i]]];
            sum += f[i];
        }
        assert(sum==b-a);
        return f;
    }


    FreqMem::~FreqMem() {
        if(alloc) {
            delete[] big_mem;
            delete[] small_mem;
        }
    }
}
