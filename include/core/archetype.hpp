#pragma once

#include <algorithm>
#include <cstring>
#include <memory>
#include <numeric>
#include <tuple>
#include <type_traits>

#include "../detail/swap_remove.hpp"

#include "component.hpp"
#include "concepts.hpp"
#include "entity.hpp"

namespace realm {

/**
 * @brief archetype represents a set of components.
 */
struct archetype
{
private:
    using components_t = std::vector<component>;

    struct data
    {
        size_t size{ 0 };
        size_t mask{ 0 };
    };

    const data info{ 0, 0 };
    const size_t components_count{ 0 };

    inline data generate_data(const components_t& comps) const
    {
        return std::accumulate(
          comps.begin(), comps.end(), (data{}), [](data&& res, auto&& c) {
              res.mask |= c.meta.mask;
              res.size += c.layout.size;
              return res;
          });
    }

public:
    const components_t components;

    archetype() {}
    archetype(const components_t& comps)
      : components{ std::move(comps) }
      , info{ generate_data(comps) }
      , components_count{ comps.size() }
    {}

    template<typename... T>
    static archetype of() noexcept requires ComponentPack<T...>
    {
        std::cout << __VALID_PRETTY_FUNC__ << "\n";
        components_t comps;
        (comps.push_back(component::of<std::remove_reference_t<T>>()), ...);
        return archetype{ comps };
    }

    constexpr bool operator==(const archetype& other) const
    {
        return other.mask() == mask();
    }

    template<Component T>
    constexpr bool has() const
    {
        return has(component::of<T>());
    }

    constexpr bool has(const component& comp) const
    {
        return (mask() & comp.meta.mask) == comp.meta.mask;
    }

    constexpr bool subset(size_t other) const { return (mask() & other) == other; }

    constexpr bool subset(const archetype& other) const { return subset(other.mask()); }

    constexpr size_t mask() const { return info.mask; }
    /**
     * Return the value of sizeof([all components...])
     * @return total size of all components (memory not count)
     */
    constexpr size_t size() const { return info.size; }
    constexpr size_t count() const { return components_count; }
};

namespace detail {
// TODO: Probly exist better method to filter non Components from a variadic template
// using concepts/SFINAE to filter?
template<BaseComponent T>
static inline void
__unpack_archetype_helper(std::vector<component>& comps)
{
    comps.push_back(component::of<std::unwrap_ref_decay_t<T>>());
}
template<typename F>
static inline void
__unpack_archetype_helper(std::vector<component>&)
{}
template<typename... T>
static inline archetype
unpack_archetype()
{
    std::vector<component> comps;
    (__unpack_archetype_helper<std::unwrap_ref_decay_t<T>>(comps), ...);
    return archetype{ comps };
}
} // namespace detail

// forward declaration
struct archetype_chunk_root
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

    archetype_chunk_root(const struct archetype& archetype)
      : archetype{ archetype.components }
      , per_chunk{ CHUNK_LAYOUT.size / (uint32_t) archetype.size() }
    {}

    ~archetype_chunk_root() { cached_free = nullptr; }

    archetype_chunk* find_free();
    archetype_chunk* create_chunk();
};

struct archetype_chunk
{
public:
    using offsets_t = std::unordered_map<size_t, size_t>;
    using entities_t = std::vector<entity>;
    using pointer = void*;

    const struct archetype archetype;
    entities_t entities;

    archetype_chunk(const struct archetype archetype, uint32_t max_capacity)
      : archetype{ archetype }, max_capacity(max_capacity)
    {
        std::cout << "new chunk created"
                  << "\n";
    }

    ~archetype_chunk()
    {
        if (data != nullptr) {
            free((void*) (data));
            data = nullptr;
            // parent = nullptr;
        }
    }

    pointer allocate(uint chunk_size, uint alignment)
    {
        entities.reserve(max_capacity);

        for (const auto& component : archetype.components) {
            /* data_size += memory_layout::align_up(
              memory_layout::align_up(data_size, CHUNK_COMPONENT_ALIGNMENT),
              component.layout.align);*/
            data_size += component.layout.align_up(data_size);
            offsets.emplace(component.meta.hash, data_size);
            data_size += component.layout.size * max_capacity;
        }

        data_size += (chunk_size % data_size);

        assert(data_size == chunk_size);

        return data = (std::aligned_alloc(alignment, data_size));
    }

    entity insert(entity entt)
    {
        for (const auto& component : archetype.components) {
            component.invoke(get_pointer(len, component));
        }
        entities[len++] = entt;
        return entt;
    }

    entity remove(uint index)
    {
        // do de-fragmentation, (is this costly?)
        auto end{ len - 1 };
        detail::swap_remove(index, entities);
        copy_to(end, *this, index);
        for (const auto& component : archetype.components) {
            component.invoke(get_pointer(end, component));
        }
        len--;
        return entities[index];
    }

    template<typename T>
    T* get(uint32_t index) const
    {
        return ((T*) get_pointer(index, component::of<T>()));
    }

    // allows queries to use entity-types as parameter
    template<Entity T>
    const entity* get(uint32_t index) const
    {
        return &entities[index];
    }

    pointer get_pointer(uint32_t index, const component& type) const
    {
        return (void*) ((std::byte*) data + offset_to(index, type));
    }

    void copy_to(uint from, archetype_chunk& other, uint to)
    {
        for (auto&& component : archetype.components) {
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
    // todo: move to private
    pointer data{ nullptr };

private:
    size_t offset_to(uint index, const component& type) const
    {
        auto offset = offsets.at(type.meta.hash);
        return offset + (index * type.layout.size);
    }

    uint32_t len{ 0 };
    uint32_t max_capacity{ 0 };
    uint32_t data_size{ 0 };
    offsets_t offsets;
};

archetype_chunk*
archetype_chunk_root::find_free()
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
archetype_chunk_root::create_chunk()
{
    chunks.push_back(std::move(std::make_unique<archetype_chunk>(archetype, per_chunk)));
    return chunks.back().get();
}
} // namespace realm
