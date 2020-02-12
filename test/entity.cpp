#include "util/catch.cpp"


TEST_CASE("entity_batch") {
    const size_t N = 10;
    auto at = realm::archetype::of<pos, vel, name>();
    auto world = realm::world{N};

    REQUIRE(world.size() == 0);
    auto entts = world.batch<pos, vel, name>(N);
	REQUIRE(world.size() == N);

	for (auto entt : entts) { 
		REQUIRE(world.get_archetype(entt) == at);
	}
}


TEST_CASE("entity_transfer")
{
    const size_t VX = 200;
    auto at = realm::archetype::of<pos, vel, name>();
    auto world = realm::world{};
    auto entt = world.create(at);

    world.get<vel>(entt).x = VX;
    REQUIRE(world.get<vel>(entt).x == VX);

    world.remove<pos, name>(entt);
    REQUIRE_FALSE(world.has<pos, name>(entt));
    REQUIRE(world.get<vel>(entt).x == VX);

    world.add<pos>(entt);
    REQUIRE(world.has<pos>(entt));
    REQUIRE(world.get<vel>(entt).x == VX);
    REQUIRE(world.chunks.size() == 3);
}

TEST_CASE("entity_remove")
{
    auto at = realm::archetype::of<pos, vel, name>();
    auto world = realm::world{};
    
    world.batch(1000, at);

    REQUIRE(world.size() == 1000);

    world.destroy(10);

    REQUIRE(world.size() == 999);


}