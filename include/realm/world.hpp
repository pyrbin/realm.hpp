#pragma once

#include "archetype.hpp"
#include "concept.hpp"
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

    entity create(const archetype& at)
    {
        auto chunk = get_chunk(at);
        auto entt = entities.create(entity_location{ chunk->size(), chunk });
        return chunk->insert(entt);
    }

    template<typename... T>
    entity create() requires UniquePack<T...>
    {
        return create(archetype::of<T...>());
    }

    template<typename... T>
    std::vector<entity> batch(uint32_t n) requires UniquePack<T...>
    {
        return batch(n, archetype::of<T...>());
    }

    std::vector<entity> batch(uint32_t n, const archetype& at)
    {
        std::vector<entity> entts;
        for (uint32_t i{ 0 }; i < n; i++) { entts.push_back(create(at)); }
        return entts;
    }

    template<Component T>
    T& get(entity entt)
    {
        auto [index, ptr] = *entities.get(entt);
        return *ptr->get<std::decay_t<T>>(index);
    }

    int32_t size() const noexcept { return entities.size(); }
    int32_t capacity() const noexcept { return entities.capacity(); }

    // Functor objects
    template<typename Functor>
    void query(Functor&& f)
    {
        query_helper(&f, &std::decay_t<Functor>::operator());
    }

private:
    archetype_chunk_parent::chunk_ptr get_chunk(const archetype& at)
    {
        if (!chunks.contains(at.mask()))
            chunks.emplace(at.mask(), archetype_chunk_parent{ at });
        auto cp = chunks.at(at.mask());
        return cp.find_free();
    }

    // https://stackoverflow.com/questions/55756181/use-lambda-to-modify-references-identified-by-a-packed-parameter
    template<typename Class, typename... Args>
    void query_helper(Class* obj,
                      void (Class::*f)(Args...) const) requires UniquePack<Args...>
    {
        entities.each(
          [&](auto&& entt, auto&& loc) { (obj->*f)(get<std::decay_t<Args>>(entt)...); });
    }
    // allows queries to use entity-types as parameter
    template<Entity T>
    entity get(entity entt)
    {
        return entity{ entt };
    }
};
}
