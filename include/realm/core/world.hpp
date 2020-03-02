#pragma once

#include <vector>

#include <realm/core/archetype.hpp>
#include <realm/core/entity.hpp>
#include <realm/core/scheduler.hpp>
#include <realm/core/system.hpp>

namespace realm {
/**
 * @brief World
 * The world is the core collection of the ECS.
 * Contains all the entities_, components & systems_ & provides simple
 * functions to mutate and read the data.
 */
struct world
{
    static const size_t default_max_entities = 100000;

    using ptr = world*;
    using chunks_t = std::vector<std::unique_ptr<archetype_chunk_root>>;
    using hash_map_t = robin_hood::unordered_flat_map<size_t, unsigned>;
    using singletons_t = std::vector<std::unique_ptr<singleton_component>>;

    // TODO: make private & put query functions inside a struct & make it friend of world
    chunks_t chunks;
    hash_map_t chunks_map;

private:
    scheduler systems_;
    entity_manager entities_;

    u64 singleton_mask_{ 0 };
    singletons_t singletons_;
    hash_map_t singletons_map_;

    /**
     * Returns a free (has available space) chunk of a specified archetype.
     * If no archetype is found a new one will be allocated.
     * @param at Archetype to find
     * @return Pointer to chunk
     */
    archetype_chunk::ptr get_free_chunk(const archetype& at)
    {
        archetype_chunk_root::ptr root{ nullptr };
        const auto it = chunks_map.find(at.mask());

        if (it == chunks_map.end()) {
            // No root chunk where found, create new root & allocate
            auto ptr = std::make_unique<archetype_chunk_root>(at);
            root = ptr.get();
            chunks.emplace_back(std::move(ptr));
            chunks_map.emplace(at.mask(), chunks.size() - 1);
        } else {
            // A valid chunk root was found
            root = chunks.at((*it).second).get();
        }

        // Returns a free chunk (or allocates a new one if no available)
        return root->find_free();
    }

    /**
     * Replaces an entities archetype with a new one.
     * Removes and copies components if a transfer between chunks is needed
     * @param entt
     * @param new_at
     */
    void modify_archetype(const entity entt, const archetype& new_at) noexcept
    {
        auto old_location = entities_.get(entt);
        auto chunk = get_free_chunk(new_at);

        if (chunk->archetype == old_location->chunk->archetype)
            return;

        const auto idx = chunk->insert(entt);

        // copy and remove from old chunk to new
        old_location->chunk->copy_to(old_location->chunk_index, chunk, idx);
        const auto moved = old_location->chunk->remove(old_location->chunk_index);

        // to remove the last entity is swapped with the removed/modified entities index
        // to prevent fragmentation, thus its location has to be updated
        entities_.update(moved, { old_location->chunk_index, old_location->chunk });

        // update location info of the modified entity
        entities_.update(entt, { idx, chunk });
    }

public:
    /**
     * Allocate a world with specified capacity
     * @param capacity
     */
    explicit world(const u32 capacity = default_max_entities)
        : entities_{ capacity }
    {}

    /**
     * Create a singleton component of a type
     * @tparam T The type of the singleton component
     * @return
     */
    template<typename T>
    internal::enable_if_component<T, void> singleton()
    {
        auto meta = component_meta::of<T>();

        // If singleton is already registered
        if (is_singleton<T>()) {
            return;
        }

        // Update mask and insert singleton instance
        singleton_mask_ |= meta.mask;
        singletons_.push_back(std::make_unique<singleton_instance<T>>(T{}));
        singletons_map_.emplace(meta.hash, singletons_.size() - 1);
    }

    /**
     * Get a singleton component instance from the world.
     * @tparam T The type of the singleton component
     * @return Reference to the component
     */
    template<typename T>
    internal::enable_if_component<T, T&> get_singleton()
    {
        auto meta = component_meta::of<internal::pure_t<T>>();
        auto idx = singletons_map_.at(meta.hash);
        auto singleton = static_cast<singleton_instance<T>*>(singletons_.at(idx).get());
        return *singleton->get();
    }

    /**
     * Check if a component is stored as a singleton in the world
     * @param component Component to check
     * @return If component is a singleton
     */
    bool is_singleton(const component& component) const
    {
        if (singleton_mask_ == 0)
            return false;
        return archetype::subset(singleton_mask_, component.meta.mask);
    }

    /**
     * Check if a component type is stored as a singleton in the world
     * @tparam T The type of the singleton component
     * @return If component type is a singleton
     */
    template<typename T>
    internal::enable_if_component<T, bool> is_singleton()
    {
        auto meta = component_meta::of<internal::pure_t<T>>();
        if (singleton_mask_ == 0)
            return false;
        return archetype::subset(singleton_mask_, meta.mask);
    }

    /**
     * Create an entity with components from an archetype
     * @param at The archetype
     * @return An entity id
     */
    entity create(const archetype& at)
    {
        auto chunk = get_free_chunk(at);
        const auto entt = entities_.create({ chunk->size(), chunk });
        chunk->insert(entt);
        return entt;
    }

    /**
     * Create an entity with a components from a parameter pack
     * @tparam T Component types
     * @return An entity id
     */
    template<typename... T>
    internal::enable_if_component_pack<entity, T...> create()
    {
        return create(archetype::of<T...>());
    }

    /**
     * Batch creation of entities with components from a parameter pack
     * @tparam T Component type
     * @param n Number of entities to create
     * @return A vector of created entity ids
     */
    template<typename... T>
    internal::enable_if_component_pack<std::vector<entity>, T...> batch(u32 n)
    {
        return batch(n, archetype::of<T...>());
    }

    /**
     * Batch creation of entities with components from an archetype
     * @param at The archetype
     * @param n Number of entities to create
     * @return A vector of created entity ids
     */
    std::vector<entity> batch(const u32 n, const archetype& at)
    {
        std::vector<entity> entities_{};
        for (u32 i{ 0 }; i < n; i++) {
            entities_.push_back(create(at));
        }
        return entities_;
    }

    /**
     * Check if an entity id exists in the world/is valid
     * @param entt The entity id
     * @return
     */
    bool exists(const entity entt) const { return entities_.exists(entt); }

    /**
     * Destroy an entity, deallocating its component and making the id invalid.
     * @param entt The entity id
     */
    void destroy(const entity entt)
    {
        if (!exists(entt))
            return;

        auto location = entities_.get(entt);

        // copy and remove from old chunk to new
        const auto moved = location->chunk->remove(location->chunk_index);

        // update switched entity (as defragmentation is done on removal)
        entities_.update(moved, { location->chunk_index, location->chunk });
        entities_.remove(entt);
    }

    /**
     * Get a specific component reference from an entity.
     * @tparam T Component type
     * @param entt  Entity id
     * @return A component reference of type T&
     */
    template<typename T>
    internal::enable_if_component<T, T&> get(const entity entt)
    {
        auto [index, chunk_ptr] = *entities_.get(entt);
        return static_cast<T&>(*chunk_ptr->get<internal::pure_t<T>>(index));
    }

    /**
     * Get a specific component reference from an entity.
     * @tparam T Component type
     * @param entt  Entity id
     * @param data
     * @return A component reference of type T&
     */
    template<typename T>
    internal::enable_if_component<T, T&> set(const entity entt, T&& data)
    {
        auto [index, chunk_ptr] = *entities_.get(entt);
        return static_cast<T&>(*chunk_ptr->set<internal::pure_t<T>>(index, std::forward<T>(data)));
    }

    /**
     * Add components to an entity
     * @tparam T Component types
     * @param entt Entity to add to
     * @return
     */
    template<typename... T>
    internal::enable_if_component_pack<void, T...> add(const entity entt)
    {
        assert(!has<T...>(entt));
        const auto at = get_archetype(entt);

        // Create new archetype info with added components
        auto new_components = std::vector<component>{ at.components };
        auto info = archetype::data::of<T...>();
        info += { at.size(), at.mask() };
        (new_components.push_back(component::of<T>()), ...);

        // Modify archetype with new archetype info
        modify_archetype(entt, { new_components, info });
    }

    /**
     * Remove components from an entity
     * @tparam T Component types
     * @param entt Entity to remove from
     * @return
     */
    template<typename... T>
    internal::enable_if_component_pack<void, T...> remove(const entity entt)
    {
        assert(has<T...>(entt));
        auto at = get_archetype(entt);
        auto to_remove = archetype::of<T...>();
        auto to_remove_info = archetype::data::of<T...>();

        // Create new archetype info with removed components
        std::vector<component> new_components{};
        archetype::data info{ at.size(), at.mask() };
        info -= to_remove_info;
        for (auto& comp : at.components) {
            if (!to_remove.has(comp)) {
                new_components.push_back(component{ comp });
            }
        }

        // Modify archetype with new archetype info
        modify_archetype(entt, { new_components, info });
    }

    /**
     * If an entity has a specified component
     * @tparam T Component type
     * @param entt Entity to check
     * @return If the entity has the component
     */
    template<typename... T>
    internal::enable_if_component_pack<bool, T...> has(const entity entt) const
    {
        auto at = get_archetype(entt);
        return (... && at.has<T>());
    }

    /**
     * Get the archetype of an entity
     * @param entt The entity id
     * @return
     */
    const archetype& get_archetype(const entity entt) const
    {
        return entities_.get(entt)->chunk->archetype;
    }

    /**
     * Insert a system into the world with optional arguments for the systems constructor.
     * @tparam T System type
     * @tparam Args System constructor argument type
     * @param args System constructor arguments
     * @return
     */
    template<typename T, typename... Args>
    constexpr internal::enable_if_system<T, void> insert(Args&&... args)
    {
        systems_.insert<T>(std::forward<Args>(args)...);
    }

    /**
     * Insert a system into the world.
     * @tparam T System type
     * @param t System object
     * @return
     */
    template<typename T>
    constexpr internal::enable_if_system<T, void> insert(T&& t)
    {
        systems_.insert(std::forward<T>(t));
    }

    /**
     * @brief Update in parallel
     * Calls update on every system that has been inserted in the world on matching
     * archetype chunks.
     */
    void update() { systems_.exec(this); }

    /**
     * @brief Update sequentially (no parallelization)
     * Calls update on every system that has been inserted in the world on matching
     * archetype chunks. The call order is determined by the insert order of the
     * systems where the first inserted will be the first to be called.
     */
    void update_seq() { systems_.exec_seq(this); }

    [[nodiscard]] int32_t size() const noexcept { return entities_.size(); }

    [[nodiscard]] int32_t capacity() const noexcept { return entities_.capacity(); }

    [[nodiscard]] int32_t count_() const noexcept { return systems_.size(); }
};
} // namespace realm