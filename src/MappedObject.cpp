#include <limits>
#include <stdexcept>

#include "MappedObject.hpp"

namespace mapped_object
{
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