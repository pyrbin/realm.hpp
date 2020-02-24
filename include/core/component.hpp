#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <vector>

#include "../util/identifier.hpp"

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

    constexpr memory_layout() = default;
    constexpr memory_layout(const unsigned size, const unsigned align)
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
    static constexpr memory_layout of()
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
    static constexpr int align_up(const int size, const int align) noexcept
    {
        return (size + (align - 1)) & !(align - 1);
    }

    [[nodiscard]] constexpr int align_up(const int size) const noexcept
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
    const uint64_t hash{ 0 };
    const uint64_t mask{ 0 };

    constexpr component_meta() = default;

    constexpr component_meta(const uint64_t hash, const uint64_t mask)
      : hash{ hash }, mask{ mask } {};

    /**
     * Create component meta of a type.
     * @tparam T
     * @return
     */
    template<typename T>
    static constexpr internal::enable_if_component<T, component_meta> of()
    {
        return component_meta{ internal::identifier_hash_v<internal::pure_t<T>>,
                               internal::identifier_mask_v<internal::pure_t<T>> };
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

    constexpr component() = default;

    constexpr component(component_meta meta,
                        memory_layout layout,
                        constructor_t* alloc,
                        constructor_t* destroy)
      : meta{ meta }, layout{ layout }, alloc{ alloc }, destroy{ destroy }
    {}

    constexpr bool operator==(const component& other) const
    {
        return other.meta.hash == meta.hash;
    }

    /**
     * Create a component of a specified type
     * @tparam T
     * @return
     */
    template<typename T>
    static constexpr internal::enable_if_component<T, component> of()
    {
        return { component_meta::of<T>(),
                 memory_layout::of<T>(),
                 [](void* ptr) { new (ptr) T{}; },
                 [](void* ptr) { static_cast<T*>(ptr)->~T(); } };
    }
};

/**
 * @brief Singleton component
 * Singleton component base class
 */
struct singleton_component
{
    singleton_component() = default;
    singleton_component(component&& comp) : component_info{ comp } {};
    virtual ~singleton_component() = default;
    const component component_info;
};

/**
 * @brief Singleton instance
 * Stores a single component using a unique_ptr.
 * Currently used in world to store singleton components.
 * @tparam T
 */
template<typename T>
struct singleton_instance final : singleton_component
{
    const std::unique_ptr<T> instance;

    explicit singleton_instance(T& t)
      : singleton_component{ component::of<T>() }
      , instance{ std::unique_ptr<T>(std::move(t)) }
    {}

    template<typename... Args>
    explicit singleton_instance(Args&&... args)
      : singleton_component{ component::of<T>() }
      , instance{ std::make_unique<T>(std::forward<Args>(args)...) }
    {}

    /**
     * Get the underlying component instance
     * @return Pointer to component instance
     */
    T* get() { return static_cast<T*>(instance.get()); }
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
    size_t operator()(const realm::component& c) const noexcept
    {
        return (hash<size_t>{}(c.meta.hash));
    }
};
} // namespace std