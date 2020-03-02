#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <realm/core/archetype.hpp>
#include <realm/util/tuple_util.hpp>

namespace realm {
/**
 * Forward declarations
 */

struct world;

template<typename... Ts>
struct view;

template<typename F>
constexpr internal::enable_if_query_fn<F, void> query(world* world, F&& f);

template<typename F>
constexpr internal::enable_if_query_fn<F, void> query_seq(world* world, F&& f);

/**
 * @brief System meta
 * Contains meta information about a system.
 * Eg. which components it mutates/reads, mask etc.
 */
struct system_meta
{
    const size_t mask{ 0 };
    const size_t mut_mask{ 0 };
    const size_t read_mask{ 0 };

    template<typename... Args>
    static constexpr system_meta from_pack()
    {
        using components = internal::component_tuple<Args...>;
        size_t read{ 0 };
        size_t mut{ 0 };
        (from_pack_helper<Args>(read, mut), ...);
        return { archetype::mask_from_tuple<components>(), mut, read };
    }

    template<typename F, typename... Args>
    static constexpr system_meta of(void (F::*f)(Args...) const)
    {
        return from_pack<Args...>();
    }

    template<typename F, typename... Args>
    static constexpr system_meta of(void (F::*f)(view<Args...>) const)
    {
        return from_pack<Args...>();
    }

private:
    template<typename T>
    static constexpr void from_pack_helper(size_t& read, size_t& mut)
    {
        if constexpr (!internal::is_entity<T> && std::is_const_v<std::remove_reference_t<T>>) {
            read |= component_meta::of<internal::pure_t<T>>().mask;
        } else if constexpr (!internal::is_entity<T>) {
            mut |= component_meta::of<internal::pure_t<T>>().mask;
        }
    }
};

/**
 * @brief System reference
 * Base system reference holder
 */
struct system_ref
{
    const u64 id{ 0 };
    const system_meta meta{ 0, 0 };
    const std::string name{ "" };

    system_ref() = default;
    ;
    virtual ~system_ref() = default;
    virtual bool compare(size_t hash) const = 0;
    virtual bool mutates(size_t hash) const = 0;
    virtual bool reads(size_t hash) const = 0;
    virtual void invoke(world*) const = 0;
    virtual void invoke_seq(world*) const = 0;

protected:
    system_ref(const u64 id, system_meta meta, std::string name)
        : id{ id }
        , meta{ meta }
        , name{ std::move(name) } {};
};

/**
 * @brief System proxy
 * A proxy class used to communicate with a defined system.
 * Used to invoke the update function and uses query_helper functions to
 * execute the query logic.
 * @tparam T Underlying system class
 */
template<typename T>
struct system_proxy final : public system_ref
{
private:
    /*! @brief Underlying system pointer */
    const std::unique_ptr<T> instance_;

    /*! @brief Creates a lamdba object to system update function */
    template<typename R = void, typename... Args>
    static constexpr auto update_lambda(const system_proxy<T>* proxy, void (T::*f)(Args...) const)
    {
        return [proxy, f](Args... args) -> void {
            (proxy->instance_.get()->*f)(std::forward<Args>(args)...);
        };
    }

public:
    /**
     * Construct a system proxy from an object
     * @param t Underlying system to make a proxy to
     */
    explicit system_proxy(T& t)
        : system_ref{ internal::identifier_hash_v<T>, system_meta::of(&T::update),
            typeid(T).name() }
        , instance_{ std::unique_ptr<T>(std::move(t)) }
    {}

    /**
     * Construct a system proxy with arguments for the underlying system.
     * @tparam Args Argument types
     * @param args Arguments for underlying system
     */
    template<typename... Args>
    explicit system_proxy(Args&&... args)
        : system_ref{ internal::identifier_hash_v<T>, system_meta::of(&T::update),
            typeid(T).name() }
        , instance_{ std::make_unique<T>(std::forward<Args>(args)...) }
    {}

    bool compare(const size_t other) const override { return archetype::subset(other, meta.mask); }

    bool mutates(const size_t other) const override
    {
        return archetype::subset(meta.mut_mask, other);
    }

    bool reads(const size_t other) const override
    {
        return archetype::subset(meta.read_mask, other);
    }

    /**
     * Call the system query on a world in parallel
     * @param world
     */
    [[nodiscard]] void invoke(world* world) const override
    {
        query(world, update_lambda(this, &T::update));
    }

    /**
     * Call the system query on a world sequentially
     * @param world
     */
    [[nodiscard]] void invoke_seq(world* world) const override
    {
        query_seq(world, update_lambda(this, &T::update));
    }
};
} // namespace realm