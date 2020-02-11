#include <chrono>
#include <functional>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "../include/realm.hpp"
#include "util/components.hpp"
#include "util/timer.hpp"

struct movement_system
{
    realm::world* world;

    movement_system(realm::world* world) : world{ world } {}

    void update(double dt)
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

    void update(double dt)
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
game_update(double dt)
{
    comf_sys.update(dt);
    move_sys.update(dt);
}

void
BENCH_CASE_1M()
{
    std::cout << "[BENCH] Constructing " << N << " entities"
              << "\n";
    timer timer;

    world.batch<pos, dir, wierd>(N);
    // for (int i = 0; i < N; i++) { world.create<pos, dir, wierd>(); }
    std::cout << "[BENCH] Results: " << timer.elapsed() << " seconds\n";
}

void
BENCH_CASE_UPDATE_SIMPLE()
{
    double min = 99999999;

    std::cout << "[BENCH] Updating " << N << " entities "
              << "with 2 systems\n";

    for (int i = 0; i < 50; i++) {
        timer timer;
        game_update(1.0);
        double elapsed = timer.elapsed();
        if (elapsed < min) { min = elapsed; }
    }

    std::cout << "[BENCH] Results: " << min << " seconds\n";
}

int
main()
{
    BENCH_CASE_1M();
    BENCH_CASE_UPDATE_SIMPLE();
}