exolang
=======

Goal
----
The aim for exolang is to develop into a jiting/compiling rapid prototyping "script" language, using LLVM for its code generation.


Examples
--------
Please take a look at the [examples](https://github.com/twixraider81/exolang/tree/master/examples) and/or [test](https://github.com/twixraider81/exolang/tree/master/src/tests) subdirectories for various script examples.


Build quickstart
-----
After the build is complete, the binary will reside under build/exolang.
You can check the all command line options via:

./build/exolang -h



Here are some examples:

Run the helloworld script:

./build/exolang examples/helloworld.exo

Run the helloworld script and get a debug trace:

./build/exolang -l2 examples/helloworld.exo

Compile the script, and display the resulting LLVM IR:

./build/exolang -e -i examples/helloworld.exo

Compile the script with the LLVM C++ Backend (will result in C++ Code as helloworld.s):

./build/exolang -t cpp -S -i examples/helloworld.exo 


Build prerequisites
-------------
I develop on a Debian system. In order to install the prerequisites there, it should be suffice to do:

apt-get install llvm llvm-dev libc++-dev libboost-all-dev libgc-dev gdb valgrind libunwind8-dev libedit-dev swig ninja-build

Next call the bootstrap script:

./bootstrap.sh -vl

This will attempt to build the lemon-- parser generator, fetch the quex lexer in order to rebuild the lexer/parser code and download the waf build tool.
As we need a fairly recent LLVM build, the bootstrap script will also build LLVM with RTTI enabled (needed by the boost libraries). This will take quite some time.

./bootstrap.sh
- -c : force cleanup (delete downloaded folders)
- -l : build llvm
- -v : verbose, print what the script is doing
- -k : keep, keep downloaded and temorary files


Building
--------
Next configure the build. For now since it's under heavy development:

./waf clean; ./waf configure --mode=debug --llvm=./bin/bin/llvm-config --gc=disable

(./waf clean; ./waf configure --mode=release --llvm=./bin/bin/llvm-config --gc=enable in release mode)

After that start the build process via:

./waf buildparser buildlexer build

waf has a few extra commands at your disposal. Check:

./waf --help

To start gdb and load a script

./waf --gdb=src/tests/object.exo

To start valgrind/memcheck and load a script

./waf --memcheck=src/tests/object.exo

To run all test scripts

./waf --runtests


Thanks & 3rd Party licenses
---------------------------
Lemon parser generator	- <http://www.hwaci.com/sw/lemon/>

ksherlock Lemon C++ Updates	- <https://github.com/ksherlock/lemon-->

Quex lexer generator	- <http://quex.sourceforge.net/>

waf			- <https://code.google.com/p/waf/>
