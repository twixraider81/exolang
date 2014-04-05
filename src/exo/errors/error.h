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

#ifndef ERROR_H_
#define ERROR_H_

#define ERRORMSG(msg) std::cerr << "Error: " << msg << " in " __FILE__ << ":" << __LINE__ << std::endl
#define ERRORRET(msg,retval) ERRORMSG(msg); return(retval)

#ifdef EXO_DEBUG
# define POINTERCHECK(p) if(p == NULL) ERRORMSG( "invalid pointer in " << __FILE__ << ":" << __LINE__ )
# ifdef EXO_TRACE
# define TRACESECTION(section,msg) std::cout << section << ": " << msg << " in " << __FILE__ << ":" << __LINE__ << std::endl;
# define TRACE(msg) TRACESECTION("Trace",msg);
# endif
#else
# define POINTERCHECK(p)
# define TRACESECTION(msg,section)
# define TRACE(msg)
#endif

#endif /* ERROR_H_ */
