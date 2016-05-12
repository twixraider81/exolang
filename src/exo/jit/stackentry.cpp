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

#include "exo/jit/stackentry.h"

namespace exo
{
	namespace jit
	{
		StackEntry::StackEntry( llvm::BasicBlock* b, llvm::BasicBlock* e ) : block( b ), exit( e )
		{
		};

		StackEntry::~StackEntry()
		{
		};

		llvm::Value* StackEntry::getSymbol( std::string name )
		{
			std::map<std::string,llvm::Value*>::iterator it = symbols.find( name );
			if( it == symbols.end() ) {
				EXO_THROW( UnknownVar() << exo::exceptions::VariableName( name ) );
			}

			return( it->second );
		}

		void StackEntry::setSymbol( std::string name, llvm::Value* value )
		{
			std::map<std::string,llvm::Value*>::iterator it = symbols.find( name );
			if( it == symbols.end() ) {
				value->setName( name );
				symbols.insert( std::pair<std::string,llvm::Value*>( name, value ) );
			} else {
				symbols[name] = value;
			}
		}

		void StackEntry::delSymbol( std::string name )
		{
			std::map<std::string,llvm::Value*>::iterator it = symbols.find( name );
			if( it == symbols.end() ) {
				EXO_THROW( UnknownVar() << exo::exceptions::VariableName( name ) );
			}

			symbols.erase( name );
		}
	}
}
