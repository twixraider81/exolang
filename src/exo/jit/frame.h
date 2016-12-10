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

#ifndef FRAME_H_
#define FRAME_H_

#include "exo/exo.h"
#include "exo/jit/llvm.h"

namespace exo
{
	namespace jit
	{
		class SymbolTable;

		class Frame
		{
			public:
				llvm::BasicBlock*					insertBlock;
				llvm::BasicBlock*					breakBlock;
				llvm::BasicBlock*					conditionBlock;
				std::shared_ptr<Frame>				parent;
				std::unordered_map< std::string /* name */, std::pair< llvm::Value*/* LLVM memory address */, bool /* isReference */> >	symbols;

				Frame( llvm::BasicBlock* i, llvm::BasicBlock* b, llvm::BasicBlock* c );
				~Frame();

				llvm::Value*	Get( std::string name );
				void 			Set( std::string name, llvm::Value* value, bool isRef );
				void 			Del( std::string name );
				bool			isRef( std::string name );
		};
	}
}

#endif /* FRAME_H_ */
