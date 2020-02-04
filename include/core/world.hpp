#pragma once

#include "archetype.hpp"
#include "concepts.hpp"
#include "entity.hpp"
#include "system.hpp"
#include <vector>

namespace realm {

namespace detail {
/**
 * TODO: anyway to use world's fetch_helper function in world::fetch's
 * require clause?
 */
template<typename T, typename... Args>
void
___fetch_helper_d(T* obj, void (T::*f)(Args...) const) requires realm::FetchPack<Args...>;

} // namespace detail

struct world
{
    // todo: use hashmap for better lookup performance
    using chunks_t = std::vector<std::unique_ptr<archetype_chunk_parent>>;
    using entities_t = entity_pool;

private:
    chunks_t chunks;
    entities_t entities;

    template<typename T, typename... Args>
    void fetch_helper(T* obj, void (T::*f)(Args...) const) requires FetchPack<Args...>
    {
        std::vector<component> vec;


        build_comp_set<Args...>(vec);

        archetype at{ vec };
        std::cout << at.size() << " query helper size\n";
        for (auto&& root : chunks) {
            if (at.subset(root->archetype)) {
                for (auto&& chunk : root->chunks) {
                    std::cout << chunk->size() << "ok?";
                    for (uint32_t i{ 0 }; i < chunk->size(); i++) {
                        (obj->*f)(*chunk->template get<std::decay_t<Args>>(
                          std::forward<uint32_t>(i))...);
                    }
                }
            }
        }
    }

    archetype_chunk* get_chunk(const archetype& at)
    {
        std::cout << at.components.size() << "<-- hmmm \n";
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

public:
    world(uint32_t capacity = 100) : entities{ capacity } {}

    entity create(const archetype& at)
    {
        auto chunk = get_chunk(at);
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
    requires requires(T&& f)
    {
        detail::___fetch_helper_d(&f, &T::operator());
    }
    void fetch(T&& f) { this->fetch_helper(&f, &T::operator()); }

    int32_t size() const noexcept { return entities.size(); }
    int32_t capacity() const noexcept { return entities.capacity(); }
};

} // namespace realm
