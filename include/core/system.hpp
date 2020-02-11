
#pragma once

#include <cstddef>
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

struct world;

namespace internal {

template<typename F, typename... Args>
inline constexpr void
__query_inner(world* world, F* obj, void (F::*f)(Args...) const);

} // namespace internal

struct system_base
{
protected:
    template<typename T>
    using system_ptr = std::unique_ptr<T>;

public:
    struct archetype archetype;

    inline system_base(){};
    inline system_base(const struct archetype& at) : archetype{ at } {};
    virtual inline ~system_base() = default;

    virtual inline constexpr bool compare(size_t hash) const = 0;
    virtual inline void operator()(world*) const = 0;
};

template<typename T>
struct system_functor : public system_base
{
private:
    template<typename R = void, typename... Args>
    struct archetype create_archetype(R (T::*f)(Args...) const)
    {
        using query_type = std::tuple<std::unwrap_ref_decay_t<Args>...>;
        using cleaned_type = internal::clean_query_tuple_t<query_type>;
        return archetype::from_identity(std::type_identity<cleaned_type>{});
    }

    const system_ptr<T> inner_system;

public:
    template<typename... Args>
    inline constexpr system_functor(Args&&... args)
      : inner_system{ std::make_unique<T>(std::forward<Args>(args)...) }
      , system_base{ create_archetype(&T::update) }
    {}

    inline constexpr bool compare(size_t hash) const override
    {
        return archetype.subset(hash);
    }

    inline void operator()(world* world) const override
    {
        // TODO: anyway to use realm::query instead?
        internal::__query_inner(world, inner_system.get(), &T::update);
    }
};

} // namespace realm