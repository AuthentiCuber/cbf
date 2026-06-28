# CBF

A Brainfuck inerpreter written in C.
Originally part of [brainfuckery](https://github.com/AuthentiCuber/brainfuckery)

## Quick Start

### Building

At this stage there are no dependencies or special flags needed, so just compile with your favorite compiler and flags e.g.

    $ cc main.c -o cbf

### Usage

Currently there are two modes, determined by the `--literal` (or `-l` for short) flag.
When present, the following positional argument is treated as literal bf code and fed to the interpreter:

    $ ./cbf --literal "++++++++[>++++++++<-]>."
      H

Otherwise, the sole argument is a path to a `.bf` file which is read and executed.

    $ ./cbf helloworld.bf
      Hello, World!
