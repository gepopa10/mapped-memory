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
            if (object_index > std::numeric_limits<uint16_t>::max())
            {
                throw std::overflow_error("object_index would overflow uint16_t");
            }
        }
        static void set_mapped_region(IMappedRegion *region)
        {
            mapped_region = region;
            total_window_size = offset::config::window_size;
        }
        static constexpr size_t get_total_data_size_jump()
        {
            return total_data_size_jump;
        }

        std::uint16_t object_index = 0;
        // inline to share amongst translation units, alternative is to define in cpp
        static inline IMappedRegion *mapped_region = nullptr;

        using T1 = int32_t;
        using T2 = int16_t;
        using T3 = int8_t;
        static constexpr size_t total_data_size_jump = sizeof(T1) + sizeof(T2) + sizeof(T3); // 4 + 2 + 1

        static inline size_t total_window_size = offset::config::window_size;

        template <typename T, size_t data_holder_offset = 0>
        struct Accessor : private offset::OffsetCalculator<T, total_data_size_jump, data_holder_offset>
        {
            using underlying_type = T;
            std::uint16_t current_element_index = 0;
            std::uint16_t current_window_index = 0;

            size_t get_object_index() const
            {
                // Calculate the parent MappedObject address from this Accessor's address
                const char *this_addr = reinterpret_cast<const char *>(this);
                size_t accessor_offset = 0;

                // Determine which accessor this is based on data_holder_offset
                if constexpr (data_holder_offset == 0)
                    accessor_offset = offsetof(MappedObject, data1);
                else if constexpr (data_holder_offset == sizeof(T1))
                    accessor_offset = offsetof(MappedObject, data2);
                else if constexpr (data_holder_offset == sizeof(T1) + sizeof(T2))
                    accessor_offset = offsetof(MappedObject, data3);

                const char *parent_addr = this_addr - accessor_offset;
                const MappedObject *parent = reinterpret_cast<const MappedObject *>(parent_addr);
                return parent->object_index;
            }

            template <typename U>
            void check_overflow(U value)
            {
                if (value > std::numeric_limits<T>::max())
                {
                    throw std::overflow_error("Value would overflow target type in push_back");
                }
                if constexpr (std::is_signed_v<U>)
                {
                    if (value < std::numeric_limits<T>::min())
                    {
                        throw std::overflow_error("Value would overflow target type in push_back");
                    }
                }
            }

            template <typename U>
            void push_back(U value)
            {
                check_overflow(value);

                if (current_element_index != 0 && current_element_index % offset::config::window_size == 0)
                {
                    if (current_window_index >= std::numeric_limits<uint16_t>::max())
                    {
                        throw std::overflow_error("current_window_index would overflow uint16_t");
                    }

                    // as soon as one of the object request to push outside the window we need to grow,
                    // but we need to avoid that other grow it afterwards also!
                    if (current_window_index == (total_window_size / offset::config::window_size - 1))
                    {
                        mapped_region->flush_and_grow();
                        total_window_size += offset::config::window_size;
                    }

                    if (current_element_index >= std::numeric_limits<uint16_t>::max())
                    {
                        throw std::overflow_error("current_element_index would overflow uint16_t");
                    }
                    current_window_index++;
                }

                const auto total_byte_offset = this->compute_offset(get_object_index(), current_element_index, current_window_index);
                char *base_addr = static_cast<char *>(mapped_region->get_address());
                T *target = reinterpret_cast<T *>(base_addr + total_byte_offset);
                *target = value;
                current_element_index++;
            }

            T &operator[](size_t index)
            {
                const auto total_byte_offset = this->compute_offset(get_object_index(), index, index / offset::config::window_size);
                char *base_addr = static_cast<char *>(mapped_region->get_address());
                T *target = reinterpret_cast<T *>(base_addr + total_byte_offset);
                return *target;
            }
        };

        Accessor<T1, 0> data1;
        Accessor<T2, sizeof(T1)> data2;
        Accessor<T3, sizeof(T1) + sizeof(T2)> data3;
    };

    static_assert(std::is_standard_layout<MappedObject>::value, "MappedObject must be standard layout");
}