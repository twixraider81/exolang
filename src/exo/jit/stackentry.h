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

#ifndef STACKENTRY_H_
#define STACKENTRY_H_

#include "exo/exo.h"
#include "exo/jit/llvm.h"

namespace exo
{
	namespace jit
	{
		class StackEntry
		{
			public:
				llvm::BasicBlock*					block;
				llvm::BasicBlock*					exit;
				std::map<std::string,llvm::Value*>	symbols;

				StackEntry( llvm::BasicBlock* b, llvm::BasicBlock* e );
				~StackEntry();

				llvm::Value* getSymbol( std::string name );
				void setSymbol( std::string name, llvm::Value* value );
				void delSymbol( std::string name );
		};
	}
}

#endif /* STACKENTRY_H_ */
