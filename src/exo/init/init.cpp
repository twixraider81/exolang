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
#include "exo/init/init.h"
#include "exo/jit/llvm.h"
#include "exo/signals/signals.h"

namespace exo
{
	namespace init
	{
		bool Init::Startup( int logLevel )
		{
			// a bit ugly
			if( logLevel < 1 || logLevel > 6 ) {
				BOOST_LOG_TRIVIAL(warning) << "Invalid log level.";
			} else {
				if( logLevel == 1 ) {
					boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::trace );
				} else if( logLevel == 2 ) {
					boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::debug );
				} else if( logLevel == 3 ) {
					boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::info );
				} else if( logLevel == 4 ) {
					boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::warning );
				} else if( logLevel == 5 ) {
					boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::error );
				} else if( logLevel == 6 ) {
					boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::fatal );
				}
			}

#ifndef EXO_GC_DISABLE
			GC_INIT();
			GC_enable_incremental();

# ifdef EXO_TRACE
			setenv( "GC_PRINT_STATS", "1", 1 );
			setenv( "GC_DUMP_REGULARLY", "1", 1 );
			setenv( "GC_FIND_LEAK", "1", 1 );
# endif
#endif
			// initialize llvm
			llvm::InitializeNativeTarget();
			llvm::InitializeNativeTargetAsmPrinter();
			llvm::InitializeNativeTargetAsmParser();

			// register signal handler
			exo::signals::registerHandlers();

			return( true );
		}

		void Init::Shutdown()
		{
#ifndef EXO_GC_DISABLE
			GC_gcollect();
#endif
			exit( 0 );
		}
	}
}
