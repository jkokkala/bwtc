/**
 * @file ArithmeticCoders.cpp
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
 * Implementations of Arithmetic encoder and decoder.
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
#include "HuffmanUtil.hpp"
#include "ArithmeticCoders.hpp"
#include "globaldefs.hpp"
#include "Utils.hpp"
#include "Profiling.hpp"
#include "ArithmeticUtil.hpp"
namespace bwtc {

    ArithmeticEncoder::ArithmeticEncoder()
        : m_headerPosition(0), m_compressedBlockLength(0) {}

    ArithmeticEncoder::~ArithmeticEncoder() {}
        
    size_t ArithmeticEncoder::
        transformAndEncode(BWTBlock& block, BWTManager& bwtm, OutStream* out) {
            bool rle=true;
            bwtm.doTransform(block);
            PROFILE("ArithmeticEncoder::encodeData");
            size_t bytes_used=block.writeHeader(out);
            int maxval=255;
            int minrun=3;

            uint64 size=block.size();
            std::vector<uint32> context_lengths(256, 0);
            int a = 256;
            while(size/a < 1000000 && a>1) {
                a/=2;
            }
            
            for(int i=0;i<a;i++) context_lengths[i]=size/a;
            context_lengths[0] += size % a;

            out->writeByte(a>>8);
            out->writeByte(a&0xff);
            bytes_used+=2;


            byte* ptr=block.begin(); 
            for(int i=0;i<a;i++) {
                out->flush();
                long p=out->getPos();
                std::vector<byte> data;
                if(rle) data= RLE(ptr,context_lengths[i],maxval,minrun,out,bytes_used);
                else data=std::vector<byte>(block.begin(),block.end());
                ArithmeticUtilEncoder encoder(out);
                HuffmanUtilEncoder huffman;
//                bytes_used +=huffman.encode(data.data(),data.size(),out);
                bytes_used+=encoder.encode(data.data(),data.size());

                out->flush();
            
                ptr += context_lengths[i];
            }

            return bytes_used;

        }



    void ArithmeticDecoder::decodeBlock(BWTBlock& block, InStream* in) {
        PROFILE("ArithmeticDecoder::decodeBlock");
        bool rle=true;
        if(in->compressedDataEnding()) return;
        block.readHeader(in);

        std::vector<uint64> context_lengths;

        byte* ptr = block.begin();
        int size=0;
        int count=(in->readByte()<<8)|in->readByte();
        for(int i=0;i<count;i++) {
            in->flushBuffer();
            long p=in->pos;
            std::vector<byte> data;
            ArithmeticUtilDecoder decoder(in);
            HuffmanUtilDecoder huffman;
            int extra;
            std::vector<uint64> runs;
            if(rle) runs=readRLE(in,extra);
            int minrun=3;
            int maxval=255;
            byte temp;
 //           huffman.decodeBlock(data,in);
            decoder.decode(data);
            in->flushBuffer();
            byte prev=0;
            int cur_run=0;
            int run_iter=0;
            for(int j=0;j<data.size();j++) {
                temp=data[j];
                *(ptr++) = temp;
                if(rle) {
                if(temp==prev && j!=0) cur_run++;
                else {
                    prev=temp;
                    cur_run=1;
                }
                if(cur_run>=minrun && temp<=maxval) {
                    int length=runs[run_iter++];
                    for(int k=0;k<length-1;k++) {
                        *(ptr++)=temp;
                    }
                }
                }
            }
            size+=data.size()+extra;
        }

        block.setSize(size);
    }
    ArithmeticDecoder::ArithmeticDecoder() {}

    ArithmeticDecoder::~ArithmeticDecoder() {}
    std::vector<byte> ArithmeticEncoder::RLE(byte* orig, uint32 length, byte maxval, int minrun, OutStream* out, size_t& bytes_used) {
        
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

    std::vector<uint64> ArithmeticDecoder::readRLE(InStream* in, int& extra) {
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

