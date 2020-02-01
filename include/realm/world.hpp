#pragma once

#include "archetype.hpp"
#include "concept.hpp"
#include "entity.hpp"
#include "system.hpp"
#include "unordered_map"
#include "vector"

namespace realm {
struct world
{
public:
    // todo: use hashmap for better lookup performance
    using chunks_t = std::vector<std::unique_ptr<archetype_chunk_parent>>;
    using entities_t = entity_pool;

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
        std::vector<entity> entts;
        for (uint32_t i{ 0 }; i < n; i++) { entts.push_back(create(at)); }
        return entts;
    }

    template<FetchComponent T>
    T& get(entity entt)
    {
        auto [index, ptr] = *entities.get(entt);
        return static_cast<T&>(*ptr->get<std::unwrap_ref_decay_t<T>>(index));
    }

    template<NotWrapped T, typename... Args>
    system_functor<T>* insert(Args&&... args)
    {
        auto system = new system_functor<T>(T{ std::forward<Args>(args)... });
        static_cast<system_functor<T>*>(system)->init(&T::update);
        return system;
    }

    // Functor objects
    template<typename T>
    void query(T&& f)
    {
        query_helper(&f, &std::unwrap_ref_decay_t<T>::operator());
    }

    int32_t size() const noexcept { return entities.size(); }
    int32_t capacity() const noexcept { return entities.capacity(); }

    // TODO: move to private
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

private:
    chunks_t chunks;
    entities_t entities;

    // https://stackoverflow.com/questions/55756181/use-lambda-to-modify-references-identified-by-a-packed-parameter
    template<typename T, typename... Args>
    void query_helper(T* obj, void (T::*f)(Args...) const)
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

    template<typename... Args>
    static inline std::vector<component>& build_comp_set(std::vector<component>& v)
    {
        (build_comp_helper<Args>(v), ...);
        return v;
    }
    template<typename T>
    static inline void build_comp_helper(std::vector<component>& v)
    {
        using unwrapped_t = std::unwrap_ref_decay_t<T>;
        v.push_back(component::of<unwrapped_t>());
    }
    template<Entity F>
    static inline void build_comp_helper(std::vector<component>& v)
    {}
};
} // namespace realm
