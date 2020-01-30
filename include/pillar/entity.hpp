#pragma once

#include "util.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>
#include <vector>

namespace pillar {

/**
 * @brief Entities are represented as 64-bit integers split in half,
 * where each 32-bit half represents an index & generation
 */
using entity_t = uint64_t;
using entity_id = entity_t;

struct entity_handle
{
    uint32_t index;
    uint32_t generation;
};

struct archetype_chunk;
using chunk_ptr = archetype_chunk*;

struct entity_location
{
    uint32_t chunk_index{ 0 };
    chunk_ptr chunk{ nullptr };
};

/**
 * @brief entity_pool, based on a Rust slot-map/beach-map.
 * @ref https://docs.rs/beach_map/
 *
 */
class entity_pool
{
public:
    entity_pool(uint32_t capacity)
      : locations{ capacity }
      , handles{ capacity }
      , slots{ capacity }
      , first_available{ -1 }
    {}
    ~entity_pool()
    {
        for (auto&& loc : locations) {
            if (loc.chunk != nullptr) { loc.chunk = nullptr; }
        }
    }
    entity_t create(entity_location loc)
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

    void update(entity_t entt, const entity_location& loc)
    {
        auto to_update = get(entt);
        to_update->chunk = loc.chunk;
        to_update->chunk_index = loc.chunk_index;
    }

    void free(entity_t entt)
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
        util::swap_remove(loc_index, slots);
        util::swap_remove(loc_index, locations);
    }

    entity_location* get(entity_t entt)
    {
        auto handle = extract_handle(entt);
        auto [index, generation] = handles.at(handle.index);
        return generation == handle.generation
                 ? &locations.at(handles.at(handle.index).index)
                 : nullptr;
    }

    bool exists(entity_t entt) const noexcept
    {
        auto handle = extract_handle(entt);
        return handle.index <= handles.size()
                 ? handle.generation == handles.at(handle.index).generation
                 : false;
    }

    size_t size() const { return handles.size(); }

    void each(std::function<void(const entity_t, const entity_location*)> fn)
    {
        for (unsigned i{ 0 }; i < handles.size(); i++) {
            auto id = merge_handle(handles[i]);
            fn(id, const_cast<const entity_location*>(get(id)));
        }
    }

    void each_mut(std::function<void(const entity_t, entity_location*)> fn)
    {
        for (unsigned i{ 0 }; i < handles.size(); i++) {
            auto id = merge_handle(handles[i]);
            fn(id, (get(id)));
        }
    }

    static inline constexpr entity_t merge_handle(uint32_t index,
                                                  uint32_t generation) noexcept
    {
        return entity_t{ generation } << 32 | entity_t{ index };
    }

    static inline constexpr entity_t merge_handle(entity_handle handle) noexcept
    {
        auto [index, generation] = handle;
        return merge_handle(index, generation);
    }

    static inline constexpr uint32_t index(entity_t entt) noexcept
    {
        return static_cast<uint32_t>(entt);
    }

    static inline constexpr uint32_t generation(entity_t entt) noexcept
    {
        return (entt >> 32);
    }

    static inline constexpr entity_handle extract_handle(entity_t entt) noexcept
    {
        return { index(entt), generation(entt) };
    }

private:
    std::vector<entity_location> locations;
    std::vector<entity_handle> handles;
    std::vector<uint32_t> slots;
    int64_t first_available;
};

} // namespace pillar