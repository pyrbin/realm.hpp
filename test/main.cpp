#include "../include/realm.hpp"

#include <chrono>
#include <functional>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

struct timer final
{
    timer() : start{ std::chrono::system_clock::now() } {}

    void elapsed()
    {
        auto now = std::chrono::system_clock::now();
        std::cout << std::chrono::duration<double>(now - start).count() << " seconds"
                  << std::endl;
    }

private:
    std::chrono::time_point<std::chrono::system_clock> start;
};

struct pos
{
    float x = 0.0f;
    float y = 0.0f;
};

struct dir
{
    float x = 4.0f;
    float y = 0.0f;
};

struct wierd
{
    float thingy = 0.0;
    int dingy = 0;
    bool mingy = false;
    std::string stringy;
};

using timedelta = double;

struct movement_system
{
    realm::world* world;

    movement_system(realm::world* world) : world{ world } {}

    void update(timedelta dt)
    {
        realm::query(world, [dt](pos& p, const dir& d) {
            p.x += d.x * dt;
            p.y += d.y * dt;
        });
    }
};

struct comflab_system
{
    realm::world* world;

    comflab_system(realm::world* world) : world{ world } {}

    void update(timedelta dt)
    {
        realm::query(world, [dt](wierd& comflab) {
            comflab.thingy *= 1.000001f;
            comflab.mingy = !comflab.mingy;
            comflab.dingy++;
        });
    }
};

const int N = 1000000;

inline realm::world world{ N };
inline comflab_system comf_sys{ &world };
inline movement_system move_sys{ &world };

inline void
game_update(timedelta dt)
{
    comf_sys.update(dt);
    move_sys.update(dt);
}

void
TEST_CASE_1M()
{
    std::cout << "Constructing " << N << " entities"
              << "\n";
    timer timer;
    world.batch<pos, dir>(N);
    timer.elapsed();
}

void
TEST_CASE_UPDATE()
{
    std::cout << "Updating " << N << " entities "
              << "with 2 systems\n";
    timer timer;
    game_update(1.0);
    timer.elapsed();
}

int
main()
{
    TEST_CASE_1M();
    TEST_CASE_UPDATE();
}