#pragma once

#include "component.hpp"
#include "concept.hpp"
#include "entity.hpp"
#include "util.hpp"

#include "assert.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <string.h>
#include <type_traits>
#include <unordered_map>

namespace realm {

/**
 * @brief archetype represents a set of components.
 *
 */
struct archetype
{
private:
    using components_t = std::vector<component>;
    size_t combined_mask{ 0 };
    size_t data_size{ 0 };

public:
    components_t components;

    archetype() {}
    archetype& operator=(const archetype& other)
    {
        combined_mask = other.combined_mask;
        data_size = other.data_size;
        for (auto component : other.components) { components.push_back(component); }
        return *this;
    }

    template<typename... T>
    static archetype of() noexcept requires UniquePack<T...>
    {
        archetype archetype{};
        (archetype.add(component::of<T>()), ...);
        return archetype;
    }

    constexpr bool operator==(const archetype& other) const
    {
        return other.mask() == mask();
    }

    void add(const component& comp)
    {
        components.push_back(comp);
        combined_mask |= comp.meta.mask;
        data_size += comp.layout.size;
    }

    template<typename... T>
    void add() requires UniquePack<T...>
    {
        (add(component::of<T>()), ...);
    }

    void remove(const component& comp)
    {
        components.erase(std::find(components.cbegin(), components.cend(), comp));
        combined_mask &= ~comp.meta.mask;
        data_size -= comp.layout.size;
    }

    template<typename... T>
    void remove() requires UniquePack<T...>
    {
        (remove(component::of<T>()), ...);
    }

    bool has(const component& comp)
    {
        return (combined_mask & comp.meta.mask) == comp.meta.mask;
    }

    template<typename T>
    void has()
    {
        return has(component::of<T>());
    }

    bool subset(const archetype& other)
    {
        return (combined_mask & other.mask()) == other.mask();
    }

    bool subset(size_t other) { return (combined_mask & other) == other; }

    constexpr size_t mask() const { return combined_mask; }
    constexpr size_t size() const { return data_size; }
};

// forward declaration
struct archetype_chunk_parent
{
    using chunk_ptr = std::unique_ptr<archetype_chunk>;

    static const uint32_t CHUNK_SIZE_16KB{ 16 * 1024 };
    static const uint32_t CHUNK_COMPONENT_ALIGNMENT{ 64 };

    static constexpr const memory_layout CHUNK_LAYOUT{ CHUNK_SIZE_16KB,
                                                       CHUNK_COMPONENT_ALIGNMENT };

    std::vector<chunk_ptr> chunks;
    archetype_chunk* cached_free{ nullptr };

    const struct archetype archetype;
    const uint32_t per_chunk;

    archetype_chunk_parent(const struct archetype& archetype)
      : archetype{ archetype }
      , per_chunk{ CHUNK_LAYOUT.size / (uint32_t) archetype.size() }
    {}

    ~archetype_chunk_parent() { cached_free = nullptr; }

    archetype_chunk* find_free();
    archetype_chunk* create_chunk();
};

struct archetype_chunk
{
public:
    using offsets_t = std::unordered_map<size_t, size_t>;
    using entities_t = std::vector<entity>;
    using pointer = void*;

    const archetype_chunk_parent* parent;

    archetype_chunk(archetype_chunk_parent* parent, uint32_t max_capacity)
      : parent{ parent }, max_capacity(max_capacity)
    {}

    ~archetype_chunk()
    {
        parent = nullptr;
        if (data != nullptr) {
            free((void*) (data));
            data = nullptr;
            // parent = nullptr;
        }
    }

    const struct archetype* archetype() { return &parent->archetype; }

    pointer allocate(uint chunk_size, uint alignment)
    {
        entities.reserve(max_capacity);

        for (const auto& component : archetype()->components) {
            /* data_size += memory_layout::align_up(
              memory_layout::align_up(data_size, CHUNK_COMPONENT_ALIGNMENT),
              component.layout.align);*/
            data_size += component.layout.align_up(data_size);
            offsets.emplace(component.meta.hash, data_size);
            data_size += component.layout.size * max_capacity;
        }

        data_size += (chunk_size % data_size);

        assert(data_size == chunk_size);

        return data = (aligned_alloc(alignment, data_size));
    }

    entity insert(entity entt)
    {
        for (const auto& component : archetype()->components) {
            component.invoke(get_pointer(len, component));
        }
        entities[len++] = entt;
        return entt;
    }

    entity remove(uint index)
    {
        // do de-fragmentation, (is this costly?)
        auto end{ len - 1 };
        util::swap_remove(index, entities);
        copy_to(end, *this, index);
        for (const auto& component : archetype()->components) {
            component.invoke(get_pointer(end, component));
        }
        len--;
        return entities[index];
    }

    template<typename T>
    T* get(unsigned int index)
    {
        auto comp = component::of<T>();
        return ((T*) get_pointer(index, comp));
    }

    // allows queries to use entity-types as parameter
    template<Entity T>
    entity* get(unsigned int index)
    {
        return &entities[index];
    }

    pointer get_pointer(unsigned index, const component& type)
    {
        return (void*) ((std::byte*) data + offset_to(index, type));
    }

    void copy_to(uint from, archetype_chunk& other, uint to)
    {
        for (auto&& component : archetype()->components) {
            memcpy(other.get_pointer(to, component),
                   get_pointer(from, component),
                   component.layout.size);
        }
    }

    // const archetype& archetype() const { return parent->archetype; }

    constexpr uint32_t capacity() const noexcept { return max_capacity; }
    constexpr uint32_t size() const noexcept { return len; }

    bool full() const noexcept { return len >= max_capacity; }
    bool used() const noexcept { return data != nullptr; }

private:
    size_t offset_to(uint index, const component& type) const
    {
        auto offset = offsets.at(type.meta.hash);
        return offset + (index * type.layout.size);
    }

    pointer data{ nullptr };

    uint32_t len{ 0 };
    uint32_t max_capacity{ 0 };
    uint32_t data_size{ 0 };

    entities_t entities;
    offsets_t offsets;
};

archetype_chunk*
archetype_chunk_parent::find_free()
{

    if (cached_free && !cached_free->full()) return cached_free;

    auto it = std::find_if(
      chunks.begin(), chunks.end(), [](auto& b) { return !b->full() && b->used(); });

    archetype_chunk* ptr{ nullptr };

    if (it != chunks.end()) {
        ptr = (*it).get();
    } else {
        ptr = create_chunk();
        ptr->allocate(CHUNK_LAYOUT.size, CHUNK_LAYOUT.align);
    }

    cached_free = ptr;
    return ptr;
}

archetype_chunk*
archetype_chunk_parent::create_chunk()
{
    chunks.push_back(std::move(std::make_unique<archetype_chunk>(this, per_chunk)));
    return chunks.back().get();
}
} // namespace realm
