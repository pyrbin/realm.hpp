#pragma once

#include <algorithm>

namespace realm {
/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {
/**
 * @brief Swap remove
 *
 * Removes element at index and swap with last value in a
 * vector. Does not free memory.
 *
 * @tparam T vector type
 * @param index element to remove
 * @param target vector to target
 */
template<typename T>
constexpr void swap_remove(unsigned index, T& target)
{
    std::swap(target[target.size() - 1], target[index]);
    target.resize(target.size() - 1);
}
} // namespace internal
} // namespace realm