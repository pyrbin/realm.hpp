#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

#include <realm/core/archetype.hpp>
#include <realm/core/world.hpp>
#include <realm/util/tuple_util.hpp>
#include <realm/util/type_traits.hpp>

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
    using values = std::tuple<Ts...>;
    using references = std::tuple<std::add_lvalue_reference_t<Ts>...>;
    using components = internal::component_tuple<Ts...>;

    using ptr = view*;

    /**
     * Creates a view of a chunk.
     * Beware, there is currently no assurance that the chunk contains
     * the components defined by the view.
     * @param chunk
     */
    explicit constexpr view(archetype_chunk::ptr chunk)
        : chunk_{ chunk }
    {}

    /**
     * Creates a view of a chunk with a world.
     * If a component is a singleton in the provided world
     * the view will fetch that component instead
     * of from the chunk.
     * @param chunk
     * @param world_ptr
     */
    constexpr view(archetype_chunk::ptr chunk, world::ptr world_)
        : chunk_{ chunk }
        , world_{ world_ }
    {}

    static inline const size_t mask = archetype::mask_from_tuple<components>();

    class iterator
    {
    public:
        using self_type = iterator;
        using value_type = values;
        using reference = references;
        using iterator_category = std::forward_iterator_tag;

        explicit constexpr iterator(view::ptr view)
            : view_{ view }
            , index_{ 0 }
        {}

        constexpr bool valid() { return _chunk_view->_chunk != nullptr; }

        void step()
        {
            index_++;
            if (index_ >= view_->chunk_->size()) {
                view_ = nullptr;
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
            return (std::forward_as_tuple(view_->template get<internal::pure_t<Ts>>(index_)...));
        }

        constexpr bool operator!=(const self_type& other) { return view_ != other.view_; }

    private:
        view::ptr view_;
        unsigned int index_;
    };

    iterator begin() { return iterator(this); }

    iterator end() { return iterator(nullptr); }

    /**
     * Get a component from the chunk for a specific entity
     * If the provided component is registered as a singleton, fetch it from world
     * instead.
     * @tparam T Component type
     * @param index
     * @return A component reference of type T
     */
    template<typename T>
    [[nodiscard]] internal::enable_if_component<T, T&> get(const u32 index) const
    {
        if (world_ != nullptr && world_->is_singleton<T>()) {
            // TODO: this call increases system update by a lot
            // definitely something to look into for performance
            return get_singleton<T>();
        }

        return *chunk_->get<T>(index);
    }

    template<typename T>
    [[nodiscard]] internal::enable_if_entity<T, const entity&> get(const u32 index) const
    {
        return *chunk_->get_entity_at(index);
    }

    template<typename T>
    [[nodiscard]] internal::enable_if_component<T, T&> get_singleton() const
    {
        return world_->get_singleton<T>();
    }

private:
    archetype_chunk* chunk_{ nullptr };
    world::ptr world_{ nullptr };
};
} // namespace realm