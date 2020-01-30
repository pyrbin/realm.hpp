#pragma once

#include <cstddef>
#include <string>
#include <typeinfo>
#include <vector>

#include "meta.hpp"

namespace pillar {

/**
 * @brief Describes a particular layout of memory. Inspired by rust
 * alloc::Layout.
 * @ref https://doc.rust-lang.org/std/alloc/struct.Layout.html
 *
 */
struct memory_layout
{
    const size_t size{ 0 };
    const size_t align{ 0 };

    constexpr memory_layout() {}
    constexpr memory_layout(size_t size, size_t align) : size{ size }, align{ align }
    {
        /**
         * todo: add some necessary checks, eg. align has to be power of 2
         * see rust impl. for details
         */
    }

    template<typename T>
    static constexpr memory_layout of()
    {
        return { sizeof(T), alignof(T) };
    }

    /**
     * @brief Returns the amount of padding we must insert in @param size to satisfy the
     * layouts alignment
     * @ref
     * https://doc.rust-lang.org/std/alloc/struct.Layout.html#method.padding_needed_for
     * @param size size to control
     * @return padding to insert
     */
    inline constexpr size_t padding_needed_for(const size_t size) const noexcept
    {
        return (size + align - 1) & (!align + 1);
    }
};

struct component_meta
{
    const size_t hash{ 0 };
    const size_t mask{ 0 };

    template<typename T>
    static constexpr component_meta of()
    {
        auto hash = type_meta<T>::hash;
        return { hash, (size_t)(1 << hash) };
    }
};

struct component
{
    const component_meta meta;
    const memory_layout layout;

    constexpr component() {}
    constexpr component(component_meta meta, memory_layout layout)
      : meta{ meta }, layout{ layout }
    {}

    constexpr bool operator==(const component& other) const
    {
        return other.meta.hash == meta.hash;
    }

    template<typename T>
    static constexpr component of()
    {
        auto hash = type_meta<T>::hash;
        return { component_meta::of<T>(), memory_layout::of<T>() };
    }
};

} // namespace pillar