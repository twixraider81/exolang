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

#ifndef JIT_H_
#define JIT_H_

#include "exo/jit/llvm.h"
#include "exo/jit/target.h"

namespace exo
{
	namespace jit
	{
		class JIT
		{
			std::unique_ptr<llvm::Module>	module;
			std::shared_ptr<Target>			target;
			llvm::legacy::PassManager		passManager;
			std::set<std::string>			imports;

			public:
				JIT( std::unique_ptr<llvm::Module> m, std::shared_ptr<Target> t, std::set<std::string> i );
				~JIT();

				int Execute();
				int Execute( std::string fName );
				int Emit( int type = 0, std::string fileName = "" );
		};
	}
}

#endif /* JIT_H_ */
