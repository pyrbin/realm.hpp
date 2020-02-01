#pragma once

#include "archetype.hpp"
#include "concept.hpp"
#include "world.hpp"

#include <functional>
#include <tuple>

template<typename T>
using func = std::function<T>;

namespace realm {

struct world;

template<typename... Args>
requires QueryInput<Args...> struct query
{
    using query_type = std::tuple<Args...>;

    const struct archetype at;

    query() : at{ build_comp_set<Args...>(std::vector<component>{}) } {}

    class chunk_iterator
    {
    public:
        typedef chunk_iterator self_type;
        typedef query_type value_type;
        typedef query_type& reference;
        typedef query_type* pointer;
        typedef std::forward_iterator_tag iterator_category;

        chunk_iterator(archetype_chunk& chunk, uint index)
          : chunk{ chunk }, index{ index }
        {}

        ~chunk_iterator()
        { /* chunk = nullptr; */
        }

        self_type operator++()
        {
            self_type i = *this;
            index++;
            return i;
        }

        self_type operator++(int junk)
        {
            index++;
            return *this;
        }

        const value_type operator*()
        {
            return std::forward_as_tuple(
              *chunk.template get<std::decay_t<Args>>(std::forward<uint32_t>(index))...);
        }

        const value_type operator->()
        {
            return std::forward_as_tuple(
              *chunk.template get<std::decay_t<Args>>(std::forward<uint32_t>(index))...);
        }

        bool operator==(const self_type& other) { return index != other.index; }

        bool operator!=(const self_type& other) { return index != other.index; }

    private:
        uint index{ 0 };
        archetype_chunk& chunk;
    };

    struct chunk_view
    {
        archetype_chunk* chunk;

    public:
        chunk_iterator begin() { return chunk_iterator(*chunk, 0); }
        chunk_iterator end() { return chunk_iterator(*chunk, 1); }
    };

    chunk_view&& fetch(world* world) { chunk_view{ world->get_chunk(at) }; }

    template<typename... T>
    static inline std::vector<component>& build_comp_set(std::vector<component>&& v)
    {
        (build_comp_helper<Args>(v), ...);
        return v;
    }
    template<typename T>
    static inline void build_comp_helper(std::vector<component>& v)
    {
        using unwrapped_t = std::unwrap_ref_decay_t<T>;
        v.push_back(component::of<unwrapped_t>());
    }
    template<Entity F>
    static inline void build_comp_helper(std::vector<component>& v)
    {}

private:
};

} // namespace realm
