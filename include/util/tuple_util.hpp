#pragma once

#include <functional>
#include <tuple>

namespace realm {
namespace internal {
/**
 * @brief Removes element at index and swap with last value in a
 * vector. Does not free memory.
 *
 * @tparam T vector type
 * @param index element to remove
 * @param target vector to target
 */
template<typename F, typename T, typename U>
inline constexpr decltype(auto)
apply_invoke(F&& func, T&& first, U&& tuple)
{
    return std::apply(std::forward<F>(func),
                      std::tuple_cat(std::forward_as_tuple(std::forward<T>(first)),
                                     std::forward<U>(tuple)));
}

} // namespace internal
} // namespace realm
