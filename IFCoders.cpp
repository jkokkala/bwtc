/**
 * @file IFCoders.cpp
 * @author Jussi Kokkala <jkokkala@gmail.com>
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
 * Implementations of IF encoder and decoder.
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

#include "IFCoders.hpp"
#include "globaldefs.hpp"
#include "Utils.hpp"
#include "Profiling.hpp"
using namespace std;
namespace bwtc {

    IFEncoder::IFEncoder(){}

    IFEncoder::~IFEncoder(){}
    
    vector<uint32> sort_index(vector<uint32>& freqs) {
        vector<uint32> order(freqs.size());

        for(int i=0;i<order.size();i++) order[i]=i;
        for(int i=0;i<order.size();i++) {
            for(int j=i+1;j<order.size();j++) {
                if(freqs[order[i]]>freqs[order[j]]) {
                    swap(order[i],order[j]);
                }
            }
        }
        return order;
    }

    size_t IFEncoder::transformAndEncode(BWTBlock& block, BWTManager& bwtm, OutStream* out) {
        bwtm.doTransform(block);
        PROFILE("IFEncoder::encodeData");
        size_t bytes_used=block.writeHeader(out);
        vector<byte> vec = RLE(block.begin(),block.size(),255,4,out,bytes_used);


        byte* data = vec.data();
        vector<uint32> freqs(256,0);
        for(int i=0;i<vec.size();i++) freqs[data[i]]++;
        bytes_used+=utils::gammaEncode(freqs,out,1);
        vector<uint32> order=sort_index(freqs);
        map<int,int> m;
        vector<bool> proc(256,false);
        for(int i=0;i<255;i++) {
            int ch = order[i];
            proc[ch]=true;
            if(freqs[ch]==0) continue;
            bool first=false;
            vector<int> occ(freqs[ch]);
            uint32 count=0;
            int it=1;
            for(int j=0;j<vec.size();j++) {
                if(first) {
                    if(!proc[vec[j]]) count++;
                    else if(vec[j]==ch) {
                        occ[it++]=count;
                        count=0;
                    }

                }
                
                if(!first&&vec[j]==ch) {
                    first=true;

                    occ[0]=j;
                }
                
            }
            bytes_used+=utils::gammaEncode(occ,out,1);
        }
        out->flush();
        return bytes_used;
    }

    void IFDecoder::decodeBlock(BWTBlock& block, InStream* in) {

        PROFILE("IFDecoder::decodeBlock");
        if(in->compressedDataEnding()) return;


        block.readHeader(in);
        int extra;

        vector<uint64> runs = readRLE(in,extra);
        std::vector<bool> mark;
        std::vector<uint32> freqs(256);
        utils::gammaDecode(freqs,in,1);
        uint32 len=0;
        for(int i=0;i<freqs.size();i++) len+=freqs[i];
        vector<uint32> order=sort_index(freqs);
        mark.resize(len,false);
        vector<byte> vec(len);
        block.setSize(len+extra);
        for(byte i=0;i<255;i++) {
            int ch=order[i];
            if(freqs[ch]==0) continue;
            vector<uint32> occ(freqs[ch]);
            utils::gammaDecode(occ,in,1);
            vec[occ[0]]=ch;
            mark[occ[0]]=true;
            int last=occ[0];
            for(int j=1;j<occ.size();j++) {
                for(int k=0;k<=occ[j];k++) {
                    last++;
                    while(mark[last]) last++;
                }
                vec[last]=ch;
                mark[last]=true;
            }
        }
        in->flushBuffer();
        for(int i=0;i<len;i++) if(!mark[i]) vec[i]=order[255];
        byte* block_ptr=block.begin();
        byte prev=0;
        int cur_run=0,minrun=4,maxval=255,run_iter=0;
        for(int i=0;i<vec.size();i++) {
            byte temp = vec[i];
            *(block_ptr++) = temp;
            if(temp==prev && i!=0) cur_run++;
            else {
                prev=temp;
                cur_run=1;
            }
            if(cur_run>=minrun && temp<=maxval) {
                int length=runs[run_iter++];
                for(int j=0;j<length-1;j++) {
                    *(block_ptr++)=temp;
                }
            }
        }
    }

    IFDecoder::IFDecoder() {}

    IFDecoder::~IFDecoder() {}
    std::vector<byte> IFEncoder::RLE(byte* orig, uint32 length, byte maxval, int minrun, OutStream* out, size_t& bytes_used) {

        std::vector<byte> data;
        std::vector<uint64> runlengths;
        data.reserve(length);
        int current_runlength=1;
        byte current_runchar=0;
        byte* ptr = orig;
        data.push_back(*ptr++);
        current_runchar=data[0];
        for(int i=1;i<length;i++) {
            byte cur = (*ptr++);
            if(cur==current_runchar) {
                current_runlength++;
            } else {
                if(current_runlength>=minrun && current_runchar<=maxval) {
                    runlengths.push_back(current_runlength-minrun+1);
                }
                current_runchar=cur;
                current_runlength=1;
            }
            if(current_runlength>minrun && cur <= maxval) {
                // run continues
            } else {
                data.push_back(cur);
            }

        }
        if(current_runlength >=minrun && current_runchar <= maxval) runlengths.push_back(current_runlength-minrun+1);
        out->flush();
        int pos=out->getPos();
        for(int i=0;i<6;i++) out->writeByte(0);
        out->write48bits(runlengths.size(),pos);
        bytes_used+=6;
        bytes_used+= utils::gammaEncode(runlengths,out);
        out->flush();

        return data;
    }
    std::vector<uint64> IFDecoder::readRLE(InStream* in, int& extra) {
        int num_runs= in->read48bits();
        std::vector<uint64> runs(num_runs);
        extra=0;
        utils::gammaDecode(runs,in);
        for (uint64 k = 0; k < num_runs; ++k) {
            extra += runs[k] -1;
        }
        in->flushBuffer();
        return runs;

    }


} // namespace bwtc

