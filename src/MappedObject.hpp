#pragma once

#include <cstddef>
#include <type_traits>
#include <cstdint>

#include "OffsetCalculator.hpp"
#include "IMappedRegion.hpp"

namespace mapped_object
{
    class MappedObject
    {
    public:
        MappedObject(size_t object_index) : object_index(object_index)
        {
        }
        static void set_mapped_region(IMappedRegion *region)
        {
            mapped_region = region;
        }
        static constexpr size_t get_total_data_size_jump()
        {
            return total_data_size_jump;
        }

    private:
        size_t object_index = 0;
        static inline IMappedRegion *mapped_region = nullptr;

        int32_t *data1_holder;
        using T1 = std::remove_pointer_t<decltype(data1_holder)>;
        int16_t *data2_holder;
        using T2 = std::remove_pointer_t<decltype(data2_holder)>;
        int8_t *data3_holder;
        using T3 = std::remove_pointer_t<decltype(data3_holder)>;
        static constexpr size_t total_data_size_jump = sizeof(T1) + sizeof(T2) + sizeof(T3); // 4 + 2 + 1

        static inline size_t total_window_size = offset::config::window_size;

        template <typename T, size_t data_holder_offset = 0>
        struct Accessor
        {
            using underlying_type = T;
            const size_t &object_index;
            offset::OffsetCalculator<T, total_data_size_jump, data_holder_offset> offset_calculator;
            size_t current_element_index = 0;
            size_t current_window_index = 0;

            Accessor(const size_t &obj_idx) : object_index(obj_idx) {}

            void push_back(T value)
            {
                if (current_element_index != 0 && current_element_index % offset::config::window_size == 0)
                {
                    // as soon as one of the object request to push outside the window we need to grow,
                    // but we need to avoid that other grow it afterwards also!
                    if (current_window_index == (total_window_size / offset::config::window_size - 1))
                    {
                        mapped_region->flush_and_grow();
                        total_window_size += offset::config::window_size;
                    }

                    current_window_index++;
                }

                const auto total_byte_offset = offset_calculator.compute_offset(object_index, current_element_index, current_window_index);
                char *base_addr = static_cast<char *>(mapped_region->get_address());
                T *target = reinterpret_cast<T *>(base_addr + total_byte_offset);
                *target = value;
                current_element_index++;
            }

            T& operator[](size_t index){
                const auto total_byte_offset = offset_calculator.compute_offset(object_index, index, index / offset::config::window_size);
                char *base_addr = static_cast<char *>(mapped_region->get_address());
                T *target = reinterpret_cast<T *>(base_addr + total_byte_offset);
                return *target;
            }
        };

    public:
        Accessor<T1, 0> data1{object_index};
        Accessor<T2, sizeof(T1)> data2{object_index};
        Accessor<T3, sizeof(T1) + sizeof(T2)> data3{object_index};
    };
}