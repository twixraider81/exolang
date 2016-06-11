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

#ifndef SYMBOLTABLE_H_
#define SYMBOLTABLE_H_

#include "exo/exo.h"
#include "exo/jit/llvm.h"

namespace exo
{
	namespace jit
	{
		class SymbolTable
		{
			public:
				std::unordered_map< std::string, std::pair< llvm::Value*, bool> >	symbols;

				llvm::Value*	Get( std::string name );
				void 			Set( std::string name, llvm::Value* value, bool isRef = false );
				void 			Del( std::string name );
				bool			isRef( std::string name );
		};
	}
}

#endif /* SYMBOLTABLE_H_ */
