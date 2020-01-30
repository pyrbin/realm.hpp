#include "../include/realm.hpp"
#include <iostream>

using namespace realm;
using namespace std;

struct pos
{
    int x, y = 0;
};

struct vel
{
    int x, y = 0;
};

int
main()
{
    const size_t n = 20;

    entity_pool entities{ n };
    archetype_chunk chunk1{ archetype::of<pos, vel>() };
    chunk1.allocate(n / 2);

    for (int i = 0; i < 10; i++) {
        auto entt = chunk1.acquire([&](uint idx) {
            return entities.create({ idx, &chunk1 });
        });
        entities.get(entt)->chunk->get<vel>(entities.get(entt)->chunk_index)->x = i;
        std::cout
          << entities.get(entt)->chunk->get<vel>(entities.get(entt)->chunk_index)->x
          << "\n";
    }
}