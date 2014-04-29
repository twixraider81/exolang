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

#ifndef SIGNALS_H_
#define SIGNALS_H_

#include <csignal>
#include <execinfo.h>
#include <unistd.h>

#include <libunwind.h>
#include <boost/units/detail/utility.hpp>

namespace exo
{
	namespace signals
	{
		/**
		 * Registers a Segmentation fault handler which in case results in Segfault exception.
		 * If compiled with debug enabled it will additionally print a 10 frame backtrace on stdout
		 */
		void sigsegHandler( int signal, siginfo_t *si, void *arg );

		/**
		 * Registers a termination/interrupt handler to initialize shutdown.
		 */
		void sigtermHandler( int signal, siginfo_t *si, void *arg );


		/*
		 * Register signalhandlers, currently only a segfault exception throw is registered
		 * TODO: check which other signals could be caught - http://en.wikipedia.org/wiki/Unix_signal
		 */
		void registerHandlers();
	}
}


#endif /* SIGNALS_H_ */
