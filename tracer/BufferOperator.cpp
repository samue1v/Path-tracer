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
