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

#include "exo/jit/symboltable.h"

namespace exo
{
	namespace jit
	{
		llvm::Value* SymbolTable::Get( std::string name )
		{
			auto symbol = symbols.find( name );
			if( symbol == symbols.end() ) {
				return( nullptr );
			}

			return( symbol->second.first );
		}

		void SymbolTable::Set( std::string name, llvm::Value* value, bool isRef )
		{
			value->setName( name );
			symbols.insert( std::make_pair( name, std::make_pair( value, isRef ) ) );
		}

		// TODO: call destructors
		void SymbolTable::Del( std::string name )
		{
			auto symbol = symbols.find( name );
			if( symbol != symbols.end() ) {
				symbols.erase( symbol );
			}
		}

		bool SymbolTable::isRef( std::string name )
		{
			auto symbol = symbols.find( name );
			if( symbol == symbols.end() ) {
				return( false );
			}

			return( symbol->second.second );
		}
	}
}
