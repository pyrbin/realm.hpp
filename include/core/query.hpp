#pragma once

#include <memory>
#include <vector>

#include "archetype.hpp"
#include "concepts.hpp"
#include "world.hpp"

namespace realm {

namespace detail {

// add reference to type if
template<typename T>
struct add_ref_if_comp
{
    typedef T type;
};
template<BaseComponent T>
struct add_ref_if_comp<T>
{
    typedef std::add_lvalue_reference_t<T> type;
};

template<typename T>
using add_ref_if_comp_t = typename add_ref_if_comp<T>::type;

template<class T, class U>
concept SameAs = std::is_same_v<T, U>;

template<typename... T>
struct query_helper
{
    typedef std::tuple<T...> query_type;
    typedef std::tuple<detail::add_ref_if_comp_t<T>...> fetch_type;

    template<typename F, typename... Args>
    static constexpr void fetch_constrait(
      F* obj,
      void (F::*f)(Args...) const) requires SameAs<std::tuple<Args...>, fetch_type>;
};

} // namespace detail

template<typename... T>
struct query
{
public:
    typedef typename detail::query_helper<T...>::query_type query_type;
    typedef typename detail::query_helper<T...>::fetch_type fetch_type;

    const struct archetype archetype;

    query() requires QueryPack<T...> : archetype{ detail::unpack_archetype<T...>() } {}

    /* TODO: implement iterator

    struct iterator
    {};

    iterator fetch(struct world* world);
    */

    template<typename F>
    requires requires(F&& f)
    {
        detail::query_helper<T...>::fetch_constrait(&f, &F::operator());
    }
    void fetch(struct world* world, F&& f)
    {
        world->iter_chunk_roots(archetype, [&](archetype_chunk_root* root) {
            for (auto& chunk : root->chunks) {
                for (uint32_t i{ 0 }; i < chunk->size(); i++) {
                    (f)(*chunk->template get<std::decay_t<T>>(
                      std::forward<uint32_t>(i))...);
                }
            }
        });
    }

private:
};

} // namespace realm
