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
#include "exo/jit/codegen.h"
#include "exo/jit/type/types.h"

namespace exo
{
	namespace jit
	{
		Codegen::Codegen( std::string cname, std::string target ) : builder( llvm::getGlobalContext() )
		{
			name = cname;
			module = new llvm::Module( cname, llvm::getGlobalContext() );
			module->setTargetTriple( target );
		}

		Codegen::Codegen( std::string cname ) : Codegen( cname, llvm::sys::getProcessTriple() )
		{

		}

		Codegen::~Codegen()
		{
		}

		void Codegen::pushBlock( llvm::BasicBlock* block)
		{
			blocks.push( new Block() );
			blocks.top()->block = block;

			// reset out current insert point
			builder.SetInsertPoint( blocks.top()->block );

			TRACESECTION( "CONTEXT", "pushing new block onto stack, new stacksize:" << blocks.size() );
		}

		void Codegen::popBlock()
		{
			blocks.pop();

			// reset out current insert point
			if( blocks.size() > 0 ) {
				builder.SetInsertPoint( blocks.top()->block );
			}

			TRACESECTION( "CONTEXT", "poping block from stack, new stacksize:" << blocks.size() );
		}

		llvm::BasicBlock* Codegen::getCurrentBlock()
		{
			 return( blocks.top()->block );
		}

		std::map<std::string, llvm::Value*>& Codegen::Variables()
		{
			return( blocks.top()->variables );
		}

		/*
		 * FIXME: wont be needed anymore once we get proper type system
		 */
		llvm::Type* Codegen::getType( exo::ast::Type* type, llvm::LLVMContext& context )
		{
			if( type->info == &typeid( exo::jit::types::IntegerType ) ) {
				return( llvm::Type::getInt64Ty( context ) );
			} else if( type->info == &typeid( exo::jit::types::FloatType ) ) {
				return( llvm::Type::getDoubleTy( context ) );
			} else if( type->info == &typeid( exo::jit::types::BooleanType ) ) {
				return( llvm::Type::getInt1Ty( context ) );
			}

			return( llvm::Type::getVoidTy( context ) );
		}


		llvm::Value* Codegen::Generate( exo::ast::Node* node )
		{
			BOOST_THROW_EXCEPTION( exo::exceptions::UnexpectedNode( typeid(*node).name() ) );
			return( NULL );
		}

		llvm::Value* Codegen::Generate( exo::ast::Tree* tree )
		{
			llvm::FunctionType *ftype = llvm::FunctionType::get( llvm::Type::getVoidTy( module->getContext() ), false);
			entry = llvm::Function::Create( ftype, llvm::GlobalValue::InternalLinkage, "main", module );
			llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), "entry", entry, 0 );

			pushBlock( block );

			Generate( tree->stmts );

			llvm::Value* retval = llvm::ReturnInst::Create( module->getContext(), block );
			popBlock();

			return( retval );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtList* stmts )
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

		llvm::Value* Codegen::Generate( exo::ast::VarDecl* decl )
		{
			TRACESECTION( "IR", "creating variable $" << decl->name << " " << decl->type->info->name() << " in (" << name << ")" );

			Variables()[ decl->name ] = new llvm::AllocaInst( getType( decl->type, module->getContext() ), decl->name.c_str(), getCurrentBlock() );

			TRACESECTION( "IR", "new variable map size: " << Variables().size() );

			if( decl->expression ) {
				exo::ast::VarAssign* a = new exo::ast::VarAssign( decl->name, decl->expression );
				a->Generate( this );
			}

			return( Variables()[ decl->name ] );
		}

		llvm::Value* Codegen::Generate( exo::ast::VarAssign* assign )
		{
			TRACESECTION( "IR", "assigning variable $" << assign->name << " in (" << name << ")" );

			if( Variables().find( assign->name ) == Variables().end() ) {
				BOOST_THROW_EXCEPTION( exo::exceptions::UnknownVar( assign->name ) );
			}

			return( new llvm::StoreInst( assign->expression->Generate( this ), Variables()[ assign->name ], false, getCurrentBlock() ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::ValueInt* val )
		{
			TRACESECTION( "IR", "generating integer " << val->value << " in (" << name << ")" );
			exo::jit::types::IntegerType* iType = new exo::jit::types::IntegerType( &module->getContext(), val->value );
			return( iType->value );
		}

		llvm::Value* Codegen::Generate( exo::ast::ValueFloat* val )
		{
			TRACESECTION( "IR", "generating float: " << val->value << " in (" << name << ")" );
			exo::jit::types::FloatType* fType = new exo::jit::types::FloatType( &module->getContext(), val->value );
			return( fType->value );
		}

		llvm::Value* Codegen::Generate( exo::ast::ValueBool* val )
		{
			TRACESECTION( "IR", "generating boolean: " << val->value << " in (" << name << ")" );
			exo::jit::types::BooleanType* bType = new exo::jit::types::BooleanType( &module->getContext(), val->value );
			return( bType->value );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstExpr* expr )
		{
			TRACESECTION( "IR", "generating constant expression: " << expr->name << " in (" << name << ")" );
			return( expr->expression->Generate( this ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::BinaryOp* op )
		{
			TRACESECTION( "IR", "generating binary operation: " << op->op << " in (" << name << ")" );

			llvm::Value* lhs = op->lhs->Generate( this );
			llvm::Value* rhs = op->rhs->Generate( this );

			if( op->op == "+" ) {
				return( builder.CreateAdd( lhs, rhs, "" ) );
			} else if( op->op == "-" ) {
				return( builder.CreateSub( lhs, rhs, "" ) );
			} else if( op->op == "*" ) {
				return( builder.CreateMul( lhs, rhs, "" ) );
			} else if( op->op == "/" ) {
				return( builder.CreateSDiv( lhs, rhs, "" ) );
			} else {
				BOOST_THROW_EXCEPTION( exo::exceptions::UnknownBinaryOp( op->op ) );
				return( NULL );
			}
		}

		llvm::Value* Codegen::Generate( exo::ast::VarExpr* expr )
		{
			TRACESECTION( "IR", "generating variable expression $" << expr->variable << " in (" << name << ")" );

			if( Variables().find( expr->variable ) == Variables().end() ) {
				BOOST_THROW_EXCEPTION( exo::exceptions::UnknownVar( expr->variable ) );
			}

			return( builder.CreateLoad( Variables()[ expr->variable ], expr->variable ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::CmpOp* op )
		{
			TRACESECTION( "IR", "generating comparison " << op->op << " in (" << name << ")" );

			llvm::Value* lhs = op->lhs->Generate( this );
			llvm::Value* rhs = op->rhs->Generate( this );

			return( lhs );
		}

		llvm::Value* Codegen::Generate( exo::ast::FunDecl* decl )
		{
			TRACESECTION( "IR", "generating function " << decl->name << " in (" << name << ")" );

			std::vector<llvm::Type*> fArgs;
			std::vector<exo::ast::VarDecl*>::iterator it;

			for( it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				TRACESECTION( "IR", "generating argument " << (**it).name );
				fArgs.push_back( getType( (*it)->type, module->getContext() ) );
			}

			llvm::FunctionType* fType = llvm::FunctionType::get( getType( decl->returnType, module->getContext() ), fArgs, false );
			llvm::Function* function = llvm::Function::Create( fType, llvm::GlobalValue::InternalLinkage, decl->name, module );
			llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), decl->name, function, 0 );

			pushBlock( block );

			for( it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				TRACESECTION( "IR", "generating " << typeid(**it).name() );
				(*it)->Generate( this );
			}

			Generate( decl->stmts );

			/*
			 * TODO: Generate null, auto returns or else fail
			 */
			//llvm::ReturnInst::Create( module->getContext(), block );
			//llvm::verifyFunction( *function );
			popBlock();

			return( function );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtReturn* stmt )
		{
			TRACESECTION( "IR", "generating return statement" );
			return( builder.CreateRet( stmt->expression->Generate( this ) ) );
		}
	}
}
