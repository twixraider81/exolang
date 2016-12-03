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
		llvm::BasicBlock* Stack::Push( llvm::BasicBlock* insertPoint, llvm::BasicBlock* breakTo )
		{
			if( frames.size() > 0 ) {
				// propagate exit/continue block in loops
				if( breakTo == nullptr ) {
					breakTo = frames.top()->breakTo;
				}

				std::unique_ptr<Frame> frame = std::make_unique<Frame>( insertPoint, breakTo );
				frame->parent = frames.top()->parent;

				frames.push( std::move( frame ) );
			} else {
				frames.push( std::make_unique<Frame>( insertPoint, breakTo ) );
			}

			return( frames.top()->insertPoint );
		}

		llvm::BasicBlock* Stack::Pop()
		{
			llvm::BasicBlock* insertPoint;

			if( frames.size() == 0 ) {
				EXO_THROW_MSG( "Stack empty while trying to reduce further." );
			}

			if( frames.top()->breakTo == nullptr ) {
				insertPoint = frames.top()->insertPoint;
				frames.pop();

				if( frames.size() > 0 ) {
					insertPoint = frames.top()->insertPoint;
				}
			} else {
				insertPoint = frames.top()->breakTo;
				frames.pop();
			}

			return( insertPoint );
		}

		llvm::BasicBlock* Stack::Join( llvm::BasicBlock* block )
		{
			EXO_DEBUG_LOG( trace, "Joining (" << frames.top()->insertPoint->getName().str() << ") with (" << block->getName().str() << ")" );
			frames.top()->insertPoint = block;
			return block;
		}

		llvm::BasicBlock* Stack::Block()
		{
			return( frames.top()->insertPoint );
		}

		llvm::BasicBlock* Stack::Exit()
		{
			if( frames.size() > 1 ) {
				if( frames.top()->breakTo != nullptr ) { // custom exit block, e.g. in loops with continue block
					return( frames.top()->breakTo );
				} else { // normal block, e.g. previous stack entry

					std::unique_ptr<Frame> current = std::unique_ptr<Frame>( frames.top().release() );
					llvm::BasicBlock* breakTo = nullptr;
					frames.pop();

					if( frames.size() > 0 ) {
						breakTo = frames.top()->insertPoint;
					} else {
						EXO_THROW_MSG( "Can not lookup exit block." );
					}

					frames.push( std::move(current) );
					return( breakTo );
				}
			}

			EXO_THROW_MSG( "Can not lookup exit block." );
			return( nullptr );
		}

		std::string Stack::blockName()
		{
			 return( frames.top()->insertPoint->getName().str() );
		}

		llvm::Value* Stack::Get( std::string name )
		{
			return( frames.top()->Get( name ) );
		}

		void Stack::Set( std::string name, llvm::Value* value, bool isRef )
		{
			frames.top()->Set( name, value, isRef );
		}

		void Stack::Del( std::string name )
		{
			frames.top()->Del( name );
		}

		bool Stack::isRef( std::string name )
		{
			return( frames.top()->isRef( name ) );
		}
	}
}
