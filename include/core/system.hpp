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
query_helper(world* world, F* obj, void (F::*f)(Args...) const);

}

struct system_ref
{
    const size_t mask{ 0 };
    inline system_ref(){};
    inline system_ref(size_t mask) : mask{ mask } {};
    virtual inline ~system_ref() = default;
    virtual constexpr bool compare(size_t hash) const = 0;
    virtual void operator()(world*) const = 0;
};

template<typename T>
struct system_proxy : public system_ref
{
private:
    const std::unique_ptr<T> system;

    template<typename R = void, typename... Args>
    static inline size_t make_mask(R (T::*f)(Args...) const)
    {
        using type =
          internal::clean_query_tuple_t<std::tuple<std::unwrap_ref_decay_t<Args>...>>;
        return archetype::mask_from_identity(std::type_identity<type>{});
    }

public:
    template<typename... Args>
    inline system_proxy(Args&&... args)
      : system{ std::make_unique<T>(std::forward<Args>(args)...) }
      , system_ref{ make_mask(&T::update) }
    {}

    inline constexpr bool compare(size_t other) const override
    {
        return archetype::subset(other, mask);
    }

    inline void operator()(world* world) const override
    {
        internal::query_helper(world, system.get(), &T::update);
    }
};

} // namespace realm