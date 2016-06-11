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

#include "exo/jit/frame.h"
#include "exo/jit/symboltable.h"

namespace exo
{
	namespace jit
	{
		Frame::Frame( llvm::BasicBlock* i, llvm::BasicBlock* b ) :
			insertPoint( i ),
			breakTo( b ),
			localSymbols( std::make_shared<SymbolTable>() ),
			globalSymbols( std::make_shared<SymbolTable>() )
		{
		};

		// TODO: call destructors of local symbols
		Frame::~Frame()
		{
		};

		llvm::Value* Frame::Get( std::string name )
		{
			llvm::Value* value = localSymbols->Get( name );

			if( value == nullptr ) {
				value = globalSymbols->Get( name );

				if( value == nullptr ) {
					EXO_THROW( UnknownVar() << exo::exceptions::VariableName( name ) );
				}
			}

			return( value );
		}

		void Frame::Set( std::string name, llvm::Value* value, bool isRef )
		{
			localSymbols->Set( name, value, isRef );
		}

		void Frame::Del( std::string name )
		{
			localSymbols->Del( name );
		}

		bool Frame::isRef( std::string name )
		{
			llvm::Value* value = localSymbols->Get( name );

			if( value == nullptr ) {
				value = globalSymbols->Get( name );

				if( value == nullptr ) {
					EXO_THROW( UnknownVar() << exo::exceptions::VariableName( name ) );
				}

				return( globalSymbols->isRef( name ) );
			}

			return( localSymbols->isRef( name ) );
		}
	}
}
