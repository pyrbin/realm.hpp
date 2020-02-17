#pragma once

#include "../util/clean_query.hpp"
#include "../util/type_traits.hpp"

#include "chunk_view.hpp"

namespace realm {

// TODO: readd realm::entity to queries

template<typename... T>
struct query
{
    typedef std::tuple<T...> values;
    typedef std::tuple<std::add_lvalue_reference_t<T>...> references;
    typedef internal::clean_query_tuple_t<std::tuple<internal::pure_t<T>...>> components;

    const size_t mask;

    inline constexpr query()
      : mask{ archetype::mask_from_identity(std::type_identity<components>{}) } {};

    inline constexpr query(std::type_identity<std::tuple<T...>>)
      : mask{ archetype::mask_from_identity(std::type_identity<components>{}) } {};

    inline constexpr chunk_entity_view<T...> fetch(world* world)
    {
        return chunk_entity_view<T...>(world);
    }

    inline constexpr chunk_view fetch_chunk(world* world)
    {
        return chunk_view(world, mask);
    }
};

} // namespace realm