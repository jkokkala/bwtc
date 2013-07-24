#include "InterpolativeCoders.hpp"
#include "Profiling.hpp"
#include "Utils.hpp"
using namespace std;
namespace bwtc {
    size_t InterpolativeEncoder::transformAndEncode(BWTBlock& block, BWTManager& bwtm,OutStream* out) {
        bitsInBuffer=0;
        buffer=0;
        this->out=out;
        bwtm.doTransform(block);
        PROFILE("InterpolativeEncoder::encodeData");
        bytes_used=block.writeHeader(out);
        vector<byte> data;
        if(IP_RLE) data= RLE(block.begin(),block.size(),255,MIN_RLE_RUN,out,bytes_used);
        else data=vector<byte>(block.begin(),block.begin()+block.size());
        encode(data);

        while(bitsInBuffer) output_bit(0);
        return bytes_used;
    }

    // compute the frequencies for characters given (bytes) in string T[a..b]
    freq InterpolativeEncoder::ranged_freq(uint32 a, uint32 b,vector<byte>& bytes) {
        freq ret(bytes.size());
        for(int j=0;j<bytes.size();j++) ret.bytes[j]=bytes[j];
        for(int j=0;j<bytes.size();j++) ret.bytemap[bytes[j]]=j;

        // if the string is short enough (<M*logN), it is faster to just iterate over the string
        if(b-a<bytes.size()*utils::logFloor(b-a+1)) {
            for(int i=a;i<=b;i++) {
                ret[ret.bytemap[block_begin[i]]]++;
            }
            return ret;
        }
        // else binary search the number of occurrences in T[a..b] for each character j by using index[j]
        int start,end;
        for(int i=0;i<bytes.size();i++) {
            vector<int>& v = index[bytes[i]];
            int low=0,high=v.size()-1;
            if((v.size()==0)||v[0]>b || v[high]<a) ret[i]=0;
            else {
                // first occurrence
                while(low+1<high) {
                    int mid=(low+high)/2;
                    if(v[mid]<a) low=mid;
                    else high=mid;
                }
                for(start=low;start<=high;start++) {
                    if(v[start]>=a) break;
                }
                // last occurrence
                low=start;high=v.size()-1;
                while(low+1<high) {
                    int mid=(low+high)/2;
                    if(v[mid]>b) high=mid;
                    else low=mid;
                }
                for(end=high;end>=low;end--) {
                    if(v[end]<=b) break;
                }

                // number of occurrences
                ret[i]=end-start+1;

            }
        }
        return ret;
    }

    void InterpolativeEncoder::encode(vector<byte>& block) {
        block_begin=block.data();

        //for each character i construct a sorted list of occurrences by index ( index[i] )
        index.clear();
        index.resize(256);
        for(int i=0;i<256;i++) index[i].reserve(block.size()/10);
        for(int i=0;i<block.size();i++) {
            index[block_begin[i]].push_back(i);
        }

        //compute and encode total frequencies
        freq curr(256);
        curr = ranged_freq(0,block.size()-1,curr.bytes);
        vector<int> vec;
        for(int i=0;i<256;i++) vec.push_back(curr[i]);
        int sum=0;
        for(int j=0;j<256;j++) sum+=vec[j];
        bytes_used+=utils::gammaEncode(vec,out,1);


        curr.clean(); // remove 0s

        encode_recursive(0,block.size(),curr);
        
        out->flush();
    }

    void InterpolativeEncoder::encode_recursive(int index, uint32 size, freq& freqs) {
        if(freqs.size()==1) {
            return;
        } 
        if(IP_RLE && MIN_RLE_RUN==1 && freqs.size()==2) {
            if(size%2 ==1) return;
            if(block_begin[index]<block_begin[index+1]) output_bit(0);
            else output_bit(1);
            return;
        } 
        
        if(size==2) {
            if(block_begin[index]<block_begin[index+1]) output_bit(0);
            else output_bit(1);
            return;
        }

        int half  = size/2;

        freq lfreq=ranged_freq(index,index+half-1,freqs.bytes);
        output(lfreq,freqs,half);
        for(int i=0;i<freqs.size();i++) freqs[i]-=lfreq[i];
        lfreq.clean();
        freqs.clean();
        if(half>1) encode_recursive(index,half,lfreq);
        if(size-half>1) encode_recursive(index+half,size-half,freqs);

    }

    // output frequency list (values) given its parent (shape) and the sum of frequencies
    void InterpolativeEncoder::output(freq& values, freq& shape, int sum) {

        // find the maximum frequency in parent
        int max=0,maxi;
        for(int i=0;i<values.size();i++) {
            if(shape[i]>max) {
                max=shape[i];
                maxi=i;
            }

        }

        //encode the other frequencies
        for(int i=0;i<values.size();i++) {
            if(i==maxi) continue; // skip the one with maximum frequency in parent
            
            output_num(values[i],sum<shape[i]?sum:shape[i]);
           sum-=values[i];
           if(sum==0) return; // if the rest of the list is guaranteed to be 0s, skip
        }
    }

    void InterpolativeDecoder::decodeBlock(BWTBlock& block, InStream* in) {
        PROFILE("InterpolativeEncoder::decodeBlock");
        this->in=in;
        block.readHeader(in);
        int extra;

        std::vector<uint64> runs;
        if(IP_RLE)runs=readRLE(in,extra);

        int minrun=MIN_RLE_RUN;
        int maxval=255;
        freq total(256);
        int size=0;
        vector<int> vec(256);

        utils::gammaDecode(vec,in,1); // read total frequencies


        for(int i=0;i<256;i++) {
            total[i]=vec[i];
            size+=total[i];
        }


        vector<byte> rawdata;
        if(IP_RLE) {
            rawdata.resize(size);
            output=rawdata.data();
        } else {
            block.setSize(size);
            output=block.begin();
        }
        total.clean();


        decode_recursive(0,size,total);
        in->flushBuffer();

        // If using RLE, expand the string
        if(IP_RLE) {
            byte temp;
            byte prev=0;
            byte* ptr=block.begin();
            block.setSize(size+extra);
            int cur_run=0;
            int run_iter=0;
            for(int j=0;j<rawdata.size();j++) {
                temp=rawdata[j];
                *(ptr++) = temp;
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


    }


    // read a frequencey list (freqs) given its parents frequencies (shape) and the total number of characters (sum)
    void InterpolativeDecoder::input(freq& freqs, freq& shape,int sum) {

        // calculate the maximum value in parent freq list
        int max=0;
        int maxi;
        for(int i=0;i<freqs.size();i++) {
            if(shape[i]>max) {
                max=shape[i];
                maxi=i;
            }
        }

        // decode the other frequencies
        for(int i=0;i<freqs.size();i++) 
        {
            if(sum==0) freqs[i]=0; // if the rest of the list is guaranteed to be 0s, skip
            else if(i!=maxi) {
                sum -= freqs[i] = input_num(sum<shape[i]? sum : shape[i]);
//                sum-=freqs[i]=input_bits(utils::logFloor(shape[i])+1); // read freqs[i] unless shape[i] is the maximum
            }
        }

        freqs[maxi]=sum; // the maximum value is [size of string - sum of other frequencies]
    }


    // recursively decode T[index .. index+size) given frequencies
    void InterpolativeDecoder::decode_recursive(int index, uint32 size, freq& freqs) {
        assert(size>0);
        if(size==1) {
            for(int i=0;i<freqs.size();i++) {
                if(freqs[i]>0) {
                    output[index]=freqs.bytes[i];
                }
            }
            return;
        }

        // if the string consists of a single character, skip reading
        if(freqs.size()==1) {
            for(int i=index;i<index+size;i++) output[i]=freqs.bytes[0];
            return;
        }  

        // if using RLE and input consists of two characters, the input can only be 01010, 10101, 0101 or 1010 etc.
        if(IP_RLE && MIN_RLE_RUN==1 && freqs.size()==2) {
            int a;
            if(size%2 ==1) {
                if(freqs[0]>freqs[1]) a=0;
                else a=1;
            }
            else a = in->readBit();
            
            for(int i=0;i<size;i++) output[index+i]=freqs.bytes[(i&1)^a];
            return;
        }
        
        // small optimization: if the length of the string is 2, stop the recursion
        if(size==2) {
            int a = in->readBit();
            output[index]=freqs.bytes[a];
            output[index+1]=freqs.bytes[1-a];
            return;
        }

        int half  = size/2;


        freq lfreq=freqs; //initialize left part
        input(lfreq,freqs,half); //input left part

        freq rfreq=freqs-lfreq; // right = parent-left
        
        lfreq.clean(); // remove 0s
        rfreq.clean(); // remove 0s

        decode_recursive(index,half,lfreq); // decode left part
        decode_recursive(index+half,size-half,rfreq); // decode right part
    }



    // compute and output run length data
    std::vector<byte> InterpolativeEncoder::RLE(byte* orig, uint32 length, byte maxval, int minrun, OutStream* out, size_t& bytes_used) {

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

    // read run length data
    std::vector<uint64> InterpolativeDecoder::readRLE(InStream* in, int& extra) {
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

    // output a single bit
    void InterpolativeEncoder::output_bit(int bit) {
        buffer = (buffer<<1) | bit;
        bitsInBuffer++;
        while(bitsInBuffer>=8) {
            bitsInBuffer-=8;
            out->writeByte((buffer>>bitsInBuffer)&0xff);
        }

    }

    // output given the first (LSB) n bits of b
    void InterpolativeEncoder::output_bits(uint32 b,int n) {
        if(bitsInBuffer+n<64 || true){
            buffer=(buffer<<n)|b;
            bitsInBuffer+=n;
        }
        while(bitsInBuffer>=8) {
            bitsInBuffer-=8;
            out->writeByte((buffer>>bitsInBuffer)&0xff);
        }

    }

    // input an integer of n bits
    uint32 InterpolativeDecoder::input_bits(int n) {
        uint32 num=0;
        while(n>=8) {
            num=(num<<8)|in->readByte();
            n-=8;
        }
        while(n --> 0) num=(num<<1)|in->readBit();
        return num;
    }


    void InterpolativeEncoder::output_num(int num, uint32 r){
        int bits = utils::logFloor(r)+1;
        int mx = (1<<bits)-1;
        int wast = mx - r ;
        int longer = r - wast + 1;
        int offset = longer/2;
        num = (num - offset+ r + 1)%(r+1);
        if(num < wast)
            output_bits(num,bits-1);
        else {
            num = (((num-wast)/2 + wast)<<1) | ((num-wast)%2);
            output_bits(num,bits);
        }

        
    }

    uint32 InterpolativeDecoder::input_num(uint32 r) {
        int bits = utils::logFloor(r)+1;
        int mx = (1<<bits)-1;
        int wast = mx - r ;
        int longer = r - wast + 1;
        int offset = longer/2;
        int n = input_bits(bits-1);
        uint32 num;
        if(n<wast)
            num = n;
        else
            num = (n-wast)*2+wast+input_bits(1);
        return (num+offset)%(r+1);
    }
}

