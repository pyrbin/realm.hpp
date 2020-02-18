#pragma once

#include "../util/clean_query.hpp"
#include "../util/tuple_util.hpp"
#include "../util/type_traits.hpp"

#include "archetype.hpp"
#include "view.hpp"

#include <algorithm>
#include <memory>
#include <vector>
#include <execution>

namespace realm {

namespace internal {

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
template<typename ExePo, typename F, typename... Args>
inline constexpr void
query_helper(ExePo policy, world* world,
             F* object,
             void (F::*f)(view<Args...>) const)
{

    std::for_each(policy, world->chunks.begin(), world->chunks.end(), [&](auto& root) {
        if (!root->archetype.subset(view<Args...>::mask)) return;
        std::for_each(std::execution::par,
                      root->chunks.begin(),
                      root->chunks.end(),
                      [&](auto& chunk) { (object->*f)(view<Args...>(chunk.get())); });
    });
}

/**
 * Normal query
 */
template<typename ExePo, typename F, typename... Args>
inline constexpr void
query_helper(ExePo policy, world* world,
             F* object,
             void (F::*f)(Args...) const)
{


    std::for_each(policy, world->chunks.begin(), world->chunks.end(), 
      [&](auto& root) {
        if (!root->archetype.subset(view<Args...>::mask)) return;
          std::for_each(std::execution::par,
                        root->chunks.begin(),
                        root->chunks.end(),
                        [&](auto& chunk) {
                            for (uint32_t i{ 0 }; i < chunk->size(); i++) {
                                (object->*f)(
                                  *chunk->template get<internal::pure_t<Args>>(i)...);
                            }
                        });
      });
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
    using pure_t = internal::pure_t<F>;

    internal::query_helper(world, &f, &pure_t::operator());
}

template<typename ExePo, typename F>
inline constexpr void
query(ExePo policy, world* world, F&& f)
{
    using pure_t = internal::pure_t<F>;

    internal::query_helper(policy, world, &f, &pure_t::operator());
}

} // namespace realm