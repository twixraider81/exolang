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
			ptrType = intType->getPointerTo();
			voidType = llvm::Type::getVoidTy( module->getContext() );
		}

		Codegen::~Codegen()
		{
		}

		void Codegen::popBlock()
		{
			BOOST_LOG_TRIVIAL(trace) << "Pop \"" << this->getBlockName() << "\" from stack, new size: " << this->blocks.size();

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

			BOOST_LOG_TRIVIAL(trace) << "Push \"" << this->getBlockName() << "\" onto stack, new size: " << this->blocks.size();
		}

		llvm::BasicBlock* Codegen::getBlock()
		{
			 return( this->blocks.top()->block );
		}

		std::string Codegen::getBlockName()
		{
			 return( this->blocks.top()->name );
		}

		llvm::Value* Codegen::getBlockSymbol( std::string name )
		{
			std::map<std::string,llvm::Value*>::iterator it = this->blocks.top()->symbols.find( name );
			if( it == this->blocks.top()->symbols.end() ) {
				EXO_THROW_EXCEPTION( UnknownVar, "Unknown variable $" + name );
			}

			return( it->second );
		}

		void Codegen::setBlockSymbol( std::string name, llvm::Value* value )
		{
			std::map<std::string,llvm::Value*>::iterator it = this->blocks.top()->symbols.find( name );
			if( it == this->blocks.top()->symbols.end() ) {
				value->setName( name );
				this->blocks.top()->symbols.insert( std::pair<std::string,llvm::Value*>( name, value ) );
			} else {
				this->blocks.top()->symbols[name] = value;
			}
		}

		void Codegen::delBlockSymbol( std::string name )
		{
			std::map<std::string,llvm::Value*>::iterator it = this->blocks.top()->symbols.find( name );
			if( it == this->blocks.top()->symbols.end() ) {
				EXO_THROW_EXCEPTION( UnknownVar, "Unknown variable $" + name );
			}

			this->blocks.top()->symbols.erase( name );
		}

		/*
		 * TODO: allow raw ctypes with proper names, we want int, bool, etc for ourself
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
			} else if( type->name == "null" ) {
				return( llvm::Type::getVoidTy( this->module->getContext() ) );
			}

			llvm::Type* ltype = module->getTypeByName( EXO_CLASS( type->name ) );
			if( ltype != NULL ) {
				return( ltype->getPointerTo() );
			}

			EXO_THROW_EXCEPTION( UnknownClass, "Unknown class \"" + type->name  + "\"" );
			return( NULL ); // satisfy IDE
		}

		int Codegen::getPropertyPosition( std::string className, std::string propName )
		{
			std::vector<std::string>::iterator it = std::find( this->propertyIndex[ EXO_CLASS( className ) ].begin(), this->propertyIndex[ EXO_CLASS( className ) ].end(), propName );

			if( it == this->propertyIndex[ EXO_CLASS( className ) ].end() ) {
				EXO_THROW_EXCEPTION( UnknownVar, "Unknown property \"" + propName  + "\"" );
			}

			return( std::distance( this->propertyIndex[ EXO_CLASS( className ) ].begin(), it ) );
		}

		int Codegen::getMethodPosition( std::string className, std::string methodName )
		{
			std::vector<std::string>::iterator it = std::find( this->methodIndex[ EXO_CLASS( className ) ].begin(), this->methodIndex[ EXO_CLASS( className ) ].end(), methodName );

			if( it == this->methodIndex[ EXO_CLASS( className ) ].end() ) {
				EXO_THROW_EXCEPTION( InvalidCall, "Unknown method \"" + methodName  + "\"" );
			}

			return( std::distance( this->methodIndex[ EXO_CLASS( className ) ].begin(), it ) );
		}

		llvm::Function* Codegen::getCallee( std::string cName )
		{
			llvm::Function* callee = this->module->getFunction( cName );

			if( callee == 0 ) {
				EXO_THROW_EXCEPTION( UnknownFunction, "Unable to lookup \"" + cName + "\"!" );
			}

			return( callee );
		}



		// ok
		llvm::Value* Codegen::Generate( exo::ast::CallFun* call )
		{
			BOOST_LOG_TRIVIAL(debug) << "Call to \"" << call->name << "\" in (" << this->getBlockName() << ")";
			llvm::Function* callee = this->getCallee( call->name );

			if( callee->arg_size() != call->arguments->list.size() && !callee->isVarArg() ) {
				EXO_THROW_EXCEPTION( InvalidCall, "Expected arguments mismatch for \"" + call->name + "\"" );
			}

			std::vector<llvm::Value*> arguments;
			for( std::vector<exo::ast::Expr*>::iterator it = call->arguments->list.begin(); it != call->arguments->list.end(); it++ ) {
				arguments.push_back( builder.CreateLoad( (*it)->Generate( this ) ) );
			}

			llvm::Value* retval = builder.CreateCall( callee, arguments, call->name );
			llvm::Value* memory = builder.CreateAlloca( retval->getType() );
			builder.CreateStore( retval, memory );
			return( memory );
		}

		// TODO: check if the invoker is actually a type / sub type
		llvm::Value* Codegen::Generate( exo::ast::CallMethod* call )
		{
			llvm::Value* variable = call->expression->Generate( this );

			if( !EXO_IS_OBJECT( variable ) ) {
				EXO_THROW_EXCEPTION( InvalidCall, "Can only invoke method on an object." );
			}

			variable = builder.CreateLoad( variable );
			std::string cName = variable->getType()->getPointerElementType()->getStructName();

			int position = this->getMethodPosition( cName, call->name );
			llvm::Value* idx[] = {
				llvm::ConstantInt::get( llvm::Type::getInt32Ty( this->module->getContext() ), 0 ),
				llvm::ConstantInt::get( llvm::Type::getInt32Ty( this->module->getContext() ), position )
			};

			BOOST_LOG_TRIVIAL(debug) << "Call to \"" << cName << "->" << call->name << "\"@" << position << " in (" << this->getBlockName() << ")";
			llvm::Value* vtbl = this->module->getNamedGlobal( EXO_VTABLE( cName ) );
			llvm::Value* callee = builder.CreateLoad( builder.CreateInBoundsGEP( vtbl, idx, call->name ) );
			//llvm::Function* method = llvm::dyn_cast<llvm::Function>( callee );
			llvm::Function* method = this->methods[cName][position];

			if( method->arg_size() != ( call->arguments->list.size() + 1 ) && !method->isVarArg() ) {
				EXO_THROW_EXCEPTION( InvalidCall, "Expected arguments mismatch for \"" + cName + "->" + call->name + "\"!" );
			}

			std::vector<llvm::Value*> arguments;

			llvm::Function::arg_iterator it = method->arg_begin();
			if( EXO_OBJECT_CLASSNAME( it ) != EXO_OBJECT_CLASSNAME( variable ) ) {
				BOOST_LOG_TRIVIAL(debug) << "Cast " << cName << " to " << std::string( EXO_OBJECT_CLASSNAME( it ) ) << " in (" << this->getBlockName() << ")";
				arguments.push_back( builder.CreateBitCast( variable, it->getType() ) );
			} else {
				arguments.push_back( variable );
			}

			for( std::vector<exo::ast::Expr*>::iterator it = call->arguments->list.begin(); it != call->arguments->list.end(); it++ ) {
				arguments.push_back( builder.CreateLoad( (*it)->Generate( this ) ) );
			}

			llvm::Value* retval = builder.CreateCall( callee, arguments );
			llvm::Value* memory = builder.CreateAlloca( retval->getType() );
			builder.CreateStore( retval, memory );
			return( memory );
		}



		// ok
		llvm::Value* Codegen::Generate( exo::ast::ConstBool* val )
		{
			BOOST_LOG_TRIVIAL(debug) << "Generating boolean \"" << val->value << "\" in (" << this->getBlockName() << ")";

			llvm::Type* type = llvm::Type::getInt1Ty( this->module->getContext() );
			llvm::Value* memory = builder.CreateAlloca( type );

			if( val->value ) {
				builder.CreateStore( llvm::ConstantInt::getTrue( type ), memory );
			} else {
				builder.CreateStore( llvm::ConstantInt::getFalse( type ), memory );
			}
			return( memory );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::ConstFloat* val )
		{
			BOOST_LOG_TRIVIAL(debug) << "Generating float \"" << val->value << "\" in (" << this->getBlockName() << ")";
			llvm::Type* type = llvm::Type::getDoubleTy( this->module->getContext() );
			llvm::Value* memory = builder.CreateAlloca( type );
			builder.CreateStore( llvm::ConstantFP::get( type, val->value ), memory );
			return( memory );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::ConstInt* val )
		{
			BOOST_LOG_TRIVIAL(debug) << "Generating integer \"" << val->value << "\" in (" << this->getBlockName() << ")";
			llvm::Type* type = llvm::Type::getInt64Ty( this->module->getContext() );
			llvm::Value* memory = builder.CreateAlloca( type );
			builder.CreateStore( llvm::ConstantInt::get( type, val->value ), memory );
			return( memory );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::ConstNull* val )
		{
			BOOST_LOG_TRIVIAL(debug) << "Generating null in (" << this->getBlockName() << ")";
			llvm::Type* type = llvm::Type::getInt1Ty( this->module->getContext() );
			llvm::Value* memory = builder.CreateAlloca( type );
			builder.CreateStore( llvm::Constant::getNullValue( type ), memory );
			return( memory );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::ConstStr* val )
		{
			BOOST_LOG_TRIVIAL(debug) << "Generating string \"" << val->value << "\" in (" << this->getBlockName() << ")";
			llvm::Type* type = llvm::Type::getInt8PtrTy( this->module->getContext() );
			llvm::Value* memory = builder.CreateAlloca( type );
			llvm::Value* str = builder.CreateBitCast( builder.CreateGlobalString( val->value ), type );
			builder.CreateStore( str, memory );
			return( memory );
		}



		/*
		 * a class structure is CURRENTLY declared as follows:
		 * %__vtbl_struct_%className { a structure containing signatures to the class methods }
		 * %__vtbl_%className { a global instance of the vtbl structure with approriate method pointers }
		 * %className { a structure with all own + inherited properties }
		 * TODO: how do we overwrite methods & properties in a proper way?
		 */
		llvm::Value* Codegen::Generate( exo::ast::DecClass* decl )
		{
			BOOST_LOG_TRIVIAL(debug) << "Declaring class \"" << decl->name << "\" in (" << this->getBlockName() << ")";

			// if we have a parent, do inherit properties and methods
			if( decl->parent != "" ) {
				std::string pName = EXO_CLASS( decl->parent );
				llvm::StructType* parent;

				if( !( parent = module->getTypeByName( pName ) ) ) {
					EXO_THROW_EXCEPTION( UnknownClass, "Unknown parent class \"" + pName + "\"" );
				}

				/*
				for( llvm::StructType::element_iterator it = parent->element_begin(); it != parent->element_end(); it++ ) {
					properties.push_back( *it );
				}
				*/

				this->properties[ EXO_CLASS( decl->name ) ]			= this->properties[ EXO_CLASS( pName ) ];
				this->propertyIndex[ EXO_CLASS( decl->name ) ]		= this->propertyIndex[ EXO_CLASS( pName ) ];
				this->methodIndex[ EXO_CLASS( decl->name ) ]		= this->methodIndex[ EXO_CLASS( pName ) ];
				this->methods[ EXO_CLASS( decl->name ) ]			= this->methods[ EXO_CLASS( pName ) ];
				this->vtblSignatures[ EXO_CLASS( decl->name ) ]		= this->vtblSignatures[ EXO_CLASS( pName ) ];
				this->vtblInitializers[ EXO_CLASS( decl->name ) ]	= this->vtblInitializers[ EXO_CLASS( pName ) ];
			}


			// generate our properties
			for( std::vector<exo::ast::DecProp*>::iterator it = decl->block->properties.begin(); it != decl->block->properties.end(); it++ ) {
				BOOST_LOG_TRIVIAL(debug) << "Property " << (*it)->property->type->name << " $" << (*it)->property->name;
				this->propertyIndex[ EXO_CLASS( decl->name ) ].push_back( (*it)->property->name );
				this->properties[ EXO_CLASS( decl->name ) ].push_back( this->getType( (*it)->property->type ) );
			}
			llvm::StructType* classStruct = llvm::StructType::create( this->module->getContext(), this->properties[ EXO_CLASS( decl->name ) ], EXO_CLASS( decl->name ) );


			// generate our methods & vtbl
			std::vector<exo::ast::DecMethod*>::iterator mit;
			for( mit = decl->block->methods.begin(); mit != decl->block->methods.end(); mit++ ) {
				std::string oName = (*mit)->method->name;
				(*mit)->method->name = EXO_METHOD( decl->name, (*mit)->method->name );

				// a pointer to a class structure as first parameter for methods
				(*mit)->method->arguments->list.insert( (*mit)->method->arguments->list.begin(), new exo::ast::DecVar( "this", new exo::ast::Type( decl->name ) ) );
				(*mit)->method->Generate( this );

				// mark position & signature in our vtbl
				llvm::Function* cMethod = module->getFunction( (*mit)->method->name );
				this->methodIndex[ EXO_CLASS( decl->name ) ].push_back( oName );
				this->methods[ EXO_CLASS( decl->name ) ].push_back( cMethod );
				this->vtblSignatures[ EXO_CLASS( decl->name ) ].push_back( cMethod->getFunctionType()->getPointerTo() );
				this->vtblInitializers[ EXO_CLASS( decl->name ) ].push_back( cMethod );
			}

			llvm::StructType* vtbl = llvm::StructType::create( module->getContext(), this->vtblSignatures[ EXO_CLASS( decl->name ) ], EXO_VTABLE_STRUCT( decl->name ) );
			llvm::GlobalVariable* value = new llvm::GlobalVariable( *module, vtbl, false, llvm::GlobalValue::ExternalLinkage, 0, EXO_VTABLE( decl->name ) );
			value->setInitializer( llvm::ConstantStruct::get( vtbl, this->vtblInitializers[ EXO_CLASS( decl->name ) ] ) );
			return( value );
		}

		/*
		 * TODO: merge with DecFunProto
		 */
		llvm::Value* Codegen::Generate( exo::ast::DecFun* decl )
		{
			BOOST_LOG_TRIVIAL(debug) << "Generating function \"" << decl->name << "\" in (" << this->getBlockName() << ")";

			std::vector<llvm::Type*> fArgs;
			for( std::vector<exo::ast::DecVar*>::iterator it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				llvm::Type* arg = this->getType( (*it)->type );
				BOOST_LOG_TRIVIAL(trace) << "Argument " << (*it)->type->name << " $" << (*it)->name;
				fArgs.push_back( arg );
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
				llvm::Value* memory = builder.CreateAlloca( it->getType() );
				builder.CreateStore( it, memory );
				this->setBlockSymbol( decl->arguments->list.at( i )->name, memory );
				i++;
			}

			decl->stmts->Generate( this );

			if( block->getTerminator() == NULL && !this->getType( decl->returnType )->isVoidTy() ) {
				BOOST_LOG_TRIVIAL(trace) << "Generating null return in (" << getBlockName() << ")";
				builder.CreateRet( llvm::Constant::getNullValue( this->getType( decl->returnType ) ) );
			}

			this->popBlock();

			return( fun );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::DecFunProto* decl )
		{
			BOOST_LOG_TRIVIAL(debug) << "Declaring function prototype \"" << decl->name << "\" in (" << this->getBlockName() << ")";

			std::vector<llvm::Type*> fArgs;
			for( std::vector<exo::ast::DecVar*>::iterator it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				llvm::Type* arg = this->getType( (*it)->type );
				BOOST_LOG_TRIVIAL(trace) << "Argument $" << (*it)->name;
				fArgs.push_back( arg );
			}

			llvm::Function* fun = llvm::Function::Create(
				llvm::FunctionType::get( this->getType( decl->returnType ), fArgs, decl->hasVaArg ),
				llvm::GlobalValue::ExternalLinkage,
				decl->name,
				this->module
			);

			return( fun );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::DecVar* decl )
		{
			BOOST_LOG_TRIVIAL(debug) << "Allocating " << decl->type->name << " $" << decl->name << " on stack in (" << this->getBlockName() << ")";
			this->setBlockSymbol( decl->name, builder.CreateAlloca( this->getType( decl->type ) ) );

			if( decl->expression ) {
				boost::scoped_ptr<exo::ast::OpBinaryAssign> a( new exo::ast::OpBinaryAssign( new exo::ast::ExprVar( decl->name ), decl->expression ) );
				a->Generate( this );
			}

			return( this->getBlockSymbol( decl->name ) );
		}



		// ok
		llvm::Value* Codegen::Generate( exo::ast::ExprVar* expr )
		{
			return( this->getBlockSymbol( expr->name ) );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::ExprProp* expr )
		{
			llvm::Value* variable = expr->expression->Generate( this );

			if( !EXO_IS_OBJECT( variable ) ) {
				EXO_THROW_EXCEPTION( InvalidOp, "Can only fetch property of an object!" );
			}

			variable = builder.CreateLoad( variable );
			int position = this->getPropertyPosition( EXO_OBJECT_CLASSNAME( variable ), expr->name );
			llvm::Value* idx[] = {
					llvm::ConstantInt::get( llvm::Type::getInt32Ty( this->module->getContext() ), 0 ),
					llvm::ConstantInt::get( llvm::Type::getInt32Ty( this->module->getContext() ), position )
			};

			BOOST_LOG_TRIVIAL(debug) << "Load property \"" << std::string( EXO_OBJECT_CLASSNAME( variable ) ) << "->" << expr->name << "\"@" << position << " in (" << this->getBlockName() << ")";
			return( builder.CreateInBoundsGEP( variable, idx, expr->name ) );
		}



		llvm::Value* Codegen::Generate( exo::ast::OpBinary* op )
		{
			llvm::Value* lhs = builder.CreateLoad( op->lhs->Generate( this ) );
			llvm::Value* rhs = builder.CreateLoad( op->rhs->Generate( this ) );
			llvm::Value* result;

			if( typeid(*op) == typeid( exo::ast::OpBinaryAdd ) ) {
				BOOST_LOG_TRIVIAL(debug) << "Generating addition in (" << this->getBlockName() << ")";
				result = builder.CreateAdd( lhs, rhs, "add" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinarySub ) ) {
				BOOST_LOG_TRIVIAL(debug) << "Generating substraction in (" << this->getBlockName() << ")";
				result = builder.CreateSub( lhs, rhs, "sub" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryMul ) ) {
				BOOST_LOG_TRIVIAL(debug) << "Generating multiplication in (" << this->getBlockName() << ")";
				result = builder.CreateMul( lhs, rhs, "mul" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryDiv ) ) {
				BOOST_LOG_TRIVIAL(debug) << "Generating division in (" << this->getBlockName() << ")";
				result = builder.CreateSDiv( lhs, rhs, "div" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryLt ) ) {
				BOOST_LOG_TRIVIAL(debug) << "Generating lower than comparison in (" << this->getBlockName() << ")";
				result = builder.CreateICmpSLT( lhs, rhs, "cmp" );
			} else {
				EXO_THROW_EXCEPTION( InvalidOp, "Unknown binary operation." );
			}

			llvm::Value* memory = builder.CreateAlloca( result->getType() );
			builder.CreateStore( result, memory );
			return( memory );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::OpBinaryAssign* assign )
		{
			llvm::Value* variable = assign->lhs->Generate( this );
			llvm::Value* value = builder.CreateLoad( assign->rhs->Generate( this ) );

			BOOST_LOG_TRIVIAL(trace) << "Assign value type: " << value->getType()->getTypeID() << ", variable type: " << variable->getType()->getPointerElementType ()->getTypeID();
			return( builder.CreateStore( value, variable ) );
		}



		/*
		 * TODO: call destructor
		 */
		llvm::Value* Codegen::Generate( exo::ast::OpUnaryDel* op )
		{
			exo::ast::ExprVar* var = dynamic_cast<exo::ast::ExprVar*>( op->rhs );
			llvm::Value* variable = op->rhs->Generate( this );

			if( EXO_IS_OBJECT( variable ) ) {
				BOOST_LOG_TRIVIAL(debug) << "Deleting $" << var->name << " from heap in (" << this->getBlockName() << ")";
				llvm::Function* gcfree = this->getCallee( EXO_DEALLOC );
				std::vector<llvm::Value*> arguments;
				arguments.push_back( builder.CreateBitCast( builder.CreateLoad( variable ), this->ptrType ) );
				builder.CreateCall( gcfree, arguments );
			}

			this->delBlockSymbol( var->name );

			llvm::Value* retval = llvm::ConstantInt::getTrue( llvm::Type::getInt1Ty( module->getContext() ) );
			llvm::Value* memory = builder.CreateAlloca( retval->getType() );
			builder.CreateStore( retval, memory );
			return( memory );
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

			llvm::Type* type;
			if( !(type = module->getTypeByName( EXO_CLASS( init->name ) ) ) ) {
				EXO_THROW_EXCEPTION( UnknownClass, "Unknown class \"" + init->name + "\"" );
			}

			BOOST_LOG_TRIVIAL(debug) << "Allocating memory for \"" << init->name << "\" on heap in (" << this->getBlockName() << ")";
			llvm::Function* gcmalloc = this->getCallee( EXO_ALLOC );

			std::vector<llvm::Value*> arguments;
			arguments.push_back( llvm::ConstantExpr::getSizeOf( type ) );
			llvm::Value* value = builder.CreateBitCast( builder.CreateCall( gcmalloc, arguments ), type->getPointerTo() );
			llvm::Value* memory = builder.CreateAlloca( type->getPointerTo() );
			builder.CreateStore( value, memory );
			return( memory );
		}



		// ok
		llvm::Value* Codegen::Generate( exo::ast::StmtExpr* stmt )
		{
			BOOST_LOG_TRIVIAL(trace) << "Generating expression statement in (" << this->getBlockName() << ")";
			return( stmt->expression->Generate( this ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtIf* stmt )
		{
			return( this->builder.CreateRet( llvm::Constant::getNullValue( this->intType ) ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtReturn* stmt )
		{
			BOOST_LOG_TRIVIAL(debug) << "Generating return statement in (" << this->getBlockName() << ")";
			return( builder.CreateRet( builder.CreateLoad( stmt->expression->Generate( this ) ) ) );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::StmtList* stmts )
		{
			std::vector<exo::ast::Stmt*>::iterator it;
			llvm::Value *last = NULL;

			for( it = stmts->list.begin(); it != stmts->list.end(); it++ ) {
				last = (*it)->Generate( this );
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
			 * this is main()
			 */
			this->entry = llvm::Function::Create( llvm::FunctionType::get( this->intType, false ), llvm::GlobalValue::InternalLinkage, this->name, this->module );
			llvm::BasicBlock* block = llvm::BasicBlock::Create( this->module->getContext(), "entry", this->entry, 0 );

			pushBlock( block, this->name );

			Generate( tree->stmts );

			llvm::Value* retVal = block->getTerminator();
			if( retVal == NULL ) {
				BOOST_LOG_TRIVIAL(debug) << "Generating null return in (" << this->name << ")";
				retVal = this->builder.CreateRet( llvm::Constant::getNullValue( this->intType ) );
			}

			popBlock();

			return( retVal );
		}
	}
}
