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

namespace exo
{
	namespace jit
	{
		class Context;

		class JIT
		{
			exo::jit::Context*			context;
			llvm::ExecutionEngine*		engine;
			llvm::FunctionPassManager 	*fpm;

			public:
				/**
				 * Builds a LLVM MCJIT IR and loads the LLVM IR of the AST
				 * TODO: make use of -0x levels - http://llvm.org/docs/doxygen/html/namespacellvm_1_1CodeGenOpt.html
				 */
				JIT( exo::jit::Context* c );
				~JIT();
		};
	}
}

#endif /* JIT_H_ */
