#pragma once

#include "archetype.hpp"
#include "entity.hpp"
#include "system.hpp"

#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

namespace realm {

struct world
{
    static const size_t DEFAULT_MAX_ENTITIES = 100000;

    using chunks_t =
      robin_hood::unordered_flat_map<size_t, std::unique_ptr<archetype_chunk_root>>;
    using systems_t = std::vector<std::unique_ptr<system_base>>;
    using entities_t = internal::entity_pool;

    chunks_t chunks;
    entities_t entities;
    systems_t systems;

private:
    inline archetype_chunk* get_free_chunk(const archetype& at)
    {

        archetype_chunk_root* root{ nullptr };
        auto it = chunks.find(at.mask());

        if (it == chunks.end()) {
            auto ptr = std::make_unique<archetype_chunk_root>(at);
            root = ptr.get();
            chunks.emplace(at.mask(), std::move(ptr));
        } else {
            root = chunks.at(at.mask()).get();
        }

        return root->find_free();
    }

    inline void modify_archetype(entity entt, const archetype& new_at) noexcept
    {
        auto old_location = entities.get(entt);
        auto chunk = get_free_chunk(new_at);

        if (chunk->archetype == old_location->chunk->archetype) return;

        // insert into new archetype chunk
        auto idx = chunk->insert(entt);

        // copy and remove from old chunk to new
        old_location->chunk->copy_to(old_location->chunk_index, chunk, idx);
        auto moved = old_location->chunk->remove(old_location->chunk_index);

        // update location info of entity
        entities.update(entt, { idx, chunk });
    }

public:
    world(uint32_t capacity = DEFAULT_MAX_ENTITIES) : entities{ capacity } {}

    entity create(const archetype& at)
    {
        auto chunk = get_free_chunk(at);
        auto entt = entities.create({ chunk->size(), chunk });
        chunk->insert(entt);
        return entt;
    }

    template<typename... T>
    internal::enable_if_component_pack<entity, T...> create()
    {
        return create(archetype::of<T...>());
    }

    template<typename... T>
    internal::enable_if_component_pack<std::vector<entity>, T...> batch(uint32_t n)
    {
        return batch(n, archetype::of<T...>());
    }

    std::vector<entity> batch(uint32_t n, const archetype& at)
    {
        std::vector<entity> entts{};
        for (uint32_t i{ 0 }; i < n; i++) { entts.push_back(create(at)); }
        return entts;
    }

    bool exists(entity entt) { return entities.exists(entt); }

    template<typename T>
    internal::enable_if_component<T, T&> get(entity entt)
    {
        auto [index, chunk_ptr] = *entities.get(entt);
        return static_cast<T&>(*chunk_ptr->get<std::unwrap_ref_decay_t<T>>(index));
    }

    template<typename... T>
    internal::enable_if_component_pack<void, T...> add(entity entt)
    {
        assert(!has<T...>(entt));

        auto at = get_archetype(entt);
        auto comps = std::vector<component>{ at.components };
        (comps.push_back(component::of<T>()), ...);
        auto new_at = archetype{ comps };
        modify_archetype(entt, new_at);
    }

    template<typename... T>
    inline constexpr internal::enable_if_component_pack<void, T...> remove(entity entt)
    {
        auto at = get_archetype(entt);
        auto to_remove = archetype::of<T...>();

        std::vector<component> new_components{};

        for (auto& comp : at.components) {
            if (!to_remove.has(comp)) { new_components.push_back(component{ comp }); }
        }

        auto new_at = archetype{ new_components };
        modify_archetype(entt, new_at);
    }

    template<typename... T>
    inline constexpr internal::enable_if_component_pack<bool, T...> has(entity entt)
    {
        auto at = get_archetype(entt);
        return (... && at.has<T>());
    }

    const archetype& get_archetype(entity entt)
    {
        return entities.get(entt)->chunk->archetype;
    }

    template<typename T, typename... Args>
    inline constexpr system_functor<T>* insert(Args&&... args)
    {
        auto ptr = std::make_unique<system_functor<T>>(std::forward<Args>(args)...);
        return static_cast<system_functor<T>*>(
          systems.emplace_back(std::move(ptr)).get());
    }

    inline void update()
    {
        for (auto& sys : systems) { sys->operator()(this); }
    }

    template<typename T>
    void iter_chunks(const archetype& at, T&& fn)
    {
        for (auto& [hash, root] : chunks) {
            if (at.subset(hash)) {
                for (auto& chunk : root->chunks) { fn(chunk.get()); }
            }
        }
    }

    template<typename T>
    void iter_chunk_roots(const archetype& at, T&& fn)
    {
        for (auto& [hash, root] : chunks) {
            if (at.subset(hash)) { fn(root.get()); }
        }
    }

    int32_t size() const noexcept { return entities.size(); }
    int32_t capacity() const noexcept { return entities.capacity(); }
    int32_t system_count() const noexcept { return systems.size(); }
};

} // namespace realm
