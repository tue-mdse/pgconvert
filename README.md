pgconvert
=========

Tool to reduce parity games.

Installation
------------

To build the tool an installation of mCRL2 is required. Assuming that mCRL2
has been installed to `/path/to/mcrl2`, assuming the standard layout of the
mCRL2 installation, configure and install using

    cmake -DMCRL2_INCLUDE=/path/to/mcrl2/include -DMCRL2_LIB=/path/to/mcrl2/lib/mcrl2
    make

Usage
-----

The tool allows you to use the following reductions:

* `-escc` reduction of the game by identifying strongly connected components of equivalent vertices,
* `-ebisim` reduction of the game using strong bisimulation,
* `-efmib` reduction of the game using forced-move identifying bisimulation, also referred to as governed bisimulation, or idempotence-identifying bisimulation if it concerns Boolean equation systems.
* `-estut` reduction using stuttering equivalence,
* `-egstut` reduction using governed stuttering equivalence,
* `-egstut2` reduction by first identifying sccs (`-escc`) and then using governed stuttering equivalence reduction, and
* `-ewgstut` reduction using weak stuttering equivalence reduction, this reduction is experimental, and most likely unsound.

