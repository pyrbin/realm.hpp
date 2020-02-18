#pragma once

#include "../util/clean_query.hpp"
#include "../util/type_traits.hpp"

#include "archetype.hpp"
#include "view.hpp"

#include <algorithm>
#include <execution>
#include <memory>
#include <vector>

namespace realm {

/**
 * Queries the world for components defined in the functor object
 * @tparam F Lambda/functor type
 * @param world World to query
 * @param f Lambda/Functor object
 * @return
 */
template<typename F>
inline constexpr void
query(world* world, F&& f)
{
    using pure_t = internal::pure_t<F>;
    internal::query_helper(world, &f, &pure_t::operator());
}

/**
 * Queries the world for components defined in the functor object with an std::execution
 * policy. Each chunk will be run in parallel using std::for_each with provided policy.
 * @tparam ExePo Policy type
 * @tparam F Lambda/functor type
 * @param policy Policy (can be either par or par_unseq)
 * @param world World to query
 * @param f Lambda/Functor object
 * @return
 */
template<typename ExePo, typename F>
inline constexpr void
query(ExePo policy, world* world, F&& f)
{
    using pure_t = internal::pure_t<F>;

    internal::query_helper(policy, world, &f, &pure_t::operator());
}

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {

/**
 * @brief Per-chunk (view) query
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
 * @brief Per-entity query
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
 * @brief Parallel per-chunk (view) query
 */
template<typename ExePo, typename F, typename... Args>
inline constexpr void
query_helper(ExePo policy, world* world, F* object, void (F::*f)(view<Args...>) const)
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
 * @brief Parallel per-entity query
 */
template<typename ExePo, typename F, typename... Args>
inline constexpr void
query_helper(ExePo policy, world* world, F* object, void (F::*f)(Args...) const)
{
    std::for_each(policy, world->chunks.begin(), world->chunks.end(), [&](auto& root) {
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

/**
 * @brief Retrieves mask from a query function (used in systems)
 */
template<typename F, typename... Args>
inline constexpr size_t
query_mask(void (F::*f)(Args...) const)
{
    return (view<Args...>::mask);
}

/**
 * @brief Retrieves mask from a per-chunk (view) query function (used in systems)
 */
template<typename F, typename... Args>
inline constexpr size_t
query_mask(void (F::*f)(view<Args...>) const)
{
    return (view<Args...>::mask);
}
} // namespace internal
} // namespace realm