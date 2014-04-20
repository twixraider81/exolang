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

#include "exo/jit/block.h"
#include "exo/ast/nodes.h"
#include "exo/ast/tree.h"
#include "exo/jit/context.h"
#include "exo/jit/type/types.h"

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


		llvm::Value* Context::Generate( exo::ast::Node* node )
		{
			BOOST_THROW_EXCEPTION( exo::exceptions::UnexpectedNode( typeid(*node).name() ) );
			return( NULL );
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
			TRACESECTION( "IR", "generating statements (" << name << ")" );

			std::vector<exo::ast::Stmt*>::iterator it;
			llvm::Value *last = NULL;

			for( it = stmts->list.begin(); it != stmts->list.end(); it++ ) {
				TRACESECTION( "IR", "generating " << typeid(**it).name() );
				last = (*it)->Generate( this );
			}

			return( last );
		}

		llvm::Value* Context::Generate( exo::ast::VarDecl* decl )
		{
			TRACESECTION( "IR", "creating variable $" << decl->name << " " << decl->type->info->name() << " in (" << name << ")" );

			llvm::AllocaInst* memory;
			if( decl->type->info->name() == typeid( exo::jit::types::IntegerType ).name() ) {
				exo::jit::types::IntegerType* type = new exo::jit::types::IntegerType( context, 0 );
				memory = new llvm::AllocaInst( type->type, decl->name.c_str(), getCurrentBlock() );
			} else if( decl->type->info->name() == typeid( exo::jit::types::FloatType ).name() ) {
				exo::jit::types::FloatType* type = new exo::jit::types::FloatType( context, 0 );
				memory = new llvm::AllocaInst( type->type, decl->name.c_str(), getCurrentBlock() );
			} else if( decl->type->info->name() == typeid( exo::jit::types::BooleanType ).name() ) {
				exo::jit::types::BooleanType* type = new exo::jit::types::BooleanType( context, false );
				memory = new llvm::AllocaInst( type->type, decl->name.c_str(), getCurrentBlock() );
			} else {
				exo::jit::types::IntegerType* type = new exo::jit::types::IntegerType( context, 0 );
				memory = new llvm::AllocaInst( type->type, decl->name.c_str(), getCurrentBlock() );
			}
			Variables()[ decl->name ] = memory;

			TRACESECTION( "IR", "new variable map size: " << Variables().size() );

			if( decl->expression ) {
				exo::ast::VarAssign* a = new exo::ast::VarAssign( decl->name, decl->expression );
				a->Generate( this );
			}

			return( memory );
		}

		llvm::Value* Context::Generate( exo::ast::VarAssign* assign )
		{
			TRACESECTION( "IR", "assigning variable $" << assign->name << " in (" << name << ")" );

			if( Variables().find( assign->name ) == Variables().end() ) {
				BOOST_THROW_EXCEPTION( exo::exceptions::UnknownVar( assign->name ) );
			}

			return( new llvm::StoreInst( assign->expression->Generate( this ), Variables()[ assign->name ], false, getCurrentBlock() ) );
		}

		llvm::Value* Context::Generate( exo::ast::ValueInt* val )
		{
			TRACESECTION( "IR", "generating integer " << val->value << " in (" << name << ")" );
			exo::jit::types::IntegerType* iType = new exo::jit::types::IntegerType( context, val->value );
			return( iType->value );
		}

		llvm::Value* Context::Generate( exo::ast::ValueFloat* val )
		{
			TRACESECTION( "IR", "generating float: " << val->value << " in (" << name << ")" );
			exo::jit::types::FloatType* fType = new exo::jit::types::FloatType( context, val->value );
			return( fType->value );
		}

		llvm::Value* Context::Generate( exo::ast::ValueBool* val )
		{
			TRACESECTION( "IR", "generating boolean: " << val->value << " in (" << name << ")" );
			exo::jit::types::BooleanType* bType = new exo::jit::types::BooleanType( context, val->value );
			return( bType->value );
		}

		llvm::Value* Context::Generate( exo::ast::ConstExpr* expr )
		{
			TRACESECTION( "IR", "generating constant expression: " << expr->name << " in (" << name << ")" );
			return( expr->expression->Generate( this ) );
		}
	}
}
