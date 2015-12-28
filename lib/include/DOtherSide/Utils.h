#pragma once

#include <algorithm>
#include <functional>
#include <type_traits>

namespace DOS
{

template <typename T>
struct wrapped_array {
    wrapped_array(T* first, T* last) : begin_ {first}, end_ {last} {}
    wrapped_array(T* first, std::ptrdiff_t size)
        : wrapped_array {first, first + size} {}

    T*  begin() const noexcept { return begin_; }
    T*  end() const noexcept { return end_; }

    T* begin_;
    T* end_;
};

template <typename T>
wrapped_array<T> wrap_array(T* first, std::ptrdiff_t size) noexcept
{ return {first, size}; }

template <typename T, typename G>
std::vector<T> toVector(G* first, std::ptrdiff_t size) noexcept
{
    const wrapped_array<G> array = wrap_array(first, size);
    std::vector<T> result;
    std::copy(array.begin(), array.end(), result.begin());
    return result;
}

template <typename T, typename K, typename R = typename std::result_of<K(T)>::type>
std::vector<R> toVector(T* first, std::ptrdiff_t size, K f) noexcept
{
    wrapped_array<T> array = wrap_array<T>(first, size);
    std::vector<R> result;
    std::transform(array.begin(), array.end(), result.begin(), f);
    return result;
}
}
