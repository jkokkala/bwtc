/**
 * @file MTFCoders.cpp
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
 * Implementations of MTF encoder and decoder.
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

#include "MTFCoders.hpp"
#include "HuffmanUtil.hpp"
#include "HuffmanRLUtil.hpp"
#include "globaldefs.hpp"
#include "Utils.hpp"
#include "Profiling.hpp"

namespace bwtc {

    MTFEncoder::MTFEncoder(char encoder)
        : m_headerPosition(0), m_compressedBlockLength(0), m_encoder(encoder) {}

    MTFEncoder::~MTFEncoder() {}

    size_t MTFEncoder::transformAndEncode(BWTBlock& block, BWTManager& bwtm, OutStream* out) {

        bwtm.doTransform(block);
        PROFILE("MTFEncoder::encodeData");
        size_t bytes_used=block.writeHeader(out)+6;
        //initialize rank list
        for(int i=0;i<=0xff;i++) m_rankList.push_back(i);

        std::vector<byte> data;


        int zero_run_length=0;
        std::vector<uint64> zero_runs;
        for(byte* i = block.begin();i!=block.end();i++) {
            for(int pos=0;pos<=0xff;pos++) {
                if(m_rankList[pos]==(*i)) {
                    if(m_encoder=='0') {
                        if(pos==0) {
                            zero_run_length++;
                            if(zero_run_length>1) break;
                        }
                        else if(zero_run_length>0) {
                            zero_runs.push_back(zero_run_length);
                            zero_run_length=0;
                        }
                    }
                    data.push_back(pos);
                    for(int j=pos;j>0;j--) {
                        m_rankList[j]=m_rankList[j-1];                            
                    }
                    m_rankList[0]=*i;
                    break;
                }
            }
        }
        if(zero_run_length>0) zero_runs.push_back(zero_run_length);
        if(m_encoder=='F' ||m_encoder=='0') {
            HuffmanUtilEncoder huffman;
            bytes_used+=huffman.encode(data.data(),data.size(),out);
        } else {
            HuffmanRLUtilEncoder huffman;
            bytes_used+=huffman.encode(data.data(),data.size(),out);

        }
        out->flush();
        if(m_encoder=='0') {
            // Store the lengths of runs.
            uint64 buffer = 0;
            int32 bitsInBuffer = 0;
            for (uint64 k = 0; k < zero_runs.size(); ++k) {
                uint64 num=zero_runs[k];
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
        }




        return bytes_used;

    }



    void MTFDecoder::decodeBlock(BWTBlock& block, InStream* in) {
        PROFILE("MTFDecoder::decodeBlock");
        if(in->compressedDataEnding()) return;
        block.readHeader(in);
        std::vector<byte> data;

        if(m_decoder=='F' || m_decoder=='0') {
            HuffmanUtilDecoder huffman;
            huffman.decodeBlock(data,in);
        } else {
            HuffmanRLUtilDecoder huffman;
            huffman.decodeBlock(data,in);
        }

        if(m_decoder=='0') {
            in->flushBuffer();
            std::vector<uint64> zero_run_lengths;
            int zero_count=0;
            for(int i=0;i<data.size();i++) if(data[i]==0) zero_count++;
            zero_run_lengths.resize(zero_count);
            int extra=0;
            for (uint64 k = 0; k < zero_count; ++k) {
                
                int zeros = 0;
                while (!in->readBit())
                    ++zeros;
                uint64 value = 0;        
                for (int32 t = 0; t < zeros; ++t) {
                    int32 bit = in->readBit();
                    value = (value << 1) | bit;
                }
                value |= (1 << zeros);
                zero_run_lengths[k] = value;
                extra += value -1;
            }
            std::vector<byte> data_old=data;
            data.resize(data.size()+extra);
            int zerorun=0;
            int k=0;
            for(int i=0;i<data_old.size();i++) {
                if(data_old[i]!=0) data[k++]=data_old[i];
                else {
                    int runlength=zero_run_lengths[zerorun++];
                    while (runlength-- > 0) data[k++]=0;
                }
            }
            in->flushBuffer();
            

        }
        block.setSize(data.size());

        //initialize rank list
        for(int i=0;i<=0xff;i++) m_rankList.push_back(i);

        byte* block_ptr=block.begin();
        for(int i=0;i<data.size();i++) {
            byte rank = data[i];
            byte temp = m_rankList[rank];
            *(block_ptr++) = temp;
            for(int pos=rank-1;pos>=0;pos--) {
                m_rankList[pos+1]=m_rankList[pos];
            }

            m_rankList[0]=temp;
        }
    }
    MTFDecoder::MTFDecoder(char decoder) : m_decoder(decoder) {}

    MTFDecoder::~MTFDecoder() {}


} // namespace bwtc

