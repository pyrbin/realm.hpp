#pragma once

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <memory>
#include <numeric>
#include <stdlib.h>
#include <tuple>
#include <type_traits>

#include "../external/robin_hood.hpp"
#include "../util/swap_remove.hpp"
#include "../util/type_traits.hpp"

#include "component.hpp"
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

    inline archetype(const components_t& components, const data& info)
      : components{ components }, info{ info }, components_count{ components.size() }
    {}

public:
    const components_t components;

    inline archetype() {}

    template<typename... T>
    static inline constexpr internal::enable_if_component_pack<archetype, T...> of()
    {
        return archetype({ component::of<T>()... },
                         { (sizeof(T) + ... + 0), mask_of<T...>() });
    }

    template<typename... T>
    static inline constexpr internal::enable_if_component_pack<archetype, T...>
      from_identity(std::type_identity<std::tuple<T...>>)
    {
        return archetype::of<T...>();
    }

    template<typename... T>
    static inline constexpr size_t mask_of() noexcept
    {
        size_t mask{ 0 };
        ( // Iterate each type and get component mask
          [&](component&& component) mutable {
              mask |= component.meta.mask;
          }(component::of<T>()),
          ...);
        return mask;
    }

    inline constexpr bool operator==(const archetype& other) const
    {
        return other.mask() == mask();
    }

    template<typename T>
    inline constexpr internal::enable_if_component<T, bool> has() const
    {
        return has(component::of<T>());
    }

    template<typename T>
    inline constexpr void each(T&& t) const
    {}

    inline constexpr bool has(const component& comp) const
    {
        return (mask() & comp.meta.mask) == comp.meta.mask;
    }

    inline constexpr bool subset(size_t other) const { return (mask() & other) == other; }

    inline constexpr bool subset(const archetype& other) const
    {
        return subset(other.mask());
    }

    inline constexpr size_t mask() const { return info.mask; }
    /**
     * Return the value of sizeof([all components...])
     * @return total size of all components (memory not count)
     */
    inline constexpr size_t size() const { return info.size; }
    inline constexpr size_t count() const { return components_count; }
};

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
      : archetype{ archetype }
      , per_chunk{ CHUNK_LAYOUT.size / (uint32_t) archetype.size() }
    {}

    ~archetype_chunk_root() { cached_free = nullptr; }

    archetype_chunk* find_free();
    archetype_chunk* create_chunk();
};

struct archetype_chunk
{
public:
    using offsets_t = robin_hood::unordered_flat_map<size_t, uint64_t>;
    using entities_t = std::vector<entity>;
    using pointer = void*;

    const struct archetype archetype;
    entities_t entities;

    inline archetype_chunk(const struct archetype archetype, uint32_t max_capacity)
      : archetype{ archetype }, max_capacity(max_capacity)
    {}

    ~archetype_chunk() { dealloc(); }

    inline pointer alloc(unsigned chunk_size, unsigned alignment)
    {
        entities.resize(max_capacity);
        for (const auto& component : archetype.components) {
            /* data_size += memory_layout::align_up(
              memory_layout::align_up(data_size, CHUNK_COMPONENT_ALIGNMENT),
              component.layout.align);*/
            data_size += component.layout.align_up(data_size);
            offsets.emplace(component.meta.hash, data_size);
            data_size += component.layout.size * max_capacity;
        }

        assert(data_size <= chunk_size);

#if defined(_WIN32) || defined(__CYGWIN__)
        data = (_aligned_malloc(data_size, alignment));
#else
        data = (std::aligned_alloc(alignment, data_size));
#endif

        return data;
    }

    inline void dealloc()
    {
        if (data != nullptr) {
#if defined(_WIN32) || defined(__CYGWIN__)
            _aligned_free((void*) data);
#else
            free((void*) (data));
#endif
            data = nullptr;
            // parent = nullptr;
        }
    }

    inline unsigned insert(entity entt)
    {
        for (const auto& component : archetype.components) {
            component.alloc(get_pointer(len, component));
        }
        entities[len++] = entt;
        return len - 1;
    }

    inline entity remove(unsigned index)
    {
        // do de-fragmentation, (is this costly?)
        auto end{ (len--) - 1 };
        if (len == 0) return entities[index];
        internal::swap_remove(index, entities);
        copy_to(end, this, index);
        for (const auto& component : archetype.components) {
            component.destroy(get_pointer(end, component));
        }
        return entities[index];
    }

    template<typename T>
    inline internal::enable_if_component<T, T*> get(uint32_t index) const
    {
        return ((T*) get_pointer(index, component::of<T>()));
    }

    // allows queries to use entity-types as parameter
    template<typename T>
    inline internal::enable_if_entity<T, const entity*> get(uint32_t index) const
    {
        // TODO: this feels like UB/not good
        return &entities[index];
    }

    inline pointer get_pointer(uint32_t index, const component& type) const
    {
        return (void*) ((std::byte*) data + offset_to(index, type));
    }

    inline void copy_to(unsigned from, const archetype_chunk* other, unsigned to)
    {
        for (const auto& component : archetype.components) {
            if (other->archetype.has(component)) {
                memcpy(other->get_pointer(to, component),
                       get_pointer(from, component),
                       component.layout.size);
            }
        }
    }

    // const archetype& archetype() const { return parent->archetype; }

    inline constexpr uint32_t capacity() const noexcept { return max_capacity; }
    inline constexpr uint32_t size() const noexcept { return len; }

    inline bool full() const noexcept { return len >= max_capacity; }
    inline bool allocated() const noexcept { return data != nullptr; }
    // todo: move to private
    pointer data{ nullptr };

private:
    inline size_t offset_to(unsigned index, const component& type) const
    {
        auto offset = offsets.at(type.meta.hash);
        return offset + (index * type.layout.size);
    }

    uint32_t len{ 0 };
    uint32_t max_capacity{ 0 };
    uint32_t data_size{ 0 };
    offsets_t offsets;
};

inline archetype_chunk*
archetype_chunk_root::find_free()
{

    if (cached_free && !cached_free->full()) return cached_free;

    auto it = std::find_if(
      chunks.begin(), chunks.end(), [](auto& b) { return !b->full() && b->allocated(); });

    archetype_chunk* ptr{ nullptr };

    if (it != chunks.end()) {
        ptr = (*it).get();
    } else {
        ptr = create_chunk();
        ptr->alloc(CHUNK_LAYOUT.size, CHUNK_LAYOUT.align);
    }

    cached_free = ptr;
    return ptr;
}

inline archetype_chunk*
archetype_chunk_root::create_chunk()
{
    chunks.push_back(std::move(std::make_unique<archetype_chunk>(archetype, per_chunk)));
    return chunks.back().get();
}
} // namespace realm

namespace std {

template<>
struct hash<realm::archetype>
{
    size_t operator()(const realm::archetype& at) const
    {
        return (hash<size_t>{}(at.mask()));
    }
};

template<>
struct hash<realm::archetype_chunk_root>
{
    size_t operator()(const realm::archetype_chunk_root& at) const
    {
        return (hash<realm::archetype>{}(at.archetype));
    }
};

} // namespace std
