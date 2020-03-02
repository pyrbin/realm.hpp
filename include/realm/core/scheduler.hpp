#pragma once

#include <execution>
#include <vector>

#include <realm/core/system.hpp>

namespace realm {
struct world;
struct scheduler;
/**
 * @brief Execution block
 * Describes an execution block of systems_.
 * Each system in a block shares write access to components
 * and has to be executed in order.
 */
struct execution_block
{
    friend struct scheduler;

    using ptr = execution_block*;
    using list = std::vector<execution_block>;
    using sys_list = std::vector<system_ref*>;

    u64 component_mask;

    execution_block(u64 mask, sys_list sys)
        : component_mask{ mask }
        , systems_{ sys }
    {}

    auto& get_system(u32 idx) const { return systems_.at(idx); }
    auto size() const { return systems_.size(); }

    /**
     * Execute each system on a world
     * @param world The world to operate on
     */
    void exec(world* world) const noexcept
    {
        for (auto& sys : systems_) {
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
        for (auto& sys : systems_) {
            sys->invoke_seq(world);
        }
    }

private:
    sys_list systems_;
};

/**
 * @brief Scheduler
 * Handles execution of systems to guarantee safe multi-threaded executions on components.
 * If two systems share write access to a component they will be bundled into
 * the same execution block.
 */
struct scheduler
{
    scheduler()
    {
        // Create the readonly execution block
        // will always be the first block in the vector
        blocks_.push_back({ 0, {} });
    }

    ~scheduler()
    {
        for (auto& block : blocks_) {
            for (int i{ 0 }; i < block.systems_.size(); i++) {
                delete block.systems_[i];
                block.systems_[i] = nullptr;
            }
        }
    }

    /**
     * Insert & create a system into the scheduler with provided arguments.
     * @tparam T
     * @tparam Args
     * @param args
     */
    template<typename T, typename... Args>
    void insert(Args&&... args)
    {
        insert(T{ std::forward<Args>(args)... });
    }

    /**
     * Insert a system into the scheduler.
     * @tparam T
     * @param t
     */
    template<typename T>
    void insert(T&& t)
    {
        auto ref = new system_proxy<T>(std::forward<T>(t));
        count_++;

        // If system doesn't mutate any component
        // add to readonly execution block
        if (ref->meta.mut_mask == 0) {
            blocks_[0].systems_.push_back(ref);
            return;
        }

        execution_block::ptr execution_block{ nullptr };

        // Skip first block as its the readonly block
        for (auto i{ 1 }; i < blocks_.size(); i++) {
            auto& block = blocks_[i];
            // If system matches this block
            if (archetype::intersection(ref->meta.mut_mask, block.component_mask)) {
                if (execution_block == nullptr) {
                    execution_block = &block;
                } else {
                    // Merge execution blocks because a cross dependecy is found
                    // eg. if ref = Sys(A,B) & we have blocks A, B they have to be merged
                    execution_block->component_mask |= block.component_mask;
                    execution_block->systems_.insert(execution_block->systems_.end(),
                        block.systems_.begin(), block.systems_.end());
                    blocks_.erase(blocks_.begin() + i--);
                }
            }
        }

        if (execution_block != nullptr) {
            // If we found a suitable block insert system
            execution_block->systems_.push_back(ref);
            return;
        };

        // No matching blocks found, create new
        blocks_.push_back({ ref->meta.mut_mask, { ref } });
    }

    /**
     * Execute each block in parallel on provided world
     * @param world The world to operate on
     */
    void exec(world* world)
    {
        // Execute each block in parallel.
        // Blocks are guaranteed to have no write/read dependencies
        std::for_each(std::execution::par_unseq, blocks_.begin(), blocks_.end(),
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
        std::for_each(
            blocks_.begin(), blocks_.end(), [world](auto& block) { block.exec_seq(world); });
    }

    const auto& get_block(u32 idx) const noexcept { return blocks_.at(idx); }
    auto size() const noexcept { return count_; }
    auto blocks_size() const noexcept { return blocks_.size(); }

    void print_exec(std::ostream& os) const
    {
        os << "==== Execution block order ====\n";

        for (auto& block : blocks_) {
            os << "Execution Block: mask(" << block.component_mask << ")\n";
            for (auto& sys : block.systems_) {
                os << "Invoking Sys: " << sys->name << "\n";
            }
        }
    }

private:
    execution_block::list blocks_;
    /*! @brief System count */
    size_t count_{ 0 };
};
} // namespace realm