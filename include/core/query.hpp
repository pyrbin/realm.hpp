#pragma once

#include "../util/clean_query.hpp"
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
query_helper(world* world, F* ftor, void (F::*f)(Args...) const)
{

    using tuple = internal::clean_query_tuple_t<std::tuple<internal::pure_t<Args>...>>;
    auto mask = archetype::mask_from_tuple<tuple>();

    for (auto& root : world->chunks) {
        if (!root->archetype.subset(mask)) continue;
        for (auto& chunk : root->chunks) {
            for (uint32_t i{ 0 }; i < chunk->size(); i++) {
                (ftor->*f)(*chunk->template get<internal::pure_t<Args>>(i)...);
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
query_helper(world* world, F* ftor, void (F::*f)(view<Args...>) const)
{

    for (auto& root : world->chunks) {
        if (!root->archetype.subset(view<Args...>::mask)) continue;
        for (auto& chunk : root->chunks) { (ftor->*f)(view<Args...>(chunk.get())); }
    }
}

} // namespace internal

template<typename F>
inline constexpr void
query(world* world, F&& f)
{
    internal::query_helper(world, &f, &F::operator());
}

} // namespace realm