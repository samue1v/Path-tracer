#include "BufferOperator.hpp"

void TauswortheOperator::doOperation(Buffer &buffer, size_t size,
                                     VmaAllocator allocator) {
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> dist(
      129, std::numeric_limits<uint32_t>::max());

  auto *pPRNG =
      reinterpret_cast<Tracer::PRNG32 *>(buffer.allocationInfo.pMappedData);

  for (size_t i = 0; i < size; ++i) {
    pPRNG[i].state.x = dist(gen);
    pPRNG[i].state.y = dist(gen);
    pPRNG[i].state.z = dist(gen);
    pPRNG[i].state.w = dist(gen);
    pPRNG[i].value = 0.f;
  }
}

MultiJitterOperator::MultiJitterOperator(uint32_t width, uint32_t height, uint32_t rpp) : range(width,height), _rpp(rpp){}

void MultiJitterOperator::doOperation(Buffer &buffer, size_t size,
    VmaAllocator allocator) {

  uint32_t interval = 5;

  uint32_t swu = range.x / interval;
  uint32_t shu = range.y / interval;

  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> dist(
      0, swu - 1);

  auto *pHitData =
      reinterpret_cast<Tracer::hitData *>(buffer.allocationInfo.pMappedData);

  for(uint32_t i = 0; i < (range.x-1) * (range.y-1) * _rpp; i += swu){
    pHitData[i + dist(gen)].color = glm::vec4(0.0,0.0,1.0,1.0);
    if(i % (range.x)  > 0)
      pHitData[i].color = glm::vec4(1.0,1.0,1.0,1.0);
    if(i >0 && i%(swu*swu*interval) == 0){
      for(uint32_t j = i; j < i+range.x;j++){
        pHitData[j].color = glm::vec4(1.0,1.0,1.0,1.0);
      }
    }
  }


}
