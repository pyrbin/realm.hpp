#pragma once

#include <assert.h>
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

#include "../util/type_hash.hpp"

namespace realm {
/**
 * @brief Memory layout
 * Describes a particular layout of memory. Inspired by Rust's alloc::Layout.
 * @ref https://doc.rust-lang.org/std/alloc/struct.Layout.html
 */
struct memory_layout
{
    const unsigned size{ 0 };
    const unsigned align{ 0 };

    inline constexpr memory_layout() {}
    inline constexpr memory_layout(unsigned size, unsigned align)
      : size{ size }, align{ align }
    {
        /**
         * TODO: add some necessary checks, eg. align has to be power of 2
         * see Rust impl. for examples
         */
    }

    /**
     * Create a memory layout of a specified type
     * @tparam T
     * @return
     */
    template<typename T>
    inline static constexpr memory_layout of()
    {
        return { sizeof(T), alignof(T) };
    }

    /**
     * @brief Returns the amount of padding that has to be added to size to satisfy the
     * layouts alignment
     * @param size
     * @param align
     * @return Padding to insert
     */
    static inline constexpr int align_up(const int size, const int align) noexcept
    {
        return (size + (align - 1)) & !(align - 1);
    }

    inline constexpr int align_up(const int size) const noexcept
    {
        return align_up(size, align);
    }
};

/**
 * @brief Component meta
 * Describes component metadata of a specified type.
 */
struct component_meta
{
    const size_t hash{ 0 };
    const size_t mask{ 0 };

    /**
     * Create component meta of a type.
     * @tparam T
     * @return
     */
    template<typename T>
    static inline constexpr internal::enable_if_component<T, component_meta> of()
    {
        size_t hash = internal::type_hash<std::unwrap_ref_decay_t<T>>::value;
        return { hash, (size_t)(1 << hash) };
    }
};

/**
 * @brief Component
 * Describes a component (metadata, memory layout & functions for construction and
 * destruction).
 */
struct component
{
    using constructor_t = void(void*);

    const component_meta meta;
    const memory_layout layout;

    constructor_t* alloc{ nullptr };
    constructor_t* destroy{ nullptr };

    inline constexpr component(){};

    inline constexpr component(component_meta meta,
                               memory_layout layout,
                               constructor_t* alloc,
                               constructor_t* destroy)
      : meta{ meta }, layout{ layout }, alloc{ alloc }, destroy{ destroy }
    {}

    inline constexpr bool operator==(const component& other) const
    {
        return other.meta.hash == meta.hash;
    }

    /**
     * Create a component of a specified type
     * @tparam T
     * @return
     */
    template<typename T>
    static inline constexpr internal::enable_if_component<T, component> of()
    {
        return { component_meta::of<T>(),
                 memory_layout::of<T>(),
                 [](void* ptr) { new (ptr) T{}; },
                 [](void* ptr) { ((T*) ptr)->~T(); } };
    }
};
} // namespace realm

/**
 * @cond TURN_OFF_DOXYGEN
 * Internal details not to be documented.
 */

namespace std {
template<>
struct hash<realm::component>
{
    size_t operator()(const realm::component& c) const
    {
        return (hash<size_t>{}(c.meta.hash));
    }
};
} // namespace std