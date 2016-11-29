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

#ifndef TARGET_H_
#define TARGET_H_

#include "exo/jit/llvm.h"

namespace exo
{
	namespace jit
	{
		/**
		 * Simple helper class to assits with target machine specific operations
		 */
		class Target
		{
			public:
				/**
				 * The LLVM target machine, based upon architecture, cpu type
				 */
				std::unique_ptr<llvm::TargetMachine>	targetMachine;

				/**
				 * Code generation optimization Level (0-3)
				 */
				llvm::CodeGenOpt::Level					codeGenOpt;

				/**
				 * llvm context
				 */
				llvm::LLVMContext						context;

				/**
				 * Constructs the targetmachine (thru LLVM) based, based upon architecture, cpu type and optimizatzion level
				 */
				Target( std::string archName, std::string cpuName, int optimizeLvl );
				~Target();

				/**
				 * Create a module, suitable for our target
				 */
				std::unique_ptr<llvm::Module>	createModule( std::string moduleName );

				/**
				 * String representation/Name of our Target
				 */
				std::string						getName();
		};
	}
}

#endif /* TARGET_H_ */
