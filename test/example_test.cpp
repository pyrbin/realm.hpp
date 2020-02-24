#include "../include/core/world.hpp"
#include "util/catch.cpp"
#include "util/components.hpp"

struct runtime
{
    float dt{ 2 };
};

struct move_system
{
    void update(pos& p, vel& v, const runtime& r) const
    { 
        p.x += v.x * r.dt;
    }
};

inline const size_t N = 10;
inline const size_t UPDATE_N = 10;

inline const float Arg1 = 20.0;
inline realm::world world{ N };

TEST_CASE("simple_example")
{
    world.insert<move_system>();
    world.singleton<runtime>();

    auto entt = world.create<pos, vel>();
    world.set<vel>(entt, { 115, 115 });

    for (int i{ 0 }; i < UPDATE_N; i++) {
        world.update();
    }

    REQUIRE(world.get<const pos>(entt).x == (115 * 2) * UPDATE_N);
}
