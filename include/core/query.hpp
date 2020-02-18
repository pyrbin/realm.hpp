#pragma once

#include "../util/clean_query.hpp"
#include "../util/type_traits.hpp"

#include "archetype.hpp"
#include "world.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace realm {

namespace internal {

template<typename F, typename... Args>
inline constexpr void
query_helper(world* world, F* ftor, void (F::*f)(Args...) const)
{

    using type =
      internal::clean_query_tuple_t<std::tuple<std::unwrap_ref_decay_t<Args>...>>;
    auto mask = archetype::mask_from_identity(std::type_identity<type>{});

    for (auto& root : world->chunks) {
        if (!root->archetype.subset(mask)) continue;
        for (auto& chunk : root->chunks) {
            for (uint32_t i{ 0 }; i < chunk->size(); i++) {
                (ftor->*f)(*chunk->template get<std::unwrap_ref_decay_t<Args>>(i)...);
            }
        }
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