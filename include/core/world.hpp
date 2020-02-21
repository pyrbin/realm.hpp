#pragma once

#include "archetype.hpp"
#include "entity.hpp"
#include "system.hpp"

#include <execution>
#include <functional>
#include <vector>

namespace realm {
namespace internal {
struct query_main;
}

/**
 * @brief World
 * The world is the core collection of the ECS.
 * Contains all the entities, components & systems & provides simple
 * functions to mutate and read the data.
 */
struct world
{
    static const size_t DEFAULT_MAX_ENTITIES = 100000;

    using chunks_t = std::vector<std::unique_ptr<archetype_chunk_root>>;
    using chunks_map_t = robin_hood::unordered_flat_map<size_t, unsigned>;

    using systems_t = std::vector<std::unique_ptr<internal::system_ref>>;
    using systems_map_t = robin_hood::unordered_flat_map<size_t, unsigned>;

    using entities_t = entity_pool;

    // TODO: make private & put query functions inside a struct & make it friend of world
    chunks_t chunks;

private:
    // chunks_t chunks;
    chunks_map_t chunks_map;

    systems_t systems;
    systems_map_t systems_map;

    entities_t entities;

    /**
     * Returns a free (has available space) chunk of a specified archetype.
     * If no archetype is found a new one will be allocated.
     * @param at Archetype to find
     * @return Pointer to chunk
     */
    inline archetype_chunk* get_free_chunk(const archetype& at)
    {
        archetype_chunk_root* root{ nullptr };
        auto it = chunks_map.find(at.mask());

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
    inline void modify_archetype(entity entt, const archetype& new_at) noexcept
    {
        auto old_location = entities.get(entt);
        auto chunk = get_free_chunk(new_at);

        if (chunk->archetype == old_location->chunk->archetype) return;

        auto idx = chunk->insert(entt);

        // copy and remove from old chunk to new
        old_location->chunk->copy_to(old_location->chunk_index, chunk, idx);
        auto moved = old_location->chunk->remove(old_location->chunk_index);

        // to remove the last entity is swapped with the removed/modified entities index
        // to prevent fragmentation, thus its location has to be updated
        entities.update(moved, { old_location->chunk_index, old_location->chunk });

        // update location info of the modified entity
        entities.update(entt, { idx, chunk });
    }

public:
    /**
     * Allocate a world with specified capacity
     * @param capacity
     */
    world(uint32_t capacity = DEFAULT_MAX_ENTITIES) : entities{ capacity } {}

    /**
     * Create an entity with components from an archetype
     * @param at The archetype
     * @return An entity id
     */
    entity create(const archetype& at)
    {
        auto chunk = get_free_chunk(at);
        auto entt = entities.create({ chunk->size(), chunk });
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
    internal::enable_if_component_pack<std::vector<entity>, T...> batch(uint32_t n)
    {
        return batch(n, archetype::of<T...>());
    }

    /**
     * Batch creation of entities with components from an archetype
     * @param at The archetype
     * @param n Number of entities to create
     * @return A vector of created entity ids
     */
    std::vector<entity> batch(uint32_t n, const archetype& at)
    {
        std::vector<entity> entts{};
        for (uint32_t i{ 0 }; i < n; i++) { entts.push_back(create(at)); }
        return entts;
    }

    /**
     * Check if an entity id exists in the world/is valid
     * @param entt The entity id
     * @return
     */
    bool exists(entity entt) { return entities.exists(entt); }

    /**
     * Destroy an entity, deallocating its component and making the id invalid.
     * @param entt The entity id
     */
    void destroy(entity entt)
    {
        if (!exists(entt)) return;

        auto location = entities.get(entt);

        // copy and remove from old chunk to new
        auto moved = location->chunk->remove(location->chunk_index);

        // update switched entity (as defragmentation is done on removal)
        entities.update(moved, { location->chunk_index, location->chunk });
        entities.free(entt);
    }

    /**
     * Get a specific component reference from an entity.
     * @tparam T Component type
     * @param entt  Entity id
     * @return A component reference of type T&
     */
    template<typename T>
    internal::enable_if_component<T, T&> get(entity entt)
    {
        auto [index, chunk_ptr] = *entities.get(entt);
        return static_cast<T&>(*chunk_ptr->get<std::unwrap_ref_decay_t<T>>(index));
    }

    /**
     * Add components to an entity
     * @tparam T Component types
     * @param entt Entity to add to
     * @return
     */
    template<typename... T>
    internal::enable_if_component_pack<void, T...> add(entity entt)
    {
        assert(!has<T...>(entt));
        auto at = get_archetype(entt);

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
    inline internal::enable_if_component_pack<void, T...> remove(entity entt)
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
            if (!to_remove.has(comp)) { new_components.push_back(component{ comp }); }
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
    inline internal::enable_if_component_pack<bool, T...> has(entity entt)
    {
        auto at = get_archetype(entt);
        return (... && at.has<T>());
    }

    /**
     * Get the archetype of an entity
     * @param entt The entity id
     * @return
     */
    inline const archetype& get_archetype(entity entt)
    {
        return entities.get(entt)->chunk->archetype;
    }

    /**
     * Insert a system into the world with optional arguments for the systems constructor.
     * @tparam T System type
     * @tparam Args System constructor argument type
     * @param args System constructor arguments
     * @return
     */
    template<typename T, typename... Args>
    inline constexpr internal::enable_if_system<T, void> insert(Args&&... args)
    {
        auto ptr =
          std::make_unique<internal::system_proxy<T>>(std::forward<Args>(args)...);
        systems_map.emplace(ptr->id, systems.size());
        systems.push_back(std::move(ptr));
    }

    /**
     * Insert a system into the world.
     * @tparam T System type
     * @param t System object
     * @return
     */
    template<typename T>
    inline constexpr internal::enable_if_system<T, void> insert(T& t)
    {
        auto ptr = std::make_unique<internal::system_proxy<T>>(std::move(t));
        systems_map.emplace(ptr->id, systems.size());
        systems.push_back(std::move(ptr));
    }

    /**
     * Ejects/removes a system from the world.
     * @warning calls the systems destructor
     * @tparam T System type
     * @return
     */
    // TODO: dont destroy the system, just remove & return a pointer
    template<typename T>
    inline constexpr internal::enable_if_system<T, void> eject()
    {
        size_t id = internal::type_hash<T>::value;
        size_t idx = systems_map.find(id)->second;

        systems.at(idx).reset();

        internal::swap_remove(idx, systems);

        size_t other_id = systems.at(idx)->id;

        systems_map.find(other_id)->second = idx;
    }

    /**
     * @brief Update
     * Calls update on every system that has been inserted in the world on matching
     * archetype chunks. The call order is determined by the insert order of the
     * systems where the first inserted will be the first to be called.
     */
    inline void update()
    {
        for (auto& sys : systems) { sys->invoke(this); }
    }

    /**
     * @brief Update
     * Calls update on every system that has been inserted in the world on matching
     * archetype chunks using specified execution policy (eg. execution::par).
     * Systems won't run in parallel but each matching chunk per system will.
     * The call order is determined by the insert order of the  systems where the first
     * inserted will be the first to be called.
     */
    template<typename ExePo>
    inline void update(ExePo policy)
    {
        for (auto& sys : systems) { sys->invoke(policy, this); }
    }

    int32_t size() const noexcept { return entities.size(); }
    int32_t capacity() const noexcept { return entities.capacity(); }
    int32_t system_count() const noexcept { return systems.size(); }
};
} // namespace realm