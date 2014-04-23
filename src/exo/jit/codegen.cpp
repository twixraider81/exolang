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

		void Codegen::pushBlock( llvm::BasicBlock* block, std::string name )
		{
			blocks.push( new Block() );
			blocks.top()->block = block;
			blocks.top()->name = name;

			// reset our new insert point
			builder.SetInsertPoint( blocks.top()->block );

			TRACESECTION( "CONTEXT", "pushing \"" << getCurrentBlockName() << "\" onto stack, new stacksize:" << blocks.size() );
		}

		void Codegen::popBlock()
		{
			TRACESECTION( "CONTEXT", "poping \"" << getCurrentBlockName() << "\" from stack, new stacksize:" << blocks.size() );

			blocks.pop();

			// reset our insert point
			if( blocks.size() > 0 ) {
				builder.SetInsertPoint( blocks.top()->block );
			}
		}

		llvm::BasicBlock* Codegen::getCurrentBasicBlock()
		{
			 return( blocks.top()->block );
		}

		std::string Codegen::getCurrentBlockName()
		{
			 return( blocks.top()->name );
		}

		std::map<std::string, llvm::Value*>& Codegen::getCurrentBlockVars()
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
			} else if( type->info == &typeid( exo::jit::types::ClassType ) ) {
				return( module->getTypeByName( type->name ) );
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
			entry = llvm::Function::Create( ftype, llvm::GlobalValue::InternalLinkage, name, module );
			llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), "entry", entry, 0 );

			pushBlock( block, name );

			Generate( tree->stmts );

			llvm::Value* retval = llvm::ReturnInst::Create( module->getContext(), block );

			popBlock();

			return( retval );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtList* stmts )
		{
			TRACESECTION( "IR", "generating statements (" << getCurrentBlockName() << ")" );

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
			TRACESECTION( "IR", "creating variable $" << decl->name << " " << decl->type->info->name() << " in (" << getCurrentBlockName() << ")" );

			getCurrentBlockVars()[ decl->name ] = new llvm::AllocaInst( getType( decl->type, module->getContext() ), decl->name.c_str(), getCurrentBasicBlock() );

			TRACESECTION( "IR", "new variable map size: " << getCurrentBlockVars().size() );

			if( decl->expression ) {
				exo::ast::VarAssign* a = new exo::ast::VarAssign( decl->name, decl->expression );
				a->Generate( this );
			}

			return( getCurrentBlockVars()[ decl->name ] );
		}

		llvm::Value* Codegen::Generate( exo::ast::VarAssign* assign )
		{
			TRACESECTION( "IR", "assigning variable $" << assign->name << " in (" << getCurrentBlockName() << ")" );

			if( getCurrentBlockVars().find( assign->name ) == getCurrentBlockVars().end() ) {
				BOOST_THROW_EXCEPTION( exo::exceptions::UnknownVar( assign->name ) );
			}

			return( new llvm::StoreInst( assign->expression->Generate( this ), getCurrentBlockVars()[ assign->name ], false, getCurrentBasicBlock() ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::ValueInt* val )
		{
			TRACESECTION( "IR", "generating integer \"" << val->value << "\" in (" << getCurrentBlockName() << ")" );
			exo::jit::types::IntegerType* iType = new exo::jit::types::IntegerType( &module->getContext(), val->value );
			return( iType->value );
		}

		llvm::Value* Codegen::Generate( exo::ast::ValueFloat* val )
		{
			TRACESECTION( "IR", "generating float \"" << val->value << "\" in (" << getCurrentBlockName() << ")" );
			exo::jit::types::FloatType* fType = new exo::jit::types::FloatType( &module->getContext(), val->value );
			return( fType->value );
		}

		llvm::Value* Codegen::Generate( exo::ast::ValueBool* val )
		{
			TRACESECTION( "IR", "generating boolean \"" << val->value << "\" in (" << getCurrentBlockName() << ")" );
			exo::jit::types::BooleanType* bType = new exo::jit::types::BooleanType( &module->getContext(), val->value );
			return( bType->value );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstExpr* expr )
		{
			TRACESECTION( "IR", "generating constant expression \"" << expr->name << "\" in (" << getCurrentBlockName() << ")" );
			return( expr->expression->Generate( this ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::BinaryOp* op )
		{
			TRACESECTION( "IR", "generating binary operation: " << op->op << " in (" << getCurrentBlockName() << ")" );

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
			TRACESECTION( "IR", "generating variable expression $" << expr->variable << " in (" << getCurrentBlockName() << ")" );

			if( getCurrentBlockVars().find( expr->variable ) == getCurrentBlockVars().end() ) {
				BOOST_THROW_EXCEPTION( exo::exceptions::UnknownVar( expr->variable ) );
			}

			return( builder.CreateLoad( getCurrentBlockVars()[ expr->variable ], expr->variable ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::CmpOp* op )
		{
			TRACESECTION( "IR", "generating comparison " << op->op << " in (" << getCurrentBlockName() << ")" );

			llvm::Value* lhs = op->lhs->Generate( this );
			llvm::Value* rhs = op->rhs->Generate( this );

			return( lhs );
		}

		llvm::Value* Codegen::Generate( exo::ast::FunDecl* decl )
		{
			TRACESECTION( "IR", "generating function " << decl->name << " in (" << getCurrentBlockName() << ")" );

			std::vector<llvm::Type*> fArgs;
			std::vector<exo::ast::VarDecl*>::iterator it;

			for( it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				TRACESECTION( "IR", "generating argument $" << (**it).name );
				fArgs.push_back( getType( (*it)->type, module->getContext() ) );
			}

			llvm::FunctionType* fType = llvm::FunctionType::get( getType( decl->returnType, module->getContext() ), fArgs, false );
			llvm::Function* function = llvm::Function::Create( fType, llvm::GlobalValue::InternalLinkage, decl->name, module );
			llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), decl->name, function, 0 );

			pushBlock( block, decl->name );

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
			TRACESECTION( "IR", "generating return statement in (" << getCurrentBlockName() << ")" );
			return( builder.CreateRet( stmt->expression->Generate( this ) ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::FunCall* call )
		{
			TRACESECTION( "IR", "generating function call " << call->name << " in (" << getCurrentBlockName() << ")" );

			llvm::Function* callee = module->getFunction( call->name );

			if( callee == 0 ) {
				BOOST_THROW_EXCEPTION( exo::exceptions::UnknownFunction( call->name ) );
			}

			if( callee->arg_size() != call->arguments->list.size() ) {
				BOOST_THROW_EXCEPTION( exo::exceptions::InvalidCall( call->name, "expected arguments mismatch" ) );
			}

			std::vector<exo::ast::Expr*>::iterator it;
			std::vector<llvm::Value*> arguments;

			for( it = call->arguments->list.begin(); it != call->arguments->list.end(); it++ ) {
				TRACESECTION( "IR", "generating argument $" << typeid(**it).name() );
				arguments.push_back( (*it)->Generate( this ) );
			}

			return( builder.CreateCall( callee, arguments, "call" ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtExpr* stmt )
		{
			TRACESECTION( "IR", "generating expression statement in (" << getCurrentBlockName() << ")" );
			return( stmt->expression->Generate( this ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::FunDeclProto* decl )
		{
			TRACESECTION( "IR", "prototyping function " << decl->name << " in (" << getCurrentBlockName() << ")" );

			std::vector<llvm::Type*> fArgs;
			std::vector<exo::ast::VarDecl*>::iterator it;

			for( it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				TRACESECTION( "IR", "generating argument $" << (**it).name );
				fArgs.push_back( getType( (*it)->type, module->getContext() ) );
			}

			llvm::FunctionType* fType = llvm::FunctionType::get( getType( decl->returnType, module->getContext() ), fArgs, false );
			llvm::Function* function = llvm::Function::Create( fType, llvm::GlobalValue::ExternalLinkage, decl->name, module );

			return( function );
		}

		llvm::Value* Codegen::Generate( exo::ast::ClassDecl* decl )
		{
			TRACESECTION( "IR", "creating class " << decl->name << "; " << decl->block->properties.size() << " properties; " << decl->block->methods.size() << " methods in (" << getCurrentBlockName() << ")" );

			std::vector<llvm::Type*> properties;
			std::vector<exo::ast::VarDecl*>::iterator pit;
			for( pit = decl->block->properties.begin(); pit != decl->block->properties.end(); pit++ ) {
				TRACESECTION( "IR", "generating property $" << (*pit)->name << " (" << decl->name << ")" );
				properties.push_back( getType( (*pit)->type, module->getContext() ) );
			}

			llvm::StructType* structClass = llvm::StructType::create( module->getContext(), properties, decl->name );


			std::vector<llvm::Type*> methods;
			std::vector<exo::ast::FunDecl*>::iterator mit;
			for( mit = decl->block->methods.begin(); mit != decl->block->methods.end(); mit++ ) {
				// think about how to construct sane names
				std::string mName = "__" + decl->name + "_" + (*mit)->name;

				TRACESECTION( "IR", "generating method " << (*mit)->name << " (" << decl->name << " - " << mName << ")" );

				std::vector<llvm::Type*> mArgs;
				std::vector<exo::ast::VarDecl*>::iterator it;

				// pointer to a class struct as 1.st param
				mArgs.push_back( llvm::PointerType::getUnqual( structClass ) );
				for( it = (*mit)->arguments->list.begin(); it != (*mit)->arguments->list.end(); it++ ) {
					TRACESECTION( "IR", "generating argument $" << (*it)->name );
					mArgs.push_back( getType( (*it)->type, module->getContext() ) );
				}

				llvm::FunctionType* mType = llvm::FunctionType::get( getType( (*mit)->returnType, module->getContext() ), mArgs, false );
				llvm::Function* method = llvm::Function::Create( mType, llvm::GlobalValue::InternalLinkage, mName, module );
				llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), mName, method, 0 );

				pushBlock( block, (*mit)->name );

				for( it = (*mit)->arguments->list.begin(); it != (*mit)->arguments->list.end(); it++ ) {
					TRACESECTION( "IR", "generating " << typeid(**it).name() );
					(*it)->Generate( this );
				}

				Generate( (*mit)->stmts );

				popBlock();
			}

			/*
			 * FIXME: should probably return nothing
			 */
			return( llvm::ConstantInt::getTrue( llvm::Type::getInt1Ty( module->getContext() ) ) );
		}
	}
}
