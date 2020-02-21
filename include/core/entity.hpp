#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>
#include <vector>

#include "../util/swap_remove.hpp"

namespace realm {
/**
 * @brief Entity
 * Entities are represented as 64-bit integers split in half,
 * where respective 32-bit half represents an index & generation
 */
using entity = uint64_t;

/**
 *  @brief Entity handle
 *  Describes an entity handle
 */
struct entity_handle
{
    uint32_t index;
    uint32_t generation;
};

struct archetype_chunk;

/**
 *  @brief Entity location
 *  Describes an entity chunk location
 */
struct entity_location
{
    /*! @brief Index where in chunk the entity stored */
    uint32_t chunk_index{ 0 };
    /*! @brief Which chunk the entity is stored */
    archetype_chunk* chunk{ nullptr };
};

/**
 * @brief Entity Manager
 * A collection of entities based on a Rust slot-map/beach-map.
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
    entity_manager(uint32_t capacity) : first_available{ -1 }
    {
        slots.reserve(capacity);
        locations.reserve(capacity);
        handles.reserve(capacity);
    }

    ~entity_manager()
    {
        for (auto&& loc : locations) {
            loc.chunk_index = 0;
            loc.chunk = nullptr;
        }
    }

    /**
     * Create an entity with given location
     * @param loc entity location
     * @return The entity id for the entity
     */
    entity create(const entity_location& loc)
    {
        // TODO: add comments
        entity_handle handle;
        auto index = first_available;
        if (index != -1) {
            first_available =
              handles.at(index).index == index ? -1 : handles.at(index).index;

            handles.at(index).index = ((uint32_t) locations.size());
            handle = { (uint32_t) index, handles.at(index).generation };
        } else {
            handles.push_back({
              (uint32_t) locations.size(),
              0,
            });
            handle = { ((uint32_t) handles.size()) - 1, 0 };
        }
        slots.push_back(handle.index);
        locations.push_back(loc);
        return merge_handle(handle);
    }

    /**
     * Remove an entity.
     * @param entt The entity id to remove
     */
    void remove(entity entt)
    {
        // TODO: add comments
        auto handle = extract_handle(entt);
        auto [index, generation] = handles.at(handle.index);
        if (generation != handle.generation) return;
        auto loc_index = handles.at(handle.index).index;
        handles.at(slots.at(slots.size() - 1)).index = loc_index;
        handles.at(handle.index).generation++;
        handles.at(handle.index).index =
          first_available != -1 ? first_available : handle.index;
        first_available = handle.index;
        internal::swap_remove(loc_index, slots);
        internal::swap_remove(loc_index, locations);
    }

    /**
     * Get a const pointer to a entity location of an entity
     * @param entt The entity id
     * @return The entity location
     */
    const entity_location* get(entity entt)
    {
        auto handle = extract_handle(entt);
        auto [index, generation] = handles.at(handle.index);
        return generation == handle.generation
                 ? &locations.at(handles.at(handle.index).index)
                 : nullptr;
    }

    /**
     * Get a mutable pointer to a entity location of an entity
     * @param entt The entity id
     * @return The entity location
     */
    entity_location* get_mut(entity entt)
    {
        auto handle = extract_handle(entt);
        auto [index, generation] = handles.at(handle.index);
        return generation == handle.generation
                 ? &locations.at(handles.at(handle.index).index)
                 : nullptr;
    }

    /**
     * Update an entities location
     * @param entt The entity id
     * @param loc The new entity location
     */
    void update(entity entt, entity_location&& loc)
    {
        get_mut(entt)->chunk = std::move(loc.chunk);
        get_mut(entt)->chunk_index = std::move(loc.chunk_index);
    }

    /**
     * Check if an entity id exists in the pool/is valid
     * @param entt The entity id
     * @return
     */
    bool exists(entity entt) const noexcept
    {
        auto handle = extract_handle(entt);
        return handle.index <= handles.size()
                 ? handle.generation == handles.at(handle.index).generation
                 : false;
    }

    /**
     * Iterate each entity immutable
     * @param fn
     */
    void each(std::function<void(const entity, const entity_location*)> fn)
    {
        for (unsigned i{ 0 }; i < handles.size(); i++) {
            auto id = merge_handle(handles[i]);
            fn(id, const_cast<const entity_location*>(get(id)));
        }
    }

    /**
     * Iterate each entity mutable
     * @param fn
     */
    void each_mut(std::function<void(const entity, entity_location*)> fn)
    {
        for (unsigned i{ 0 }; i < handles.size(); i++) {
            auto id = merge_handle(handles[i]);
            fn(id, (get_mut(id)));
        }
    }

    int32_t size() const noexcept { return slots.size(); }

    int32_t capacity() const noexcept { return slots.capacity(); }

    /**
     * Create an entity id from an index and generation
     * @param index
     * @param generation
     * @return An entity id
     */
    static inline constexpr entity merge_handle(uint32_t index,
                                                uint32_t generation) noexcept
    {
        return entity{ generation } << 32 | entity{ index };
    }

    /**
     * Create an entity id from an entity handle
     * @param handle
     * @return
     */
    static inline constexpr entity merge_handle(entity_handle handle) noexcept
    {
        auto [index, generation] = handle;
        return merge_handle(index, generation);
    }

    /**
     * Get the index of an entity id
     * @param entt
     * @return
     */
    static inline constexpr uint32_t index(entity entt) noexcept
    {
        return static_cast<uint32_t>(entt);
    }

    /**
     * Get the generation of an entity id
     * @param entt
     * @return
     */
    static inline constexpr uint32_t generation(entity entt) noexcept
    {
        return (entt >> 32);
    }

    /**
     * Create an entity handle from ab entity id
     * @param entt
     * @return
     */
    static inline constexpr entity_handle extract_handle(entity entt) noexcept
    {
        return { index(entt), generation(entt) };
    }

private:
    std::vector<entity_location> locations;
    std::vector<entity_handle> handles;
    std::vector<uint32_t> slots;
    int first_available;
};
} // namespace realm