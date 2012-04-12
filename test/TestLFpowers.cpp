#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <numeric>
#include <cstdlib>
#include <algorithm>

#include "../globaldefs.hpp"
#include "../MainBlock.hpp"
#include "../bwtransforms/BWTransform.hpp"
#include "../bwtransforms/SA-IS-bwt.hpp"
#include "../bwtransforms/sais.hxx"

using namespace bwtc;

inline int my_random(int p, int r) {
  return p + rand() % (r - p + 1);
}

void test(int testcases, int max_n, int max_sigma) {
  fprintf(stderr,"TEST, testcases = %d, max_n = %d, max_sigma = %d\n",
      testcases, max_n, max_sigma);
  int progress_step = testcases / 1000;
  if (progress_step == 0) progress_step = 1;
  for (int tc = 0; tc < testcases; ++tc) {
    if ( ( tc % progress_step ) == 0 ) {
      fprintf(stderr,"Progress: %5.2Lf%%\r",
          ((long double) 100.0 * tc ) / testcases);
    }

    // generate random string
    int n = my_random(1, max_n);
    byte *t = new byte[n];
    for (int j = 0; j < n; ++j) {
      t[j] = my_random(0, max_sigma - 1);
    }
    std::vector<byte> v(n + 1);
    std::copy(t, t + n, v.begin());

    // compute SA
    std::reverse(t, t + n);
    int *sa = new int[n];
    saisxx(t, sa, n);

    // compute BWT
    byte *bwt = new byte[n + 1];
    bwt[0] = t[n - 1];
    int p = 1;
    int eob_pos = 0;
    for (int i = 0; i < n; ++i) {
      if (sa[i] == 0) {
        eob_pos = p;
        bwt[p++] = '$';
      } else {
        bwt[p++] = t[sa[i] - 1];
      }
    }

    // compute LF mapping
    int count[256];
    std::fill(count, count + 256, 0);
    count[0] = 1;
    for (int i = 0; i < n + 1; ++i) {
      if (i != eob_pos) {
        count[(int)bwt[i] + 1]++;
      }
    }
    std::partial_sum(count, count + 256, count);
    int *LF = new int[n + 1];
    for (int i = 0; i < n + 1; ++i) {
      if (i != eob_pos) {
        LF[i] = count[bwt[i]]++;
      }
    }
    LF[eob_pos] = 0;

    // get LF powers using sais
    int starting_positions = my_random(1, 256);
    BWTransform* transform = giveTransformer('a');
    MainBlock* data;
    data = new MainBlock(&v, NULL, (uint64)n);
    transform->connect(data);
    std::vector<uint32> LFpowers;
    LFpowers.resize(starting_positions);
    std::vector<byte>* result = transform->doTransform(LFpowers);


    // get the same LF powers just from LF array
    int *LFpow = new int[n + 1];
    LFpow[0] = eob_pos;
    for (int i = 1; i <= n; ++i) {
      LFpow[i] = LF[LFpow[i - 1]];
    }
    std::vector<uint32> LFpowers_simple;
    LFpowers_simple.resize(starting_positions);
    std::fill(LFpowers_simple.begin(), LFpowers_simple.end(), 0);
    int block_size = (n + 1) / starting_positions;

    if (starting_positions <= n + 1) {
      for (int j = 0; j < starting_positions; ++j) {
        LFpowers_simple[j] = LFpow[j * block_size];
      }
    } else {
      LFpowers_simple[0] = eob_pos;
      // the rest is 0s.
    }

    // check if they are the same
    bool correct = true;
    if (LFpowers.size() != LFpowers_simple.size()) correct = false;
    if (correct) {
      for (uint32 j = 0; j < LFpowers.size(); ++j) {
        if (LFpowers[j] != LFpowers_simple[j]) {
          correct = false;
          break;
        }
      }
    }
    if (!correct) {
      fprintf(stderr,"Doesn't work for t = ");
      for (int i = 0; i < n; ++i) {
        fprintf(stderr, "%c", t[i]);
      }
      fprintf(stderr,"\n");
      fprintf(stderr,"starting_positions = %d\n", starting_positions);
      fprintf(stderr,"sais  returned: ");
      for (uint32 j = 0; j < LFpowers.size(); ++j) {
        fprintf(stderr,"%d ", LFpowers[j]);
      }
      fprintf(stderr,"\n");
      fprintf(stderr,"naive returned: ");
      for (uint32 j = 0; j < LFpowers_simple.size(); ++j) {
        fprintf(stderr,"%d ", LFpowers_simple[j]);
      }
      fprintf(stderr,"\n");
      fprintf(stderr,"LFpows:\n");
      for (int j = 0; j <= n; ++j) {
        fprintf(stderr,"  LFpow[%d] = %d\n", j, LFpow[j]);
      }
      fprintf(stderr,"\n");
      exit(1);
    }

    delete[] t;
    delete[] sa;
    delete[] bwt;
    delete[] LF;
    delete transform;
    delete data;
    delete result;
    delete[] LFpow;
  }
}

int main() {
  srand(time(0) + getpid());
  test(10000000,  3, 256);
  test(10000000, 10, 256);
  test(1000000, 100, 256);
  test(100000, 1000, 256);
  test(10000, 10000, 256);
  test(1000, 100000, 256);
  test(100, 1000000, 256);
  test(10, 10000000, 256);
  fprintf(stderr,"All tests passed.\n");
  return 0;
}

