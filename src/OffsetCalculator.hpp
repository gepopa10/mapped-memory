#pragma once

#include <cstddef>

namespace offset
{

    namespace config
    {
        inline size_t window_size = 2;
        inline size_t nb_objects = 2;
    }

    template <typename T, size_t total_data_size_jump, size_t data_holder_offset = 0>
    class OffsetCalculator
    {
    public:
        size_t compute_offset(size_t object_index, size_t element_index, size_t window_index) const
        {
            // object 0 data 1.1 1.2 ... 1.window_size, data 2.1 2.2 ... 2.window_size
            // object 1 data 1.1 1.2 ... 1.window_size, data 2.1 2.2 ... 2.window_size
            // ...
            // object 0 data 1.window_size+1 1.2 ... 1.2xwindow_size, data 2.window_size+1 2.2 ... 2.2xwindow_size
            // object 1 data 1.window_size+1 1.2 ... 1.2xwindow_size, data 2.window_size+1 2.2 ... 2.2xwindow_size

            const size_t data_size_jump = config::window_size * data_holder_offset;
            const size_t object_data_jump = total_data_size_jump * config::window_size;
            const size_t window_jump = object_data_jump * config::nb_objects;
            const size_t total_byte_offset = window_jump * window_index +
                                             object_index * object_data_jump +
                                             data_size_jump +
                                             (element_index % config::window_size) * sizeof(T);
            return total_byte_offset;
        }
    };

}