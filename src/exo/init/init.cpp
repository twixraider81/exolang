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
		bool Init::Startup()
		{
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

		int Init::Shutdown()
		{
#ifndef EXO_GC_DISABLE
			GC_gcollect();
#endif
			return( 0 );
		}
	}
}
