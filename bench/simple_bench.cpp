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

// TODO: Use a benchmark library/framework

struct movement_system
{
    void update(pos& p, const dir& d) const
    {
        p.x += d.x;
        p.y += d.y;
    }
};

struct comflab_system
{
    void update(wierd& comflab) const
    {
        comflab.thingy *= 1.000001f;
        comflab.mingy = !comflab.mingy;
        comflab.dingy++;
    }
};

const int N = 1000000;

inline realm::world world{ N };

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

    for (int i = 0; i < 10; i++) {
        timer timer;
        world.update();
        double elapsed = timer.elapsed();
        if (elapsed < min) { min = elapsed; }
    }

    std::cout << "[BENCH] Results: " << min << " seconds\n";
}

void
BENCH_CASE_UPDATE_PAR()
{

    double min = 99999999;

    std::cout << "[BENCH] Updating " << N << " entities in parallel "
              << "with 2 systems\n";

    for (int i = 0; i < 10; i++) {
        timer timer;
        world.update(std::execution::par_unseq);
        double elapsed = timer.elapsed();
        if (elapsed < min) { min = elapsed; }
    }

    std::cout << "[BENCH] Results: " << min << " seconds\n";
}

int
main()
{
    BENCH_CASE_1M();
    world.insert<movement_system>();
    world.insert<comflab_system>();
    BENCH_CASE_UPDATE_SIMPLE();
    BENCH_CASE_UPDATE_PAR();
}