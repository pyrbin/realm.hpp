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
    using chunks_t = std::vector<std::unique_ptr<archetype_chunk_parent>>;
    using entities_t = entity_pool;

    chunks_t chunks;
    entities_t entities;

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
        return *ptr->get<T>(index);
    }

    // Functor objects
    template<typename T>
    void query(T&& f)
    {
        query_helper(&f, &std::decay_t<T>::operator());
    }

    int32_t size() const noexcept { return entities.size(); }
    int32_t capacity() const noexcept { return entities.capacity(); }

private:
    archetype_chunk* get_chunk(const archetype& at)
    {
        auto it = std::find_if(
          chunks.begin(), chunks.end(), [at](auto& b) { return b->archetype == at; });
        archetype_chunk_parent* root{ nullptr };
        if (it == chunks.end()) {
            chunks.push_back(std::move(std::make_unique<archetype_chunk_parent>(at)));
            root = chunks.back().get();
        } else {
            root = (*it).get();
        }
        return root->find_free();
    }

    // https://stackoverflow.com/questions/55756181/use-lambda-to-modify-references-identified-by-a-packed-parameter
    template<typename T, typename... Args>
    void query_helper(T* obj, void (T::*f)(Args...) const) requires UniquePack<Args...>
    {
        auto at = archetype::of<Args...>();
        for (auto&& root : chunks) {
            if (!at.subset(root->archetype)) {
                for (auto&& chunk : root->chunks) {
                    for (uint32_t i{ 0 }; i < chunk->size(); i++) {
                        (obj->*f)(*chunk->template get<std::decay_t<Args>>(
                          std::forward<uint32_t>(i))...);
                    }
                }
            }
        }
    }
};
}
