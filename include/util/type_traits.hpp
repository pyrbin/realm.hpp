#pragma once

#include <type_traits>

namespace realm {
template<typename... Ts>
struct view;

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
                                      std::is_default_constructible_v<T> &&
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
 * If a parameter pack qualifies as a pack of components
 * @tparam T
 */
template<typename... T>
inline constexpr bool is_query_args =
  is_unique<std::remove_const_t<pure_t<T>>...> &&
  (... && ((is_component<pure_t<T>> && std::is_reference_v<T>) || is_entity<T>) );

/**
 * If a parameter pack qualifies as a pack of components
 * @tparam T
 */
template<typename... T>
inline constexpr bool is_view_args = is_unique<std::remove_const_t<pure_t<T>>...> &&
                                     (... && (is_component<T> || is_entity<T>) );

/**
 * If function is a member function
 */
template<typename T>
struct is_member_function : std::false_type
{};
template<typename T, typename R, typename... Ts>
struct is_member_function<R (T::*)(Ts...) const> : std::true_type
{};
template<typename T>
constexpr bool is_member_function_v = is_member_function<T>::value;

/**
 * If function is valid query function
 */
template<typename T, typename = void>
struct valid_query_functor : std::false_type
{};

template<typename T, typename R, typename... Args>
struct valid_query_functor<R (T::*)(Args...) const,
                           std::enable_if_t<is_query_args<Args...>>> : std::true_type
{};
template<typename T, typename R, typename... Args>
struct valid_query_functor<R (T::*)(view<Args...>) const,
                           std::enable_if_t<is_view_args<Args...>>> : std::true_type
{};
template<typename T>
constexpr bool valid_query_functor_v = valid_query_functor<T>::value;

/**
 * If object has valid update function (eg. is suitable to be a system)
 */
template<typename T>
inline constexpr bool has_valid_update_fn = (is_member_function_v<decltype(&T::update)> &&
                                             valid_query_functor_v<decltype(&T::update)>);

/**
 * Enable if utilities
 * TODO: replace most of these with C++20 concepts
 */
template<typename T, typename R = T>
using enable_if_component = std::enable_if_t<is_component<T>, R>;

template<typename T, typename R = T>
using enable_if_entity = std::enable_if_t<is_entity<T>, R>;

template<typename R, typename... T>
using enable_if_component_pack = std::enable_if_t<is_component_pack<T...>, R>;

template<typename T, typename R = T>
using enable_if_system = std::enable_if_t<has_valid_update_fn<T>, R>;

template<typename T, typename R = T>
using enable_if_query_fn =
  std::enable_if_t<valid_query_functor_v<decltype(&T::operator())>, R>;
} // namespace internal
} // namespace realm