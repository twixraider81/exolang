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

#ifndef STACK_H_
#define STACK_H_

#include "exo/exo.h"
#include "exo/jit/stackentry.h"

namespace exo
{
	namespace jit
	{
		class Stack
		{
			private:
				std::stack< std::shared_ptr<StackEntry> >	entries;

			public:
				Stack();
				~Stack();

				llvm::BasicBlock*	Push( llvm::BasicBlock* block, llvm::BasicBlock* exit = nullptr );
				llvm::BasicBlock*	Pop();
				llvm::BasicBlock*	Join( llvm::BasicBlock* block );
				llvm::BasicBlock*	Block();
				llvm::BasicBlock*	Exit();

				std::string 		blockName();

				llvm::Value*		getSymbol( std::string name );
				void				setSymbol( std::string name, llvm::Value* value );
				void				delSymbol( std::string name );
		};
	}
}

#endif /* STACK_H_ */
