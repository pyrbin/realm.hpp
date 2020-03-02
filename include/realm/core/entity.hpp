#pragma once

#include <functional>
#include <vector>

#include <realm/core/types.hpp>
#include <realm/util/swap_remove.hpp>

namespace realm {
/**
 * @brief Entity
 * Entities are represented as 64-bit integers split in half,
 * where respective 32-bit half represents an index & generation
 */
using entity = u64;

/**
 *  @brief Entity handle
 *  Describes an entity handle
 */
struct entity_handle
{
    using ptr = entity_handle*;
    using ref = const entity_handle&;
    using list = std::vector<entity_handle>;

    u32 index;
    u32 generation;
};

struct archetype_chunk;

/**
 *  @brief Entity location
 *  Describes an entity chunk location
 */
struct entity_location
{
    using ptr = entity_location*;
    using ref = const entity_location&;
    using list = std::vector<entity_location>;

    /*! @brief Index where in chunk the entity stored */
    u32 chunk_index{ 0 };
    /*! @brief Which chunk the entity is stored */
    archetype_chunk* chunk{ nullptr };
};

/**
 * @brief Entity Manager
 * A collection of entities_ based on a Rust slot-map/beach-map.
 * Uses indirection to guarantee a dense/packed storage of entities.
 * @ref https://docs.rs/beach_map/
 */
class entity_manager
{
public:
    /**
     * Create an entity pool with specified capacity
     * @param capacity
     */
    explicit entity_manager(const u32 capacity)
        : first_available_{ -1 }
    {
        slots_.reserve(capacity);
        locations_.reserve(capacity);
        handles_.reserve(capacity);
    }

    ~entity_manager()
    {
        for (auto&& loc : locations_) {
            loc.chunk_index = 0;
            loc.chunk = nullptr;
        }
    }

    /**
     * Create an entity with given location
     * @param loc entity location
     * @return The entity id for the entity
     */
    entity create(entity_location::ref location)
    {
        // TODO: add comments
        entity_handle handle{};
        const auto index = first_available_;
        if (index != -1) {
            first_available_ = handles_.at(index).index == index ? -1 : handles_.at(index).index;

            handles_.at(index).index = u32(locations_.size());
            handle = { u32(index), handles_.at(index).generation };
        } else {
            handles_.push_back({
                static_cast<u32>(locations_.size()),
                0,
            });
            handle = { u32(handles_.size()) - 1, 0 };
        }
        slots_.push_back(handle.index);
        locations_.push_back(location);
        return merge_handle(handle);
    }

    /**
     * Remove an entity.
     * @param entt The entity id to remove
     */
    void remove(const entity entt)
    {
        // TODO: add comments
        const auto handle = extract_handle(entt);
        auto [index, generation] = handles_.at(handle.index);
        if (generation != handle.generation)
            return;
        const auto loc_index = handles_.at(handle.index).index;
        handles_.at(slots_.at(slots_.size() - 1)).index = loc_index;
        handles_.at(handle.index).generation++;
        handles_.at(handle.index).index = first_available_ != -1 ? first_available_ : handle.index;
        first_available_ = handle.index;
        internal::swap_remove(loc_index, slots_);
        internal::swap_remove(loc_index, locations_);
    }

    /**
     * Get a const pointer to a entity location of an entity
     * @param entt The entity id
     * @return The entity location
     */
    const entity_location::ptr get(const entity entt) const
    {
        const auto handle = extract_handle(entt);
        auto [index, generation] = handles_.at(handle.index);
        return generation == handle.generation ?
            (entity_location::ptr)&locations_.at(handles_.at(handle.index).index) :
            nullptr;
    }

    /**
     * Get a mutable pointer to a entity location of an entity
     * @param entt The entity id
     * @return The entity location
     */
    entity_location::ptr get_mut(const entity entt)
    {
        const auto handle = extract_handle(entt);
        auto [index, generation] = handles_.at(handle.index);
        return generation == handle.generation ? &locations_.at(handles_.at(handle.index).index) :
                                                 nullptr;
    }

    /**
     * Update an entities location
     * @param entt The entity id
     * @param loc The new entity location
     */
    void update(const entity entt, entity_location&& loc)
    {
        get_mut(entt)->chunk = loc.chunk;
        get_mut(entt)->chunk_index = loc.chunk_index;
    }

    /**
     * Check if an entity id exists in the pool/is valid
     * @param entt The entity id
     * @return
     */
    [[nodiscard]] bool exists(const entity entt) const noexcept
    {
        const auto handle = extract_handle(entt);
        return handle.index <= handles_.size() ?
            handle.generation == handles_.at(handle.index).generation :
            false;
    }

    [[nodiscard]] int32_t size() const noexcept { return slots_.size(); }

    [[nodiscard]] int32_t capacity() const noexcept { return slots_.capacity(); }

    /**
     * Create an entity id from an index and generation
     * @param index
     * @param generation
     * @return An entity id
     */
    static constexpr entity merge_handle(const u32 index, const u32 generation) noexcept
    {
        return entity{ generation } << 32 | entity{ index };
    }

    /**
     * Create an entity id from an entity handle
     * @param handle
     * @return
     */
    static constexpr entity merge_handle(entity_handle::ref handle) noexcept
    {
        auto [index, generation] = handle;
        return merge_handle(index, generation);
    }

    /**
     * Get the index of an entity id
     * @param entt
     * @return
     */
    static constexpr u32 index(entity entt) noexcept { return static_cast<u32>(entt); }

    /**
     * Get the generation of an entity id
     * @param entt
     * @return
     */
    static constexpr u32 generation(entity entt) noexcept { return (entt >> 32); }

    /**
     * Create an entity handle from ab entity id
     * @param entt
     * @return
     */
    static constexpr entity_handle extract_handle(entity entt) noexcept
    {
        return { index(entt), generation(entt) };
    }

private:
    entity_location::list locations_;
    entity_handle::list handles_;
    std::vector<u32> slots_;
    i32 first_available_;
};
} // namespace realm