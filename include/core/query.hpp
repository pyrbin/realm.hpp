#pragma once

#include "archetype.hpp"
#include "concepts.hpp"
#include "world.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace realm {

namespace detail {

template<typename F, typename... Args>
inline constexpr void
__query_inner(F* obj, void (F::*f)(Args...) const) requires FetchPack<Args...>;

template<typename F, typename... Args>
inline constexpr void
__query_inner(world* world,
              F* obj,
              void (F::*f)(Args...) const) requires FetchPack<Args...>
{

    auto at = detail::unpack_archetype<std::unwrap_ref_decay_t<Args>...>();
    for (auto& [hash, root] : world->chunks) {
        if (!at.subset(hash)) continue;
        for (auto& chunk : root->chunks) {
            for (uint32_t i{ 0 }; i < chunk->size(); i++) {
                (obj->*f)(*chunk->template get<std::unwrap_ref_decay_t<Args>>(
                  std::forward<uint32_t>(i))...);
            }
        }
    }
}

} // namespace detail

template<typename F>
requires requires(F& f)
{
    detail::__query_inner(&f, &F::operator());
}
inline constexpr void
query(world* world, F&& f)
{
    detail::__query_inner(world, &f, &F::operator());
}

} // namespace realm
