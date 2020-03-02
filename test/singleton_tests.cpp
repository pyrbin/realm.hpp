#include "util/catch.cpp"

struct runtime_s
{
    double dt{ 1000 };
};

struct alignas(64) keyboard_mapping_s
{
    keyboard_mapping_s() { std::fill(size, size + 1023, 'r'); };
    char size[1024]{};
};

struct big_s
{
    char size[16 * 1000]{};
};

struct example_system
{
    void update(name&, pos&, const runtime_s& r) const {}
};

struct example_view_system
{
    void update(realm::view<name, pos, const keyboard_mapping_s> v) const
    {
        for (auto [n, p, r] : v) {
        }
    }
};

inline const size_t N = 10;
inline const float Arg1 = 20.0;
inline realm::world world{ N };

TEST_CASE("system_insert")
{
    world.singleton<keyboard_mapping_s>();
    world.singleton<runtime_s>();
    world.get_singleton<runtime_s>().dt = 1;

    world.create<pos, name, big_s>();

    REQUIRE(world.get_singleton<keyboard_mapping_s>().size[29] == 'r');
    REQUIRE(world.get_singleton<runtime_s>().dt == 1);

    world.insert<example_system>();
    world.insert<example_view_system>();

    world.update();
}