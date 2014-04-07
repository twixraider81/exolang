exolang
=======

Goal
----
The aim is to develop a jiting/compiling rapid prototyping "script" language. (Cause developing in C/C++/C#/Java is horribly slow and espcially incase of C error prone).
It will borrow or base on PHP syntax while trying to alleviate the pains or wrong doings of that it.

Quickstart
----------
./bootstrap.sh						(you will need to build lemon/quex to rebuild the lexer/parser for now)
./bootstrap.sh -l					(if you want to build llvm for /usr)

./waf clean configure --mode=debug	(clean & configure for release/debug release, trace release prints alot of messages currently)

Take a look at ./waf help to specifiy other options for you system, like an llvm-config (./waf clean configure --llvm=llvm-config-3.5)

After that:
./waf buildparser buildlexer build

build/exolang <filename>			(i.e tests/1.exo)
build/exolang -i <filename>			(i.e tests/1.exo)
build/exolang						(to parse stdin)

build/exolang -h					(to show help and available options)

Prerequisites
-------------
On Debian simply do:
apt-get install libgc-dev llvm-3.5 libc++-dev libboost1.53-all-dev


Thanks & 3rd Party licenses
---------------------------
Lemon parser generator	- <http://www.hwaci.com/sw/lemon/>
Quex lexer generator	- <http://quex.sourceforge.net/>
Boehm GC				- <https://github.com/ivmai/bdwgc>
Loren Segal				- <http://gnuu.org/2009/09/18/writing-your-own-toy-compiler/>
