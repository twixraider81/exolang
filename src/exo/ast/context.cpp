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

#include "exo/types/types.h"
#include "exo/ast/llvm.h"

#include "exo/ast/lexer/lexer"
#include "exo/ast/parser/parser.h"

#include "exo/ast/block.h"
#include "exo/ast/context.h"
#include "exo/ast/nodes/nodes.h"

namespace exo
{
	namespace ast
	{
		Context::Context( std::string cname, llvm::LLVMContext* c )
		{
			name = cname;
			context = c;
			module = new llvm::Module( name, *context );
		}

		void Context::pushBlock( llvm::BasicBlock *block)
		{
			blocks.push( new Block() );
			blocks.top()->block = block;
		}

		void Context::popBlock()
		{
			Block *top = blocks.top();
			blocks.pop();
			delete top;
		}

		void Context::Generate( exo::ast::nodes::StmtList* stmts )
		{
			llvm::FunctionType *ftype = llvm::FunctionType::get( llvm::Type::getVoidTy( *context ), false);
			entry = llvm::Function::Create(ftype, llvm::GlobalValue::InternalLinkage, "main", module);

			llvm::BasicBlock* block = llvm::BasicBlock::Create( *context, "entry", entry, 0 );
			pushBlock( block );
			stmts->Generate( *this );
			llvm::ReturnInst::Create( *context, block);
			popBlock();

#ifdef EXO_TRACE
			TRACE( "Generated LLVM IR:" );
			llvm::PassManager manager;
			manager.add( llvm::createPrintModulePass( &llvm::outs() ) );
			manager.run( *module );
#endif
		}

		void Context::Execute()
		{

		}
	}
}
