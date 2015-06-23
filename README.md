pgconvert
=========

Tool to reduce parity games.

Prerequisites
-------------

Compilation of pgconvert requires the boost regex library to be available.

Installation
------------

After checking out the git repository, make sure to also update the submodules (for commandline and logging libraries) using the following command (note the argument `--recursive`)

    git submodule update --init --recursive

To build the tool, configure and build using

    cmake
    make

If you want to install the tool (by default to `/usr/local`)

Usage
-----

The tool takes parity games in [PGSolver](https://github.com/tcsprojects/pgsolver) format, and allows you to use several reductions. The following reductions are supported:

* `-escc` reduction of the game by identifying strongly connected components of equivalent vertices,
* `-ebisim` reduction of the game using strong bisimulation,
* `-efmib` reduction of the game using forced-move identifying bisimulation, also referred to as governed bisimulation, or idempotence-identifying bisimulation if it concerns Boolean equation systems.
* `-estut` reduction using stuttering equivalence,
* `-egstut` reduction using governed stuttering equivalence,
* `-egstut2` reduction by first identifying sccs (`-escc`) and then using governed stuttering equivalence reduction, and
* `-ewgstut` reduction using weak stuttering equivalence reduction, this reduction is experimental, and most likely unsound.

Additionally, the tool supports the following options:

* `--timings[=FILE]` append timing measurements to FILE. Measurements are written to standard error if no FILE is provided
* `-q, --quiet` do not display warning messages
* `-v, --verbose` display short intermediate messages
* `-d, --debug` display detailed intermediate messages
* `--log-level=LEVEL` display intermediate messages up to and including LEVEL
* `-h, --help` display help informatixon
* `--version` display version information

For example, to reduce the parity game in the file `example.gm` using governed stuttering equivalence and store the result in `reduced.gm`, execute the following command:

    pgsolver -egstut example.gm reduced.gm
