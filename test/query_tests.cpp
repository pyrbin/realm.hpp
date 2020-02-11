#include "util/catch.cpp"

TEST_CASE("query_read")
{
    const size_t N = 10;
    auto at = realm::archetype::of<pos, vel, name>();
    auto world = realm::world{ N };

    for (int i{ 0 }; i < N; i++) {
        auto entt = world.create(at);
        world.get<pos>(entt).x = i;
    }

    int i{ 0 };


    realm::query(&world, [&](const pos& p) {
        REQUIRE(p.x == i++);
    });

    REQUIRE(i == N);

}