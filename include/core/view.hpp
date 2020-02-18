#pragma once

#include "../util/clean_query.hpp"
#include "../util/type_traits.hpp"

#include "archetype.hpp"
#include "world.hpp"

#include <algorithm>
#include <memory>
#include <tuple>
#include <vector>

namespace realm {

struct world;

template<typename... T>
struct view
{
public:
    typedef std::tuple<T...> values;
    typedef std::tuple<std::add_lvalue_reference_t<T>...> references;
    typedef internal::clean_query_tuple_t<std::tuple<internal::pure_t<T>...>> components;

    inline constexpr view(archetype_chunk* chunk) : chunk{ chunk } {}

    static inline const size_t mask = archetype::mask_of<T...>();

    class iterator
    {
    public:
        typedef iterator self_type;
        typedef values value_type;
        typedef references reference;
        typedef std::forward_iterator_tag iterator_category;

        inline constexpr iterator(view* chunk_view) : chunk_view{ chunk_view }, index{ 0 }
        {}

        inline constexpr bool valid() { return chunk_view->chunk != nullptr; }

        inline void step()
        {
            index++;
            if (index >= chunk_view->chunk->size()) {
                chunk_view = nullptr;
                return;
            }
        }

        inline self_type operator++()
        {
            self_type i = *this;
            step();
            return i;
        }

        inline self_type operator++(int junk)
        {
            step();
            return *this;
        }

        inline reference operator*()
        {
            return std::forward_as_tuple(
              *chunk_view->chunk->template get<internal::pure_t<T>>(index)...);
        }

        inline constexpr bool operator!=(const self_type& other)
        {
            return chunk_view != other.chunk_view;
        }

    private:
        view* chunk_view;
        unsigned int index;
    };

    inline iterator begin() { return iterator(this); }

    inline iterator end() { return iterator(nullptr); }

private:
    archetype_chunk* chunk;
};

} // namespace realm