<h1 align="center">realm.hpp</h1>
<p align="center">(WIP) An <b>experimental</b> header-only ECS framework. Written in C++20/17.</p>

### Table of Contents

- [Introduction](#introduction)
- [Features](#features)
- [API](#api)
- [Benchmark](#benchmark)
- [TODO](#todo)
- [Thanks](#thanks)
- [Developer](#developer)

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
perfect aligned contiguous memory of components & easy parallelization of systems (compatible with STL algorithms).

## Features
* ~~Using C++20 Concepts for a more implicit & non error-prone API~~
    * ~~Eg. `component::of<vel&, vel>` would not compile.~~
    * Concept features not used atm due to problems with IntelliSense on VStudio w/ cmake.
    
## API

```c++

struct example_system {
    void update(pos& p, const vel& v) const {
        p.x += v.x;
        p.y += v.y;
    }

    void update(realm::entity e, realm::world& w) const {

    }
    
    void update(realm::view<pos, vel> view) const {
        for(auto& [p, v] : view) {
            p.x += v.x;
            p.y += v.y;
        }
    }
}


```
## Benchmark
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
