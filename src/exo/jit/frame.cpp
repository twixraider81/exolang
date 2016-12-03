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

namespace exo
{
	namespace jit
	{
		Frame::Frame( llvm::BasicBlock* i, llvm::BasicBlock* b ) :
			insertPoint( i ),
			breakTo( b )
		{
		};

		// TODO: call destructors of local symbols
		Frame::~Frame()
		{
		};

		llvm::Value* Frame::Get( std::string name )
		{
			auto symbol = symbols.find( name );

			if( symbol != symbols.end() ) { // found symbol in local scope
				return( symbol->second.first );
			}

			Frame* pFrame = parent.get();
			while( pFrame != nullptr ) { // check parenting scope
				symbol = parent->symbols.find( name );
				if( symbol != symbols.end() ) {
					return( symbol->second.first );
				}

				pFrame = pFrame->parent.get();
			}

			EXO_THROW( UnknownVar() << exo::exceptions::VariableName( name ) );
		}

		void Frame::Set( std::string name, llvm::Value* value, bool isRef )
		{
			auto symbol = symbols.find( name );

			value->setName( name );

			if( symbol != symbols.end() ) { // found symbol in local scope
				symbol->second.first = value;
				return;
			}

			Frame* pFrame = parent.get();
			while( pFrame != nullptr ) { // check parenting scope
				symbol = parent->symbols.find( name );
				if( symbol != symbols.end() ) {
					symbol->second.first = value;
				}

				pFrame = pFrame->parent.get();
			}

			// no symbol found
			symbols.insert( std::make_pair( name, std::make_pair( value, isRef ) ) );
		}

		void Frame::Del( std::string name )
		{
			auto symbol = symbols.find( name );

			if( symbol == symbols.end() ) {
				EXO_THROW( UnknownVar() << exo::exceptions::VariableName( name ) );
			}

			symbols.erase( symbol );
		}

		bool Frame::isRef( std::string name )
		{
			auto symbol = symbols.find( name );

			if( symbol != symbols.end() ) { // found symbol in local scope
				return( symbol->second.second );
			}

			Frame* pFrame = parent.get();
			while( pFrame != nullptr ) { // check parenting scope
				symbol = parent->symbols.find( name );
				if( symbol != symbols.end() ) {
					return( symbol->second.second );
				}

				pFrame = pFrame->parent.get();
			}

			EXO_THROW( UnknownVar() << exo::exceptions::VariableName( name ) );
		}
	}
}
