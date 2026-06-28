# CBF

A Brainfuck inerpreter written in C.
Originally part of [brainfuckery](https://github.com/AuthentiCuber/brainfuckery)

## Quick Start

### Building

Just compile with your favorite compiler and flags e.g.

    $ cc main.c -o cbf

The only thing to note is that you can set a custom memory size with the MEM_SIZE macro:

    $ cc -D MEM_SIZE=100000 main.c -o cbf

### Usage

A summary can be obtained with

    $ ./cbf --help

A file containting brainfuck code can be run with the `run` subcommand:

    $ ./cbf run helloworld.bf
    Hello, World!

Support is planned for and interactive repl with the `repl` subcommand, which is yet to be implemented.
