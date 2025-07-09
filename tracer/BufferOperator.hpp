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

#endif
