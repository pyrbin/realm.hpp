#pragma once

#include "archetype.hpp"
#include "entity.hpp"
#include "unordered_map"
#include "vector"

namespace realm {
struct world
{
public:
    using chunks_t = std::unordered_map<size_t, archetype_chunk_parent>;
    using entitites_t = entity_pool;

    entitites_t entities;
    chunks_t chunks;

    world(uint32_t capacity = 100) : entities{ capacity } {}

    template<typename... T>
    entity_t create()
    {
        auto at = archetype::of<T...>();
        auto chunk = get_chunk(at);
        auto entt = entities.create({ chunk->size(), chunk });
        return chunk->insert(entt);
    }

    template<typename... T>
    std::vector<entity_t> batch(uint32_t n)
    {
        std::vector<entity_t> entts;
        for (int i{ 0 }; i < n; i++) { entts.push_back(create<T...>()); }
        return entts;
    }

    template<typename T>
    T& get(entity_t entt)
    {
        auto [index, ptr] = *entities.get(entt);
        return *ptr->get<T>(index);
    }

    int32_t size() const noexcept { return entities.size(); }
    int32_t capacity() const noexcept { return entities.capacity(); }

private:
    archetype_chunk_parent::chunk_ptr get_chunk(const archetype& at)
    {
        if (!chunks.contains(at.mask()))
            chunks.emplace(at.mask(), archetype_chunk_parent{ at });
        auto cp = chunks.at(at.mask());
        return cp.find_free();
    }
};
}
