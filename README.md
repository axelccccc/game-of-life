# game-of-life

Simple command-line adaptation of [Conway's Game of Life](https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life) cellular automaton in C++.

It supports using custom seeds as .txt files and uses multithreading to speed up computation.

Compilation
===========

> This version uses `<getopt.h>` to parse arguments which is a UNIX system header.
> It thus won't compile on Windows, but will probably get changed in the future.

Using CMake : 
```bash
mkdir build && cd build && cmake .. && make
```

Or just compile the source with C++11 and linking your OS' thread library.

Usage
=====

```
game-of-life [-p <particle>] [-a <alignment>] <file.txt>
```

`-p` : Character to be used as cell graphics
`-a` : Alignment of seed file in display (center, top-left, bottom-right, ...)
file.txt : seed file

> Some example seed files are included in the `assets` folder