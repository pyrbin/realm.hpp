#pragma once

#include <type_traits>

namespace realm {

namespace detail {

template<typename...>
inline constexpr bool is_unique = std::true_type{};

template<typename T, typename... Rest>
inline constexpr bool is_unique<T, Rest...> =
  std::bool_constant<(!std::is_same_v<T, Rest> && ...) && is_unique<Rest...>>{};

template<class T>
struct is_viable_component : std::is_class<T>::value
{};
}

template<typename T>
concept Component = !std::is_reference<T>::value;
}