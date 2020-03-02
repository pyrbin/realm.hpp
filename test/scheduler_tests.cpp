#include "util/catch.cpp"

struct example_system
{
    const float value;

    example_system(float value)
        : value{ value } {};

    void update(vel& v, const pos&) const
    {
        v.x += value;
        v.y += value;
        v.z += value;
    }
};

struct example_system_const
{
    void update(const vel&, const pos&) const {}
};

struct example_system_mutate
{
    void update(vel&, pos&) const {}
};

struct example_system_name
{
    void update(name&) const {}
};

struct example_system_combined
{
    void update(vel&, name&, const pos&) const {}
};

inline realm::world world{ 10 };

TEST_CASE("scheduler_block_order")
{
    world.batch<pos, vel, name>(10);
    realm::scheduler scheduler;

    scheduler.insert<example_system>(static_cast<float>(20));
    scheduler.insert<example_system_const>();
    scheduler.insert<example_system_mutate>();
    scheduler.insert<example_system_name>();

    REQUIRE(scheduler.get_block(0).size() >= 1);

    REQUIRE(scheduler.blocks_size() == 3);

    // scheduler.print_exec(std::cout);
    scheduler.insert<example_system_combined>();

    REQUIRE(scheduler.blocks_size() == 2);
    REQUIRE(scheduler.get_block(1).size() == 4);

    scheduler.exec(&world);
}