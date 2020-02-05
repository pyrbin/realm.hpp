#pragma once

#include "archetype.hpp"
#include "concepts.hpp"
#include "entity.hpp"
#include "system.hpp"

#include <functional>
#include <unordered_map>
#include <vector>

namespace realm {

struct world
{
    using chunks_t = std::unordered_map<size_t, std::unique_ptr<archetype_chunk_root>>;
    using entities_t = entity_pool;

    chunks_t chunks;
    entities_t entities;

private:
    archetype_chunk* get_free_chunk(const archetype& at)
    {

        archetype_chunk_root* root{ nullptr };

        if (!chunks.contains(at.mask())) {
            auto ptr = std::make_unique<archetype_chunk_root>(at);
            root = ptr.get();
            chunks.emplace(at.mask(), std::move(ptr));
        } else {
            root = chunks.at(at.mask()).get();
        }

        return root->find_free();
    }

public:
    world(uint32_t capacity = 100) : entities{ capacity } {}

    entity create(const archetype& at)
    {
        auto chunk = get_free_chunk(at);
        auto entt = entities.create(entity_location{ chunk->size(), chunk });
        return chunk->insert(entt);
    }

    template<typename... T>
    entity create() requires ComponentPack<T...>
    {
        return create(archetype::of<T...>());
    }

    template<typename... T>
    std::vector<entity> batch(uint32_t n) requires ComponentPack<T...>
    {
        return batch(n, archetype::of<T...>());
    }

    std::vector<entity> batch(uint32_t n, const archetype& at)
    {
        std::vector<entity> entts{};
        for (uint32_t i{ 0 }; i < n; i++) { entts.push_back(create(at)); }
        return entts;
    }

    template<BaseComponent T>
    T& get(entity entt)
    {
        auto [index, ptr] = *entities.get(entt);
        return static_cast<T&>(*ptr->get<std::unwrap_ref_decay_t<T>>(index));
    }

    template<System T, typename... Args>
    system_functor<T>* insert(Args&&... args)
    {
        auto system = new system_functor<T>(T{ std::forward<Args>(args)... });
        static_cast<system_functor<T>*>(system)->init(&T::update);
        return system;
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
};

} // namespace realm
