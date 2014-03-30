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
TOOLDIR="$DIR/tmp"

while getopts "cv" opt; do
	case "$opt" in
		c) # clean build tools dir
			rm -rf $TOOLDIR/llvm*
			rm -rf $TOOLDIR/re2c-code-git*
			exit 0
		;;
		v) # set verbosity
			set -x
		;;
	esac
done

# check for tools
TOOLS="curl git bison flex make gcc python svn curl"
for TOOL in $TOOLS; do
	if ! which "$TOOL"; then
		echo "$TOOL not found; exiting"
		exit 0;
	fi
done

mkdir -p "$TOOLDIR"

# build re2c
cd "$TOOLDIR"
if ! test -d "$TOOLDIR/re2c-code-git"; then
	git clone git://git.code.sf.net/p/re2c/code-git re2c-code-git
	cd "$TOOLDIR/re2c-code-git/re2c"
	./autogen.sh
	./configure
	make -j -l 2.5
	make install
fi

# build llvm
cd "$TOOLDIR"
LLVMDIR="$TOOLDIR/llvm-3.4"
LLVMARCHIVE="$TOOLDIR/llvm-3.4.src.tar.gz"
CLANGARCHIVE="$TOOLDIR/clang-3.4.src.tar.gz"
CLANGEARCHIVE="$TOOLDIR/clang-tools-extra-3.4.src.tar.gz"
RTARCHIVE="$TOOLDIR/compiler-rt-3.4.src.tar.gz"

if [ ! -d "$LLVMDIR" ]; then
	test -f "$LLVMARCHIVE" || curl -v -o "$LLVMARCHIVE" "http://llvm.org/releases/3.4/llvm-3.4.src.tar.gz"
	tar -xzf "$LLVMARCHIVE" -C "$TOOLDIR"
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

mkdir -p "$TOOLDIR/llvm-build"
cd "$TOOLDIR/llvm-build"

CC=cc CXX=c++ ../llvm-3.4/configure --enable-optimized --enable-jit --enable-targets=host-only
make -j -l 2.5
make install

# done
cd "$DIR"

# fetch waf
if [ ! -f "waf" ]; then
	curl -v -o "$DIR/waf" "http://waf.googlecode.com/files/waf-1.7.15"
	chmod a+rx "$DIR/waf"
fi