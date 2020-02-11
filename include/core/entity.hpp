#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>
#include <vector>

#include "../internal/swap_remove.hpp"

namespace realm {

/**
 * @brief Entities are represented as 64-bit integers split in half,
 * where each 32-bit half represents an index & generation
 */
using entity = uint64_t;

struct entity_handle
{
    uint32_t index;
    uint32_t generation;
};


struct archetype_chunk;

namespace internal {


struct entity_location
{
    uint32_t chunk_index{ 0 };
    archetype_chunk* chunk{ nullptr };
};

/**
 * @brief entity_pool, based on a Rust slot-map/beach-map.
 * @ref https://docs.rs/beach_map/
 *
 */
class entity_pool
{
public:
    entity_pool(uint32_t capacity) : first_available{ -1 }
    {
        slots.reserve(capacity);
        locations.reserve(capacity);
        handles.reserve(capacity);
    }

    ~entity_pool()
    {
        for (auto&& loc : locations) {
            loc.chunk_index = 0;
            loc.chunk = nullptr;
        }
    }

    entity create(const entity_location& loc)
    {
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

    void free(entity entt)
    {
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

    const entity_location* get(entity entt)
    {
        auto handle = extract_handle(entt);
        auto [index, generation] = handles.at(handle.index);
        return generation == handle.generation
                 ? &locations.at(handles.at(handle.index).index)
                 : nullptr;
    }
    
    entity_location* get_mut(entity entt)
    {
        auto handle = extract_handle(entt);
        auto [index, generation] = handles.at(handle.index);
        return generation == handle.generation
                 ? &locations.at(handles.at(handle.index).index)
                 : nullptr;
    }

    void update(entity entt, entity_location&& loc)
    {
        get_mut(entt)->chunk = std::move(loc.chunk);
        get_mut(entt)->chunk_index = std::move(loc.chunk_index);
    }

    bool exists(entity entt) const noexcept
    {
        auto handle = extract_handle(entt);
        return handle.index <= handles.size()
                 ? handle.generation == handles.at(handle.index).generation
                 : false;
    }

    void each(std::function<void(const entity, const entity_location*)> fn)
    {
        for (unsigned i{ 0 }; i < handles.size(); i++) {
            auto id = merge_handle(handles[i]);
            fn(id, const_cast<const entity_location*>(get(id)));
        }
    }

    void each_mut(std::function<void(const entity, entity_location*)> fn)
    {
        for (unsigned i{ 0 }; i < handles.size(); i++) {
            auto id = merge_handle(handles[i]);
            fn(id, (get_mut(id)));
        }
    }

    int32_t size() const noexcept { return handles.size(); }
    int32_t capacity() const noexcept { return handles.capacity(); }

    static inline constexpr entity merge_handle(uint32_t index,
                                                uint32_t generation) noexcept
    {
        return entity{ generation } << 32 | entity{ index };
    }

    static inline constexpr entity merge_handle(entity_handle handle) noexcept
    {
        auto [index, generation] = handle;
        return merge_handle(index, generation);
    }

    static inline constexpr uint32_t index(entity entt) noexcept
    {
        return static_cast<uint32_t>(entt);
    }

    static inline constexpr uint32_t generation(entity entt) noexcept
    {
        return (entt >> 32);
    }

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

} // namespace internal

} // namespace realm
