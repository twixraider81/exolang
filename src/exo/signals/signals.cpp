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
#include "signals.h"

namespace exo
{
	namespace signals
	{
		void segfaultHandler( int signal, siginfo_t *si, void *arg )
		{
ERRORMSG( "Segmentation fault" )
			exit(0);
		}

		void registerHandlers()
		{
			struct sigaction sa;

			memset( &sa, 0, sizeof(sigaction) );
			sigemptyset( &sa.sa_mask );
			sa.sa_sigaction = segfaultHandler;
			sa.sa_flags = SA_SIGINFO;

			if( sigaction( SIGSEGV, &sa, NULL ) == -1 ) {
ERRORMSG( "Failed to register segementation fault handler" )
			}
		}
	}
}
