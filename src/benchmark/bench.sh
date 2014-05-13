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
DIR=`dirname $0`

echo "php for:"
time php $DIR/for.php >& /dev/null

echo "exo for:"
time build/exolang $DIR/for.exo >& /dev/null

echo "php fibonacci(35):"
time php $DIR/fib.php

echo "exo fibonacci(35):"
time build/exolang DIR/fib.exo
