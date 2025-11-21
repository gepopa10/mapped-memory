#pragma once

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <algorithm>

inline void create_or_extend_file(const std::string &filename, size_t new_size_bytes)
{
    constexpr size_t MIN_SIZE = 0; // 1 KB minimum
    new_size_bytes = std::max(new_size_bytes, MIN_SIZE);

    // Create file if it doesn't exist
    if (!std::filesystem::exists(filename))
    {
        std::ofstream file(filename, std::ios::binary);
        if (!file)
        {
            throw std::runtime_error("Failed to create file: " + filename);
        }
        file.close();
    }
    
    // Now resize (works for both new and existing files)
    std::filesystem::resize_file(filename, new_size_bytes);
}