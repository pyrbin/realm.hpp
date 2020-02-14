#pragma once

#include "../util/clean_query.hpp"
#include "../util/type_traits.hpp"

#include "archetype.hpp"
#include "world.hpp"

#include <algorithm>
#include <memory>
#include <vector>
#include <execution>
#include <experimental/generator>
#include <experimental/coroutine>

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

template<typename F, typename... Args>
inline constexpr void
__query_inner_par(world* world, F* obj, void (F::*f)(Args...) const)
{

    using cleaned_type =
      internal::clean_query_tuple_t<std::tuple<std::unwrap_ref_decay_t<Args>...>>;
    auto mask = archetype::mask_from_identity(std::type_identity<cleaned_type>{});

    std::for_each(std::execution::par,
                  world->chunks.begin(),
                  world->chunks.end(),
                  [&](auto& pair) {
                      auto& root = pair.second;
                      if (!root->archetype.subset(mask)) return;
                      std::for_each(std::execution::par,
                                    root->chunks.begin(),
                                    root->chunks.end(),
                                    [&](auto& chunk) {
                            for (uint32_t i{ 0 }; i < chunk->size(); i++) {
                                (obj->*f)(
                                  *chunk->template get<std::unwrap_ref_decay_t<Args>>(
                                    std::forward<uint32_t>(i))...);
                            }
                  });
    });


}

} // namespace internal

// TODO: only allow functors with arguments that are ref or const-ref to components
template<typename F>
inline constexpr void
query(world* world, F&& f)
{
    internal::__query_inner(world, &f, &F::operator());
}

template<typename F>
inline constexpr void
query_par(world* world, F&& f)
{
    internal::__query_inner_par(world, &f, &F::operator());
}


namespace exp {
    using std::experimental::generator;

    // TODO: allow realm::entity fetches.
    template<typename... T>
    struct query
    {
        using references = std::tuple<std::add_lvalue_reference_t<T>...>;
        using components = internal::clean_query_tuple_t<std::tuple<internal::pure_t<T>...>>;

        const size_t mask;

        inline constexpr query() : mask
        {
            archetype::mask_from_identity(std::type_identity<components>{})
        }
        {};

        generator<references> fetch(world* world)
        {
            for (auto& chunk : fetch_chunk(world)) {
                for (uint32_t i{ 0 }; i < chunk->size(); i++) {
                    co_yield std::forward_as_tuple(
                      *chunk->template get<internal::pure_t<T>>(
                        std::forward<uint32_t>(i))...);
                }
            }
        }

        generator<archetype_chunk*> fetch_chunk(world* world)
        {
            for (auto& [hash, root] : world->chunks) {
                if (!root->archetype.subset(mask)) continue;
                for (auto& chunk : root->chunks) {
                    co_yield chunk.get();
                }
            }
        }
 
    };

}

} // namespace realm
