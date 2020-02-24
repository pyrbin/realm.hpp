#pragma once

#include "../util/type_traits.hpp"

#include "view.hpp"
#include "world.hpp"

#include <algorithm>
#include <execution>
#include <memory>
#include <vector>

namespace realm {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace internal {
// TODO: insert all helper functions into a struct that is friend of world & make
// world::chunks private

/** @brief Gets the mask of a query. Excludes singleton components */
template<typename... Args>
constexpr size_t
query_mask(world* world)
{
    auto at = archetype::from_tuple<typename view<Args...>::components>();
    size_t mask{ 0 };
    for (auto& comp : at.components) {
        // If component is a singleton don't add to mask
        // mask is used to check archetype chunks
        if (world->is_singleton(comp)) continue;
        mask |= comp.meta.mask;
    }
    return mask;
}

/** @brief Per-chunk (view) query */
template<typename F, typename... Args>
constexpr internal::enable_if_query_fn<F, void>
query_helper(world* world, F* object, void (F::*f)(view<Args...>) const)
{
    auto mask = query_mask<Args...>(world);
    for (auto& root : world->chunks) {
        if (!root->archetype.subset(mask)) continue;
        for (auto& chunk : root->chunks) {
            view<Args...> view{ chunk.get(), world };
            (object->*f)(std::move(view));
        }
    }
}

/** @brief Per-entity query */
template<typename F, typename... Args>
constexpr internal::enable_if_query_fn<F, void>
query_helper(world* world, F* object, void (F::*f)(Args...) const)
{
    auto mask = query_mask<Args...>(world);
    for (auto& root : world->chunks) {
        if (!root->archetype.subset(mask)) continue;
        for (auto& chunk : root->chunks) {
            view<std::remove_reference_t<Args>...> view{ chunk.get(), world };
            for (uint32_t i{ 0 }; i < chunk->size(); i++) {
                (object->*f)(view.template get<std::remove_reference_t<Args>>(i)...);
            }
        }
    }
}

/** @brief Parallel per-chunk (view) query */
template<typename ExePo, typename F, typename... Args>
constexpr internal::enable_if_query_fn<F, void>
query_helper(ExePo policy, world* world, F* object, void (F::*f)(view<Args...>) const)
{
    auto mask = query_mask<Args...>(world);
    std::for_each(policy, world->chunks.begin(), world->chunks.end(), [&](auto& root) {
        if (!root->archetype.subset(mask)) return;
        std::for_each(std::execution::par,
                      root->chunks.begin(),
                      root->chunks.end(),
                      [&](auto& chunk) {
                          (object->*f)(view<Args...>{ chunk.get(), world });
                      });
    });
}

/** @brief Parallel per-entity query */
template<typename ExePo, typename F, typename... Args>
constexpr internal::enable_if_query_fn<F, void>
query_helper(ExePo policy, world* world, F* object, void (F::*f)(Args...) const)
{
    auto mask = query_mask<Args...>(world);
    std::for_each(policy, world->chunks.begin(), world->chunks.end(), [&](auto& root) {
        if (!root->archetype.subset(mask)) return;
        std::for_each(std::execution::par,
                      root->chunks.begin(),
                      root->chunks.end(),
                      [&](auto& chunk) {
                          view<internal::pure_t<Args>...> view{ chunk.get(), world };
                          for (uint32_t i{ 0 }; i < chunk->size(); i++) {
                              (object->*f)(
                                view.template get<internal::pure_t<Args>>(i)...);
                          }
                      });
    });
}

} // namespace internal

/**
 * @brief Query
 * Queries the world for components defined in the functor object
 * @tparam F Lambda/functor type
 * @param world World to query
 * @param f Lambda/Functor object
 * @return
 */
template<typename F>
constexpr internal::enable_if_query_fn<F, void>
query(world* world, F&& f)
{
    using pure_t = internal::pure_t<F>;
    internal::query_helper(std::execution::par_unseq, world, &f, &pure_t::operator());
}

/**
 * @brief Query with policy
 * Queries the world for components defined in the functor object with an std::execution
 * policy. Each chunk will be run in parallel using std::for_each with provided policy.
 * @tparam ExePo Policy type
 * @tparam F Lambda/functor type
 * @param policy Policy (can be either par or par_unseq)
 * @param world World to query
 * @param f Lambda/Functor object
 * @return
 */
template<typename F>
constexpr internal::enable_if_query_fn<F, void>
query_seq(world* world, F&& f)
{
    using pure_t = internal::pure_t<F>;
    internal::query_helper(world, &f, &pure_t::operator());
}

} // namespace realm