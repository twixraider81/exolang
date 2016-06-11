#!/bin/bash
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set -u
set -e

DIR=`pwd`
UNAME=`uname -s | grep -m1 -ioE '[a-z]+' | awk 'NR==1{print $0}'` # detection of OS
BINDIR="$DIR/bin"
TMPDIR="$DIR/tmp"
LLVM=0
KEEP=0

while getopts "cvkl" opt; do
	case "$opt" in
		c) # clean dir
			rm -vrf $TMPDIR
			rm -vrf $BINDIR
			rm -vrf $DIR/build
			rm -vrf $DIR/waf
			rm -vrf $DIR/.lock-*
			rm -vrf $DIR/.waf-*
			rm -vrf $DIR/.waf3-*
			exit 0
		;;
		v) # set verbosity
			set -x
		;;
		k) # keep files
			KEEP=1
		;;
		l) # build llvm
			LLVM=1
		;;
	esac
done


# check for required tools
TOOLS="curl gcc g++ cc tar unzip python make bison flex cmake ninja"
for TOOL in $TOOLS; do
	if ! which "$TOOL"; then
		echo "$TOOL not found; exiting"
		exit 0;
	fi
done


mkdir -p "$BINDIR"
mkdir -p "$TMPDIR"


# fetch quex
cd "$TMPDIR"
if ! test -d "$BINDIR/quex"; then
	ARQUEX="quex-0.65.11.zip"
	URLQUEX="http://sourceforge.net/projects/quex/files/DOWNLOAD/$ARQUEX/download"

	test -f "$TMPDIR/$ARQUEX" || curl -v -o "$TMPDIR/$ARQUEX" -L $URLQUEX
	#test -d "$BINDIR/quex" || tar -xzf "$TMPDIR/$ARQUEX" -C "$TMPDIR"

	test -d "$BINDIR/quex" || unzip -d "$TMPDIR" "$TMPDIR/$ARQUEX"
	mv "$TMPDIR/${ARQUEX:0:-4}" "$BINDIR/quex"

	if [ "$KEEP" == "0" ]; then
		rm -f "$TMPDIR/$ARQUEX"
	fi
fi


# build lemon
cd "$TMPDIR"
if ! test -f "$BINDIR/lemon/lemon"; then
	mkdir -p "$BINDIR/lemon"
	cd "$BINDIR/lemon"
	#test -d "lempar.c" || curl -v -L -o "lempar.c" "https://www.sqlite.org/src/raw/tool/lempar.c?name=404ea3dc27dbeed343f0e61b1d36e97b9f5f0fb6"
	#test -d "lemon.c" || curl -v -L -o "lemon.c" "https://www.sqlite.org/src/raw/tool/lemon.c?name=cfbfe061a4b2766512f6b484882eee2c86a14506"
	#gcc -o lemon lemon.c

	test -d "lempar.cpp" || curl -v -L -o "lempar.cpp" "https://raw.githubusercontent.com/ksherlock/lemon--/master/lempar.cpp"
	test -d "lemon.c" || curl -v -L -o "lemon.c" "https://raw.githubusercontent.com/ksherlock/lemon--/master/lemon.c"
	g++ -std=c++14 -Wno-write-strings -DLEMONPLUSPLUS=1 -o lemon lemon.c

	if [ "$KEEP" == "0" ]; then
		rm -f lemon.c
	fi
fi


# build llvm
cd "$TMPDIR"
if [[ "$LLVM" == "1" && ! -f "$BINDIR/bin/llvm-config" ]]; then
	cd "$TMPDIR"
	LLVMDIR="$TMPDIR/llvm"

	URLLLVM="http://llvm.org/releases/3.8.0"
	ARLLVM="llvm-3.8.0.src.tar.xz"
	ARCFE="cfe-3.8.0.src.tar.xz"
	ARCLANGE="clang-tools-extra-3.8.0.src.tar.xz"
	ARCOMPRT="compiler-rt-3.8.0.src.tar.xz"
	ARPOLLY="polly-3.8.0.src.tar.xz"
	AROPENMP="openmp-3.8.0.src.tar.xz"
	ARLLDB="lldb-3.8.0.src.tar.xz"
	ARLLD="lld-3.8.0.src.tar.xz"

	test -f "$TMPDIR/$ARLLVM" || curl -v -o "$TMPDIR/$ARLLVM" -L $URLLLVM/$ARLLVM
	test -d "$LLVMDIR" || tar -xJf "$TMPDIR/$ARLLVM" -C "$TMPDIR"
	test -d "$LLVMDIR" || mv "$TMPDIR/${ARLLVM:0:-7}" "$LLVMDIR"

	test -f "$TMPDIR/$ARCFE" || curl -v -o "$TMPDIR/$ARCFE" -L $URLLLVM/$ARCFE
	test -d "$LLVMDIR/tools/clang" || tar -xJf "$TMPDIR/$ARCFE" -C "$TMPDIR"
	test -d "$LLVMDIR/tools/clang" || mv "$TMPDIR/${ARCFE:0:-7}" "$LLVMDIR/tools/clang"

	test -f "$TMPDIR/$ARCLANGE" || curl -v -o "$TMPDIR/$ARCLANGE" -L $URLLLVM/$ARCLANGE
	test -d "$LLVMDIR/tools/clang/tools/extra" || tar -xJf "$TMPDIR/$ARCLANGE" -C "$TMPDIR"
	test -d "$LLVMDIR/tools/clang/tools/extra" || mv "$TMPDIR/${ARCLANGE:0:-7}" "$LLVMDIR/tools/clang/tools/extra"

	test -f "$TMPDIR/$ARCOMPRT" || curl -v -o "$TMPDIR/$ARCOMPRT" -L $URLLLVM/$ARCOMPRT
	test -d "$LLVMDIR/projects/compiler-rt" || tar -xJf "$TMPDIR/$ARCOMPRT" -C "$TMPDIR"
	test -d "$LLVMDIR/projects/compiler-rt" || mv "$TMPDIR/${ARCOMPRT:0:-7}" "$LLVMDIR/projects/compiler-rt"

	test -f "$TMPDIR/$ARPOLLY" || curl -v -o "$TMPDIR/$ARPOLLY" -L $URLLLVM/$ARPOLLY
	test -d "$LLVMDIR/tools/polly" || tar -xJf "$TMPDIR/$ARPOLLY" -C "$TMPDIR"
	test -d "$LLVMDIR/tools/polly" || mv "$TMPDIR/${ARPOLLY:0:-7}" "$LLVMDIR/tools/polly"

	test -f "$TMPDIR/$AROPENMP" || curl -v -o "$TMPDIR/$AROPENMP" -L $URLLLVM/$AROPENMP
	test -d "$LLVMDIR/tools/openmp" || tar -xJf "$TMPDIR/$AROPENMP" -C "$TMPDIR"
	test -d "$LLVMDIR/tools/openmp" || mv "$TMPDIR/${AROPENMP:0:-7}" "$LLVMDIR/tools/openmp"

	test -f "$TMPDIR/$ARLLDB" || curl -v -o "$TMPDIR/$ARLLDB" -L $URLLLVM/$ARLLDB
	test -d "$LLVMDIR/tools/lldb" || tar -xJf "$TMPDIR/$ARLLDB" -C "$TMPDIR"
	test -d "$LLVMDIR/tools/lldb" || mv "$TMPDIR/${ARLLDB:0:-7}" "$LLVMDIR/tools/lldb"

	test -f "$TMPDIR/$ARLLD" || curl -v -o "$TMPDIR/$ARLLD" -L $URLLLVM/$ARLLD
	test -d "$LLVMDIR/tools/lld" || tar -xJf "$TMPDIR/$ARLLD" -C "$TMPDIR"
	test -d "$LLVMDIR/tools/lld" || mv "$TMPDIR/${ARLLD:0:-7}" "$LLVMDIR/tools/lld"

	mkdir -p "$TMPDIR/llvm-build"
	cd "$TMPDIR/llvm-build"

	cmake -G Ninja ../llvm -DCMAKE_INSTALL_PREFIX="$BINDIR" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_POLLY=ON -DLLVM_ENABLE_CXX1Y=ON -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_FFI=ON -DLLVM_ENABLE_RTTI=ON
	cmake --build .
	cmake --build . --target install

	cd "$TMPDIR"

	if [ "$KEEP" == "0" ]; then
		rm -rf $LLVMDIR llvm-build
		rm -f $ARLLVM $ARCFE $ARCLANGE $ARCOMPRT $ARPOLLY $AROPENMP $ARLLDB $ARLLD
	fi
fi


cd "$DIR"


# fetch waf
if [ ! -f "waf" ]; then
	curl -v -o "$DIR/waf" "https://waf.io/waf-1.8.21"
	chmod a+rx "$DIR/waf"
fi


# i need this for my IDE, ugh
test -d "$BINDIR/usr-include" || ln -s "/usr/include" "$BINDIR/usr-include"


if [ "$KEEP" == "0" ]; then
	rm -vrf "$TMPDIR"
fi