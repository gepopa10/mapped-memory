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

// C++17 alternative to the c++20 std::in_range

template <class T, class U>
constexpr bool cmp_equal(T t, U u) noexcept
{
    if constexpr (std::is_signed_v<T> == std::is_signed_v<U>)
        return t == u;
    else if constexpr (std::is_signed_v<T>)
        return t >= 0 && std::make_unsigned_t<T>(t) == u;
    else
        return u >= 0 && std::make_unsigned_t<U>(u) == t;
}

template <class T, class U>
constexpr bool cmp_not_equal(T t, U u) noexcept
{
    return !cmp_equal(t, u);
}

template <class T, class U>
constexpr bool cmp_less(T t, U u) noexcept
{
    if constexpr (std::is_signed_v<T> == std::is_signed_v<U>)
        return t < u;
    else if constexpr (std::is_signed_v<T>)
        return t < 0 || std::make_unsigned_t<T>(t) < u;
    else
        return u >= 0 && t < std::make_unsigned_t<U>(u);
}

template <class T, class U>
constexpr bool cmp_greater(T t, U u) noexcept
{
    return cmp_less(u, t);
}

template <class T, class U>
constexpr bool cmp_less_equal(T t, U u) noexcept
{
    return !cmp_less(u, t);
}

template <class T, class U>
constexpr bool cmp_greater_equal(T t, U u) noexcept
{
    return !cmp_less(t, u);
}

template <class R, class T>
constexpr bool in_range(T t) noexcept
{
    if constexpr (std::is_same_v<R, T>) return true;

    if constexpr (std::is_floating_point_v<R>)
    {
        // Accept any non-NaN value; infinities are representable by float/double
        return !(std::is_floating_point_v<T> && !(t == t));
    }

    return cmp_greater_equal(t, std::numeric_limits<R>::min()) &&
           cmp_less_equal(t, std::numeric_limits<R>::max());
}