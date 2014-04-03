exolang
=======


Quickstart
----------
./bootstrap.sh						(you will need to build lemon/quex to rebuild the lexer/parser for now)
./bootstrap.sh -l					(if you want to build llvm for /usr)

./waf clean configure --mode=debug	(clan & configure for debug release, spits alot of messages currently)
./waf buildparser buildlexer build	(parser must be built before the lexer)

build/exolang -i <filename>			(i.e tests/1.exo)
build/exolang						(to parse stdin)
build/exolang -h					(to show help)

Thanks & 3rd Party licenses
---------------------------
Quex lexer	- <http://quex.sourceforge.net/>
Getopt_pp	- <http://code.google.com/p/getoptpp/>
Loren Segal	- <http://gnuu.org/2009/09/18/writing-your-own-toy-compiler/>
