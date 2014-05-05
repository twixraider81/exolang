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
			entry = NULL;
		}

		Codegen::~Codegen()
		{
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

		void Codegen::pushBlock( llvm::BasicBlock* block, std::string name )
		{
			blocks.push( new Block() );
			blocks.top()->block = block;
			blocks.top()->name = name;

			// reset our new insert point
			builder.SetInsertPoint( blocks.top()->block );

			BOOST_LOG_TRIVIAL(trace) << "Pushing \"" << getCurrentBlockName() << "\" onto stack, stacksize:" << blocks.size();
		}

		llvm::BasicBlock* Codegen::getCurrentBasicBlock()
		{
			 return( blocks.top()->block );
		}

		std::string Codegen::getCurrentBlockName()
		{
			 return( blocks.top()->name );
		}

		std::map<std::string,llvm::Value*>& Codegen::getCurrentBlockVars()
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

			llvm::Type* ltype = module->getTypeByName( EXO_CLASS( type->name ) );
			if( ltype != NULL ) {
				return( ltype );
			}

			EXO_THROW_EXCEPTION( UnknownClass, "Unknown class: " + type->name );
			return( NULL ); // satisfy IDE
		}




		llvm::Value* Codegen::Generate( exo::ast::CallFun* call )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating function call to \"" << call->name << "\" in (" << getCurrentBlockName() << ")";
			EXO_GET_CALLEE( callee, call->name );

			if( callee->arg_size() != call->arguments->list.size() && !callee->isVarArg() ) {
				EXO_THROW_EXCEPTION( InvalidCall, "Expected arguments mismatch for function " + call->name );
			}

			std::vector<exo::ast::Expr*>::iterator it;
			std::vector<llvm::Value*> arguments;

			for( it = call->arguments->list.begin(); it != call->arguments->list.end(); it++ ) {
				arguments.push_back( (**it).Generate( this ) );
			}

			return( builder.CreateCall( callee, arguments, "call" ) );
		}

		/*
		 * TODO: use vtbl (after we got it working)
		 */
		llvm::Value* Codegen::Generate( exo::ast::CallMethod* call )
		{
			exo::ast::ExprVar* var = dynamic_cast<exo::ast::ExprVar*>( call->expression );

			/*
			 * TODO: make a function of this...
			 */
			if( !var ) {
				EXO_THROW_EXCEPTION( InvalidCall, "Can only do a method call on a variable!" );
			}

			std::string vName = var->variable;
			std::map<std::string,llvm::Value*>::const_iterator vit = getCurrentBlockVars().find( vName );
			if( vit == getCurrentBlockVars().end() ) {
				EXO_THROW_EXCEPTION( UnknownVar, "Unknown variable $" + vName );
			}

			llvm::Value* lVar = getCurrentBlockVars()[ vName ];
			if( !lVar->getType()->getPointerElementType()->isStructTy() ) {
				EXO_THROW_EXCEPTION( InvalidCall, "Can only invoke method on an object." );
			}

			std::string cName = lVar->getType()->getPointerElementType()->getStructName();
			std::string mName = EXO_METHOD( cName, call->name );
			BOOST_LOG_TRIVIAL(trace) << "Generating call to $" << vName << "->" << call->name << "/" << mName << " in (" << getCurrentBlockName() << ")";

			EXO_GET_CALLEE( callee, mName );

			// make space for $this ptr
			if( callee->arg_size() != ( call->arguments->list.size() + 1 ) && !callee->isVarArg() ) {
				EXO_THROW_EXCEPTION( InvalidCall, "Expected arguments mismatch for function " + mName );
			}

			std::vector<exo::ast::Expr*>::iterator it;
			std::vector<llvm::Value*> arguments;

			arguments.push_back( getCurrentBlockVars()[ vName ] );
			for( it = call->arguments->list.begin(); it != call->arguments->list.end(); it++ ) {
				arguments.push_back( (**it).Generate( this ) );
			}

			return( builder.CreateCall( callee, arguments, "call" ) );
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
					EXO_THROW_EXCEPTION( UnknownClass, "Unknown class: " + pName );
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
				properties.push_back( getType( (**pit).property->type, module->getContext() ) );
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
			BOOST_LOG_TRIVIAL(trace) << "Generating function \"" << decl->name << "\" in (" << getCurrentBlockName() << ")";

			std::vector<llvm::Type*> fArgs;
			std::vector<exo::ast::DecVar*>::iterator it;

			for( it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				llvm::Type* arg = getType( (**it).type, module->getContext() );

				// only structs by ptr for now
				if( arg->isStructTy() ) {
					BOOST_LOG_TRIVIAL(trace) << "Generating argument by pointer $" << (**it).name;
					fArgs.push_back( llvm::PointerType::getUnqual( arg ) );
				} else {
					BOOST_LOG_TRIVIAL(trace) << "Generating argument by value $" << (**it).name;
					fArgs.push_back( arg );
				}
			}

			llvm::FunctionType* fType = llvm::FunctionType::get( getType( decl->returnType, module->getContext() ), fArgs, decl->hasVaArg );
			llvm::Function* function = llvm::Function::Create( fType, llvm::GlobalValue::InternalLinkage, decl->name, module );
			llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), decl->name, function, 0 );

			pushBlock( block, decl->name );

			for( it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				llvm::Type* arg = getType( (**it).type, module->getContext() );

				// for classes/structs we have ptr and do not generate defaults
				if( !arg->isStructTy() ) {
					(**it).Generate( this );
				}
			}

			Generate( decl->stmts );

			if( block->getTerminator() == NULL ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating null return in (" << getCurrentBlockName() << ")";
				builder.CreateRet( llvm::Constant::getNullValue( getType( decl->returnType, module->getContext() ) ) );
			}

			popBlock();

			return( function );
		}

		llvm::Value* Codegen::Generate( exo::ast::DecFunProto* decl )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating prototype function \"" << decl->name << "\" in (" << getCurrentBlockName() << ")";

			std::vector<llvm::Type*> fArgs;
			std::vector<exo::ast::DecVar*>::iterator it;

			for( it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				llvm::Type* arg = getType( (**it).type, module->getContext() );

				// only structs by ptr for now
				if( arg->isStructTy() ) {
					BOOST_LOG_TRIVIAL(trace) << "Generating argument by pointer $" << (**it).name;
					fArgs.push_back( llvm::PointerType::getUnqual( arg ) );
				} else {
					BOOST_LOG_TRIVIAL(trace) << "Generating argument by value $" << (**it).name;
					fArgs.push_back( arg );
				}
			}

			llvm::FunctionType* fType = llvm::FunctionType::get( getType( decl->returnType, module->getContext() ), fArgs, decl->hasVaArg );
			return( llvm::Function::Create( fType, llvm::GlobalValue::ExternalLinkage, decl->name, module ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::DecVar* decl )
		{
			std::string dName = decl->name;
			llvm::Type* lType = getType( decl->type, module->getContext() );

			if( lType->isStructTy() ) {
				BOOST_LOG_TRIVIAL(trace) << "Creating " << decl->type->name << " $" << decl->name << " on heap in (" << getCurrentBlockName() << ")";
				EXO_GET_CALLEE( gcmalloc, "GC_malloc" );

				std::vector<llvm::Value*> arguments;
				arguments.push_back( llvm::ConstantExpr::getSizeOf( lType ) );

				getCurrentBlockVars()[ dName ] = builder.CreateBitCast( builder.CreateCall( gcmalloc, arguments ), llvm::PointerType::getUnqual( lType ) );
			} else {
				BOOST_LOG_TRIVIAL(trace) << "Creating " << decl->type->name << " $" << decl->name << " on stack in (" << getCurrentBlockName() << ")";
				getCurrentBlockVars()[ dName ] = builder.CreateAlloca( lType );
			}

			if( decl->expression ) {
				exo::ast::OpBinaryAssign* a = new exo::ast::OpBinaryAssign( new exo::ast::ExprVar( dName ), decl->expression );
				a->Generate( this );
			}

			return( getCurrentBlockVars()[ dName ] );
		}


		llvm::Value* Codegen::Generate( exo::ast::ExprVar* expr )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating variable expression $" << expr->variable << " in (" << getCurrentBlockName() << ")";

			std::string vName = expr->variable;

			if( getCurrentBlockVars().find( vName ) == getCurrentBlockVars().end() ) {
				EXO_THROW_EXCEPTION( UnknownVar, "Unknown variable $" + vName );
			}

			// see if we can somehow smooth this check, see also binopassign and unaryopdel
			if( !getCurrentBlockVars()[ vName ]->getType()->getPointerElementType()->isStructTy() ) {
				BOOST_LOG_TRIVIAL(trace) << "Load variable $" << vName << " from stack in (" << getCurrentBlockName() << ")";
				return( builder.CreateLoad( getCurrentBlockVars()[ vName ] ) );
			} else {
				BOOST_LOG_TRIVIAL(trace) << "Load variable $" << vName << " from heap in (" << getCurrentBlockName() << ")";
				return( getCurrentBlockVars()[ vName ] );
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
				EXO_THROW_EXCEPTION( UnknownBinaryOp, "Unknown binary operation." );
			}

			return( result );
		}

		llvm::Value* Codegen::Generate( exo::ast::OpBinaryAssign* assign )
		{
			exo::ast::ExprVar* var = dynamic_cast<exo::ast::ExprVar*>( assign->lhs );

			if( !var ) {
				EXO_THROW_EXCEPTION( UnknownVar, "Can only assign to a variable!" );
			}

			std::string vName = var->variable;
			llvm::Value* rhs = assign->rhs->Generate( this );

			if( !getCurrentBlockVars()[ vName ]->getType()->getPointerElementType()->isStructTy() ) {
				BOOST_LOG_TRIVIAL(trace) << "Store variable $" << vName << " on stack in (" << getCurrentBlockName() << ")";
				return( builder.CreateStore( rhs, getCurrentBlockVars()[ vName ] ) );
			} else {
				BOOST_LOG_TRIVIAL(trace) << "Store variable $" << vName << " on heap in (" << getCurrentBlockName() << ")";
				return( getCurrentBlockVars()[ vName ] );
			}
		}


		/*
		 * TODO: call destructor
		 */
		llvm::Value* Codegen::Generate( exo::ast::OpUnaryDel* op )
		{
			exo::ast::ExprVar* var = dynamic_cast<exo::ast::ExprVar*>( op->rhs );

			if( !var ) {
				EXO_THROW_EXCEPTION( UnknownVar, "Can only delete a variable!" );
			}

			std::string vName = var->variable;

			std::map<std::string,llvm::Value*>::const_iterator it = getCurrentBlockVars().find( vName );
			if( it == getCurrentBlockVars().end() ) {
				EXO_THROW_EXCEPTION( UnknownVar, "Unknown variable $" + vName );
			}

			if( getCurrentBlockVars()[ vName ]->getType()->getPointerElementType()->isStructTy() ) {
				BOOST_LOG_TRIVIAL(trace) << "Deleting variable $" << vName << " from heap in (" << getCurrentBlockName() << ")";
				EXO_GET_CALLEE( gcfree, "GC_free" );

				std::vector<llvm::Value*> arguments;
				arguments.push_back( builder.CreateBitCast( getCurrentBlockVars()[ vName ], llvm::Type::getInt64PtrTy( module->getContext() ) ) );

				builder.CreateCall( gcfree, arguments );
			} else {
				// NULL var?
				BOOST_LOG_TRIVIAL(trace) << "Deleting variable $" << vName << " from stack in (" << getCurrentBlockName() << ")";
			}

			getCurrentBlockVars().erase( vName );

			/*
			 * FIXME: should probably return nothing
			 */
			return( llvm::ConstantInt::getTrue( llvm::Type::getInt1Ty( module->getContext() ) ) );
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
			// FIXME: should probably switch the return type
			llvm::Type* retType = llvm::Type::getInt64Ty( module->getContext() );
			llvm::Value* retVal;
			llvm::FunctionType *ftype = llvm::FunctionType::get( retType, false );
			entry = llvm::Function::Create( ftype, llvm::GlobalValue::InternalLinkage, name, module );
			llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), "entry", entry, 0 );

			pushBlock( block, name );

			/*
			 * register externals (GC_alloc, GC_free)
			 * TODO: only x64 :s
			 */
			llvm::Type* ptrType = llvm::Type::getInt64PtrTy( module->getContext() );
			llvm::Type* sizeType = llvm::Type::getInt64Ty( module->getContext() );
			llvm::Type* voidType = llvm::Type::getVoidTy( module->getContext() );
			std::vector<llvm::Type*> fArgs;

			fArgs.push_back( sizeType );
			llvm::Function::Create( llvm::FunctionType::get( ptrType, fArgs, false ), llvm::GlobalValue::ExternalLinkage, "GC_malloc", module );

			fArgs.clear();
			fArgs.push_back( ptrType );
			llvm::Function::Create( llvm::FunctionType::get( voidType, fArgs, false ), llvm::GlobalValue::ExternalLinkage, "GC_free", module );


			Generate( tree->stmts );

			retVal = block->getTerminator();
			if( retVal == NULL ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating null return in (" << getCurrentBlockName() << ")";
				retVal = builder.CreateRet( llvm::Constant::getNullValue( retType ) );
			}

			popBlock();

			return( retVal );
		}
	}
}
