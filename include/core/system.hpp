#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <execution>

#include "../util/clean_query.hpp"
#include "archetype.hpp"

namespace realm {

struct world;

template<typename... Ts>
struct view;

namespace internal {

template<typename F, typename... Args>
inline constexpr void
query_helper(world* world, F* obj, void (F::*f)(Args...) const);

template<typename ExePo, typename F, typename... Args>
inline constexpr void
query_helper(ExePo policy,
             world* world,
             F* object,
             void (F::*f)(Args...) const);

template<typename F, typename... Args>
inline constexpr void
query_helper(world* world, F* obj, void (F::*f)(view<Args...>) const);

template<typename ExePo, typename F, typename... Args>
inline constexpr void
query_helper(ExePo policy,
             world* world,
             F* object,
             void (F::*f)(view<Args...>) const);

template<typename F, typename... Args>
inline constexpr size_t
query_mask(void (F::*f)(Args...) const);

template<typename F, typename... Args>
inline constexpr size_t
query_mask(void (F::*f)(view<Args...>) const);

} // namespace internal

struct system_ref
{
    const size_t mask{ 0 };
    inline system_ref(){};
    inline system_ref(size_t mask) : mask{ mask } {};
    virtual inline ~system_ref() = default;
    virtual constexpr bool compare(size_t hash) const = 0;
    virtual void invoke(world*) const = 0;
    virtual void invoke(std::execution::parallel_policy policy, world* world) const = 0;
    virtual void invoke(std::execution::parallel_unsequenced_policy policy, world* world) const = 0;
};

template<typename T>
struct system_proxy : public system_ref
{
private:
    const std::unique_ptr<T> system;

public:
    template<typename... Args>
    inline system_proxy(Args&&... args)
      : system{ std::make_unique<T>(std::forward<Args>(args)...) }
      , system_ref{ internal::query_mask(&T::update) }
    {}

    inline constexpr bool compare(size_t other) const override
    {
        return archetype::subset(other, mask);
    }

    inline void invoke(world* world) const override
    {
        internal::query_helper(world, system.get(), &T::update);
    }

    inline void invoke(std::execution::parallel_policy policy,
                       world* world) const override
    {
        internal::query_helper(policy, world, system.get(), &T::update);
    }

    inline void invoke(std::execution::parallel_unsequenced_policy policy,
                       world* world) const override
    {
        internal::query_helper(policy, world, system.get(), &T::update);
    }
};

} // namespace realm