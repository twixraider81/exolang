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

#include "exo/exo.h"
#include "exo/signals/signals.h"

namespace exo
{
	namespace signals
	{
		/*
		 * Register a Segementationfault handler
		 * TODO: gracefully shutdown in main()
		 * TODO: http://stackoverflow.com/questions/77005/how-to-generate-a-stacktrace-when-my-gcc-c-app-crashes
		 */
		void sigHandler( int signal, siginfo_t *si, void *arg )
		{
			if( signal == SIGSEGV ) {
#ifdef EXO_DEBUG
				void *array[10];
				size_t size;

				size = backtrace( array, 10 );

				/* skip first stack frame (points here) */
				for( int i = 1; i < size && array != NULL; ++i )
				{
					TRACESECTION( "SIGSEGV", "(" << i << ") " << array[i] );
				}
#endif
				BOOST_THROW_EXCEPTION( exo::exceptions::Segfault() );
			}
		}

		/*
		 * Register Signal handlers, currently only Segfault registered
		 * TODO: check which other signals shoud be caught
		 * FIXME: figure out why libgc sends a SIGSEGV on terminate
		 */
		void registerHandler()
		{
			struct sigaction sa;
			memset( &sa, 0, sizeof( struct sigaction) );
			sigemptyset( &sa.sa_mask );
			sa.sa_sigaction = sigHandler;
			sa.sa_flags = SA_SIGINFO;

			if( sigaction( SIGSEGV, &sa, NULL ) == -1 ) {
				ERRORMSG( "failed to register segementation fault handler" );
			}
		}
	}
}
