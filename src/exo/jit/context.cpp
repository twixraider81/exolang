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

#include "exo/jit/llvm.h"
#include "exo/jit/type/types.h"

#include "exo/lexer/lexer"
#include "exo/parser/parser.h"

#include "exo/jit/block.h"
#include "exo/jit/context.h"
#include "exo/ast/nodes.h"

namespace exo
{
	namespace jit
	{
		Context::Context( std::string cname, llvm::LLVMContext* c )
		{
			name = cname;
			context = c;

			module = new llvm::Module( name, *context );
			module->setTargetTriple( llvm::sys::getProcessTriple() );
		}

		Context::~Context()
		{
			/*
			 * FIXME: gets freed
			 */
			if( module ) {
				//delete module;
			}
		}

		void Context::pushBlock( llvm::BasicBlock* block)
		{
			blocks.push( new Block() );
			blocks.top()->block = block;
			TRACESECTION( "CONTEXT", "pushing new block onto stack, new stacksize:" << blocks.size() );
		}

		void Context::popBlock()
		{
			//delete blocks.top()->block;
			blocks.pop();
			TRACESECTION( "CONTEXT", "poping block from stack, new stacksize:" << blocks.size() );
		}

		llvm::BasicBlock* Context::getCurrentBlock()
		{
			 return( blocks.top()->block );
		}

		std::map<std::string, llvm::Value*>& Context::Variables()
		{
			return( blocks.top()->variables );
		}

		llvm::Value* Context::Generate( exo::ast::Tree* tree )
		{
			llvm::IRBuilder<> builder( *context );

			llvm::FunctionType *ftype = llvm::FunctionType::get( llvm::Type::getVoidTy( *context ), false);
			entry = llvm::Function::Create( ftype, llvm::GlobalValue::InternalLinkage, "main", module );
			llvm::BasicBlock* block = llvm::BasicBlock::Create( *context, "entry", entry, 0 );
			builder.SetInsertPoint( block );
			pushBlock( block );

			Generate( tree->stmts );

			llvm::Value* retval = llvm::ReturnInst::Create( *context, block );
			popBlock();

			return( retval );
		}

		llvm::Value* Context::Generate( exo::ast::StmtList* stmts )
		{
			TRACESECTION( "IR", "generating statements:" );

			std::vector<exo::ast::Stmt*>::iterator it;
			llvm::Value *last = NULL;

			for( it = stmts->list.begin(); it != stmts->list.end(); it++ ) {
				TRACESECTION( "IR", "generating " << typeid(**it).name() );
				last = Generate( *it );
			}

			return( last );
		}

		llvm::Value* Context::Generate( exo::ast::VarDecl* decl )
		{
			TRACESECTION( "IR", "creating variable: $" << decl->name );

			// allocate memory and push variable onto the local stack
			llvm::AllocaInst* memory = new llvm::AllocaInst( llvm::Type::getInt64Ty( *context ), name.c_str(), getCurrentBlock() );
			Variables()[ name ] = memory;

			TRACESECTION( "IR", "new variable map size:" << Variables().size() );

			if( decl->expression ) {
				exo::ast::VarAssign* a = new exo::ast::VarAssign( decl->name, decl->expression );
				Generate( a );
			}

			return( memory );
		}

		llvm::Value* Context::Generate( exo::ast::VarAssign* assign )
		{
			TRACESECTION( "IR", "assigning variable $" << assign->name );

			if( Variables().find( assign->name ) == Variables().end() ) {
				BOOST_THROW_EXCEPTION( exo::exceptions::UnknownVar( assign->name ) );
			}

			llvm::StoreInst* store = new llvm::StoreInst( Generate( assign->expression ), Variables()[name], false, getCurrentBlock() );
			return( store );
		}

		llvm::Value* Context::Generate( exo::ast::Stmt* stmt )
		{
			return( NULL );
		}

		llvm::Value* Context::Generate( exo::ast::Expr* expr )
		{
			return( NULL );
		}
	}
}
