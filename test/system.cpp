#include "util/catch.cpp"

struct example_system
{
    const float value;

    example_system(float value) : value{ value } {};

    void update(realm::entity entt, pos& p) const {
        p.x += value + entt;
        p.y += value + entt;
        p.z += value + entt;
    }
};

inline const size_t N = 10;
inline const float Arg1 = 20.0;
inline realm::world world{ N };

TEST_CASE("system_insert")
{
    auto at = realm::archetype::of<pos, vel, name>();

    world.batch<pos, vel>(N);
    auto sys = world.insert<example_system>((float) Arg1);

    REQUIRE(sys->archetype.count() == 1);
    REQUIRE(world.system_count() == 1);

}

TEST_CASE("system_update")
{
    world.update();
    REQUIRE(world.get<pos>(0).x == Arg1 + 0);

    world.update();
    REQUIRE(world.get<pos>(N-1).x == Arg1 * 2 + ((N-1)*2));
}