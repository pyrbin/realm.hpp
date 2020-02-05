#pragma once

#include "../detail/type_traits.hpp"
#include <type_traits>

/** designs goals:
 * component::of    -> only pure classes eg, pos, vel & not const pos, pos&, pos*
 * archetype::of    -> same rules as component
 *
 * query    ::of    -> allows pure classes & const class (write & read) or an entity
 * query    ::args  -> same as query but only reference or an entity no referenec
 *
 * concepts needed:
 *
 * BaseComponent -> is_class<T> || !is_reference<T> || is_copyable<T> || is_moveable<T>
 * Component -> is_class<T> || !is_reference<T> || is_copyable<T> || is_moveable<T> ||
 * !is_const Queryable -> is_class<T>
 */

namespace realm {

template<typename... T>
concept UniquePack =
  detail::is_unique<std::remove_const_t<std::unwrap_ref_decay_t<T>>...>;

template<typename T>
concept Entity = std::is_integral_v<T>;

template<typename T>
concept BaseComponent = (std::is_class_v<T> && std::is_copy_constructible_v<T> &&
                         std::is_move_constructible_v<T>);
template<typename T>
concept Component = (BaseComponent<T> && !std::is_const_v<T> && !std::is_reference_v<T>);

template<typename... T>
concept ComponentPack = (UniquePack<T...> && (Component<T>, ...));

template<typename T>
concept FetchComponent = (BaseComponent<std::unwrap_ref_decay_t<T>> &&
                          std::is_reference_v<T>);
template<typename... T>
concept FetchPack = UniquePack<T...> && ((FetchComponent<T> || Entity<T>), ...);

template<typename T>
concept System = std::is_same_v<std::unwrap_ref_decay_t<T>, T> && !std::is_const_v<T>;

} // namespace realm