#pragma once

#include <tuple>
#include <type_traits>

#include "../core/entity.hpp"

namespace realm {
namespace internal {
template<typename T, typename... Args>
struct clean_query_tuple
{};

template<>
struct clean_query_tuple<std::tuple<entity>>
{
    using type = std::tuple<>;
};

template<typename T>
struct clean_query_tuple<std::tuple<T>>
{
    using type = std::tuple<T>;
};

template<typename T, typename... Args>
struct clean_query_tuple<std::tuple<T, Args...>>
{
    using type = decltype(
      tuple_cat(std::declval<typename clean_query_tuple<std::tuple<T>>::type>(),
                std::declval<typename clean_query_tuple<std::tuple<Args...>>::type>()));
};

template<typename T>
using clean_query_tuple_t = typename clean_query_tuple<T>::type;

} // namespace internal
} // namespace realm
