#include "../include/realm.hpp"
#include <assert.h>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

struct alignas(4) pos
{
    int x, y{ 0 };
};

struct alignas(8) vel
{
    int x, y{ 0 };
};

struct alignas(16) id
{
    string id;
    unsigned char file_version[4];
    unsigned char options[2];
};

struct alignas(64) friends
{
    vector<string> list;
};

void
printf(realm::world& world, vector<realm::entity> entts)
{
    for (int i{ 0 }; i < world.capacity(); i++) {
        std::cout << "entity: " << entts[i] << "\n";
        std::cout << "vel: " << world.get<vel>(entts[i]).x << ", "
                  << world.get<vel>(entts[i]).y << "\n";
    }
}

struct update_test
{
    void update(const pos& pos) const { cout << "system: " << pos.x << "\n"; }
    void operator()() {}
};

int
main()
{
    auto arch = realm::archetype::of<pos, vel>();

    assert(arch.count() == 2);
    assert(arch.has<pos>());

    auto wo = realm::world{ 5 };
    auto et = wo.create(arch);

    auto& pc = wo.get<const pos>(et);

    auto& pm = wo.get<pos>(et);
    pm.x = 200;
    assert(wo.get<const pos>(et).x == 200);

    auto q = realm::query<const pos, const vel>();

    wo.fetch([](pos& p, pos p1) {

    });

    wo.fetch(pm);
}