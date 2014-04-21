/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EXO_H_
#define EXO_H_

/*
 * FIXME: probably better handled by configure
 */
#ifdef EXO_DEBUG
# undef NDEBUG
#endif

#ifndef EXO_GC_DISABLE
# include <gc/gc.h>
# include <gc/gc_cpp.h>
#else
class gc {};
# define GC_gcollect()
# define GC_malloc malloc
# define GC_free free
#endif

#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdint>
#include <string>
#include <cstring>
#include <cctype>
#include <cassert>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <map>
#include <vector>
#include <stack>
#include <memory>

#include <boost/scoped_ptr.hpp>
#include <boost/exception/all.hpp>
#include <boost/throw_exception.hpp>

#include "exo/errors/error.h"
#include "exo/errors/exceptions.h"

#endif /* EXO_H_ */
