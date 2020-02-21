#include "util/catch.cpp"

struct example_system
{
    const float value;

    example_system(float value) : value{ value } {};

    void update(vel& v, const pos&, realm::entity) const
    {
        v.x += value;
        v.y += value;
        v.z += value;
    }
};

struct example_view_system
{
    void update(realm::view<pos, const vel> view) const
    {
        for (auto& [p, v] : view) {
            p.x += v.x;
            p.y += v.y;
            p.z += v.z;
        }
    }
};

inline const size_t N = 10;
inline const float Arg1 = 20.0;
inline realm::world world{ N };

TEST_CASE("system_insert")
{
    auto at = realm::archetype::of<pos, vel, name>();

    world.batch<pos, vel>(N);

    // Update order
    world.insert(example_view_system{});
    world.insert<example_system>(Arg1);

    REQUIRE(world.system_count() == 2);
}

TEST_CASE("system_update")
{
    world.update();

    REQUIRE(world.get<const vel>(0).x == Arg1);

    world.update();

    REQUIRE(world.get<const pos>(N / 2).x == Arg1);
}