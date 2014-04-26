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
#include "exo/init/init.h"

namespace exo
{
	namespace signals
	{
		void sigsegHandler( int signal, siginfo_t *si, void *arg )
		{
			if( signal == SIGSEGV ) {
#ifdef EXO_DEBUG
				void *array[10];
				size_t size;

				size = backtrace( array, 10 );

				BOOST_LOG_TRIVIAL(debug) << "Ooops, segementation fault:";
				/* skip first stack frame (points here) */
				for( int i = 1; i < size && array != NULL; ++i )
				{
					BOOST_LOG_TRIVIAL(debug) << "(" << i << ") " << array[i];
				}
#endif
				BOOST_THROW_EXCEPTION( exo::exceptions::Segfault() );
			}
		}

		void sigtermHandler( int signal, siginfo_t *si, void *arg )
		{
			BOOST_LOG_TRIVIAL(trace) << "Shutting down.";
			exo::init::Init::Shutdown();
		}

		void registerHandlers()
		{
			struct sigaction sa;
			memset( &sa, 0, sizeof( struct sigaction) );
			sigemptyset( &sa.sa_mask );
			sa.sa_sigaction = sigsegHandler;
			sa.sa_flags = SA_SIGINFO;

			if( sigaction( SIGSEGV, &sa, NULL ) == -1 ) {
				BOOST_LOG_TRIVIAL(warning) << "Failed to register segementation fault handler!";
			}


			memset( &sa, 0, sizeof( struct sigaction) );
			sigemptyset( &sa.sa_mask );
			sa.sa_sigaction = sigtermHandler;
			sa.sa_flags = SA_SIGINFO;

			if( sigaction( SIGTERM, &sa, NULL ) == -1 || sigaction( SIGINT, &sa, NULL ) == -1 || sigaction( SIGABRT, &sa, NULL ) == -1  ) {
				BOOST_LOG_TRIVIAL(warning) << "Failed to register sigterm,sigint,sigabrt handler!";
			}
		}
	}
}
