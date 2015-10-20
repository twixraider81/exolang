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
THREADS=`nproc`
LLVM=0

while getopts "cvl" opt; do
	case "$opt" in
		c) # clean dir
			rm -rf $TMPDIR
			rm -rf $BINDIR
			rm -rf $DIR/build
			rm -rf $DIR/waf
			rm -rf $DIR/.lock-*
			rm -rf $DIR/.waf-*
			exit 0
		;;
		v) # set verbosity
			set -x
		;;
		l) # build llvm
			LLVM=1
		;;
	esac
done


# check for required tools
TOOLS="curl gcc python make bison flex cmake"
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
	test -f "$TMPDIR/quex.tar.gz" || curl -v -o "$TMPDIR/quex.tar.gz" -L http://sourceforge.net/projects/quex/files/DOWNLOAD/quex-0.65.4.tar.gz/download
	tar -xzf "$TMPDIR/quex.tar.gz" -C "$BINDIR"
	mv "$BINDIR/quex-0.65.4" "$BINDIR/quex"
fi


# build lemon
if ! test -f "$BINDIR/lemon/lemon"; then
	mkdir -p "$BINDIR/lemon"
	cd "$BINDIR/lemon"
	curl -v -L -o "lempar.c" "https://www.sqlite.org/src/raw/tool/lempar.c?name=3617143ddb9b176c3605defe6a9c798793280120"
	curl -v -L -o "lemon.c" "https://www.sqlite.org/src/raw/tool/lemon.c?name=039f813b520b9395740c52f9cbf36c90b5d8df03"
	gcc -o lemon lemon.c
fi


# build llvm
if [ "$LLVM" == "1" ]; then
	cd "$TMPDIR"
	LLVMDIR="$TMPDIR/llvm"

	test -d "$LLVMDIR" || svn co http://llvm.org/svn/llvm-project/llvm/branches/release_37 llvm
	cd "$LLVMDIR/tools"
	test -d "$LLVMDIR/tools/clang" || svn co http://llvm.org/svn/llvm-project/cfe/branches/release_37 clang
	cd "$LLVMDIR/tools/clang/tools"
	test -d "$LLVMDIR/tools/clang/tools/extra" || svn co http://llvm.org/svn/llvm-project/clang-tools-extra/branches/release_37 extra
	cd "$LLVMDIR/projects" 
	test -d "$LLVMDIR/projects/compiler-rt" || svn co http://llvm.org/svn/llvm-project/compiler-rt/branches/release_37 compiler-rt
	cd "$LLVMDIR/tools" 
	test -d "$LLVMDIR/tools/polly" || svn co https://llvm.org/svn/llvm-project/polly/branches/release_37 polly

	mkdir -p "$TMPDIR/llvm-build"
	cd "$TMPDIR/llvm-build"

	CC=cc CXX=c++ ../llvm/configure --prefix=$BINDIR --enable-polly --enable-cxx1y --enable-optimized=NO --enable-assertions --enable-debug-runtime --enable-debug-symbols --enable-keep-symbols --enable-jit --enable-backtraces --enable-terminfo --enable-libffi

	make -j$THREADS
	make install
fi

cd "$DIR"


# fetch waf
if [ ! -f "waf" ]; then
	curl -v -o "$DIR/waf" "https://waf.io/waf-1.8.14"
	chmod a+rx "$DIR/waf"
fi