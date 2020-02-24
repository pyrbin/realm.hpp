#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

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
    constexpr view(archetype_chunk* chunk) : chunk{ chunk } {}

    /**
     * Creates a view of a chunk with a world.
     * If a component is a singleton in the provided world
     * the view will fetch that component instead
     * of from the chunk.
     * @param chunk
     * @param world_ptr
     */
    constexpr view(archetype_chunk* chunk, world* world_ptr)
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

        constexpr iterator(view* chunk_view) : _chunk_view{ chunk_view }, _index{ 0 }
        {}

        constexpr bool valid() { return _chunk_view->chunk != nullptr; }

        void step()
        {
            _index++;
            if (_index >= _chunk_view->chunk->size()) {
                _chunk_view = nullptr;
                return;
            }
        }

        self_type operator++()
        {
            self_type i = *this;
            step();
            return i;
        }

        self_type operator++(int junk)
        {
            step();
            return *this;
        }

        auto operator*()
        {
            return (std::forward_as_tuple(
              _chunk_view->template get<internal::pure_t<Ts>>(_index)...));
        }

        constexpr bool operator!=(const self_type& other)
        {
            return _chunk_view != other._chunk_view;
        }

    private:
        view* _chunk_view;
        unsigned int _index;
    };

    iterator begin() { return iterator(this); }

    iterator end() { return iterator(nullptr); }

    /**
     * Get a component from the chunk for a specific entity
     * If the provided component is registered as a singleton, fetch it from world instead.
     * @tparam T Component type
     * @param index
     * @return A component reference of type T
     */
    template<typename T>
    [[nodiscard]] internal::enable_if_component<T, T&> get(const uint32_t index) const
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
    [[nodiscard]] internal::enable_if_entity<T, const entity&> get(
      const uint32_t index) const
    {
        return *chunk->get_entity_at(index);
    }

    template<typename T>
    [[nodiscard]] internal::enable_if_component<T, T&> get_singleton() const
    {
        return world_ptr->get_singleton<T>();
    }

private:
    archetype_chunk* chunk{ nullptr };
    world* world_ptr{ nullptr };
};
} // namespace realm