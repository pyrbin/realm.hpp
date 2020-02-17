#include "util/catch.cpp"

struct example_system
{
    const float value;

    example_system(float value) : value{ value } {};

    void update(pos& p) const
    {
        p.x += value + 1;
        p.y += value + 1;
        p.z += value + 1;
    }
};

inline const size_t N = 10;
inline const float Arg1 = 20.0;
inline realm::world world{ N };

TEST_CASE("system_insert")
{
    auto at = realm::archetype::of<pos, vel, name>();

    world.batch<pos, vel>(N);
    auto sys = world.insert<example_system>(Arg1);

    REQUIRE(sys->compare(realm::archetype::mask_of<pos, vel>()));
    REQUIRE(world.system_count() == 1);
}

TEST_CASE("system_update")
{
    world.update();
    REQUIRE(world.get<pos>(2).x == Arg1 + 1);

    world.update();
    REQUIRE(world.get<pos>(N - 1).x == Arg1 * 2 + ((2 - 1) * 2));
}