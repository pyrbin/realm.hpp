#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace realm {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {
/**
 * Pure/underlying type. Unwraps reference and decays type.
 */
template<typename T>
using pure_t = std::unwrap_ref_decay_t<T>;

template<typename...>
constexpr bool is_unique = std::true_type{};

/**
 * Check if a parameter pack only contains unique types
 * @tparam T
 * @tparam Rest
 */
template<typename T, typename... Rest>
constexpr bool is_unique<T, Rest...> =
  std::bool_constant<(!std::is_same_v<T, Rest> && ...) && is_unique<Rest...>>{};

/**
 * If a type qualifies as a component
 * @tparam T
 */
template<typename T>
inline constexpr bool is_component = (std::is_class_v<T> &&
                                      std::is_copy_constructible_v<T> &&
                                      std::is_move_constructible_v<T>);
/**
 * If a parameter pack qualifies as a pack of components
 * @tparam T
 */
template<typename... T>
inline constexpr bool is_component_pack = is_unique<std::remove_const_t<pure_t<T>>...> &&
                                          (... && is_component<T>);

/**
 * If type is an entity id type
 */
template<typename T>
inline constexpr bool is_entity = std::is_integral_v<T>;

/**
 * Enable if utilities
 */
template<typename T, typename R = T>
using enable_if_component = std::enable_if_t<is_component<T>, R>;

template<typename T, typename R = T>
using enable_if_entity = std::enable_if_t<is_entity<T>, R>;

template<typename R, typename... T>
using enable_if_component_pack = std::enable_if_t<is_component_pack<T...>, R>;

} // namespace internal
} // namespace realm