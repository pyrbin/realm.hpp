#include "util/catch.cpp"

// TODO: fix realm::entity queries & readd tests
/* TEST_CASE("query_read")
{
    const size_t N = 10;
    auto at = realm::archetype::of<pos, vel, name>();
    auto world = realm::world{ N };

    for (int i{ 0 }; i < N; i++) {
        auto entt = world.create(at);
        world.get<pos>(entt).x = i;
    }

    int i{ 0 };

    realm::query(&world, [&](realm::entity entt, const pos& p) { REQUIRE(p.x == i++); });

    REQUIRE(i == N);
}

TEST_CASE("query_par")
{
    const size_t N = 10;
    auto at = realm::archetype::of<pos, vel, name>();
    auto world = realm::world{ N };

    for (int i{ 0 }; i < N; i++) {
        auto entt = world.create(at);
        world.get<pos>(entt).x = i;
    }

    int i{ 0 };

    realm::query_par(&world,
                     [&](realm::entity entt, const pos& p) { REQUIRE(p.x == i++); });

    REQUIRE(i == N);
}
*/

TEST_CASE("query_with_view")
{
    const size_t N = 10;
    auto at = realm::archetype::of<pos, vel, name>();
    auto world = realm::world{ N };

    for (int i{ 0 }; i < N; i++) {
        auto entt = world.create(at);
        world.get<pos>(entt).x = i;
    }

    realm::query(&world, [](realm::view<pos, vel> view) {
        for (auto [p, v] : view) { p.x += 20; }
    });
}