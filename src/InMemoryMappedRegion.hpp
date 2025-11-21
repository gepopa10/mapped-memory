#pragma once

#include "IMappedRegion.hpp"
#include <vector>
#include <cstring>

class InMemoryMappedRegion : public IMappedRegion
{
private:
    std::vector<char> buffer;
    size_t window_size_bytes;

public:
    InMemoryMappedRegion(size_t initial_size)
        : buffer(initial_size), window_size_bytes(initial_size)
    {
    }

    void *get_address() override
    {
        return buffer.data();
    }

    void flush_and_grow() override
    {
        window_size_bytes *= 2;
        buffer.resize(window_size_bytes);
    }
};