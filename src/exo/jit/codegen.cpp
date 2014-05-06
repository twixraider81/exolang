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
		/*
		 * RULE OF THUMB for LLVM. If there exists a get/create function, use it and there is no need to new/delete.
		 * also some types share ownership and need not to be deleted, like module <-> execution engine
		 */
		Codegen::Codegen( std::string cname, std::string target ) : builder( llvm::getGlobalContext() )
		{
			name = cname;
			module = new llvm::Module( cname, llvm::getGlobalContext() );
			module->setTargetTriple( target );
			entry = NULL;

			llvm::DataLayout dataLayout( module );
			intType = dataLayout.getIntPtrType( module->getContext() );
			ptrType = llvm::PointerType::getUnqual( intType );
			voidType = llvm::Type::getVoidTy( module->getContext() );
		}

		Codegen::~Codegen()
		{
		}

		void Codegen::popBlock()
		{
			BOOST_LOG_TRIVIAL(trace) << "Poping \"" << this->getCurrentBlockName() << "\" from stack, stacksize:" << this->blocks.size();

			this->blocks.pop();

			// reset our insert point
			if( this->blocks.size() > 0 ) {
				this->builder.SetInsertPoint( this->blocks.top()->block );
			}
		}

		void Codegen::pushBlock( llvm::BasicBlock* block, std::string name )
		{
			this->blocks.push( new Block() );
			this->blocks.top()->block = block;
			this->blocks.top()->name = name;

			// reset our new insert point
			builder.SetInsertPoint( this->blocks.top()->block );

			BOOST_LOG_TRIVIAL(trace) << "Pushing \"" << this->getCurrentBlockName() << "\" onto stack, stacksize:" << this->blocks.size();
		}

		llvm::BasicBlock* Codegen::getCurrentBasicBlock()
		{
			 return( this->blocks.top()->block );
		}

		std::string Codegen::getCurrentBlockName()
		{
			 return( this->blocks.top()->name );
		}

		std::map<std::string,llvm::Value*>& Codegen::getCurrentBlockVars()
		{
			return( this->blocks.top()->variables );
		}

		llvm::Value* Codegen::getCurrentBlockVar( std::string vName )
		{
			if( this->getCurrentBlockVars().find( vName ) == this->getCurrentBlockVars().end() ) {
				EXO_THROW_EXCEPTION( UnknownVar, "Unknown variable $" + vName );
			}

			return ( this->getCurrentBlockVars()[ vName ] );
		}

		/*
		 * FIXME: wont be needed anymore once we get proper type system
		 */
		llvm::Type* Codegen::getType( exo::ast::Type* type )
		{
			if( type->name == "int" ) {
				return( llvm::Type::getInt64Ty( this->module->getContext() ) );
			} else if( type->name == "float" ) {
				return( llvm::Type::getDoubleTy( this->module->getContext() ) );
			} else if( type->name == "bool" ) {
				return( llvm::Type::getInt1Ty( this->module->getContext() ) );
			} else if( type->name == "string" ) {
				return( llvm::Type::getInt8PtrTy( this->module->getContext() ) );
			}

			llvm::Type* ltype = module->getTypeByName( EXO_CLASS( type->name ) );
			if( ltype != NULL ) {
				return( ltype );
			}

			EXO_THROW_EXCEPTION( UnknownClass, "Unknown class \"" + type->name  + "\"" );
			return( NULL ); // satisfy IDE
		}


		llvm::Value* Codegen::Generate( exo::ast::CallFun* call )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating function call to \"" << call->name << "\" in (" << this->getCurrentBlockName() << ")";
			EXO_GET_CALLEE( callee, call->name );

			if( callee->arg_size() != call->arguments->list.size() && !callee->isVarArg() ) {
				EXO_THROW_EXCEPTION( InvalidCall, "Expected arguments mismatch for \"" + call->name + "\"" );
			}

			std::vector<llvm::Value*> arguments;
			for( std::vector<exo::ast::Expr*>::iterator it = call->arguments->list.begin(); it != call->arguments->list.end(); it++ ) {
				arguments.push_back( (**it).Generate( this ) );
			}

			return( builder.CreateCall( callee, arguments, call->name ) );
		}

		/*
		 * TODO: use vtbl (after we got it working)
		 */
		llvm::Value* Codegen::Generate( exo::ast::CallMethod* call )
		{
			exo::ast::ExprVar* var = dynamic_cast<exo::ast::ExprVar*>( call->expression );

			if( !var ) {
				EXO_THROW_EXCEPTION( InvalidCall, "Can only do a method call on a variable!" );
			}

			llvm::Value* variable = this->getCurrentBlockVar( var->name );
			llvm::Type* vType = variable->getType();

			if( vType->isPointerTy() && !vType->getPointerElementType()->isStructTy() ) {
				EXO_THROW_EXCEPTION( InvalidCall, "Can only invoke method on an object." );
			}

			std::string cName = vType->getPointerElementType()->getStructName();
			std::string mName = EXO_METHOD( cName, call->name );

			BOOST_LOG_TRIVIAL(trace) << "Generating call to $" << var->name << "->" << call->name << "/" << mName << " in (" << this->getCurrentBlockName() << ")";
			EXO_GET_CALLEE( callee, mName );

			if( callee->arg_size() != ( call->arguments->list.size() + 1 ) && !callee->isVarArg() ) {
				EXO_THROW_EXCEPTION( InvalidCall, "Expected arguments mismatch for \"" + call->name + "\"" );
			}

			std::vector<llvm::Value*> arguments;
			// this ptr
			arguments.push_back( variable );
			for( std::vector<exo::ast::Expr*>::iterator it = call->arguments->list.begin(); it != call->arguments->list.end(); it++ ) {
				arguments.push_back( (**it).Generate( this ) );
			}

			return( builder.CreateCall( callee, arguments, mName ) );
		}


		llvm::Value* Codegen::Generate( exo::ast::ConstBool* val )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating boolean \"" << val->value << "\" in (" << getCurrentBlockName() << ")";

			if( val->value ) {
				return( llvm::ConstantInt::getTrue( llvm::Type::getInt1Ty( module->getContext() ) ) );
			} else {
				return( llvm::ConstantInt::getFalse( llvm::Type::getInt1Ty( module->getContext() ) ) );
			}
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstFloat* val )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating float \"" << val->value << "\" in (" << getCurrentBlockName() << ")";
			double d = val->value;
			return( llvm::ConstantFP::get( module->getContext(), llvm::APFloat( d ) ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstInt* val )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating integer \"" << val->value << "\" in (" << getCurrentBlockName() << ")";
			long long i = val->value;
			return( llvm::ConstantInt::get( module->getContext(), llvm::APInt( 64, i ) ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstNull* val )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating null in (" << getCurrentBlockName() << ")";
			return( llvm::Constant::getNullValue( llvm::Type::getInt1Ty( module->getContext() ) ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstStr* val )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating string \"" << val->value << "\" in (" << getCurrentBlockName() << ")";
			llvm::Constant* stringConst = llvm::ConstantDataArray::getString( module->getContext(), val->value );
			llvm::Value* stringVar = builder.CreateAlloca( stringConst->getType() );
			builder.CreateStore( stringConst, stringVar );
			return( builder.CreateBitCast( stringVar, llvm::Type::getInt8PtrTy( module->getContext() ) ) );
		}


		/*
		 * a class structure is CURRENTLY declared as follows:
		 * %className { all own + inherited properties }
		 * %className_vtable { a virtual table containing own + inherited methods }
		 */
		llvm::Value* Codegen::Generate( exo::ast::DecClass* decl )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating class \"" << decl->name << "\"; " << decl->block->properties.size() << " properties; " << decl->block->methods.size() << " methods in (" << getCurrentBlockName() << ")";

			// generate properties
			std::vector<llvm::Type*> properties;

			// parent properties first, so it can be casted?
			if( decl->parent != "" ) {
				std::string pName = EXO_CLASS( decl->parent );
				llvm::StructType* parent = module->getTypeByName( pName );

				if( !parent ) {
					EXO_THROW_EXCEPTION( UnknownClass, "Unknown class \"" + pName + "\"" );
				}

				for( llvm::StructType::element_iterator it = parent->element_begin(); it != parent->element_end(); it++ ) {
					properties.push_back( *it );
				}
			}

			// now our own
			// FIXME: should overwrite properties, not just append
			std::vector<exo::ast::DecProp*>::iterator pit;
			for( pit = decl->block->properties.begin(); pit != decl->block->properties.end(); pit++ ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating property $" << (**pit).property->name;
				properties.push_back( this->getType( (**pit).property->type ) );
			}

			llvm::StructType* classStruct = llvm::StructType::create( module->getContext(), properties, EXO_CLASS( decl->name ) );

			// generate methods & vtbl
			std::vector<exo::ast::DecMethod*>::iterator mit;
			std::vector<llvm::Type*> vtblMethods;
			for( mit = decl->block->methods.begin(); mit != decl->block->methods.end(); mit++ ) {
				// rename as a class method
				std::string mName = EXO_METHOD( decl->name, (**mit).method->name );
				(**mit).method->name = mName;

				// pointer to class structure as 1.st param
				(**mit).method->arguments->list.insert( (**mit).method->arguments->list.begin(), new exo::ast::DecVar( "this", new exo::ast::Type( decl->name ) ) );

				methods[ EXO_CLASS( decl->name ) ].push_back( (**mit).method->name );

				llvm::Value* methodSignature = Generate( (**mit).method );
				vtblMethods.push_back( llvm::PointerType::getUnqual( module->getFunction( mName )->getFunctionType() ) );
			}

			// generate vtbl
			BOOST_LOG_TRIVIAL(trace) << "Generating vtbl \"" << EXO_VTABLE( decl->name ) << "\"";
			llvm::StructType* classVtbl = llvm::StructType::create( module->getContext(), vtblMethods, EXO_VTABLE( decl->name ) );
			//llvm::GlobalVariable* vtbl = new llvm::GlobalVariable( *module, classVtbl, true, llvm::GlobalValue::CommonLinkage, llvm::Constant::getNullValue( classVtbl ), EXO_VTABLE( decl->name ) + "_tbl" );

			/*
			 * FIXME: should probably return nothing
			 */
			return( llvm::ConstantInt::getTrue( llvm::Type::getInt1Ty( module->getContext() ) ) );
		}

		/*
		 * TODO: merge with DecFunProto
		 */
		llvm::Value* Codegen::Generate( exo::ast::DecFun* decl )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating function \"" << decl->name << "\" in (" << this->getCurrentBlockName() << ")";

			std::vector<llvm::Type*> fArgs;
			for( std::vector<exo::ast::DecVar*>::iterator it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				llvm::Type* arg = this->getType( (**it).type );

				// only structs by ptr for now
				if( arg->isStructTy() ) {
					BOOST_LOG_TRIVIAL(trace) << "Generating argument by pointer $" << (**it).name;
					fArgs.push_back( llvm::PointerType::getUnqual( arg ) );
				} else {
					BOOST_LOG_TRIVIAL(trace) << "Generating argument by value $" << (**it).name;
					fArgs.push_back( arg );
				}
			}

			llvm::Function* fun = llvm::Function::Create(
					llvm::FunctionType::get( this->getType( decl->returnType ),	fArgs, decl->hasVaArg ),
					llvm::GlobalValue::InternalLinkage,
					decl->name,
					this->module
			);
			llvm::BasicBlock* block = llvm::BasicBlock::Create( this->module->getContext(), decl->name, fun, 0 );

			this->pushBlock( block, decl->name );

			int i = 0;
			for( llvm::Function::arg_iterator it = fun->arg_begin(); it != fun->arg_end(); it++ ) {
				it->setName( decl->arguments->list.at( i )->name );
				getCurrentBlockVars()[ decl->arguments->list.at( i )->name ] = it;
				i++;
			}

			this->Generate( decl->stmts );

			if( block->getTerminator() == NULL ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating null return in (" << getCurrentBlockName() << ")";
				builder.CreateRet( llvm::Constant::getNullValue( this->getType( decl->returnType ) ) );
			}

			this->popBlock();

			return( fun );
		}

		llvm::Value* Codegen::Generate( exo::ast::DecFunProto* decl )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating prototype function \"" << decl->name << "\" in (" << this->getCurrentBlockName() << ")";

			std::vector<llvm::Type*> fArgs;
			for( std::vector<exo::ast::DecVar*>::iterator it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				llvm::Type* arg = this->getType( (**it).type );

				// only structs by ptr
				if( arg->isStructTy() ) {
					BOOST_LOG_TRIVIAL(trace) << "Generating argument by pointer $" << (**it).name;
					fArgs.push_back( llvm::PointerType::getUnqual( arg ) );
				} else {
					BOOST_LOG_TRIVIAL(trace) << "Generating argument by value $" << (**it).name;
					fArgs.push_back( arg );
				}
			}

			llvm::Function* fun = llvm::Function::Create(
				llvm::FunctionType::get( this->getType( decl->returnType ), fArgs, decl->hasVaArg ),
				llvm::GlobalValue::ExternalLinkage,
				decl->name,
				this->module
			);

			int i = 0;
			for( llvm::Function::arg_iterator it = fun->arg_begin(); it != fun->arg_end(); it++ ) {
				it->setName( decl->arguments->list.at( i )->name );
				i++;
			}

			return( fun );
		}

		llvm::Value* Codegen::Generate( exo::ast::DecVar* decl )
		{
			llvm::Type* vType = this->getType( decl->type );

			// only classes are stored on heap and are also passed around by pointer
			if( vType->isStructTy() ) {
				BOOST_LOG_TRIVIAL(trace) << "Creating " << decl->type->name << " $" << decl->name << " on heap in (" << this->getCurrentBlockName() << ")";
				EXO_GET_CALLEE( gcmalloc, EXO_ALLOC );

				std::vector<llvm::Value*> arguments;
				arguments.push_back( llvm::ConstantExpr::getSizeOf( vType ) );

				this->getCurrentBlockVars()[ decl->name ] = builder.CreateBitCast( builder.CreateCall( gcmalloc, arguments ), llvm::PointerType::getUnqual( vType ) );
			} else {
				BOOST_LOG_TRIVIAL(trace) << "Creating " << decl->type->name << " $" << decl->name << " on stack in (" << this->getCurrentBlockName() << ")";
				this->getCurrentBlockVars()[ decl->name ] = builder.CreateAlloca( vType );
			}

			if( decl->expression ) {
				boost::scoped_ptr<exo::ast::OpBinaryAssign> a( new exo::ast::OpBinaryAssign( new exo::ast::ExprVar( decl->name ), decl->expression ) );
				a->Generate( this );
			}

			return( this->getCurrentBlockVar( decl->name ) );
		}


		llvm::Value* Codegen::Generate( exo::ast::ExprVar* expr )
		{
			llvm::Value* variable = this->getCurrentBlockVar( expr->name );
			llvm::Type* vType = variable->getType();

			/*
			 * we only load pointers to non class types into a new expression.
			 * class pointers stay pointers.
			 * non-pointers stay non-pointers.
			 *
			 * see opbinaryassign and opunarydelete for the analogous operations
			 */
			if( vType->isPointerTy() && !vType->getPointerElementType()->isStructTy() ) {
				BOOST_LOG_TRIVIAL(trace) << "Load variable value $" << expr->name << " (" << this->getCurrentBlockName() << ")";
				return( builder.CreateLoad( variable ) );
			} else {
				BOOST_LOG_TRIVIAL(trace) << "Load variable pointer $" << expr->name << " in (" << this->getCurrentBlockName() << ")";
				return( variable );
			}
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
				EXO_THROW_EXCEPTION( InvalidOp, "Unknown binary operation." );
			}

			return( result );
		}

		llvm::Value* Codegen::Generate( exo::ast::OpBinaryAssign* assign )
		{
			exo::ast::ExprVar* var = dynamic_cast<exo::ast::ExprVar*>( assign->lhs );

			if( !var ) {
				EXO_THROW_EXCEPTION( InvalidOp, "Can only assign to a variable!" );
			}

			llvm::Value* variable = this->getCurrentBlockVar( var->name );
			llvm::Value* value = assign->rhs->Generate( this );
			llvm::Type* vType = variable->getType();

			if( vType->isPointerTy() && !vType->getPointerElementType()->isStructTy() ) {
				BOOST_LOG_TRIVIAL(trace) << "Store value of $" << var->name << " on stack in (" << this->getCurrentBlockName() << ")";
				return( builder.CreateStore( value, variable ) );
			} else {
				BOOST_LOG_TRIVIAL(trace) << "Set pointer to $" << var->name << " in heap in (" << this->getCurrentBlockName() << ")";
				// FIXME: we do nothing with pointers for now. rework under premise of opunarynew
				return( variable );
			}
		}


		/*
		 * TODO: call destructor
		 */
		llvm::Value* Codegen::Generate( exo::ast::OpUnaryDel* op )
		{
			exo::ast::ExprVar* var = dynamic_cast<exo::ast::ExprVar*>( op->rhs );

			if( !var ) {
				EXO_THROW_EXCEPTION( InvalidOp, "Can only delete a variable!" );
			}

			llvm::Value* variable = this->getCurrentBlockVar( var->name );
			llvm::Type* vType = variable->getType();

			// only classes are stored on heap and need to be freed
			if( vType->isPointerTy() && vType->getPointerElementType()->isStructTy() ) {
				BOOST_LOG_TRIVIAL(trace) << "Deleting $" << var->name << " from heap in (" << this->getCurrentBlockName() << ")";
				EXO_GET_CALLEE( gcfree, EXO_DEALLOC );

				std::vector<llvm::Value*> arguments;
				arguments.push_back( builder.CreateBitCast( variable, this->ptrType ) );
				variable = builder.CreateCall( gcfree, arguments );
			} else {
				variable = llvm::Constant::getNullValue( llvm::Type::getInt1Ty( module->getContext() ) );
			}

			this->getCurrentBlockVars().erase( var->name );
			return( variable );
		}

		/*
		 * TODO: call constructor
		 */
		llvm::Value* Codegen::Generate( exo::ast::OpUnaryNew* op )
		{
			exo::ast::CallFun* init = dynamic_cast<exo::ast::CallFun*>( op->rhs );

			if( !init ) {
				EXO_THROW_EXCEPTION( UnknownVar, "Invalid expression!" );
			}

			std::string vName = init->name;
			BOOST_LOG_TRIVIAL(trace) << "Creating instance of " << vName;

			return( NULL );
		}


		llvm::Value* Codegen::Generate( exo::ast::StmtExpr* stmt )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating expression statement in (" << getCurrentBlockName() << ")";
			llvm::Value* value = stmt->expression->Generate( this );
			return( value );
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

			return( phi );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtReturn* stmt )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating return statement in (" << getCurrentBlockName() << ")";
			return( builder.CreateRet( stmt->expression->Generate( this ) ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtList* stmts )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating statements (" << getCurrentBlockName() << ")";

			std::vector<exo::ast::Stmt*>::iterator it;
			llvm::Value *last = NULL;

			for( it = stmts->list.begin(); it != stmts->list.end(); it++ ) {
				last = (**it).Generate( this );
			}

			return( last );
		}


		llvm::Value* Codegen::Generate( boost::shared_ptr<exo::ast::Tree> tree )
		{
			std::vector<llvm::Type*> fArgs;

			/*
			 * register essential externals
			 */
			fArgs.push_back( this->intType );
			llvm::Function::Create( llvm::FunctionType::get( this->ptrType, fArgs, false ), llvm::GlobalValue::ExternalLinkage, EXO_ALLOC, this->module );

			fArgs.clear();
			fArgs.push_back( this->ptrType );
			llvm::Function::Create( llvm::FunctionType::get( this->voidType, fArgs, false ), llvm::GlobalValue::ExternalLinkage, EXO_DEALLOC, this->module );

			/*
			 * this is main() well the entry
			 */
			this->entry = llvm::Function::Create( llvm::FunctionType::get( this->intType, false ), llvm::GlobalValue::InternalLinkage, this->name, this->module );
			llvm::BasicBlock* block = llvm::BasicBlock::Create( this->module->getContext(), "entry", this->entry, 0 );

			pushBlock( block, this->name );

			Generate( tree->stmts );

			llvm::Value* retVal = block->getTerminator();
			if( retVal == NULL ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating null return in (" << this->name << ")";
				retVal = this->builder.CreateRet( llvm::Constant::getNullValue( this->intType ) );
			}

			popBlock();

			return( retVal );
		}
	}
}
