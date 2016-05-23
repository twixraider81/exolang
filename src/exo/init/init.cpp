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
			switch( logLevel ) {
				case 1:
					boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::trace );
					break;
				case 2:
					boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::debug );
					break;
				case 3:
					boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::info );
					break;
				case 4:
					boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::warning );
					break;
				case 5:
					boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::error );
					break;
				case 6:
					boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::fatal );
					break;
				default:
					EXO_LOG( warning, "Invalid log level." );
			}

#ifndef EXO_GC_DISABLE
			GC_INIT();
			//GC_enable_incremental();
#endif
			// initialize llvm
			llvm::InitializeAllTargets();
			llvm::InitializeAllTargetMCs();
			llvm::InitializeAllAsmPrinters();
			llvm::InitializeAllAsmParsers();

			// register signal handler
			exo::signals::registerHandlers();

			return( true );
		}

		void Init::Shutdown()
		{
#ifndef EXO_GC_DISABLE
			GC_gcollect();
#endif
		}
	}
}
