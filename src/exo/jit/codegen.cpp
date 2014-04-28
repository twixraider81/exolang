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
			// do not delete module, llvm will take ownership
		}

		void Codegen::pushBlock( llvm::BasicBlock* block, std::string name )
		{
			blocks.push( new Block() );
			blocks.top()->block = block;
			blocks.top()->name = name;

			// reset our new insert point
			builder.SetInsertPoint( blocks.top()->block );

			BOOST_LOG_TRIVIAL(trace) << "Pushing \"" << getCurrentBlockName() << "\" onto stack, stacksize:" << blocks.size();
		}

		void Codegen::popBlock()
		{
			BOOST_LOG_TRIVIAL(trace) << "Poping \"" << getCurrentBlockName() << "\" from stack, stacksize:" << blocks.size();

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
			if( type->name == "int" ) {
				return( llvm::Type::getInt64Ty( context ) );
			} else if( type->name == "float" ) {
				return( llvm::Type::getDoubleTy( context ) );
			} else if( type->name == "bool" ) {
				return( llvm::Type::getInt1Ty( context ) );
			} else if( type->name == "string" ) {
				return( llvm::Type::getInt8PtrTy( context ) );
			}

			llvm::Type* ltype = module->getTypeByName( EXO_CLASS_STRUCT(type->name) );
			if( ltype != NULL ) {
				return( ltype );
			}

			return( llvm::Type::getVoidTy( context ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::Node* node )
		{
			std::string nName = typeid(*node).name();
			delete node;

			EXO_THROW_EXCEPTION( UnexpectedNode, "Unknown AST node, can't directly generate \"" + nName + "\"" );
			return( NULL );
		}

		llvm::Value* Codegen::Generate( exo::ast::Tree* tree )
		{
			// FIXME: should probably switch the return type
			llvm::Type* retType = llvm::Type::getInt64Ty( module->getContext() );
			llvm::Value* retVal;
			llvm::FunctionType *ftype = llvm::FunctionType::get( retType, false );
			entry = llvm::Function::Create( ftype, llvm::GlobalValue::InternalLinkage, name, module );
			llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), "entry", entry, 0 );

			pushBlock( block, name );

			Generate( tree->stmts );

			retVal = block->getTerminator();
			if( retVal == NULL ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating null return in (" << getCurrentBlockName() << ")";
				retVal = builder.CreateRet( llvm::Constant::getNullValue( retType ) );
			}

			popBlock();

			// no freeing here, we get a pointer. should probably change that.
			return( retVal );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtList* stmts )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating statements (" << getCurrentBlockName() << ")";

			std::vector<exo::ast::Stmt*>::iterator it;
			llvm::Value *last = NULL;

			for( it = stmts->list.begin(); it != stmts->list.end(); it++ ) {
				last = (**it).Generate( this );
			}

			// free
			delete stmts;
			return( last );
		}

		llvm::Value* Codegen::Generate( exo::ast::DecVar* decl )
		{
			BOOST_LOG_TRIVIAL(trace) << "Creating variable $" << decl->name << " " << decl->type->name << " in (" << getCurrentBlockName() << ")";
			std::string dName = decl->name;

			getCurrentBlockVars()[ decl->name ] = new llvm::AllocaInst( getType( decl->type, module->getContext() ), decl->name.c_str(), getCurrentBasicBlock() );

			BOOST_LOG_TRIVIAL(trace) << "Amount of local variables: " << getCurrentBlockVars().size();

			if( decl->expression ) {
				BOOST_LOG_TRIVIAL(trace) << "Creating compound assignment";
				exo::ast::AssignVar* a = new exo::ast::AssignVar( decl->name, decl->expression );
				a->Generate( this );
			}

			// free
			delete decl;
			return( getCurrentBlockVars()[ dName ] );
		}

		llvm::Value* Codegen::Generate( exo::ast::AssignVar* assign )
		{
			BOOST_LOG_TRIVIAL(trace) << "Assigning variable $" << assign->name << " in (" << getCurrentBlockName() << ")";
			std::string vName = assign->name;

			if( getCurrentBlockVars().find( vName ) == getCurrentBlockVars().end() ) {
				// free
				delete assign;
				EXO_THROW_EXCEPTION( UnknownVar, "Unknown variable $" + vName );
			}

			llvm::Value* vVal = assign->expression->Generate( this );

			// free
			delete assign;
			return( new llvm::StoreInst( vVal, getCurrentBlockVars()[ vName ], false, getCurrentBasicBlock() ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstNull* val )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating null in (" << getCurrentBlockName() << ")";

			// free
			delete val;
			return( llvm::Constant::getNullValue( llvm::Type::getInt1Ty( module->getContext() ) ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstBool* val )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating boolean \"" << val->value << "\" in (" << getCurrentBlockName() << ")";

			// free
			delete val;

			if( val->value ) {
				return( llvm::ConstantInt::getTrue( llvm::Type::getInt1Ty( module->getContext() ) ) );
			} else {
				return( llvm::ConstantInt::getFalse( llvm::Type::getInt1Ty( module->getContext() ) ) );
			}
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstInt* val )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating integer \"" << val->value << "\" in (" << getCurrentBlockName() << ")";
			long long i = val->value;

			// free
			delete val;
			return( llvm::ConstantInt::get( module->getContext(), llvm::APInt( 64, i ) ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstFloat* val )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating float \"" << val->value << "\" in (" << getCurrentBlockName() << ")";
			double d = val->value;

			// free
			delete val;
			return( llvm::ConstantFP::get( module->getContext(), llvm::APFloat( d ) ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstStr* val )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating string \"" << val->value << "\" in (" << getCurrentBlockName() << ")";

			llvm::Constant* stringConst = llvm::ConstantDataArray::getString( module->getContext(), val->value );
			llvm::Value* stringVar = builder.CreateAlloca( stringConst->getType() );
			builder.CreateStore( stringConst, stringVar );

			// free
			delete val;
			return( builder.CreateBitCast( stringVar, llvm::Type::getInt8PtrTy( module->getContext() ) ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::OpBinary* op )
		{
			llvm::Value* lhs = op->lhs->Generate( this );
			llvm::Value* rhs = op->rhs->Generate( this );
			llvm::Value* result;

			if( typeid(*op) == typeid( exo::ast::OpBinaryAdd ) ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating addition in (" << getCurrentBlockName() << ")";
				result = builder.CreateAdd( lhs, rhs, "add" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinarySub ) ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating substraction in (" << getCurrentBlockName() << ")";
				result = builder.CreateSub( lhs, rhs, "sub" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryMul ) ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating multiplication in (" << getCurrentBlockName() << ")";
				result = builder.CreateMul( lhs, rhs, "mul" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryDiv ) ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating division in (" << getCurrentBlockName() << ")";
				result = builder.CreateSDiv( lhs, rhs, "div" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryLt ) ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating lower than comparison in (" << getCurrentBlockName() << ")";
				result = builder.CreateICmpSLT( lhs, rhs, "cmp" );
			} else {
				delete op;
				EXO_THROW_EXCEPTION( UnknownBinaryOp, "Unknown binary operation." );
			}

			delete op;
			return( result );
		}

		llvm::Value* Codegen::Generate( exo::ast::ExprVar* expr )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating variable expression $" << expr->variable << " in (" << getCurrentBlockName() << ")";

			std::string vName = expr->variable;

			if( getCurrentBlockVars().find( vName ) == getCurrentBlockVars().end() ) {
				EXO_THROW_EXCEPTION( UnknownVar, "Unknown variable $" + vName );
			}

			// free
			delete expr;
			return( builder.CreateLoad( getCurrentBlockVars()[ vName ],vName ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::DecFun* decl )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating function \"" << decl->name << "\" in (" << getCurrentBlockName() << ")";

			std::vector<llvm::Type*> fArgs;
			std::vector<exo::ast::DecVar*>::iterator it;

			for( it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating argument $" << (**it).name;
				fArgs.push_back( getType( (**it).type, module->getContext() ) );
			}

			llvm::FunctionType* fType = llvm::FunctionType::get( getType( decl->returnType, module->getContext() ), fArgs, decl->hasVaArg );
			llvm::Function* function = llvm::Function::Create( fType, llvm::GlobalValue::InternalLinkage, decl->name, module );
			llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), decl->name, function, 0 );

			pushBlock( block, decl->name );

			for( it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				(**it).Generate( this );
			}

			Generate( decl->stmts );

			if( block->getTerminator() == NULL ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating null return in (" << getCurrentBlockName() << ")";
				builder.CreateRet( llvm::Constant::getNullValue( getType( decl->returnType, module->getContext() ) ) );
			}

			/* only in llvm 3.5
			std::string buffer;
			llvm::raw_string_ostream ostream( buffer );
			if( !llvm::verifyFunction( *function, ostream ) ) {
				EXO_THROW_EXCEPTION( LLVM, buffer );
				delete decl;
			}
			*/

			popBlock();

			// free
			delete decl;
			return( function );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtReturn* stmt )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating return statement in (" << getCurrentBlockName() << ")";
			llvm::Value* value = stmt->expression->Generate( this );

			// free
			delete stmt;
			return( builder.CreateRet( value ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::CallFun* call )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating function call to \"" << call->name << "\" in (" << getCurrentBlockName() << ")";

			llvm::Function* callee = module->getFunction( call->name );

			if( callee == 0 ) {
				delete call;
				EXO_THROW_EXCEPTION( UnknownFunction, "Unknown function: " + call->name );
			}

			if( callee->arg_size() != call->arguments->list.size() && !callee->isVarArg() ) {
				delete call;
				EXO_THROW_EXCEPTION( InvalidCall, "Expected arguments mismatch for function " + call->name );
			}

			std::vector<exo::ast::Expr*>::iterator it;
			std::vector<llvm::Value*> arguments;

			for( it = call->arguments->list.begin(); it != call->arguments->list.end(); it++ ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating argument $" << typeid(**it).name();
				arguments.push_back( (**it).Generate( this ) );
			}

			// free
			delete call;
			return( builder.CreateCall( callee, arguments, "call" ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtExpr* stmt )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating expression statement in (" << getCurrentBlockName() << ")";
			llvm::Value* value = stmt->expression->Generate( this );

			// free
			delete stmt;
			return( value );
		}

		llvm::Value* Codegen::Generate( exo::ast::DecFunProto* decl )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating prototype function \"" << decl->name << "\" in (" << getCurrentBlockName() << ")";

			std::vector<llvm::Type*> fArgs;
			std::vector<exo::ast::DecVar*>::iterator it;

			for( it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating argument $" << (**it).name;
				fArgs.push_back( getType( (**it).type, module->getContext() ) );
			}

			llvm::FunctionType* fType = llvm::FunctionType::get( getType( decl->returnType, module->getContext() ), fArgs, decl->hasVaArg );
			llvm::Function* function = llvm::Function::Create( fType, llvm::GlobalValue::ExternalLinkage, decl->name, module );

			// free
			delete decl;
			return( function );
		}

		llvm::StructType* Codegen::createClassStruct( exo::ast::DecClass* decl )
		{
			std::vector<llvm::Type*> properties;

			// parent properties first, so it can be casted?
			if( decl->parent != "" ) {
				llvm::StructType* parent = module->getTypeByName( EXO_CLASS_STRUCT(decl->parent) );

				if( !parent ) {
					EXO_THROW_EXCEPTION( UnknownClass, "Unknown class: " + decl->parent );
				}

				for( llvm::StructType::element_iterator it = parent->element_begin(); it != parent->element_end(); it++ ) {
					properties.push_back( *it );
				}
			}

			// now our own
			std::vector<exo::ast::DecProp*>::iterator pit;
			for( pit = decl->block->properties.begin(); pit != decl->block->properties.end(); pit++ ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating property $" << (**pit).property->name;
				properties.push_back( getType( (**pit).property->type, module->getContext() ) );
			}

			return( llvm::StructType::create( module->getContext(), properties, EXO_CLASS_STRUCT(decl->name) ) );
		}

		std::vector<llvm::Type*> Codegen::createClassMethods( exo::ast::DecClass* decl )
		{
			llvm::StructType* structClass = module->getTypeByName( EXO_CLASS_STRUCT(decl->name) );

			std::vector<llvm::Type*> methods;
			std::vector<exo::ast::DecMethod*>::iterator mit;
			for( mit = decl->block->methods.begin(); mit != decl->block->methods.end(); mit++ ) {

				// TODO: think about how to construct proper names
				std::string mName = "__" + decl->name + "_" + (**mit).method->name;

				BOOST_LOG_TRIVIAL(trace) << "Generating method \"" << (**mit).method->name << "\" (" << decl->name << " - " << mName << ")";

				std::vector<llvm::Type*> mArgs;
				std::vector<exo::ast::DecVar*>::iterator it;

				// pointer to class structure as 1.st param
				mArgs.push_back( llvm::PointerType::getUnqual( structClass ) );
				for( it = (**mit).method->arguments->list.begin(); it != (**mit).method->arguments->list.end(); it++ ) {
					BOOST_LOG_TRIVIAL(trace) << "Generating argument $" << (**it).name;
					mArgs.push_back( getType( (**it).type, module->getContext() ) );
				}

				llvm::FunctionType* mType = llvm::FunctionType::get( getType( (**mit).method->returnType, module->getContext() ), mArgs, (**mit).method->hasVaArg );
				llvm::Function* method = llvm::Function::Create( mType, llvm::GlobalValue::InternalLinkage, mName, module );
				llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), mName, method, 0 );

				pushBlock( block, (**mit).method->name );

				for( it = (**mit).method->arguments->list.begin(); it != (**mit).method->arguments->list.end(); it++ ) {
					(**it).Generate( this );
				}

				Generate( (**mit).method->stmts );

				if( block->getTerminator() == NULL ) {
					BOOST_LOG_TRIVIAL(trace) << "Generating null return in (" << getCurrentBlockName() << ")";
					builder.CreateRet( llvm::Constant::getNullValue( getType( (**mit).method->returnType, module->getContext() ) ) );
				}

				popBlock();
			}

			return( methods );
		}

		llvm::StructType* Codegen::createClassVirtualTable( exo::ast::DecClass* decl )
		{
			std::vector<llvm::Type*> properties;
			return( llvm::StructType::create( module->getContext(), properties, EXO_CLASS_VTABLE(decl->name) ) );
		}

		/*
		 * a class structure is CURRENTLY declared as follows:
		 * %__className_struct { all own + inherited properties }
		 * %__className_vtable { a virtual table containing own + inherited methods }
		 */
		llvm::Value* Codegen::Generate( exo::ast::DecClass* decl )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating class \"" << decl->name << "\"; " << decl->block->properties.size() << " properties; " << decl->block->methods.size() << " methods in (" << getCurrentBlockName() << ")";

			// generate properties
			createClassStruct( decl );

			// create own methods
			createClassMethods( decl );

			// setup vtable
			createClassVirtualTable( decl );

			/*
			 * FIXME: should probably return nothing
			 */
			delete decl;
			return( llvm::ConstantInt::getTrue( llvm::Type::getInt1Ty( module->getContext() ) ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtIf* stmt )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating if statement in (" << getCurrentBlockName() << ")";

			llvm::Value* condition = llvm::ConstantInt::getTrue( module->getContext() );

			llvm::Function* entry = builder.GetInsertBlock()->getParent();

			llvm::BasicBlock* blockTrue = llvm::BasicBlock::Create( module->getContext(), "true", entry );
			llvm::BasicBlock* blockFalse = llvm::BasicBlock::Create( module->getContext(), "false" );
			llvm::BasicBlock* blockMerge = llvm::BasicBlock::Create( module->getContext(), "merge" );

			pushBlock( blockTrue, "true" );
			llvm::Value* ifV = Generate( stmt->onTrue );
			builder.CreateBr( blockMerge );
			blockTrue = builder.GetInsertBlock();

			pushBlock( blockFalse, "false" );
			entry->getBasicBlockList().push_back( blockFalse );
			llvm::Value* elseV = Generate( stmt->onFalse );
			builder.CreateBr( blockMerge );

			blockFalse = builder.GetInsertBlock();

			entry->getBasicBlockList().push_back( blockMerge );
			builder.SetInsertPoint( blockMerge );
			llvm::PHINode* phi = builder.CreatePHI( llvm::Type::getInt64Ty( module->getContext() ), 2, "iftmp" );
			phi->addIncoming( ifV, blockTrue );
			phi->addIncoming( elseV, blockFalse );

			popBlock();
			popBlock();

			//delete stmt;
			return( phi );
		}
	}
}
