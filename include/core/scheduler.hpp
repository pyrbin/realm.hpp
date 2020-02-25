#pragma once

#include "system.hpp"

#include <execution>
#include <vector>

namespace realm {

struct world;

/**
 * @brief Execution block
 * Describes an execution block of systems.
 * Each system in a block shares write access to components
 * and has to be executed in order.
 */
struct execution_block
{
    size_t component_mask;
    std::vector<system_ref*> systems;

    /**
     * Execute each system on a world
     * @param world The world to operate on
     */
    void exec(world* world) const noexcept
    {
        for (auto& sys : systems) {
            sys->invoke(world);
        }
    }

    /**
     * Execute each system on a world where the query
     * is not executed in parallel.
     * @param world
     */
    void exec_seq(world* world) const noexcept
    {
        for (auto& sys : systems) {
            sys->invoke_seq(world);
        }
    }
};

/**
 * @brief Scheduler
 * Handles execution of systems to guarantee safe multi-threaded executions on components.
 * If two systems share write access to a component they will be bundled into
 * the same execution block.
 */
struct scheduler
{
    std::vector<execution_block> blocks;
    size_t system_count{ 0 };

    scheduler()
    {
        // Create the readonly execution block
        // will always be the first block in the vector
        blocks.push_back({ 0, {} });
    }

    ~scheduler()
    {
        for (auto& block : blocks) {
            for (int i{ 0 }; i < block.systems.size(); i++) {
                delete block.systems[i];
                block.systems[i] = nullptr;
            }
        }
    }

    /**
     * Insert & create a system into the scheduler with provided arguments.
     * @tparam T
     * @tparam Args
     * @param args
     */
    template <typename T, typename... Args>
    void insert(Args&&... args)
    {
        insert(T{ std::forward<Args>(args)... });
    }

    /**
     * Insert a system into the scheduler.
     * @tparam T
     * @param t
     */
    template <typename T>
    void insert(T&& t)
    {
        auto ref = new system_proxy<T>(std::forward<T>(t));
        system_count++;

        // If system doesn't mutate any component
        // add to readonly execution block
        if (ref->meta.mut_mask == 0) {
            blocks[0].systems.push_back(ref);
            return;
        }

        execution_block* curr{ nullptr };

        // Skip first block as its the readonly block
        for (auto i{ 1 }; i < blocks.size(); i++) {
            auto& block = blocks[i];
            // If system matches this block
            if (archetype::intersection(ref->meta.mut_mask, block.component_mask)) {
                if (curr == nullptr) {
                    curr = &block;
                } else {
                    // Merge execution blocks because a cross dependecy is found
                    // eg. if ref = Sys(A,B) & we have blocks A, B they have to be merged
                    curr->component_mask |= block.component_mask;
                    curr->systems.insert(curr->systems.end(), block.systems.begin(),
                                         block.systems.end());
                    blocks.erase(blocks.begin() + i--);
                }
            }
        }

        if (curr != nullptr) {
            // If we found a suitable block insert system
            curr->systems.push_back(ref);
            return;
        };

        // No matching blocks found, create new
        blocks.push_back({ ref->meta.mut_mask, { ref } });
    }

    /**
     * Execute each block in parallel on provided world
     * @param world The world to operate on
     */
    void exec(world* world)
    {
        // Execute each block in parallel.
        // Blocks are guaranteed to have no write/read dependencies
        std::for_each(std::execution::par_unseq, blocks.begin(), blocks.end(),
                      [world](auto& block) { block.exec(world); });
    }

    /**
     * Execute each block sequentially (not parallel) on provided world
     * @param world The world to operate on
     */
    void exec_seq(world* world)
    {
        // Execute each block in parallel.
        // Blocks are guaranteed to have no write/read dependencies
        std::for_each(blocks.begin(), blocks.end(),
                      [world](auto& block) { block.exec_seq(world); });
    }

    [[nodiscard]] size_t size() const noexcept
    {
        return system_count;
    }
};

}  // namespace realm