#include <cassert>

#include <algorithm>
#include <iostream> /* for std::streamsize*/
#include <string>

#include "block_manager.h"
#include "globaldefs.h"
#include "preprocessor.h"
#include "stream.h"

namespace bwtc {

PreProcessor* GivePreProcessor(char choice, uint64 block_size,
                               const std::string& input) {
  PreProcessor* pp;
  /* Expand this to conform for different PreProcessing algorithms */
  switch (choice) {
    case 'n':
    default:
      pp = new PreProcessor(block_size);
  }
  pp->Connect(input);
  return pp;
}

PreProcessor::PreProcessor(uint64 block_size) :
    source_(NULL), block_size_(block_size), block_manager_(NULL) { }

PreProcessor::~PreProcessor() {
  delete source_;
}

void PreProcessor::BuildStats(std::vector<byte>* data,
                              std::vector<uint64>* stats, uint64 data_size) {
  std::fill(stats->begin(), stats->end(), 0);
  //TODO: at the moment only contexts of length 1 are supported
  for( uint64 i = 0; i < data_size; ++i)
    (*stats)[(*data)[i]]++; 
}

void PreProcessor::Connect(std::string source_name) {
  source_ = new InStream(source_name);
}

void PreProcessor::AddBlockManager(BlockManager* manager) {
  block_manager_ = manager;
}

MainBlock* PreProcessor::ReadBlock() {
  assert(source_);
  assert(block_manager_);
  std::vector<byte>* to = block_manager_->GetFreeBuffer();
  std::vector<uint64>* stats = block_manager_->GetFreeStats();
  /* TODO:
   * streamsize type has as many bits as long. Since the preprocessor gets
   * blocksize as an uint64 we may end up in problems if user is on 32-bit
   * platform and wants to use too large blocks (amount of bits needed to
   * represent block size is more than 32)?? */
  /*** Stub implementation ***/
  std::streamsize read = source_->ReadBlock(
      &(*to)[0], static_cast<std::streamsize>(block_size_) );
  if (!read) return NULL;
  BuildStats(to, stats, static_cast<uint64>(read)); // This should belong to BWT
  return block_manager_->MakeBlock(to, stats, static_cast<uint64>(read));
}

} //namespace bwtc
