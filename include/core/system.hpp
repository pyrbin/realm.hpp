
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
#include "../util/tuple_util.hpp"

#include "query.hpp"

namespace realm {

struct world;

template<typename... T>
struct query;

struct system_base
{
    const size_t mask{ 0 };

    inline system_base(){};
    inline system_base(size_t mask) : mask{ mask } {};
    virtual inline ~system_base() = default;
    virtual constexpr bool compare(size_t hash) const = 0;
    virtual void operator()(world*) const = 0;
};

template<typename T>
struct system_functor : public system_base
{
private:
    const std::unique_ptr<T> inner_system;
    std::function<void(world*)> execute_query_function;

    template<typename R = void, typename... Ts>
    static inline size_t make_mask(R (T::*f)(Ts...) const)
    {
        using cleaned_type =
          internal::clean_query_tuple_t<std::tuple<std::unwrap_ref_decay_t<Ts>...>>;
        return archetype::mask_from_identity(std::type_identity<cleaned_type>{});
    }

public:
    template<typename... Args>
    inline system_functor(Args&&... args)
      : inner_system{ std::make_unique<T>(std::forward<Args>(args)...) }
      , system_base{ make_mask(&T::update) }
    {
        using type = typename decltype(internal::arg_types(&T::update))::type;
        auto qref = query{ std::type_identity<type>{} };
        execute_query_function = [&](world* world) {
            auto view = qref.fetch(world);
            std::for_each(view.begin(), view.end(), [&](auto components) {
                internal::apply_invoke(&T::update, inner_system, components);
            });
        };
    }

    inline constexpr bool compare(size_t other) const override
    {
        return archetype::subset(other, mask);
    }

    inline void operator()(world* world) const override { execute_query_function(world); }
};

} // namespace realm