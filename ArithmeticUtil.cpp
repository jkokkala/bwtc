/*
 * Listing 2 -- coder.c
 *
 * This file contains the code needed to accomplish arithmetic
 * coding of a symbol.  All the routines in this module need
 * to know in order to accomplish coding is what the probabilities
 * and scales of the symbol counts are.  This information is
 * generally passed in a SYMBOL structure.
 *
 * This code was first published by Ian H. Witten, Radford M. Neal,
 * and John G. Cleary in "Communications of the ACM" in June 1987,
 * and has been modified slightly for this article.  The code
 * is  published here with permission.
 */

#include "ArithmeticUtil.hpp"

namespace bwtc {





    size_t ArithmeticUtilEncoder::encode(byte* start, uint64 size) {
        std::vector<uint64> counts(256,0);
        long p=out->getPos();

        for(byte* b = start; b!= (start+size); b++) counts[*b]++;
        int sum=0;
        for(int i=0;i<256;i++) sum+=(counts[i]=counts[i]*SCALE/size);

        long header_pos = out->getPos();
        for(int i=0;i<6;i++) out->writeByte(0);
        out->write48bits(size,header_pos);
        long bits_pos = out->getPos();
        for(int i=0;i<6;i++) out->writeByte(0);
        bytes_used=0;
        bytes_used += 6*2;

        bits_used=0;
        int it=0;
        int bsum=0;
        for(int i=0;i<256;i++)  {
            if(counts[i]==0) {
                if(sum==SCALE) {
                    while(counts[it%256]<=1) it++;
                    counts[(it++)%256]--;
                } else sum++;
                counts[i]++;
            }
        }
        int add=SCALE-sum;
        for(int i=0;i<256;i++) {
            counts[i]+=add/256;
            if(i< (add%256)){
                counts[i]++;
            }
        }

        std::vector<uint64> diffs(256,0);
        long a=bytes_used;
        bytes_used += utils::gammaEncode(counts,out);
        long pp=bytes_used;
        std::vector<SYMBOL> symbols(256);
        int cumul=0;
        for(int i=0;i<256;i++) {
            symbols[i].scale=SCALE;
            symbols[i].low_count=cumul;
            symbols[i].high_count= cumul = cumul + counts[i];
        }
        size_t tmp=bytes_used;
        byte* ptr=start;

        ptr=start;
        while(ptr != start + size) {
            encode_symbol(symbols[*ptr]);
            ptr++;
        }
        flush();
        while(bytes_used-tmp<4) {
            out->writeByte(0);
            bytes_used++;
            bits_used+=8;
        }
        std::cout<<"bytes used for counts: "<<1.0*(pp-a)/(bytes_used-a)<<"\n";
        
        out->write48bits(bits_used/8,bits_pos);
        return bytes_used;
    }

    /*
     * This routine is called to encode a symbol.  The symbol is passed
     * in the SYMBOL structure as a low count, a high count, and a range,
     * instead of the more conventional probability ranges.  The encoding
     * process takes two steps.  First, the values of high and low are
     * updated to take into account the range restriction created by the
     * new symbol.  Then, as many bits as possible are shifted out to
     * the output stream.  Finally, high and low are stable again and
     * the routine returns.
     */
    void ArithmeticUtilEncoder::encode_symbol( SYMBOL& s )
    {
        long range;
        /*
         * These three lines rescale high and low for the new symbol.
         */
        range = (long) ( high-low ) + 1;
        high = low + (unsigned int )
            (( range * s.high_count ) / SCALE - 1 );
        low = low + (unsigned int )
            (( range * s.low_count ) / SCALE );
        /*
         * This loop turns out new bits until high and low are far enough
         * apart to have stabilized.
         */
        for ( ; ; )
        {
            /*
             * If this test passes, it means that the MSDigits match, and can
             * be sent to the output stream.
             */
            if ( ( high & 0x80000000 ) == ( low & 0x80000000 ) )
            {
                output_bit(  high & 0x80000000 );
                while ( underflow_bits > 0 )
                {
                    output_bit(  ~high & 0x80000000 );
                    underflow_bits--;
                }
            }
            /*
             * If this test passes, the numbers are in danger of underflow, because
             * the MSDigits don't match, and the 2nd digits are just one apart.
             */
            else if ( ( low & 0x40000000 ) && !( high & 0x40000000 ))
            {
                underflow_bits += 1;
                low &= 0x3fffffff;
                high |= 0x40000000;
            }
            else
                return ;
            low <<= 1;
            low &= 0xffffffff;
            high <<= 1;
            high &= 0xffffffff;
            high |= 1;
        }
    }



    /*
     * At the end of the encoding process, there are still significant
     * bits left in the high and low registers.  We output two bits,
     * plus as many underflow bits as are necessary.
     */
    void ArithmeticUtilEncoder::flush()
    {
        output_bit(  low & 0x40000000 );
        underflow_bits++;
        while ( underflow_bits-- > 0 )
            output_bit( ~low & 0x40000000 );

        while(bitpos!=0) {
            output_bit(0);
        }
        out->flush();
    }
    void ArithmeticUtilEncoder::output_bit(unsigned int bit) {
        bits_used++;
         current = (current << 1) | (bit>0);
         if(++bitpos == 8) {
             out->writeByte(current);
             bytes_used++;
             bitpos=0;
             current=0;
         }

    }


    void ArithmeticUtilDecoder::decode(std::vector<byte>& data) {
        long p = in->pos;
        uint64 length = in->read48bits();
        bits_left = in->read48bits()*8;
        //       std::cout<<"Current position: "<<in->pos<<" - "<<" bits to read: "<<bits_left<<"\n";
        data.resize(length);
        std::vector<uint64> counts(256);
        std::vector<SYMBOL> symbols(256);
        utils::gammaDecode(counts,in);
        in->flushBuffer();
        uint64 cumul=0;
        for(int i=0;i<256;i++) {
            symbols[i].low_count=cumul;
            symbols[i].high_count = cumul = cumul + counts[i];
        }
        byte aa = in->readByte();
        byte bb = in->readByte();
        byte cc = in->readByte();
        byte dd = in->readByte();
        bits_left-=32;
        code = (aa<<24)|(bb<<16)|(cc<<8)|dd;
        for(int i=0;i<length;i++) {
            uint64 v = get_value();
            byte b = get_byte(symbols,v);
            if(i+1<length)    del_symbol(symbols[b]);
            data[i]=b;
        }
        in->flushBuffer();

    }

    int ArithmeticUtilDecoder::get_value() {
        long range;
        int count;

        range = (long) ( high - low ) + 1;
        count = (int)
            ((((long) ( code - low ) + 1 ) * SCALE-1 ) / range );
        return( count );

    }
    void ArithmeticUtilDecoder::del_symbol(SYMBOL& s) {
        long range;

        /*
         *  * First, the range is expanded to account for the symbol removal.
         *   */
        range = (long)( high - low ) + 1;
        high = low + (unsigned int)
            (( range * s.high_count ) / SCALE - 1 );
        low = low + (unsigned int)
            (( range * s.low_count ) / SCALE );
        /*
         *  * Next, any possible bits are shipped out.
         *   */
        for ( ; ; )
        {
            /*
             *  * If the MSDigits match, the bits will be shifted out.
             *   */
            if ( ( high & 0x80000000 ) == ( low & 0x80000000 ) )
            {
            }
            /*
             *  * Else, if underflow is threatining, shift out the 2nd MSDigit.
             *   */
            else if ((low & 0x40000000) == 0x40000000  && (high & 0x40000000) == 0 )
            {
                code ^= 0x40000000;
                low   &= 0x3fffffff;
                high  |= 0x40000000;
            }
            /*
             *  * Otherwise, nothing can be shifted out, so I return.
             *   */
            else
                return;
            low <<= 1;
            low &= 0xffffffff;
            high <<= 1;
            high &= 0xffffffff;
            high |= 1;
            code <<= 1;
            code &= 0xffffffff;
            if(bits_left-->0) code +=in->readBit();
        }

    }

}


