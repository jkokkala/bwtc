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
#include "ArithmeticUtil.hpp"
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
#include "globaldefs.hpp"
#include "Utils.hpp"
#include "Profiling.hpp"

namespace bwtc {

    MTFEncoder::MTFEncoder(char encoder)
        : m_headerPosition(0), m_compressedBlockLength(0), m_encoder(encoder) {}

    MTFEncoder::~MTFEncoder(){}

    //maxval = maximum value for rle, e.g. RLE0 maxval=0, RLE maxval=255
    //minrun = min number of occurrences in a run
    std::vector<byte> MTFEncoder::RLE(byte* orig, uint32 length, byte maxval, int minrun, OutStream* out, size_t& bytes_used,char encoder) {
        
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

    std::vector<uint64> MTFDecoder::readRLE(InStream* in, int& extra, char decoder) {
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

    size_t MTFEncoder::transformAndEncode(BWTBlock& block, BWTManager& bwtm, OutStream* out) {
        bwtm.doTransform(block);
        PROFILE("MTFEncoder::encodeData");
        size_t bytes_used=block.writeHeader(out)+6;


        Node* start=new Node(0);
        Node* curr=start;
        Node* prev;
        for(int i=1;i<=0xff;i++) {
            curr->next=new Node(i);
            curr=curr->next;
        }
        curr->next=NULL;

        bool rle=true;
        byte maxval=255;
        int minrun=1;
        if(m_encoder=='F' || m_encoder=='A') {
            rle=false;
        } else  {
            rle=true;
            maxval=255;
            minrun=3;
        } 
        std::vector<byte> data;
        size_t a=bytes_used;
        if(rle) data= RLE(block.begin(),block.size(),maxval,minrun,out,bytes_used,m_encoder);
        else data=std::vector<byte>(block.begin(),block.end());
        int pos;
        for(int  i = 0;i!=data.size();i++) {
            curr=start;
            byte cur=data[i];
            for(pos=0;pos<256;pos++) {
                if(curr->val==cur) break;
                prev=curr;
                curr=curr->next;
            }
            data[i]=pos;
            if(pos==0) continue;
            prev->next=curr->next;
            curr->next=start;
            start=curr;

        }

        /*       long ppos = out->getPos();
                 for(int i=0;i<6;i++) out->writeByte(0);
                 out->write48bits(data.size(),ppos);
                 bytes_used+=6;*/
        HuffmanUtilEncoder huffman;
        ArithmeticUtilEncoder arithmetic(out);
        out->flush();
        if(m_encoder=='A'  || m_encoder=='a')  bytes_used+=arithmetic.encode(data.data(),data.size());
        else bytes_used+=huffman.encode(data.data(),data.size(),out);
        //       bytes_used+=utils::gammaEncode(data,out,1);
        out->flush();
        return bytes_used;
    }

    void MTFDecoder::decodeBlock(BWTBlock& block, InStream* in) {

        PROFILE("MTFDecoder::decodeBlock");
        if(in->compressedDataEnding()) return;


        bool rle=true;
        byte maxval=255;
        int minrun=1;

        if(m_decoder=='F'|| m_decoder=='A') {
            rle=false;
        } else  {
            rle=true;
            maxval=255;
            minrun=3;
        }

        block.readHeader(in);
        std::vector<byte> data;
        int extra=0;
        std::vector<uint64> runs;
        if(rle) runs=readRLE(in,extra,m_decoder);

        in->flushBuffer();
        HuffmanUtilDecoder huffman;
        ArithmeticUtilDecoder arithmetic(in);


 /*       long ppos=in->pos;
        int l = in->read48bits();
        in->flushBuffer();
        data.resize(l);
        utils::gammaDecode(data,in,1);*/
 
        
        if(m_decoder=='A' || m_decoder=='a') arithmetic.decode(data);
        else huffman.decodeBlock(data,in);
        in->flushBuffer();
        block.setSize(data.size()+extra);

        //initialize rank list
        m_rankList.clear();
        for(int i=0;i<=0xff;i++) m_rankList.push_back(i);

        byte* block_ptr=block.begin();

        byte prev=0;
        int cur_run=1;
        int run_iter=0;

        int wrote=0;
        for(int i=0;i<data.size();i++) {
            byte rank = data[i];
            byte temp = m_rankList[rank];
            wrote++;
            *(block_ptr++) = temp;
            if(rle) {
                if(temp==prev && i!=0) cur_run++;
                else {
                    prev=temp;
                    cur_run=1;
                }
                if(cur_run>=minrun && temp<=maxval) {
                    int length=runs[run_iter++];
                    for(int j=0;j<length-1;j++) {
                        *(block_ptr++)=temp;
                        wrote++;
                    }
                }
            }

            for(int pos=rank-1;pos>=0;pos--) {
                m_rankList[pos+1]=m_rankList[pos];
            }

            m_rankList[0]=temp;
        }
    }

    MTFDecoder::MTFDecoder(char decoder) : m_decoder(decoder) {}

    MTFDecoder::~MTFDecoder() {}


} // namespace bwtc

