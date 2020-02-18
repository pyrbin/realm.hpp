#pragma once

#include <cstddef>
#include <execution>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include "../util/clean_query.hpp"
#include "archetype.hpp"

namespace realm {

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

struct world;

template<typename... Ts>
struct view;

namespace internal {

/**
 * Forward declarations of query functions
 */
template<typename F, typename... Args>
inline constexpr void
query_helper(world* world, F* obj, void (F::*f)(Args...) const);

template<typename ExePo, typename F, typename... Args>
inline constexpr void
query_helper(ExePo policy, world* world, F* object, void (F::*f)(Args...) const);

template<typename F, typename... Args>
inline constexpr void
query_helper(world* world, F* obj, void (F::*f)(view<Args...>) const);

template<typename ExePo, typename F, typename... Args>
inline constexpr void
query_helper(ExePo policy, world* world, F* object, void (F::*f)(view<Args...>) const);

template<typename F, typename... Args>
inline constexpr size_t
query_mask(void (F::*f)(Args...) const);

template<typename F, typename... Args>
inline constexpr size_t
query_mask(void (F::*f)(view<Args...>) const);

/**
 * @brief System reference
 * Base system reference holder
 */
struct system_ref
{
    const size_t mask{ 0 };
    inline system_ref(){};
    inline system_ref(size_t mask) : mask{ mask } {};
    virtual inline ~system_ref() = default;
    virtual constexpr bool compare(size_t hash) const = 0;
    virtual void invoke(world*) const = 0;
    virtual void invoke(std::execution::parallel_policy policy, world* world) const = 0;
    virtual void invoke(std::execution::parallel_unsequenced_policy policy,
                        world* world) const = 0;
};

/**
 * @brief System proxy
 * A proxy class used to communicate with a defined system.
 * Used to invoke the update function and uses query_helper functions to
 * execute the query logic.
 * @tparam T Underlying system class
 */
template<typename T>
struct system_proxy : public system_ref
{
private:
    /*! @brief Underlying system pointer */
    const std::unique_ptr<T> underlying_system;

public:
    /**
     * Construct a system proxy with arguments for the underlying system.
     * @tparam Args Argument types
     * @param args Arguments for underlying system
     */
    template<typename... Args>
    inline system_proxy(Args&&... args)
      : underlying_system{ std::make_unique<T>(std::forward<Args>(args)...) }
      , system_ref{ internal::query_mask(&T::update) }
    {}

    inline constexpr bool compare(size_t other) const override
    {
        return archetype::subset(other, mask);
    }

    /**
     * Call the system query on a world
     * @param world
     */
    inline void invoke(world* world) const override
    {
        internal::query_helper(world, underlying_system.get(), &T::update);
    }

    inline void invoke(std::execution::parallel_policy policy,
                       world* world) const override
    {
        internal::query_helper(policy, world, underlying_system.get(), &T::update);
    }

    inline void invoke(std::execution::parallel_unsequenced_policy policy,
                       world* world) const override
    {
        internal::query_helper(policy, world, underlying_system.get(), &T::update);
    }
};

} // namespace internal

} // namespace realm