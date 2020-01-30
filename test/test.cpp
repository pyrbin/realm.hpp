#include "../include/realm.hpp"
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
    unsigned char header_length[2];
    unsigned char state_info_length[2];
    unsigned char base_info_length[2];
    unsigned char base_pos[2];
    unsigned char key_parts[2];
    unsigned char unique_key_parts[2];
    unsigned char keys;
    unsigned char uniques;
    unsigned char language;
    unsigned char max_block_size_index;
    unsigned char fulltext_keys;
    unsigned char not_used;
};

struct alignas(64) friends
{
    vector<string> list;
};

void
printf(realm::world& world, vector<realm::entity_t> entts)
{
    for (int i{ 0 }; i < world.capacity(); i++) {
        std::cout << "entity: " << entts[i] << "\n";
        std::cout << "vel: " << world.get<vel>(entts[i]).x << ", "
                  << world.get<vel>(entts[i]).y << "\n";
    }
}

int
main()
{
    auto world = realm::world{ 1 };
    auto entts = world.batch<pos, vel>(world.capacity());
    printf(world, entts);
    // for (int i{ 0 }; i < world.capacity(); i++) { world.get<vel>(entts[i]) = { 20, 20
    // }; } printf(world, entts);
    return 0;
}