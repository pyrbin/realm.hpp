#pragma once

#include "component.hpp"
#include "entity.hpp"
#include "util.hpp"

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
    using components_t = std::unordered_map<size_t, component>;
    size_t combined_mask{ 0 };
    size_t data_size{ 0 };

public:
    components_t components;

    archetype() {}

    template<typename... T>
    static archetype of() noexcept
    {
        archetype archetype{};
        (archetype.add(component::of<T>()), ...);
        return archetype;
    }

    void add(const component& comp)
    {
        components.emplace(comp.meta.hash, comp);
        combined_mask |= comp.meta.mask;
        data_size += comp.layout.size;
    }

    template<typename... T>
    void add()
    {
        (add(component::of<T>()), ...);
    }

    void remove(const component& comp)
    {
        components.erase(comp.meta.hash);
        combined_mask &= ~comp.meta.mask;
        data_size -= comp.layout.size;
    }

    template<typename... T>
    void remove()
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

    size_t mask() const { return combined_mask; }
    size_t size() const { return data_size; }
};

struct archetype_chunk
{
public:
    using offsets_t = std::unordered_map<size_t, size_t>;
    using entities_t = std::vector<entity_t>;
    using pointer = void*;

    const archetype archetype;

    archetype_chunk(const struct archetype& archetype) : archetype(archetype) {}

    ~archetype_chunk()
    {
        if (data != nullptr) {
            free((void*) (data));
            data = nullptr;
        }
    }

    pointer allocate(size_t size)
    {
        max_entities = size;
        entities.reserve(size);
        for (auto&& [hash, component] : archetype.components) {
            data_size += component.layout.padding_needed_for(data_size);
            offsets.emplace(component.meta.hash, data_size);
            data_size += component.layout.size * max_entities;
        }
        return data = (aligned_alloc(archetype.components.begin()->second.layout.align,
                                     data_size));
    }

    entity_t acquire(std::function<entity_t(uint)> fn)
    {
        auto entt = fn(len);
        entities[len++] = entt;
        return entt;
    }

    entity_t remove(uint index)
    {
        // do de-fragmentation, (is this costly?)
        auto end{ len - 1 };
        util::swap_remove(index, entities);
        copy_to(end, *this, index);
        len--;
        return entities[index];
    }

    template<typename T>
    T* get(uint index)
    {
        auto comp = component::of<T>();
        return ((T*) get_pointer(index, comp));
    }

    pointer get_pointer(unsigned index, const component& type)
    {
        return (void*) ((std::byte*) data + offset_to(index, type));
    }

    void copy_to(uint from, archetype_chunk& other, uint to)
    {
        for (auto&& [hash, component] : archetype.components) {
            memcpy(other.get_pointer(to, component),
                   get_pointer(from, component),
                   component.layout.size);
        }
    }

    size_t capacity() const noexcept { return max_entities; }
    size_t size() const noexcept { return len; }

    bool full() const noexcept { return len >= max_entities; }
    bool used() const noexcept { return data != nullptr; }

private:
    constexpr size_t offset_to(uint index, const component& type) const
    {
        auto offset = offsets.at(type.meta.hash);
        return offset + (index * type.layout.size);
    }

    pointer data{ nullptr };

    size_t len{ 0 };
    size_t max_entities{ 0 };
    size_t data_size{ 0 };

    entities_t entities;
    offsets_t offsets;
};

} // namespace realm
