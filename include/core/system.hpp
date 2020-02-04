
#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

namespace realm {

struct system_base
{
public:
    system_base(){};
    ~system_base(){};

    virtual bool compare(size_t hash) const = 0;
    virtual void operator()(archetype_chunk*) const = 0;

protected:
    template<typename T>
    using system_ptr = std::unique_ptr<T>;

    template<typename... Args>
    using mod_ptr_t = std::function<void(Args...)>;

    using fn_ptr = std::function<void(archetype_chunk*)>;
};

template<typename T>
struct system_functor : public system_base
{
public:
    system_functor(T&& s) : sys{ std::move(std::make_unique<T>(std::move(s))) } {}

    template<typename R = void, typename... Args>
    void init(R (T::*f)(Args...) const)
    {
        archetype = archetype::of<Args...>();
        invoke = [this, f](archetype_chunk* chunk) -> void {
            auto inner_update = [this, f](Args... args) -> void {

            };
        };
    }

    bool compare(size_t hash) const override { return archetype.subset(hash); }
    void operator()(archetype_chunk* chunk) const override { invoke(chunk); }

private:
    struct archetype archetype;
    system_ptr<T> sys;
    fn_ptr invoke;
};
} // namespace realm