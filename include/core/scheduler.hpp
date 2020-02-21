#pragma once

#include "../util/type_traits.hpp"
#include "system.hpp"

#include <vector>

namespace realm {

struct world;

struct execution_block
{
    size_t component_mask;
    std::vector<system_ref*> systems;

    inline void exec(world* world) const noexcept
    {
        for (auto& sys : systems) {
            sys->invoke(world);
        }
    }

    inline void exec_seq(world* world) const noexcept
    {
        for (auto& sys : systems) { sys->invoke_seq(world); }
    }
};

struct scheduler
{
    std::vector<execution_block> blocks;
    size_t system_count{ 0 };

    inline scheduler()
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

    template<typename T, typename... Args>
    inline void insert(Args&&... args)
    {
        insert(T{ std::forward<Args>(args)... });
    }

    template<typename T>
    inline void insert(T& t)
    {
        auto ref = new system_proxy<T>(std::move(t));
        system_count++;

        // If system doesnt mutate any component
        // add to readonly execution block
        if (ref->meta.mut_mask == 0)
        {
            blocks[0].systems.push_back(ref);
            return;
        }

        execution_block* curr{ nullptr };

        // Skip first block as its the readonly block
        for (int i{ 1 }; i < blocks.size(); i++) {
            auto& block = blocks[i];
            // If system matches this block
            if (archetype::intersection(ref->meta.mut_mask, block.component_mask)) {
                if (curr == nullptr) {
                    curr = &block;
                } else {
                    // Merge execution blocks because a cross dependecy is found
                    // eg. if ref = Sys(A,B) & we have blocks A, B they have to be merged
                    curr->component_mask |= block.component_mask;
                    curr->systems.insert(
                      curr->systems.end(), block.systems.begin(), block.systems.end());
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

    inline void exec(world* world)
    {
        // Execute each block in parallel. 
        // Blocks are guaranteed to have no write/read dependencies
        std::for_each(std::execution::par_unseq,
                      blocks.begin(),
                      blocks.end(),
                      [world](auto& block) { block.exec(world); });
    }

    inline void exec_seq(world* world)
    {
        // Execute each block in parallel.
        // Blocks are guaranteed to have no write/read dependencies
        std::for_each(blocks.begin(),
                      blocks.end(),
                      [world](auto& block) { block.exec_seq(world); });
    }

    void print_exec(std::ostream& os)
    {
        os << "==== Execution block order ====\n";

        for (auto& block : blocks) {
            os << "Execution Block: mask(" << block.component_mask << ")\n";
            for (auto& sys : block.systems) {
                os << "Invoking Sys: " << sys->name << "\n";
            }
        }
    }

    inline size_t size() const noexcept { return system_count; }
};

} // namespace realm