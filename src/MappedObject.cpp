#include <limits>
#include <stdexcept>

#include "MappedObject.hpp"

namespace mapped_object
{
    template <typename T, size_t data_holder_offset>
    template <typename U>
    void MappedObject::Accessor<T, data_holder_offset>::push_back(U value)
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
    };

    template <typename T, size_t data_holder_offset>
    T &MappedObject::Accessor<T, data_holder_offset>::operator[](size_t index)
    {
        const auto total_byte_offset = this->compute_offset(get_object_index(), index, index / offset::config::window_size);
        char *base_addr = static_cast<char *>(mapped_region->get_address());
        T *target = reinterpret_cast<T *>(base_addr + total_byte_offset);
        return *target;
    }

    template <typename T, size_t data_holder_offset>
    const T &MappedObject::Accessor<T, data_holder_offset>::operator[](size_t index) const
    {
        const auto total_byte_offset = this->compute_offset(get_object_index(), index, index / offset::config::window_size);
        char *base_addr = static_cast<char *>(mapped_region->get_address());
        const T *target = reinterpret_cast<const T *>(base_addr + total_byte_offset);
        return *target;
    }

    template class MappedObject::Accessor<MappedObject::T1, 0>;
    template class MappedObject::Accessor<MappedObject::T2, sizeof(MappedObject::T1)>;
    template class MappedObject::Accessor<MappedObject::T3, sizeof(MappedObject::T1) + sizeof(MappedObject::T2)>;

    template void MappedObject::Accessor<MappedObject::T1, 0>::push_back<int>(int);
    template void MappedObject::Accessor<MappedObject::T2, sizeof(MappedObject::T1)>::push_back<int>(int);
    template void MappedObject::Accessor<MappedObject::T3, sizeof(MappedObject::T1) + sizeof(MappedObject::T2)>::push_back<int>(int);

    template void MappedObject::Accessor<MappedObject::T1, 0>::push_back<size_t>(size_t);
    template void MappedObject::Accessor<MappedObject::T2, sizeof(MappedObject::T1)>::push_back<size_t>(size_t);
    template void MappedObject::Accessor<MappedObject::T3, sizeof(MappedObject::T1) + sizeof(MappedObject::T2)>::push_back<size_t>(size_t);
}