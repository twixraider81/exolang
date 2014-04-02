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

#include <fstream>
#include <cstdlib>
#include <iostream>
#include <string>
#include <exception>
#include <cassert>

#ifdef DEBUG
#define DEBUGMSG(msg) std::cout << msg << std::endl;
#else
#define DEBUGMSG(msg)
#endif

#define ERRORMSG(msg) std::cerr << msg << std::endl;
#define ERRORRET(msg,retval) std::cerr << msg << std::endl; return(retval);

#endif /* EXO_H_ */