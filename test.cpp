#include<iostream>
#include<vector>
#include<string>
#include "ArithmeticUtil.hpp"
#include "HuffmanUtil.hpp"
#include "Profiling.hpp"
#include "Streams.hpp"
#include "globaldefs.hpp"
#include <cstdlib>
#include <cmath>
using namespace std;
using namespace bwtc;
int main() {
    OutStream* out=new RawOutStream("temp/file");
    
    
    ArithmeticUtilEncoder a(out);
    HuffmanUtilEncoder huff;
    vector<byte> vec;
    for(int i=0;i<100000000;i++) {
        vec.push_back(((rand()%256) * (rand()%256))%256);
    }

    huff.encode(vec.data(),vec.size(),out);
 //   a.encode(vec.data(),vec.size());
}
