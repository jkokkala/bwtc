/* Base class acting as an interface for probability models. */
#ifndef BWTC_BASE_PROB_MODEL_H_
#define BWTC_BASE_PROB_MODEL_H_

#include "../globaldefs.h" /* Important definitions */

namespace bwtc {

class ProbabilityModel {
 public:
  ProbabilityModel() : prev_(true) {}
  virtual ~ProbabilityModel() {}
  virtual void Update(bool bit) { prev_ = bit; }
  virtual Probability ProbabilityOfOne() {
    return kLogProbabilityScale/2; /*
    if( prev_) return 1 << (kLogProbabilityScale/2 + 2);
    else return 1 << (kLogProbabilityScale/2 - 2);*/
  }

 private:
  bool prev_;
};

} // namespace bwtc

#endif
