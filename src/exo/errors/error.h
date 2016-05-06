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

/**
 * POINTERCHECK is a little assert MACRO to check for null pointers. Should use nullptr instead?
 */
#ifdef EXO_DEBUG
# define POINTERCHECK(p)			if(p == nullptr) BOOST_LOG_TRIVIAL(error) << "Invalid pointer! In " << __FILE__ << " on " << __LINE__
# define EXO_DEBUG_LOG(lvl, ...)	BOOST_LOG_TRIVIAL(lvl) << __VA_ARGS__;
#else
# define POINTERCHECK(p)
# define EXO_DEBUG_LOG(lvl, ...)
#endif

#define EXO_LOG(lvl, ...)			BOOST_LOG_TRIVIAL(lvl) << __VA_ARGS__;

#endif /* ERROR_H_ */
