exolang
=======

Goal
----
The aim is to develop a jiting/compiling rapid prototyping "script" language.
It will be jit-compiled via LLVM.

Prerequisites
-------------
I develop on a Debian system. In order to install the prerequisites there, it's suffice to do:

apt-get install llvm llvm-dev libc++-dev libboost-all-dev libgc-dev gdb valgrind libunwind8-dev libedit-dev swig ninja-build

You will need atleast boost 1.54. Next call the bootstrap script:

./bootstrap.sh )

This will to build the lemon parser generator, fetch the quex lexer in order to rebuild the lexer/parser code and download the waf build tool.
The bootstrap script can also build a LLVM under ./bin, do do that use bootstrap as follows.

./bootstrap.sh -vkl
- -c : force cleanup (delete downloaded folders)
- -l : build llvm
- -v : verbose, print what the script is doing
- -k : keep, keep downloaded and temorary files

Building
--------
Next configure the build. For now since it's under heavy development:

./waf clean; ./waf configure --mode=debug --llvm=./bin/bin/llvm-config --gc=disable

After that start the build process via:

./waf buildparser buildlexer build

Usage
-----
After the build is complete, the binary will reside under build/exolang. To run a script do for example:
build/exolang examples/helloworld.exo

You can check the available command line options via:
build/exolang -h

I.e. run a script and get verbose trace:
build/exolang -s 1 examples/helloworld.exo

waf has a few extra commands at your disposal. Check:
./waf --help

To start gdb and load a script
./waf --gdb=src/tests/object.exo

To start valgrind/memcheck and load a script
./waf --memcheck=src/tests/object.exo

Examples
--------
Take a look at the examples directory. It contains pointers on syntax and constructs while i am working on a proper documentation.

Thanks & 3rd Party licenses
---------------------------
Lemon parser generator	- <http://www.hwaci.com/sw/lemon/>

Quex lexer generator	- <http://quex.sourceforge.net/>

waf			- <https://code.google.com/p/waf/>

Loren Segal		- <http://gnuu.org/2009/09/18/writing-your-own-toy-compiler/>

Mikael Lyngvig		- <http://llvm.lyngvig.org/Articles/Mapping-High-Level-Constructs-to-LLVM-IR>
