#pragma once

#include <tuple>
#include <type_traits>

#include <realm/util/type_traits.hpp>
#include <realm/core/entity.hpp>

namespace realm {
/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {
/**
 * Removes entity types from a tuple
 * Used to retrieve correct archetype of a query.
 * As a query can contain either components (valid)
 * or entity types.
 */
template <typename T, typename... Args>
struct remove_entity
{
};

template <>
struct remove_entity<std::tuple<entity>>
{
    using type = std::tuple<>;
};

template <>
struct remove_entity<std::tuple<>>
{
    using type = std::tuple<>;
};

template <typename T>
struct remove_entity<std::tuple<T>>
{
    using type = std::tuple<T>;
};

template <typename T, typename... Args>
struct remove_entity<std::tuple<T, Args...>>
{
    using type = decltype(
        tuple_cat(std::declval<typename remove_entity<std::tuple<T>>::type>(),
            std::declval<typename remove_entity<std::tuple<Args...>>::type>()));
};

template <typename T>
using remove_entity_t = typename remove_entity<T>::type;

template <typename... T>
using component_tuple = remove_entity_t<std::tuple<pure_t<T>...>>;
} // namespace internal
} // namespace realm
