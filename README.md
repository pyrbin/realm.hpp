<h1 align="center">realm.hpp</h1>
<p align="center">(WIP) An <b>experimental</b> header-only ECS framework. Written in C++20/17.</p>

### Table of Contents

- [Introduction](#introduction)
- [Features](#features)
- [API](#api)
- [TODO](#todo)
- [Thanks](#thanks)
- [Developer](#developer)

## Introduction
Goal of this project is to create a simple ECS framework in C++20 to increase 
my knowledge of modern C++/C++ toolchains, DoD patterns, concurrent programming and general game dev. 
This is an archetype-based ECS taking inspiration from 
[Unity DOTS](https://unity.com/dots) and smaller frameworks like
[legion](https://github.com/TomGillen/legion), 
[hecs](https://github.com/robertlong/hecs),
[decs](https://github.com/vblanco20-1/decs) & 
[entt](https://github.com/skypjack/entt). 

The main focus is a clean & minimal API with acceptable performance, 
perfect aligned contiguous memory of components & parallelization of systems.

## Features
* ~~Using C++20 Concepts for a more implicit & non error-prone API~~
    * ~~Eg. `component::of<vel&, vel>` would not compile.~~
    * Concept features not used atm due to problems with IntelliSense on VStudio w/ cmake.
    
## API

```c++
// components are just structs
struct pos {
    int x, y = 0;
};

struct vel {
    int x, y = 0;
};

// create a world of capacity 10000
auto world = realm::world(10000);

// an entity is just a 64-bit integer
auto entt = world.create<vel, pos>();

// can also create an entity with an archetype
auto at = realm::archetype::of<vel, pos>();
auto entt = world.create(at);

// if you rather want to use something more OOP-friendly
// there is an wrapper-class actor that contains functions
// for get/add/remove components.
auto actor = new realm::actor(entt, &world);

// use the type system for mutable/immutable fetch of comps
auto& p_write = world.get<pos>(entt);
auto& p_read = world.get<const pos>(entt);

// query the world using only a lamdba where each argument is 
// an component you want to fetch

realm::query(&world, [](pos&, const vel&) {
    pos.x += vel.x;
    pos.y += vel.y;
});

// Add an realm::entity type to include the current entity
realm::query(&world, [](pos&, const vel&, realm::entity entt) {
    std::cout << "querying entity: " << entt << "\n";
});


```

## TODO
* Add todo
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
        <img width="150" height="150" src="https://github.com/pyrbin.png?s=150">
        <br>
        <a href="https://github.com/pyrbin">pyrbin</a>
      </td>
     </tr>
  </tbody>
</table>
