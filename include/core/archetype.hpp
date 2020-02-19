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
#include "../util/clean_query.hpp"
#include "../util/swap_remove.hpp"
#include "../util/type_traits.hpp"

#include "component.hpp"
#include "entity.hpp"

namespace realm {

struct world;

/**
 * @brief Archetype
 * Describes the identity of a collection of components. Contains each component type in a
 * vector & combined mask/data size of components
 */
struct archetype
{
    // Allows world to use unsafe private constructor
    friend world;

    using components_t = std::vector<component>;

    /**
     * @brief Archetype data
     * Combined data size & mask of the components
     */
    struct data
    {
        size_t size{ 0 };
        size_t mask{ 0 };

        data& operator+=(const data& other)
        {
            size += other.size;
            mask |= other.mask;
            return *this;
        }

        data& operator-=(const data& other)
        {
            size -= other.size;
            mask &= ~other.mask;
            return *this;
        }

        /**
         * Create archetype data of combined component types
         * @tparam T Component type
         * @return
         */
        template<typename... T>
        inline static constexpr internal::enable_if_component_pack<data, T...> of()
        {
            return { (sizeof(T) + ... + 0), mask_of<T...>() };
        }
    };

private:
    const data info{ 0, 0 };
    const size_t component_count{ 0 };

    /**
     * Create an archetype with specified components & data.
     * @warning if this constructor is used there is no guarantee that the archetype data
     * is valid for the components.
     * @param components
     * @param info
     */
    inline archetype(const components_t& components, const data& info)
      : components{ components }, info{ info }, component_count{ components.size() }
    {}

public:
    const components_t components;

    inline archetype() {}

    /**
     * Create an archetype of a packed parameter of component types
     * @tparam T Component types
     * @return An archetype of types T
     */
    template<typename... T>
    static inline constexpr internal::enable_if_component_pack<archetype, T...> of()
    {
        return archetype{ { component::of<T>()... }, data::of<T...>() };
    }

    /**
     * Create an archetype from a tuple of component types
     * @tparam T Tuple of component types
     * @return An archetype of types in tuple T
     */
    template<typename T>
    static inline archetype from_tuple()
    {
        return [&]<typename... Ts>(std::type_identity<std::tuple<Ts...>>)
        {
            return archetype::of<Ts...>();
        }
        (std::type_identity<T>{});
    }

    /**
     * Create an archetype mask from a packed parameter of component types
     * @tparam T Component types
     * @return
     */
    template<typename... T>
    static inline constexpr internal::enable_if_component_pack<size_t, T...> mask_of()
    {
        size_t mask{ 0 };
        ( // Iterate each type and get component mask
          [&](component&& component) mutable {
              mask |= component.meta.mask;
          }(component::of<T>()),
          ...);
        return mask;
    }

    /**
     * Create an archetype mask from a tuple of component types
     * @tparam T Tuple of component types
     * @return
     */
    template<typename T>
    static inline constexpr size_t mask_from_tuple()
    {
        return [&]<typename... Ts>(std::type_identity<std::tuple<Ts...>>)
        {
            return archetype::mask_of<Ts...>();
        }
        (std::type_identity<T>{});
    }

    /**
     * Check if an archetype is a subset of another archetype
     * @param a Potential superset archetype
     * @param b Potential subset archetype
     * @return If archetype b is a subset of archetype
     */
    static inline constexpr bool subset(size_t a, size_t b) { return (a & b) == b; }

    /**
     * Check if an archetype mask is a subset of this archetype
     * @param other Potential subset mask
     * @return
     */
    inline constexpr bool subset(size_t other) const
    {
        return archetype::subset(mask(), other);
    }

    /**
     * Check if an archetype is a subset of this archetype
     * @param other Potential subset
     * @return
     */
    inline constexpr bool subset(const archetype& other) const
    {
        return subset(other.mask());
    }

    inline constexpr bool operator==(const archetype& other) const
    {
        return other.mask() == mask();
    }

    /**
     * Check if archetype contains specified component type
     * @tparam T Component type
     * @return
     */
    template<typename T>
    inline constexpr internal::enable_if_component<T, bool> has() const
    {
        return has(component::of<T>());
    }

    /**
     * Check if archetype contains specified component
     * @param component Component object
     * @return
     */
    inline constexpr bool has(const component& component) const
    {
        return (mask() & component.meta.mask) == component.meta.mask;
    }

    /**
     * Iterate each component of the archetype
     * @tparam F Lambda type
     * @param f Lambda object
     * @return
     */
    template<typename F>
    inline constexpr void iter(F&& f) const
    {
        for (auto& component : components) { f(component); }
    }
    /*! @brief Combined mask of all components in archetype */
    inline constexpr size_t mask() const { return info.mask; }
    /*! @brief The data size of all components in archetype */
    inline constexpr size_t size() const { return info.size; }
    /*! @brief The count of components in archetype */
    inline constexpr size_t count() const { return component_count; }
};

/**
 * @brief Archetype Chunk Root
 * Contains all the chunks of a certain archetype, currently stored as a vector with
 * unqiue pointers. Every archetype has a root which is the access point to that
 * archetypes chunks. Also contains some metadata for the chunks such as max entities per
 * chunk, memory size and alignment.
 */
struct archetype_chunk_root
{
    using chunk_ptr = std::unique_ptr<archetype_chunk>;

    static const uint32_t CHUNK_SIZE_16KB{ 16 * 1024 };
    static const uint32_t CHUNK_COMPONENT_ALIGNMENT{ 64 };

    static constexpr const memory_layout CHUNK_LAYOUT{ CHUNK_SIZE_16KB,
                                                       CHUNK_COMPONENT_ALIGNMENT };

    std::vector<chunk_ptr> chunks;

    /*! @brief Caches latest free chunk for less lookup */
    archetype_chunk* cached_free{ nullptr };

    /*! @brief Defining archetype for the root */
    const struct archetype archetype;

    /*! @brief Max entities per chunk */
    const uint32_t per_chunk;

    /**
     * Create an archetype root of specified archetype
     * @param archetype
     */
    archetype_chunk_root(const struct archetype& archetype)
      : archetype{ archetype }
      , per_chunk{ CHUNK_LAYOUT.size / (uint32_t) archetype.size() }
    {}

    ~archetype_chunk_root() { cached_free = nullptr; }

    /**
     * Find and returns a chunk suitable for allocation. If no suitable chunk is found a
     * new one is created and allocated
     * @return Archetype chunk pointer with free space
     */
    archetype_chunk* find_free();

    /**
     * Creates a new archetype chunk
     * @return
     */
    archetype_chunk* create_chunk();
};

/**
 * @brief Archetype chunk
 * A chunk of contiguous & aligned memory that contains components of inserted entities.
 * Component data is guaranteed to be packed as defragmentation is performed on
 * every remove.
 */
struct archetype_chunk
{
public:
    using offsets_t = robin_hood::unordered_flat_map<size_t, uint64_t>;
    using entities_t = std::vector<entity>;
    using pointer = void*;

    const struct archetype archetype;

    /* Entities allocated by the chunk. Index of an entity in the vector is equal
     * to its index in the chunk */
    entities_t entities;

    /**
     * Create a chunk of specified arcetype and max capacity (maximum entities)
     * @param archetype
     * @param max_capacity
     */
    inline archetype_chunk(const struct archetype archetype, uint32_t max_capacity)
      : archetype{ archetype }, max_capacity(max_capacity)
    {}

    ~archetype_chunk() { dealloc(); }

    /**
     * Allocates required memory for the chunk.
     * @param chunk_size Chunk size
     * @param alignment Chunk alignment
     * @return Pointer to allocated memory
     */
    inline pointer alloc(unsigned chunk_size, unsigned alignment)
    {
        entities.resize(max_capacity);
        // Memory alignment of components
        for (const auto& component : archetype.components) {
            data_size += component.layout.align_up(data_size);
            /* data_size = memory_layout::align_up(
              memory_layout::align_up(data_size, alignment), component.layout.align);*/
            offsets.emplace(component.meta.hash, data_size);
            data_size += component.layout.size * max_capacity;
        }
        // Assert data size is not more than chunk size
        assert(data_size <= chunk_size);

#if defined(_WIN32) || defined(__CYGWIN__)
        data = (_aligned_malloc(data_size, alignment));
#else
        data = (std::aligned_alloc(alignment, data_size));
#endif

        return data;
    }

    /**
     * Frees the chunks allocated memory
     */
    inline void dealloc()
    {
        if (data != nullptr) {
#if defined(_WIN32) || defined(__CYGWIN__)
            _aligned_free((void*) data);
#else
            free((void*) (data));
#endif
            data = nullptr;
        }
    }

    /**
     * Insert an entity into the chunk
     * @param entt Entity id to insert
     * @return The chunk index of the inserted entity
     */
    inline unsigned insert(entity entt)
    {
        // Allocate each component for the new entity
        for (const auto& component : archetype.components) {
            component.alloc(get_pointer(len, component));
        }

        entities[len++] = entt;
        return len - 1;
    }

    /**
     * Remove an entity & components at specified index in the chunk.
     * Swaps the entity at the last index with the entity to remove to hinder
     * fragmentation inside the chunk.
     * @param index Index to remove
     * @return Entity that has been swapped to removed index
     */
    inline entity remove(unsigned index)
    {
        auto end{ (len--) - 1 };
        if (len == 0) return entities[index];

        // Swaps last entity with entity to remove
        // TODO: determine if this defragmentation is too costly?
        internal::swap_remove(index, entities);
        copy_to(end, this, index);

        // Call destructor for every component that is to be removed
        for (const auto& component : archetype.components) {
            component.destroy(get_pointer(end, component));
        }

        return entities[index];
    }

    /**
     * Get a pointer to specified component of an entity at specified index.
     * @tparam T Component type
     * @param index Index in chunk
     * @return Component pointer
     */
    template<typename T>
    inline internal::enable_if_component<T, T*> get(uint32_t index) const
    {
        return ((T*) get_pointer(index, component::of<T>()));
    }

    /**
     * Get pointer to entity id at specified index. Used in queries when trying to fetch
     * entity.
     * @tparam T Entity type
     * @param index Index in chunk
     * @return Pointer to entity id
     */
    template<typename T>
    inline internal::enable_if_entity<T, const entity*> get(uint32_t index) const
    {
        return &entities[index];
    }

    /**
     * Get a void pointer for the address of a certain component in the chunk.
     * @param index Index in chunk
     * @param type Component object
     * @return
     */
    inline pointer get_pointer(uint32_t index, const component& type) const
    {
        return (void*) ((std::byte*) data + offset_to(index, type));
    }

    /**
     * Copies a component to another chunk using memcpy.
     * @param from Index in this chunk
     * @param other Other chunk
     * @param to Index in other chunk
     */
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

    /*! @brief Capacity of chunk */
    inline constexpr uint32_t capacity() const noexcept { return max_capacity; }
    /*! @brief Size (current entity count) of chunk */
    inline constexpr uint32_t size() const noexcept { return len; }

    /*! @brief Chunk is full */
    inline bool full() const noexcept { return len >= max_capacity; }
    /*! @brief Size (current entity count) of chunk */
    inline bool allocated() const noexcept { return data != nullptr; }

private:
    /**
     * Get the offset in chunk of a component type & specified index
     * @param index Index in chunk
     * @param type Component object
     * @return
     */
    inline size_t offset_to(unsigned index, const component& type) const
    {
        auto offset = offsets.at(type.meta.hash);
        return offset + (size_t)(index * type.layout.size);
    }

    /*! @brief Pointer to allocated data */
    pointer data{ nullptr };

    uint32_t len{ 0 };
    uint32_t max_capacity{ 0 };
    uint32_t data_size{ 0 };
    offsets_t offsets;
};

inline archetype_chunk*
archetype_chunk_root::find_free()
{
    // If we have a cached chunk & its not full, use that
    if (cached_free && !cached_free->full()) return cached_free;

    auto it = std::find_if(
      chunks.begin(), chunks.end(), [](auto& b) { return !b->full() && b->allocated(); });

    archetype_chunk* ptr{ nullptr };

    if (it != chunks.end()) {
        ptr = (*it).get();
    } else {
        // No free chunks were found, create & allocated new one
        ptr = create_chunk();
        ptr->alloc(CHUNK_LAYOUT.size, CHUNK_LAYOUT.align);
    }

    // Cache free chunk
    cached_free = ptr;
    return ptr;
}

inline archetype_chunk*
archetype_chunk_root::create_chunk()
{
    return chunks
      .emplace_back(std::move(std::make_unique<archetype_chunk>(archetype, per_chunk)))
      .get();
}
} // namespace realm

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

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