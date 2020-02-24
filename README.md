<h1 align="center">realm.hpp</h1>
<p align="center">An <b>WIP</b>, <b>experimental</b> header-only ECS framework. Written in C++20/17.</p>

### Table of Contents

- [Introduction](#introduction)
- [API](#api)
  - [Example](###example)
- [Benchmarks](#benchmarks)
- [TODO](#todo)
- [Thanks](#thanks)

## Introduction
Goal of this project is to create a simple ECS (Entity Component System) framework in `C++20/17` to increase
my knowledge of modern C++/C++ toolchains, DoD patterns, parallelization and general game dev.
This is an archetype-based ECS taking inspiration from 
[Unity DOTS](https://unity.com/dots) and smaller frameworks like
[legion](https://github.com/TomGillen/legion), 
[hecs](https://github.com/robertlong/hecs),
[decs](https://github.com/vblanco20-1/decs) & 
[entt](https://github.com/skypjack/entt). 

The main focus is a clean & minimal API with acceptable performance, 
perfect aligned contiguous memory of components & easy parallelization of systems.

Project is currently very WIP and should not be used in anything serious.

Requires a C++20 compatible compiler.   

## API

### Components
Components are just simple structs of data. The only requirement is that they have to be default, move & copy constructible.
```c++
struct pos
{
    float x{0};
    float y{0};
}
```

### Systems
Systems are defined by their update function where each argument is a 
component + access (const or not) they want to fetch.
```c++
struct example_system
{
    // Per entity
    void update(pos& p, const vel& v) const {
        // do system logic
    }
}

struct example_system_perchunk
{
    // You can use a single realm::view as an argument
    // to access components on a per-chunk basis
    void update(realm::view<pos, vel> view) const {
        for(auto [p, v] : view) {
            // do system logic
        }
    }
}

// Insert systems to a world
world.insert<example_system>();
world.insert<example_per_chunk>();
```

### Example
```c++
struct pos
{
    float x{0};
    float y{0};
}

struct time
{
    float dt{1/60};
}

struct move_system
{
    void update(pos& p, const vel& v, const time& t) const {
        p.x += v.x * t.dt;
        p.y += v.y * t.dt;
    }
}


// Create a world with capacity of 1000 entities.
auto world = realm::world(1000);

// Create a singleton component
world.singleton<time>();

// Create entities of components pos & vel.
for(auto i{0}; i < 1000; ++i) {
    auto entt = world.create<pos, vel>();
    world.set<vel>(entt, {10, 10});
}

// Insert move_system to the world
world.insert<move_system>();

// Update systems in parallel
world.update();

```

## Benchmarks
TODO

## TODO
* See `TODO` file

## Thanks
Thanks to the guides @[skypjack on software]() for helping me 
understand the various concepts of an ECS implementation.

Videos & talks:
* [Game Engine API Design](https://www.youtube.com/watch?v=W3ViIBnTTKA)
* [Overwatch ECS](https://www.youtube.com/watch?v=W3aieHjyNvw)
* [Data Driven ECS in C++17](https://www.youtube.com/watch?v=tONOW7Luln8)

## Developer

<table>
  <tbody>
    <tr>
      <td align="center" valign="top">
        <img width="100" height="100" src="https://github.com/pyrbin.png?s=150">
        <br>
        <a href="https://github.com/pyrbin">pyrbin</a>
      </td>
     </tr>
  </tbody>
</table>
