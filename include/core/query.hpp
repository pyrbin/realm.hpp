#pragma once

#include "../util/clean_query.hpp"
#include "archetype.hpp"
#include "world.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace realm {

namespace internal {

template<typename F, typename... Args>
inline constexpr void
__query_inner(world* world, F* obj, void (F::*f)(Args...) const)
{

    using cleaned_type =
      internal::clean_query_tuple_t<std::tuple<std::unwrap_ref_decay_t<Args>...>>;
    auto mask = archetype::mask_from_identity(std::type_identity<cleaned_type>{});
    for (auto& [hash, root] : world->chunks) {
        if (!root->archetype.subset(mask)) continue;
        for (auto& chunk : root->chunks) {
            for (uint32_t i{ 0 }; i < chunk->size(); i++) {
                (obj->*f)(*chunk->template get<std::unwrap_ref_decay_t<Args>>(
                  std::forward<uint32_t>(i))...);
            }
        }
    }
}

} // namespace internal

// TODO: only allow functors with arguments that are ref or const-ref to components
template<typename F>
inline constexpr void
query(world* world, F&& f)
{
    internal::__query_inner(world, &f, &F::operator());
}

} // namespace realm
