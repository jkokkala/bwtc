#ifndef INTERPOLATIVECODERS_HPP
#define INTERPOLATIVECODERS_HPP
#include<iostream>
#include<set>
#include<map>
#include<cmath>
#include<fstream>
#include<vector>
#include<string>
#include "BWTBlock.hpp"
#include "BitCoders.hpp"
#include "Streams.hpp"
#include "globaldefs.hpp"
#include "EntropyCoders.hpp"
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
            vector<uint32> freq;
            vector<byte> nbytes;

            for(int i=0;i<size();i++) if(frequencies[i]>0) {
                freq.push_back(frequencies[i]);
                nbytes.push_back(bytes[i]);
                bytemap[bytes[i]]=freq.size()-1;
            }
            frequencies=freq;
            bytes=nbytes;
        }
    };


    class InterpolativeEncoder : public EntropyEncoder {

        public:
            InterpolativeEncoder() : bytes_used(0) {} 
            size_t transformAndEncode(BWTBlock& block, BWTManager& bwtm,OutStream* out);
            void encode(vector<byte>& block);
            freq ranged_freq(uint32 a, uint32 b,vector<byte>& bytes);
            void output(freq& values, freq& shape, int sum);
            void rec_output(vector<int>& vec, vector<int>& shape, int a, int b);
    std::vector<byte> RLE(byte* orig, uint32 length, byte maxval, int minrun, OutStream* out, size_t& bytes_used);
            void encode_recursive(int index, uint32 size, freq& freqs);
            OutStream* out;
            size_t bytes_used;

            byte* block_begin;
            vector<vector<int> > index;


            void output_bit(int bit);
            void output_bits(uint32 bits,int n);
            void output_num(int num, uint32 mx);
            int bitsInBuffer;
            uint64 buffer;
            int tmpp;
    };

    class InterpolativeDecoder : public EntropyDecoder {
        public:
            InterpolativeDecoder() {}
            void decodeBlock(BWTBlock& block, InStream* in);
            void decode_recursive(int index, uint32 size, freq& freqs);
            void input(freq& f, freq& shape, int sum);
            uint32 input_num(uint32 mx);
            void rec_input(freq& f, vector<int>& shape, int a, int b, int sum);
            vector<uint64> readRLE(InStream* in, int& extra);
            InStream* in;
            byte* output;
            uint32 input_bits(int n);

    };
    const bool IP_RLE=true;
    const int MIN_RLE_RUN=3;
}
#endif
