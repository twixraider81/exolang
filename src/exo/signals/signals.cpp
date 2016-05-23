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
		/*
		 * TODO: proper handling of std::terminate, should this throw?
		 */
		void sigsegHandler( int signal, siginfo_t *si, void *arg )
		{
			if( signal == SIGSEGV ) {
				char name[256];
				unw_cursor_t cursor; unw_context_t uc;
				unw_word_t ip, sp, offp;

				unw_getcontext( &uc );
				unw_init_local( &cursor, &uc );

				while( unw_step( &cursor ) > 0 ) {
					char file[256];
					int line = 0;

					name[0] = '\0';
					unw_get_proc_name( &cursor, name, 256, &offp);
					unw_get_reg( &cursor, UNW_REG_IP, &ip );
					unw_get_reg( &cursor, UNW_REG_SP, &sp);

					std::string demangled =  boost::units::detail::demangle( name );

					if( demangled == "demangle :: error - unable to demangle specified symbol" ) {
						demangled = name;
					}

					EXO_LOG( error, ( boost::format( "(%s), ip:%lx, sp:%lx" ) % demangled % ip % sp ) );
				}

				EXO_THROW( Segfault() );
			}
		}

		void sigtermHandler( int signal, siginfo_t *si, void *arg )
		{
			EXO_LOG( trace, "Shutting down." );
			exo::init::Init::Shutdown();
		}

		void registerHandlers()
		{
			struct sigaction sa;
			memset( &sa, 0, sizeof( struct sigaction) );
			sigemptyset( &sa.sa_mask );
			sa.sa_sigaction = sigsegHandler;
			sa.sa_flags = SA_SIGINFO;

			if( sigaction( SIGSEGV, &sa, nullptr ) == -1 ) {
				EXO_LOG( warning, "Failed to register segementation fault handler!" );
			}


			memset( &sa, 0, sizeof( struct sigaction) );
			sigemptyset( &sa.sa_mask );
			sa.sa_sigaction = sigtermHandler;
			sa.sa_flags = SA_SIGINFO;

			if( sigaction( SIGTERM, &sa, nullptr ) == -1 || sigaction( SIGINT, &sa, nullptr ) == -1 || sigaction( SIGABRT, &sa, nullptr ) == -1  ) {
				EXO_LOG( warning, "Failed to register sigterm,sigint,sigabrt handler!" );
			}
		}
	}
}
