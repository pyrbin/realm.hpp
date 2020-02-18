#pragma once

#include "../util/clean_query.hpp"
#include "../util/tuple_util.hpp"
#include "../util/type_traits.hpp"

#include "archetype.hpp"
#include "view.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace realm {

namespace internal {

/**
 * Normal query
 */
template<typename F, typename... Args>
inline constexpr void
query_helper(world* world, F* object, void (F::*f)(Args...) const)
{
    for (auto& root : world->chunks) {
        if (!root->archetype.subset(view<Args...>::mask)) continue;
        for (auto& chunk : root->chunks) {
            for (uint32_t i{ 0 }; i < chunk->size(); i++) {
                (object->*f)(*chunk->template get<internal::pure_t<Args>>(i)...);
            }
        }
    }
}

/**
 * View query
 * @return
 */
template<typename F, typename... Args>
inline constexpr void
query_helper(world* world, F* object, void (F::*f)(view<Args...>) const)
{
    for (auto& root : world->chunks) {
        if (!root->archetype.subset(view<Args...>::mask)) continue;
        for (auto& chunk : root->chunks) { (object->*f)(view<Args...>(chunk.get())); }
    }
}

template<typename F, typename... Args>
inline constexpr size_t
query_mask(void (F::*f)(Args...) const)
{
    return (view<Args...>::mask);
}

template<typename F, typename... Args>
inline constexpr size_t
query_mask(void (F::*f)(view<Args...>) const)
{
    return (view<Args...>::mask);
}

} // namespace internal

template<typename F>
inline constexpr void
query(world* world, F&& f)
{
    internal::query_helper(world, &f, &F::operator());
}

} // namespace realm