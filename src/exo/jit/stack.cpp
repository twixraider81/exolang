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

#include "exo/jit/stack.h"

namespace exo
{
	namespace jit
	{
		Stack::Stack()
		{
		};

		Stack::~Stack()
		{
		};

		llvm::BasicBlock* Stack::Push( llvm::BasicBlock* block, llvm::BasicBlock* exit )
		{
			if( entries.size() > 0 ) {
				// propagate exit/continue block in loops
				if( exit == nullptr ) {
					exit = entries.top()->exit;
				}

				std::shared_ptr<StackEntry> newBlock = std::make_shared<StackEntry>( block, exit );
				// inherit parent variables
				newBlock->symbols = entries.top()->symbols;
				entries.push( newBlock );
			} else {
				entries.push( std::make_shared<StackEntry>( block, exit ) );
			}

			EXO_DEBUG_LOG( trace, "Switching to (" << entries.top()->block->getName().str() << ")" );
			if( exit != nullptr ) {
				EXO_DEBUG_LOG( trace, "Continue at exit (" << entries.top()->exit->getName().str() << ")" );
			}

			return( entries.top()->block );
		}

		llvm::BasicBlock* Stack::Pop()
		{
			llvm::BasicBlock* b;

			if( entries.size() == 0 ) {
				EXO_THROW_MSG( "Stack empty while trying to reduce further." );
			}

			if( entries.top()->exit == nullptr ) {
				b = entries.top()->block;
				entries.pop();

				if( entries.size() > 0 ) {
					EXO_DEBUG_LOG( trace, "Continuing at (" << entries.top()->block->getName().str() << ")" );
					b = entries.top()->block;
				}
			} else {
				EXO_DEBUG_LOG( trace, "Continuing at exit (" << entries.top()->exit->getName().str() << ")" );
				b = entries.top()->exit;
				entries.pop();
			}

			return( b );
		}

		llvm::BasicBlock* Stack::Join( llvm::BasicBlock* block )
		{
			EXO_DEBUG_LOG( trace, "Joining (" << entries.top()->block->getName().str() << ") with (" << block->getName().str() << ")" );
			entries.top()->block = block;
			return block;
		}

		llvm::BasicBlock* Stack::Block()
		{
			return( entries.top()->block );
		}

		llvm::BasicBlock* Stack::Exit()
		{
			if( entries.size() > 1 ) {
				if( entries.top()->exit != nullptr ) { // custom exit block, e.g. in loops with continue block
					return( entries.top()->exit );
				} else { // normal block, e.g. previous stack entry

					std::shared_ptr<StackEntry> current = entries.top();
					llvm::BasicBlock* exit = nullptr;
					entries.pop();

					if( entries.size() > 0 ) {
						exit = entries.top()->block;
					} else {
						EXO_THROW_MSG( "Can not lookup exit block." );
					}

					entries.push( current );
					return( exit );
				}
			}

			EXO_THROW_MSG( "Can not lookup exit block." );
			return nullptr;
		}

		std::string Stack::blockName()
		{
			 return( entries.top()->block->getName().str() );
		}

		llvm::Value* Stack::getSymbol( std::string name )
		{
			return( entries.top()->getSymbol( name ) );
		}

		void Stack::setSymbol( std::string name, llvm::Value* value )
		{
			entries.top()->setSymbol( name, value );
		}

		void Stack::delSymbol( std::string name )
		{
			entries.top()->delSymbol( name );
		}
	}
}
