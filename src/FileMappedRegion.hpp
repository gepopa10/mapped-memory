#pragma once

#include "IMappedRegion.hpp"

#include <string>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "utils.hpp"

class FileMappedRegion : public IMappedRegion
{
private:
    std::string filename;
    size_t window_size_bytes;
    boost::interprocess::file_mapping *file_mapping;
    boost::interprocess::mapped_region *mapped_region;

public:
    FileMappedRegion(const std::string &file, size_t initial_size)
        : filename(file), window_size_bytes(initial_size), file_mapping(nullptr), mapped_region(nullptr)
    {
        create_or_extend_file(filename, initial_size);
        file_mapping = new boost::interprocess::file_mapping(filename.c_str(), boost::interprocess::read_write);
        mapped_region = new boost::interprocess::mapped_region(*file_mapping, boost::interprocess::read_write);
    }

    ~FileMappedRegion()
    {
        delete mapped_region;
        delete file_mapping;
    }

    void *get_address() override
    {
        return mapped_region->get_address();
    }

    void flush_and_grow() override
    {
        mapped_region->flush();
        window_size_bytes *= 2;

        delete mapped_region;
        delete file_mapping;

        create_or_extend_file(filename, window_size_bytes);

        file_mapping = new boost::interprocess::file_mapping(filename.c_str(), boost::interprocess::read_write);
        mapped_region = new boost::interprocess::mapped_region(*file_mapping, boost::interprocess::read_write);
    }
};