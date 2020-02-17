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

struct chunk_view
{
public:
    const size_t mask;

    inline constexpr chunk_view(world* ptr, size_t mask) : world_ptr{ ptr }, mask{ mask }
    {}

    class iterator
    {
    public:
        typedef iterator self_type;
        typedef archetype_chunk value_type;
        typedef value_type* pointer;
        typedef value_type& reference;
        typedef std::forward_iterator_tag iterator_category;

        inline iterator(){};
        inline iterator(chunk_view* view) : view{ view }, index{ 0 }
        {
            if (valid()) {

                iter = view->world_ptr->chunks.begin();
                step();
            }
        }

        inline constexpr bool valid() { return view != nullptr; }

        inline constexpr void step()
        {
            while (valid() && !get_root()->archetype.subset(view->mask) ||
                   index >= get_root()->chunks.size()) {
                iter++;
                index = 0;
                if (iter == view->world_ptr->chunks.end()) {
                    view = nullptr;
                    break;
                }
            }
        }

        inline self_type operator++()
        {
            self_type i = *this;
            index++;
            step();
            return i;
        }

        inline self_type operator++(int junk)
        {
            index++;
            step();
            return *this;
        }

        inline reference operator*() { return *get_chunk(); }
        inline pointer operator->() { return get_chunk(); }

        inline constexpr bool operator!=(const self_type& other)
        {
            return view != other.view;
        }
        inline constexpr bool operator==(const self_type& other)
        {
            return !this->operator!=(other);
        }

    private:
        archetype_chunk_root* get_root() { return (*iter).get(); }
        archetype_chunk* get_chunk() { return get_root()->chunks.at(index).get(); }

        chunk_view* view;
        world::chunks_t::iterator iter;
        unsigned int index;
    };

    inline iterator begin() { return iterator(this); }

    inline iterator end() { return iterator(nullptr); }

private:
    world* world_ptr;
};

template<typename... T>
struct chunk_entity_view
{
public:
    typedef std::tuple<T...> values;
    typedef std::tuple<std::add_lvalue_reference_t<T>...> references;
    typedef internal::clean_query_tuple_t<std::tuple<internal::pure_t<T>...>> components;
    typedef chunk_view view_t;

    inline constexpr chunk_entity_view(world* world)
      : view(world, archetype::mask_from_identity(std::type_identity<components>{}))
    {}

    class iterator
    {
    public:
        typedef iterator self_type;
        typedef values value_type;
        typedef references reference;
        typedef std::forward_iterator_tag iterator_category;

        inline constexpr iterator(view_t* view) : view{ view }, index{ 0 }
        {
            if (valid()) { iter = view->begin(); }
        }

        inline constexpr bool valid() { return view != nullptr; }

        inline void step()
        {
            if (index >= get_chunk().size()) {
                iter++;
                index = 0;
                if (iter == view->end()) {
                    view = nullptr;
                    return;
                }
            }
        }

        inline self_type operator++()
        {
            self_type i = *this;
            index++;
            step();
            return i;
        }

        inline self_type operator++(int junk)
        {
            index++;
            step();
            return *this;
        }

        inline reference operator*()
        {
            return std::forward_as_tuple(
              *get_chunk().template get<internal::pure_t<T>>(index)...);
        }

        inline constexpr bool operator!=(const self_type& other)
        {
            return view != other.view;
        }

    private:
        archetype_chunk& get_chunk() { return *iter; }

        view_t* view;
        typename view_t::iterator iter;
        unsigned int index;
    };

    inline iterator begin() { return iterator(&view); }

    inline iterator end() { return iterator(nullptr); }

private:
    view_t view;
};

} // namespace realm