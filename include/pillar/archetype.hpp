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

namespace pillar {

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
    const archetype archetype;

    archetype_chunk(size_t size, const struct archetype& archetype)
      : entt_size{ size }, archetype(archetype)
    {
        entities.reserve(size);
        for (auto&& [hash, component] : archetype.components) {
            data_size += component.layout.padding_needed_for(data_size);
            offsets.emplace(component.meta.hash, data_size);
            data_size += component.layout.size * entt_size;
        }
        data =
          (aligned_alloc(archetype.components.begin()->second.layout.align, data_size));
    }

    ~archetype_chunk()
    {
        if (data != nullptr) {
            free((void*) (data));
            data = nullptr;
        }
    }

    entity_t acquire(std::function<entity_t(uint)> fn)
    {
        auto entt = fn(entt_count);
        entities[entt_count++] = entt;
        return entt;
    }

    entity_t remove(uint index)
    {
        // do de-fragmentation, (is this costly?)
        auto end{ entt_count - 1 };
        util::swap_remove(index, entities);
        copy_to(end, *this, index);
        entt_count--;
        return entities[index];
    }

    template<typename T>
    T* get(uint index)
    {
        auto comp = component::of<T>();
        return ((T*) get_pointer(index, comp));
    }

    void* get_pointer(unsigned index, const component& type)
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

    size_t size() const { return entt_size; }
    size_t count() const { return entt_count; }

private:
    constexpr size_t offset_to(uint index, const component& type) const
    {
        auto offset = offsets.at(type.meta.hash);
        return offset + (index * type.layout.size);
    }

    void* data{ nullptr };
    std::vector<entity_t> entities{};
    size_t entt_count{ 0 };
    size_t entt_size{ 0 };
    size_t data_size{ 0 };
    offsets_t offsets;
};

} // namespace pillar
