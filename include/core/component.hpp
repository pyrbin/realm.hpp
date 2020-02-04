#pragma once

#include <assert.h>
#include <cstddef>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>

#include "../detail/type_meta.hpp"
#include "concepts.hpp"

namespace realm {

/**
 * @brief Describes a particular layout of memory. Inspired by rust
 * alloc::Layout.
 * @ref https://doc.rust-lang.org/std/alloc/struct.Layout.html
 *
 */
struct memory_layout
{
    int size{ 0 };
    int align{ 0 };

    constexpr memory_layout() {}
    constexpr memory_layout(int size, int align) : size{ size }, align{ align }
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
    static inline constexpr int align_up(const int size, const int align) noexcept
    {
        return (size + (align - 1)) & !(align - 1);
    }

    inline constexpr int align_up(const int size) const noexcept
    {
        return align_up(size, align);
    }
};

struct component_meta
{
    size_t hash{ 0 };
    size_t mask{ 0 };

    template<Component T>
    static constexpr component_meta of()
    {
        auto hash = detail::type_meta<T>::hash;
        return { hash, (size_t)(1 << hash) };
    }
};

struct component
{
    using constructor_t = void(void*);

    component_meta meta;
    memory_layout layout;

    constructor_t* invoke{ nullptr };
    constructor_t* destroy{ nullptr };

    constexpr component() {}
    constexpr component(component_meta meta,
                        memory_layout layout,
                        constructor_t* invoke,
                        constructor_t* destroy)
      : meta{ meta }, layout{ layout }, invoke{ invoke }, destroy{ destroy }
    {}

    component& operator=(const component& other)
    {
        meta = other.meta;
        layout = other.layout;
        invoke = other.invoke;
        destroy = other.destroy;
        return *this;
    }

    constexpr bool operator==(const component& other) const
    {
        return other.meta.hash == meta.hash;
    }

    template<Component T>
    static constexpr component of()
    {
        return { component_meta::of<T>(),
                 memory_layout::of<T>(),
                 [](void* ptr) { new (ptr) T{}; },
                 [](void* ptr) { ((T*) ptr)->~T(); } };
    }
};

} // namespace realm