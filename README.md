exolang
=======


Quickstart
----------
./bootstrap.sh						(you will need to build lemon/quex to rebuild the lexer/parser for now)
./bootstrap.sh -l					(if you want to build llvm for /usr)

You will also need libgc			(on Debian do apt-get install libgc-dev)

./waf clean configure --mode=debug	(clean & configure for release/debug release, trace release prints alot of messages currently)
./waf buildparser buildlexer build	(required for now. parser must be built before the lexer)

build/exolang -i <filename>			(i.e tests/1.exo)
build/exolang						(to parse stdin)
build/exolang -h					(to show help)

Prerequisites
-------------
On Debian simply do:
apt-get install libgc-dev llvm llvm-dev llvm-runtime libc++-dev


Thanks & 3rd Party licenses
---------------------------
Lemon parser generator	- <http://www.hwaci.com/sw/lemon/>
Quex lexer generator	- <http://quex.sourceforge.net/>
Boehm GC				- <https://github.com/ivmai/bdwgc>
Getopt_pp				- <http://code.google.com/p/getoptpp/>
Loren Segal				- <http://gnuu.org/2009/09/18/writing-your-own-toy-compiler/>
