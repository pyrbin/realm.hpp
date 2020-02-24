#pragma once

#include <algorithm>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "../util/tuple_util.hpp"
#include "../util/type_traits.hpp"

#include "archetype.hpp"
#include "world.hpp"

namespace realm {

/**
 * @brief View
 * Describes a chunk view for iteration & manipulation of a single archetype chunk.
 * @tparam Ts Component types to match
 */
template<typename... Ts>
struct view
{
public:
    typedef std::tuple<Ts...> values;
    typedef std::tuple<std::add_lvalue_reference_t<Ts>...> references;
    typedef internal::component_tuple<Ts...> components;

    /**
     * Creates a view of a chunk.
     * Beware, there is currently no assurance that the chunk contains
     * the components defined by the view.
     * @param chunk
     */
    inline constexpr view(archetype_chunk* chunk) : chunk{ chunk } {}

    /**
     * Creates a view of a chunk with a world.
     * If a component is a singleton in the provided world
     * the view will fetch that component instead
     * of from the chunk.
     * @param chunk
     */
    inline constexpr view(archetype_chunk* chunk, world* world_ptr)
      : chunk{ chunk }, world_ptr{ world_ptr }
    {}

    static inline const size_t mask = archetype::mask_from_tuple<components>();

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

        inline auto operator*()
        {
            return (std::forward_as_tuple(
              chunk_view->template get<internal::pure_t<Ts>>(index)...));
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

    /**
     * Get a component from the chunk for a specific entity
     * If the provided component is registered as a singleton, fetch it from world instead.
     * @tparam T Component type
     * @param entt  Entity id
     * @return A component reference of type T
     */
    template<typename T>
    inline internal::enable_if_component<T, T&> get(uint32_t index) const
    {

        if (world_ptr != nullptr && world_ptr->is_singleton<T>()) {
            // TODO: this call increases system update by a lot
            // definitely something to look into for performance
            return get_singleton<T>();
        } else {
            return *chunk->get<T>(index);
        }
    }

    template<typename T>
    inline internal::enable_if_entity<T, const entity&> get(uint32_t index) const
    {
        return *chunk->get_entity_at(index);
    }

    template<typename T>
    inline internal::enable_if_component<T, T&> get_singleton() const
    {
        return world_ptr->get_singleton<T>();
    }

private:
    archetype_chunk* chunk{ nullptr };
    world* world_ptr{ nullptr };
};
} // namespace realm