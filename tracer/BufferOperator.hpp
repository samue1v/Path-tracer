#ifndef BUFFER_OPERATOR
#define BUFFER_OPERATOR

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "Buffer.hpp"
#include "tracer.hpp"
#include <random>
class BufferOperator {
public:
    virtual ~BufferOperator() = default;

    virtual void doOperation(Buffer & buffer, size_t size, VmaAllocator allocator) = 0;
};

class TauswortheOperator : public BufferOperator{
public:
    TauswortheOperator() = default;

    void doOperation(Buffer & buffer, size_t size, VmaAllocator allocator) override;

public: 
    std::random_device rd;

};

class MultiJitterOperator : public BufferOperator{

public:
    MultiJitterOperator(uint32_t width, uint32_t height, uint32_t rpp);

    void doOperation(Buffer & buffer, size_t size, VmaAllocator allocator) override;

public: 
    std::random_device rd;
    glm::uvec2 range;
    uint32_t _rpp;
};


#endif
