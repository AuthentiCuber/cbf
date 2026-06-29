# CBF

A Brainfuck inerpreter written in C.
Originally part of [brainfuckery](https://github.com/AuthentiCuber/brainfuckery)

## Quick Start

Build with

    $ make

This will create an executable called `cbf`
A summary of usage can be obtained with

    $ ./cbf --help

A file containting brainfuck code can be run with the `run` subcommand:

    $ ./cbf run helloworld.bf
    Hello, World!

You can enter an interactive repl with the `repl` subcommand

    $ ./cbf repl
    cbf: a simple interactive brainfuck interpreter
    Type `exit` or CTRL-D to exit
    bf> +++++++++[>++++++++<-]>.
    H
    bf> +.
    I
    bf> exit

