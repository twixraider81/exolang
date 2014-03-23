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

KEEP=0 # keep stuff or not

while getopts "ckva:b:" opt; do
	case "$opt" in
		k) # keep downloaded archives
			KEEP=1
		;;
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
TOOLS="curl git bison flex make gcc gdb texindex tar xzcat python patch cmake svn"
for TOOL in $TOOLS; do
	if ! which "$TOOL"; then
		echo "$TOOL not found; exiting"
		exit 0;
	fi
done

mkdir -p "$TOOLDIR"

# build re2c
cd "$TOOLDIR"
test -d "$TOOLDIR/re2c-code-git" || git clone git://git.code.sf.net/p/re2c/code-git re2c-code-git
cd "$TOOLDIR/re2c-code-git/re2c"
./autogen.sh
./configure
make
make install


# build llvm
cd "$TOOLDIR"
test -d "$TOOLDIR/llvm" || svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm

cd "$TOOLDIR/llvm/tools"
test -d "$TOOLDIR/llvm/tools/clang" || svn co http://llvm.org/svn/llvm-project/cfe/trunk clang

cd "$TOOLDIR/llvm/tools/clang/tools"
test -d "$TOOLDIR/llvm/tools/clang/tools/extra" || svn co http://llvm.org/svn/llvm-project/clang-tools-extra/trunk extra

cd "$TOOLDIR/llvm/projects" 
test -d "$TOOLDIR/llvm/projects/compiler-rt" || svn co http://llvm.org/svn/llvm-project/compiler-rt/trunk compiler-rt

mkdir -p "$TOOLDIR/llvm-build"
cd "$TOOLDIR/llvm-build"

CC=cc CXX=c++ ../llvm/configure --enable-optimized --enable-jit --enable-targets=host-only
make -j2
make install


# done
cd "$DIR"

# fetch waf
if [ ! -f "waf" ]; then
	curl -v -o "$DIR/waf" "http://waf.googlecode.com/files/waf-1.7.15"
	chmod a+rx "$DIR/waf"
fi