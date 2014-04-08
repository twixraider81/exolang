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

#ifndef GENERATOR_H_
#define GENERATOR_H_

namespace exo
{
	namespace jit
	{
		class JIT : public virtual gc
		{
			exo::ast::Context* context;
			llvm::ExecutionEngine* engine;
			llvm::FunctionPassManager *fpm;

			public:
				/**
				 * Builds a LLVM MCJIT IR and loads the LLVM IR of the AST
				 * TODO: make use of -0x levels - http://llvm.org/docs/doxygen/html/namespacellvm_1_1CodeGenOpt.html
				 */
				JIT( exo::ast::Context* c );
				~JIT();
		};
	}
}

#endif /* GENERATOR_H_ */
