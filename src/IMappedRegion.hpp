#pragma once

class IMappedRegion
{
public:
    virtual ~IMappedRegion() = default;

    virtual void *get_address() = 0;
    virtual void flush_and_grow() = 0;
};