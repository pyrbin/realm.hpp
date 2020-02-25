#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "../extern/robin_hood.hpp"
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
        template <typename... T>
        static constexpr internal::enable_if_component_pack<data, T...> of()
        {
            return { (sizeof(T) + ... + 0), mask_of<T...>() };
        }
    };

private:
    const data _info{ 0, 0 };
    const size_t _component_count{ 0 };

    /**
     * Create an archetype with specified components & data.
     * @warning if this constructor is used there is no guarantee that the archetype data
     * is valid for the components.
     * @param components
     * @param info
     */
    archetype(const components_t& components, const data& info)
        : _info{ info }
        , _component_count{ components.size() }
        , components{ components }  // NOLINT(clang-diagnostic-reorder)
    {
    }

public:
    const components_t components;

    archetype() = default;

    /**
     * Create an archetype of a packed parameter of component types
     * @tparam T Component types
     * @return An archetype of types T
     */
    template <typename... T>
    static constexpr internal::enable_if_component_pack<archetype, T...> of()
    {
        return archetype{ { component::of<T>()... }, data::of<T...>() };
    }

    /**
     * Create an archetype from a tuple of component types
     * @tparam T Tuple of component types
     * @return An archetype of types in tuple T
     */
    template <typename T>
    static archetype from_tuple()
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
    template <typename... T>
    static constexpr internal::enable_if_component_pack<size_t, T...> mask_of()
    {
        size_t mask{ 0 };
        (  // Iterate each type and get component mask
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
    template <typename T>
    static constexpr size_t mask_from_tuple()
    {
        return [&]<typename... Ts>(std::type_identity<std::tuple<Ts...>>)
        {
            return archetype::mask_of<Ts...>();
        }
        (std::type_identity<T>{});
    }

    /**
     * Check if an archetype is a subset of another archetype
     * @param a Potential superset archetypes mask
     * @param b Potential subset archetypes mask
     * @return If archetype b is a subset of archetype
     */
    static constexpr bool subset(const size_t a, const size_t b)
    {
        return (a & b) == b;
    }

    /**
     * Check for intersection between two archetypes
     * @param a Archetype a's mask
     * @param b Archetype b's mask
     * @return If archetypes shares atleast one component
     */
    static constexpr bool intersection(const size_t a, const size_t b)
    {
        return (a & b) == b || (b & a) == a;
    }

    /**
     * Check if an archetype mask is a subset of this archetype
     * @param other Potential subset mask
     * @return
     */
    [[nodiscard]] constexpr bool subset(const size_t other) const
    {
        return archetype::subset(mask(), other);
    }

    /**
     * Check if an archetype is a subset of this archetype
     * @param other Potential subset
     * @return
     */
    [[nodiscard]] constexpr bool subset(const archetype& other) const
    {
        return subset(other.mask());
    }

    constexpr bool operator==(const archetype& other) const
    {
        return other.mask() == mask();
    }

    /**
     * Check if archetype contains specified component type
     * @tparam T Component type
     * @return
     */
    template <typename T>
    constexpr internal::enable_if_component<T, bool> has() const
    {
        return has(component::of<T>());
    }

    /**
     * Check if archetype contains specified component
     * @param component Component object
     * @return
     */
    [[nodiscard]] constexpr bool has(const component& component) const
    {
        return (mask() & component.meta.mask) == component.meta.mask;
    }

    /**
     * Iterate each component of the archetype
     * @tparam F Lambda type
     * @param f Lambda object
     * @return
     */
    template <typename F>
    constexpr void iter(F&& f) const
    {
        for (auto& component : components) {
            f(component);
        }
    }
    /*! @brief Combined mask of all components in archetype */
    [[nodiscard]] constexpr size_t mask() const
    {
        return _info.mask;
    }
    /*! @brief The data size of all components in archetype */
    [[nodiscard]] constexpr size_t size() const
    {
        return _info.size;
    }
    /*! @brief The count of components in archetype */
    [[nodiscard]] constexpr size_t count() const
    {
        return _component_count;
    }
};

/**
 * @brief Archetype chunk root
 * Contains all the chunks of a certain archetype, currently stored as a vector with
 * unqiue pointers. Every archetype has a root which is the access point to that
 * archetypes chunks. Also contains some metadata for the chunks such as max entities per
 * chunk, memory size and alignment.
 */
struct archetype_chunk_root
{
    using chunk_ptr = std::unique_ptr<archetype_chunk>;

    static const uint32_t chunk_size_16_kb{ 16 * 1024 };
    static const uint32_t chunk_alignment{ 64 };

    static constexpr memory_layout chunk_layout{ chunk_size_16_kb, chunk_alignment };

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
        , per_chunk{ chunk_layout.size / static_cast<uint32_t>(archetype.size()) }
    {
    }

    ~archetype_chunk_root()
    {
        cached_free = nullptr;
    }

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
     * Create a chunk of specified archetype and max capacity (maximum entities)
     * @param archetype
     * @param max_capacity
     */
    archetype_chunk(struct archetype archetype, uint32_t max_capacity)
        : archetype{ std::move(archetype) }, _max_capacity(max_capacity)
    {
    }

    ~archetype_chunk()
    {
        dealloc();
    }

    /**
     * Allocates required memory for the chunk.
     * @param chunk_size Chunk size
     * @param alignment Chunk alignment
     * @return Pointer to allocated memory
     */
    pointer alloc(const unsigned chunk_size, const unsigned alignment)
    {
        entities.resize(_max_capacity);
        // Memory alignment of components
        for (const auto& component : archetype.components) {
            _data_size += component.layout.align_up(_data_size);
            /* data_size = memory_layout::align_up(
              memory_layout::align_up(data_size, alignment), component.layout.align);*/
            _offsets.emplace(component.meta.hash, _data_size);
            _data_size += component.layout.size * _max_capacity;
        }
        // Assert data size is not more than chunk size
        assert(_data_size <= chunk_size);

#if defined(_WIN32) || defined(__CYGWIN__)
        _data = (_aligned_malloc(_data_size, alignment));
#else
        data = (std::aligned_alloc(alignment, data_size));
#endif

        return _data;
    }

    /**
     * Frees the chunks allocated memory
     */
    void dealloc()
    {
        if (_data != nullptr) {
#if defined(_WIN32) || defined(__CYGWIN__)
            _aligned_free(static_cast<void*>(_data));
#else
            free((void*)(data));
#endif
            _data = nullptr;
        }
    }

    /**
     * Insert an entity into the chunk
     * @param entt Entity id to insert
     * @return The chunk index of the inserted entity
     */
    unsigned insert(const entity entt)
    {
        // Allocate each component for the new entity
        for (const auto& component : archetype.components) {
            component.alloc(get_pointer(_size, component));
        }

        entities[_size++] = entt;
        return _size - 1;
    }

    /**
     * Remove an entity & components at specified index in the chunk.
     * Swaps the entity at the last index with the entity to remove to hinder
     * fragmentation inside the chunk.
     * @param index Index to remove
     * @return Entity that has been swapped to removed index
     */
    entity remove(const unsigned index)
    {
        const auto end{ (_size--) - 1 };
        if (_size == 0)
            return entities[index];

        // Swaps last entity with entity to remove
        // TODO: determine if this de-fragmentation is too costly?
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
    template <typename T>
    [[nodiscard]] internal::enable_if_component<T, T*> get(uint32_t index) const
    {
        return static_cast<T*>(get_pointer(index, component::of<T>()));
    }

    /**
     * Set the component T of an entity at a specific index.
     * @tparam T Component type
     * @param index Index in chunk
     * @param data Component data to set
     * @return Component pointer
     */
    template <typename T>
    internal::enable_if_component<T, T*> set(const uint32_t index, T&& data) const
    {
        auto component = component::of<T>();
        std::memcpy(get_pointer(index, component), &data, component.layout.size);
        return get<T>(index);
    }

    /**
     * Get pointer to entity id at specified index. Used in queries when trying to fetch
     * entity.
     * @param index Index in chunk
     * @return Pointer to entity id
     */
    [[nodiscard]] const entity* get_entity_at(const uint32_t index) const
    {
        return &entities[index];
    }

    /**
     * Get a void pointer for the address of a certain component in the chunk.
     * @param index Index in chunk
     * @param type Component object
     * @return
     */
    [[nodiscard]] pointer get_pointer(const uint32_t index, const component& type) const
    {
        return static_cast<void*>(static_cast<std::byte*>(_data) +
                                  offset_to(index, type));
    }

    /**
     * Copies a component to another chunk using memcpy.
     * @param from Index in this chunk
     * @param other Other chunk
     * @param to Index in other chunk
     */
    void copy_to(const unsigned from, const archetype_chunk* other,
                 const unsigned to) const
    {
        for (const auto& component : archetype.components) {
            if (other->archetype.has(component)) {
                memcpy(other->get_pointer(to, component), get_pointer(from, component),
                       component.layout.size);
            }
        }
    }

    /*! @brief Capacity of chunk */
    [[nodiscard]] constexpr uint32_t capacity() const noexcept
    {
        return _max_capacity;
    }
    /*! @brief Size (current entity count) of chunk */
    [[nodiscard]] constexpr uint32_t size() const noexcept
    {
        return _size;
    }

    /*! @brief Chunk is full */
    [[nodiscard]] bool full() const noexcept
    {
        return _size >= _max_capacity;
    }
    /*! @brief Size (current entity count) of chunk */
    [[nodiscard]] bool allocated() const noexcept
    {
        return _data != nullptr;
    }

private:
    /**
     * Get the offset in chunk of a component type & specified index
     * @param index Index in chunk
     * @param type Component object
     * @return
     */
    [[nodiscard]] size_t offset_to(const unsigned index, const component& type) const
    {
        const auto offset = _offsets.at(type.meta.hash);
        return offset + static_cast<size_t>(index * type.layout.size);
    }

    /*! @brief Pointer to allocated data */
    pointer _data{ nullptr };

    uint32_t _size{ 0 };
    uint32_t _max_capacity{ 0 };
    uint32_t _data_size{ 0 };
    offsets_t _offsets;
};

inline archetype_chunk* archetype_chunk_root::find_free()
{
    // If we have a cached chunk & its not full, use that
    if (cached_free && !cached_free->full())
        return cached_free;

    const auto it = std::find_if(chunks.begin(), chunks.end(),
                                 [](auto& b) { return !b->full() && b->allocated(); });

    archetype_chunk* ptr{ nullptr };

    if (it != chunks.end()) {
        ptr = (*it).get();
    } else {
        // No free chunks were found, create & allocated new one
        ptr = create_chunk();
        ptr->alloc(chunk_layout.size, chunk_layout.align);
    }

    // Cache free chunk
    cached_free = ptr;
    return ptr;
}

inline archetype_chunk* archetype_chunk_root::create_chunk()
{
    return chunks.emplace_back(std::make_unique<archetype_chunk>(archetype, per_chunk))
        .get();
}
}  // namespace realm

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace std {
template <>
struct hash<realm::archetype>
{
    size_t operator()(const realm::archetype& at) const noexcept
    {
        return (hash<size_t>{}(at.mask()));
    }
};

template <>
struct hash<realm::archetype_chunk_root>
{
    size_t operator()(const realm::archetype_chunk_root& at) const noexcept
    {
        return (hash<realm::archetype>{}(at.archetype));
    }
};
}  // namespace std