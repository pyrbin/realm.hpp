
#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include "query.hpp"
#include "archetype.hpp"

namespace realm {

struct world;

struct system_base
{
protected:
    using archetype_t = archetype;

    template<typename T>
    using system_ptr = std::unique_ptr<T>;

public:
    inline constexpr system_base(){};
    inline ~system_base(){};

    virtual inline constexpr bool compare(size_t hash) const = 0;
    virtual inline void operator()(world*) const = 0;

};

template<typename T>
struct system_functor : public system_base
{
private:
    template<typename R = void, typename... Args>
    archetype_t create_archetype(R (T::*f)(Args...) const)
    {
        return internal::unpack_archetype<std::unwrap_ref_decay_t<Args>...>();
    }

    const system_ptr<T> inner_system;

public:
    const archetype_t archetype;

    inline constexpr system_functor(T&& s)
      : inner_system{ std::make_unique<T>(std::move(s)) }
      , archetype{ create_archetype(&T::update) }
    {}

    inline constexpr bool compare(size_t hash) const override { return archetype.subset(hash); }
    inline void operator()(world* world) const override
    {
        // TODO: anyway to use realm::query instead?
        internal::__query_inner(world, inner_system.get(), &T::update);
    }

};
} // namespace realm