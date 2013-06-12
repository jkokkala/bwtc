/*
 * Listing 1 -- coder.h
 *
 * This header file contains the constants, declarations, and
 * prototypes needed to use the arithmetic coding routines.  These
 * declarations are for routines that need to interface with the
 * arithmetic coding stuff in coder.c
 *
 */
#ifndef ARITHMETHICUTIL_HPP
#define ARITHMETHICUTIL_HPP
#define SCALE   ((1ULL<<28)-1)  /* Maximum allowed frequency count */
#define ESCAPE          256    /* The escape symbol               */
#define DONE            -1     /* The output stream empty  symbol */
#define FLUSH           -2     /* The symbol to flush the model   */

/*
 * A symbol can either be represented as an int, or as a pair of
 * counts on a scale.  This structure gives a standard way of
 * defining it as a pair of counts.
 */

#include "EntropyCoders.hpp"
#include "globaldefs.hpp"
#include "BitCoders.hpp"
#include "Streams.hpp"
#include "Utils.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
namespace bwtc {
    typedef struct {
        unsigned int low_count;
        unsigned int high_count;
        unsigned int scale;
    } SYMBOL;


    class ArithmeticUtilEncoder {

        public:
            ArithmeticUtilEncoder(OutStream* _out) : out(_out),low(0),high(0xffffffff),underflow_bits(0),current(0),bitpos(0),bytes_used(0) {};
            size_t encode(byte* start, uint64 size);
            void output_bit(unsigned int bit);
            void flush();
            void encode_symbol(SYMBOL& s );


        private:
            size_t bytes_used;
            size_t bits_used;
            OutStream* out;
            long underflow_bits;
            unsigned int code;
            unsigned int low;
            unsigned int high;
            byte current;
            byte bitpos;
    };

    class ArithmeticUtilDecoder {

        public:
            ArithmeticUtilDecoder(InStream* _in) : in(_in),low(0),high(0xffffffff) {};
            void decode(std::vector<byte>& data);
            int get_value();
            void del_symbol(SYMBOL& symbol);
            inline byte get_byte(std::vector<SYMBOL>& counts, uint64 val) {
                byte low=0;
                byte high=counts.size()-1;
                while(high-low>1) {
                    int mid=(high+low)/2;
                    if(counts[mid].low_count > val) {
                        high=mid;
                    } else low=mid;
                }
                for(int i=low;i<=high;i++) if(val >= counts[i].low_count && val < counts[i].high_count) return (byte)i;
                assert(false);
            }

        private:
            InStream* in;
            unsigned int code;
            unsigned int low;
            unsigned int high;
            int bits_left;
            





    };



}
#endif
