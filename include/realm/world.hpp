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
    using chunks_t = std::vector<archetype_chunk_parent*>;
    using entitites_t = entity_pool;

    entitites_t entities;
    chunks_t chunks;

    world(uint32_t capacity = 100) : entities{ capacity } {}

    entity create(const archetype& at)
    {
        auto chunk = get_chunk(at);
        std::cout << "after" << chunks.size() << "\n";
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
    archetype_chunk_parent::chunk_ptr get_chunk(const archetype& at)
    {
        auto it = std::find_if(
          chunks.begin(), chunks.end(), [at](auto& b) { return b->archetype == at; });
        archetype_chunk_parent* cp{ nullptr };
        if (it == chunks.end()) {
            cp = new archetype_chunk_parent{ at };
            chunks.push_back(cp);
        } else {
            cp = *it;
        }
        return cp->find_free();
    }
    // https://stackoverflow.com/questions/55756181/use-lambda-to-modify-references-identified-by-a-packed-parameter
    template<typename T, typename... Args>
    void query_helper(T* obj, void (T::*f)(Args...) const) requires UniquePack<Args...>
    {
        auto at = archetype::of<Args...>();
        std::cout << "sz" << chunks.size() << "\n";
        for (auto&& root : chunks) {
            std::cout << "true?" << at.subset(root->archetype) << "\n";

            if (!at.subset(root->archetype)) {
                std::cout << "in the zone0"
                          << "\n";
                std::cout << root->per_chunk << "\n";
                for (auto&& chunk : root->chunks) {
                    std::cout << "in the zone1"
                              << "\n";
                    for (uint32_t i{ 0 }; i < chunk->size(); i++) {
                        std::cout << "in the zone2"
                                  << "\n";
                        (obj->*f)(*chunk->template get<std::decay_t<Args>>(
                          std::forward<uint32_t>(i))...);
                    }
                }
            }
        }
    }
};
}
