#pragma once

#include "traits.hpp"
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

template<typename T>
concept BaseComponent = (std::is_class<T>::value &&
                         std::is_trivially_copyable<T>::value &&
                         std::is_copy_constructible<T>::value &&
                         std::is_move_constructible<T>::value);

template<typename... T>
concept UniquePack =
  detail::is_unique<std::remove_const_t<std::unwrap_ref_decay_t<T>>...>;

template<typename T>
concept Component = (BaseComponent<T> && !std::is_const<T>::value &&
                     !std::is_reference<T>::value);

template<typename... T>
concept ComponentPack = UniquePack<T...> && (Component<T>, ...);

template<typename T>
concept FetchComponent = (BaseComponent<T> && !std::is_reference<T>::value);

template<typename T>
concept Entity = std::is_integral_v<T>;

template<typename... T>
concept QueryPack = UniquePack<T...> && ((FetchComponent<T> || Entity<T>), ...);

template<typename T>
concept NotWrapped = std::is_same_v<std::unwrap_ref_decay_t<T>, T>;

template<typename T, typename... Args>
concept Invocable = std::is_invocable<T, Args...>::value || std::is_function<T>::value;

template<typename T, typename... Args>
concept Inner = UniquePack<Args...> && ((FetchComponent<Args> || Entity<Args>), ...);

template<typename T>
concept QueryFunction = requires(T&& f)
{
    fetch_helper(&f, &std::unwrap_ref_decay_t<T>::operator());
};

} // namespace realm