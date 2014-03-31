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
	mv "$BINDIR/quex/quex-exe.py" "$BINDIR/quex/quex.py"
fi

if ! test -f "$BINDIR/lemon/lemon"; then
	mkdir -p "$BINDIR/lemon"
	cd "$BINDIR/lemon"
	curl -v -L -o "lempar.c" "https://www.sqlite.org/src/raw/tool/lempar.c?name=01ca97f87610d1dac6d8cd96ab109ab1130e76dc"
	curl -v -L -o "lemon.c" "https://www.sqlite.org/src/raw/tool/lemon.c?name=07aba6270d5a5016ba8107b09e431eea4ecdc123"
	gcc -o lemon lemon.c
fi

cd "$DIR"
rm -rf "$TMPDIR"

# fetch waf
if [ ! -f "waf" ]; then
	curl -v -o "$DIR/waf" "http://waf.googlecode.com/files/waf-1.7.15"
	chmod a+rx "$DIR/waf"
fi