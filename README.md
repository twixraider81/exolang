exolang
=======

Quickstart
----------
- ./bootstrap.sh (you will need to build lemon/quex to rebuild the lexer/parser for now)
- ./bootstrap.sh -l (if you want to build llvm for /usr)
- ./waf configure
- ./waf buildparser buildlexer (for now)
- ./waf build --mode=debug