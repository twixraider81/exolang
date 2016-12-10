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
		llvm::BasicBlock* Stack::Push( llvm::BasicBlock* insertBlock, llvm::BasicBlock* breakBlock, llvm::BasicBlock* conditionBlock )
		{
			EXO_DEBUG_LOG( trace, "Frame pushed (" << insertBlock->getName().str() << ")" );

			if( frames.size() > 0 ) {
				// propagate exit/continue block
				if( breakBlock == nullptr ) {
					breakBlock = frames.top()->breakBlock;
				}
				if( conditionBlock == nullptr ) {
					conditionBlock = frames.top()->conditionBlock;
				}

				std::shared_ptr<Frame> frame = std::make_shared<Frame>( insertBlock, breakBlock, conditionBlock );
				frame->parent = frames.top();

				if( frame->parent != nullptr ) {
					EXO_DEBUG_LOG( trace, "Frame parent (" << frame->parent->insertBlock->getName().str() << ")" );
				}

				frames.push( std::move( frame ) );
			} else {
				frames.push( std::make_shared<Frame>( insertBlock, breakBlock, conditionBlock ) );
			}

			return( frames.top()->insertBlock );
		}

		llvm::BasicBlock* Stack::Pop()
		{
			llvm::BasicBlock* insertBlock;

			if( frames.size() == 0 ) {
				EXO_THROW_MSG( "Stack empty while trying to reduce further." );
			}

			EXO_DEBUG_LOG( trace, "Frame popped (" << frames.top()->insertBlock->getName().str() << ")" );

			if( frames.top()->breakBlock == nullptr ) {
				insertBlock = frames.top()->insertBlock;
				frames.pop();

				if( frames.size() > 0 ) {
					insertBlock = frames.top()->insertBlock;
				}
			} else {
				insertBlock = frames.top()->breakBlock;
				frames.pop();
			}

			return( insertBlock );
		}

		llvm::BasicBlock* Stack::Join( llvm::BasicBlock* block )
		{
			EXO_DEBUG_LOG( trace, "Joining (" << frames.top()->insertBlock->getName().str() << ") with (" << block->getName().str() << ")" );
			frames.top()->insertBlock = block;
			return block;
		}

		llvm::BasicBlock* Stack::Block()
		{
			return( frames.top()->insertBlock );
		}

		llvm::BasicBlock* Stack::Break()
		{
			if( frames.size() > 1 ) {
				if( frames.top()->breakBlock != nullptr ) { // custom exit block, e.g. in loops with continue block
					return( frames.top()->breakBlock );
				} else { // normal block, e.g. previous stack entry

					std::shared_ptr<Frame> current = frames.top();
					llvm::BasicBlock* breakBlock = nullptr;
					frames.pop();

					if( frames.size() > 0 ) {
						breakBlock = frames.top()->insertBlock;
					} else {
						EXO_THROW_MSG( "Can not lookup exit block." );
					}

					frames.push( current );
					return( breakBlock );
				}
			}

			EXO_THROW_MSG( "Can not lookup exit block." );
			return( nullptr );
		}

		llvm::BasicBlock* Stack::Continue()
		{
			if( frames.size() > 1 ) {
				if( frames.top()->conditionBlock != nullptr ) { // custom condition block, e.g. in loops with continue block
					return( frames.top()->conditionBlock );
				} else { // normal block, e.g. previous stack entry

					std::shared_ptr<Frame> current = frames.top();
					llvm::BasicBlock* conditionBlock = nullptr;
					frames.pop();

					if( frames.size() > 0 ) {
						conditionBlock = frames.top()->insertBlock;
					} else {
						EXO_THROW_MSG( "Can not lookup condition block." );
					}

					frames.push( current );
					return( conditionBlock );
				}
			}

			EXO_THROW_MSG( "Can not lookup condition block." );
			return( nullptr );
		}

		std::string Stack::blockName()
		{
			 return( frames.top()->insertBlock->getName().str() );
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
