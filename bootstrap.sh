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

while getopts "cv" opt; do
	case "$opt" in
		c) # clean build tools dir
			rm -rf "$TMPDIR"
			exit 0
		;;
		v) # set verbosity
			set -x
		;;
	esac
done

# check for tools
TOOLS="curl gcc python clang"
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
	test -f "$TMPDIR/quex.tar.gz" || curl -v -L -o "$TMPDIR/quex.tar.gz" "http://downloads.sourceforge.net/project/quex/DOWNLOAD/quex-0.64.8.tar.gz?r=http%3A%2F%2Fsourceforge.net%2Fprojects%2Fquex%2Ffiles%2FDOWNLOAD%2F&ts=1396263676&use_mirror=cznic"
	tar -xzf "$TMPDIR/quex.tar.gz" -C "$BINDIR"
	mv "$BINDIR/quex-0.64.8" "$BINDIR/quex"
fi

if ! test -f "$BINDIR/lemon/lemon"; then
	mkdir -p "$BINDIR/lemon"
	cd "$BINDIR/lemon"
	curl -v -L -o "lempar.c" "https://www.sqlite.org/src/raw/tool/lempar.c?name=01ca97f87610d1dac6d8cd96ab109ab1130e76dc"
	curl -v -L -o "lemon.c" "https://www.sqlite.org/src/raw/tool/lemon.c?name=07aba6270d5a5016ba8107b09e431eea4ecdc123"
	gcc -o lemon lemon.c
fi

# build llvm
cd "$TMPDIR"
LLVMDIR="$TMPDIR/llvm-3.4"
LLVMARCHIVE="$TMPDIR/llvm-3.4.src.tar.gz"
CLANGARCHIVE="$TMPDIR/clang-3.4.src.tar.gz"
CLANGEARCHIVE="$TMPDIR/clang-tools-extra-3.4.src.tar.gz"
RTARCHIVE="$TMPDIR/compiler-rt-3.4.src.tar.gz"

if [ ! -d "$LLVMDIR" ]; then
	test -f "$LLVMARCHIVE" || curl -v -o "$LLVMARCHIVE" "http://llvm.org/releases/3.4/llvm-3.4.src.tar.gz"
	tar -xzf "$LLVMARCHIVE" -C "$TMPDIR"
fi

if [ ! -d "$LLVMDIR/tools/clang" ]; then
	test -f "$CLANGARCHIVE" || curl -v -o "$CLANGARCHIVE" "http://llvm.org/releases/3.4/clang-3.4.src.tar.gz"
	tar -xzf "$CLANGARCHIVE" -C "$LLVMDIR/tools"
	mv "$LLVMDIR/tools/clang-3.4" "$LLVMDIR/tools/clang"
fi

if [ ! -d "$LLVMDIR/tools/clang/tools/extra" ]; then
	test -f "$CLANGEARCHIVE" || curl -v -o "$CLANGEARCHIVE" "http://llvm.org/releases/3.4/clang-tools-extra-3.4.src.tar.gz"
	tar -xzf "$CLANGEARCHIVE" -C "$LLVMDIR/tools/clang/tools"
	mv "$LLVMDIR/tools/clang/tools/clang-tools-extra-3.4" "$LLVMDIR/tools/clang/tools/extra"
fi

if [ ! -d "$LLVMDIR/projects/compiler-rt" ]; then
	test -f "$RTARCHIVE" || curl -v -o "$RTARCHIVE" "http://llvm.org/releases/3.4/compiler-rt-3.4.src.tar.gz"
	tar -xzf "$RTARCHIVE" -C "$LLVMDIR/projects"
	mv "$LLVMDIR/projects/compiler-rt-3.4" "$LLVMDIR/projects/compiler-rt"
fi

mkdir -p "$TMPDIR/llvm-build"
cd "$TMPDIR/llvm-build"

CC=cc CXX=c++ ../llvm-3.4/configure --enable-optimized --enable-jit --enable-targets=host-only
make -j -l 2.5
make install

cd "$DIR"
rm -rf "$TMPDIR"

# fetch waf
if [ ! -f "waf" ]; then
	curl -v -o "$DIR/waf" "http://waf.googlecode.com/files/waf-1.7.15"
	chmod a+rx "$DIR/waf"
fi