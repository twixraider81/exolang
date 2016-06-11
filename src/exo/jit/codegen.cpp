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

#include "exo/ast/nodes.h"
#include "exo/jit/codegen.h"
#include "exo/jit/stack.h"

namespace exo
{
	namespace jit
	{
		Codegen::Codegen( std::unique_ptr<llvm::Module> m, std::vector<std::string> i, std::vector<std::string> l ) :
			module( std::move(m) ),
			builder( module->getContext() ),
			includePaths( i ),
			libraryPaths( l )
		{
			stack = std::make_unique<Stack>();
		}

		Codegen::~Codegen()
		{
		}

		/*
		 * TODO: allow raw ctypes with proper names, we want int, bool, etc for ourself
		 */
		llvm::Type* Codegen::getType( exo::ast::Type* type )
		{
			if( type->isPrimitive ) {
				if( type->id->name == "int" ) {
					return( llvm::Type::getInt64Ty( module->getContext() ) );
				} else if( type->id->name == "float" ) {
					return( llvm::Type::getDoubleTy( module->getContext() ) );
				} else if( type->id->name == "bool" ) {
					return( llvm::Type::getInt1Ty( module->getContext() ) );
				} else if( type->id->name == "string" ) {
					return( llvm::Type::getInt8PtrTy( module->getContext() ) );
				} else if( type->id->name == "null" ) {
					return( llvm::Type::getVoidTy( module->getContext() ) );
				}

				EXO_THROW( UnknownPrimitive() );
			}

			llvm::Type* complex = module->getTypeByName( EXO_CLASS( type->id->name ) );
			if( complex != nullptr ) {
				return( complex->getPointerTo() );
			}

			EXO_THROW( UnknownClass() << exo::exceptions::ClassName( type->id->name ) );
			return( nullptr );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstBool* val, bool inMem )
		{
			EXO_CODEGEN_LOG( val, "Boolean:" << val->value );

			llvm::Type* type = llvm::Type::getInt1Ty( module->getContext() );
			llvm::Value* value;
			if( val->value ) {
				value = llvm::ConstantInt::getTrue( type );
			} else {
				value = llvm::ConstantInt::getFalse( type );
			}

			if( !inMem ) { // do not allocate and return
				return( value );
			}

			llvm::AllocaInst* memory = builder.CreateAlloca( type );
			builder.CreateStore( value, memory );
			return( memory );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstFloat* val, bool inMem )
		{
			EXO_CODEGEN_LOG( val, "Float:" << val->value );

			llvm::Type* type = llvm::Type::getDoubleTy( module->getContext() );
			llvm::Value* value = llvm::ConstantFP::get( type, val->value );

			if( !inMem ) { // do not allocate and return
				return( value );
			}

			llvm::AllocaInst* memory = builder.CreateAlloca( type );
			builder.CreateStore( value, memory );
			return( memory );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstInt* val, bool inMem )
		{
			EXO_CODEGEN_LOG( val, "Integer:" << val->value );

			llvm::Type* type = llvm::Type::getInt64Ty( module->getContext() );
			llvm::Value* value = llvm::ConstantInt::get( type, val->value );

			if( !inMem ) { // do not allocate and return
				return( value );
			}

			llvm::AllocaInst* memory = builder.CreateAlloca( type );
			builder.CreateStore( value, memory );
			return( memory );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstNull* val, bool inMem )
		{
			EXO_CODEGEN_LOG( val, "Null" );

			llvm::Type* type = llvm::Type::getInt1Ty( module->getContext() );
			llvm::Value* value = llvm::Constant::getNullValue( type );

			if( !inMem ) { // do not allocate and return
				return( value );
			}

			llvm::AllocaInst* memory = builder.CreateAlloca( type );
			builder.CreateStore( value, memory );
			return( memory );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstStr* val, bool inMem )
		{
			EXO_CODEGEN_LOG( val, "String:" << val->value.length() );

			llvm::Value* value = builder.CreateGlobalStringPtr( val->value );
			return( value );
			/*
			if( inMem ) {
				return( value );
			}

			return( builder.CreateLoad( value ) );
			*/
		}

		/*
		 * TODO: how do we overwrite methods & properties in a proper way?
		 */
		llvm::Value* Codegen::Generate( exo::ast::DeclClass* decl, bool inMem )
		{
			EXO_CODEGEN_LOG( decl, "Declaring class " << decl->id->name );

			std::string name = EXO_CLASS( decl->id->name );

			llvm::StructType* structr = llvm::StructType::create( module->getContext(), name );
			llvm::StructType* vstructr = llvm::StructType::create( module->getContext(), EXO_VTABLE( decl->id->name ) );
			std::vector<llvm::Type*> elems;
			/*
			std::vector<llvm::Type*> vtbl;
			int methodsSize, propSize = 0;
			*/

			// if we have a parent, do inherit properties and methods
			if( decl->parent ) {
				std::string pName = EXO_CLASS( decl->parent->name );
				llvm::StructType* parent = module->getTypeByName( pName );

				if( parent == nullptr ) {
					EXO_THROW_AT( UnknownClass() << exo::exceptions::ClassName( decl->parent->name ), decl );
				}

				elems = parent->elements();
				properties[ name ] = properties[ pName ];
				methods[ name ] = methods[ pName ];
				/*methodsSize = methods[ name ].size();*/
			} else {
				/*
				elems = { vstructr };
				methodsSize = 0;
				*/
			}


			// generate our properties
			for( auto &property : decl->properties ) {
				llvm::Type* type = getType( property->property->type.get() );
				llvm::Value* value;

				if( property->property->expression ) {
					value = property->property->expression->Generate( this );
				} else {
					value = llvm::Constant::getNullValue( type );
				}

				int position;
				try {
					position = properties.at( name ).at( property->property->name ).first;
					elems.at( position ) = type;
				} catch( ... ) {
					position = elems.size();
					elems.push_back( type );
				}

				EXO_DEBUG_LOG( trace, "Declare property " << name << "->" << property->property->name << "@" << position << " - " << toString( value ) );
				properties[ name ][ property->property->name ].first = position;
				properties[ name ][ property->property->name ].second = value;
			}
			structr->setBody( elems );


			// generate our methods
			/*
			methodsSize += decl->methods.size();
			vtbl.resize( methodsSize, llvm::FunctionType::get( llvm::Type::getVoidTy( module->getContext() ), false ) );
			vstructr->setBody( vtbl ); // populate opaque type or gep instructions will fail
			*/
			for( auto &method : decl->methods ) {
				std::string methodName = method->id->name;
				method->id->name = EXO_METHOD( decl->id->name, method->id->name );

				method->arguments->list.insert( method->arguments->list.begin(), std::make_unique<exo::ast::DeclVar>( "this", std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( decl->id->name, decl->id->inNamespace ) ) ) );
				method->Generate( this, false );

				int position;
				try {
					position = methods.at( name ).at( methodName ).first;
				} catch( ... ) {
					position = methods[ name ].size();
				}

				methods[ name ][ methodName ].first = position;
				methods[ name ][ methodName ].second = module->getFunction( method->id->name );
			}


			// generate our vtbl signature
			/*
			if( methodsSize ) {
				for( auto &method : methods[ name ] ) {
					llvm::Type* type = method.second.second->getFunctionType()->getPointerTo();
					EXO_DEBUG_LOG( trace, "Declare method " << name << "->" << method.first << "@" << method.second.first << " - " << toString( type ) );
					vtbl.at( method.second.first ) = type;
				}
			}
			vstructr->setBody( vtbl );
			elems.at( 0 ) = vstructr;
			structr->setBody( elems );
			*/

			// we dont differentiate return memory
			return( llvm::ConstantInt::getTrue( llvm::Type::getInt1Ty( module->getContext() ) ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::DeclFunProto* decl, bool inMem )
		{
			EXO_CODEGEN_LOG( decl, "Declaring function prototype " << decl->id->name );

			// build up the function argument list
			std::vector<llvm::Type*> arguments;
			for( auto &argument : decl->arguments->list ) {
				arguments.push_back( getType( argument->type.get() ) );
			}

			llvm::Function* function = llvm::Function::Create(
				llvm::FunctionType::get( getType( decl->returnType.get() ), arguments, decl->hasVaArg ),
				llvm::GlobalValue::ExternalLinkage,
				decl->id->name,
				module.get()
			);

			// for now we dont differentiate return memory
			return( function );
		}

		llvm::Value* Codegen::Generate( exo::ast::DeclFun* decl, bool inMem )
		{
			EXO_CODEGEN_LOG( decl, "Declaring function " << decl->id->name );

			// build up the function argument list
			std::vector<llvm::Type*> arguments;
			for( auto &argument : decl->arguments->list ) {
				llvm::Type* type = getType( argument->type.get() );

				// if argument is passed by reference
				if( argument->isRef ) {
					arguments.push_back( type->getPointerTo() );
				} else {
					arguments.push_back( type );
				}
			}

			// create a function block for our local variables
			llvm::Function* function = llvm::Function::Create(
					llvm::FunctionType::get( getType( decl->returnType.get() ),	arguments, decl->hasVaArg ),
					( decl->access->isPublic ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::InternalLinkage ),
					decl->id->name,
					module.get()
			);
			llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), decl->id->name, function );

			// track our scope, exit block
			builder.SetInsertPoint( stack->Push( block, stack->Block() ) );

			// create loads for the arguments passed to our function
			int i = 0;
			for( auto &argument : function->args() ) {
				exo::ast::DeclVar* var = decl->arguments->list.at( i ).get();

				// if argument is passed by reference, it should be already allocated
				if( var->isRef ) {
					stack->Set( var->name, &argument, true );
				} else {
					llvm::AllocaInst* memory = builder.CreateAlloca( argument.getType() );
					builder.CreateStore( &argument, memory );
					stack->Set( var->name, memory );
				}

				i++;
			}

			// generate our actual function statements
			llvm::Value* retVal = decl->stmts->Generate( this, false );

			// sanitize function exit
			if( retVal == nullptr || !llvm::isa<llvm::ReturnInst>(retVal) ) {
				if( getType( decl->returnType.get() )->isVoidTy() ) {
					EXO_CODEGEN_LOG( decl, "Generating void return" );
					builder.CreateRetVoid();
				} else {
					EXO_CODEGEN_LOG( decl, "Generating null return" );
					builder.CreateRet( llvm::Constant::getNullValue( getType( decl->returnType.get() ) ) );
				}
			}

			// pop our block, void local variables
			builder.SetInsertPoint( stack->Pop() );
			return( function );
		}

		llvm::Value* Codegen::Generate( exo::ast::DeclMod* decl, bool inMem )
		{
			return( llvm::ConstantInt::getTrue( llvm::Type::getInt1Ty( module->getContext() ) ) );
		}

		// TODO: add readonly variable declarations (for i.e. instance pointers)
		llvm::Value* Codegen::Generate( exo::ast::DeclVar* decl, bool inMem )
		{
			EXO_LOG( trace, "Declaring " << decl->type->id->name << ( decl->isRef ? " reference " : " " ) << "$" << decl->name );

			llvm::AllocaInst* memory;
			llvm::Value* value;
			llvm::Type* type = getType( decl->type.get() );

			if( decl->isRef ) {
				type = type->getPointerTo(); // only needed for primitives?
			}

			try {
				memory = builder.CreateAlloca( type );

				if( decl->expression ) {
					value = decl->expression->Generate( this );

					if( decl->isRef && !value->getType()->isPointerTy() ) { // we can only assign variables as reference
						EXO_THROW_AT( InvalidOp() << exo::exceptions::Message( "Can only assign variables by reference" ), decl );
					}
				} else {
					value = llvm::Constant::getNullValue( type );
				}

				builder.CreateStore( value, memory );
				stack->Set( decl->name, memory, decl->isRef );
			} catch( boost::exception &exception ) {
				exception << boost::errinfo_at_line( decl->lineNo ) << exo::exceptions::ColumnNo( decl->columnNo );
				throw;
			}

			if( !inMem ) {
				return( value );
			}

			return( memory );
		}

		llvm::Value* Codegen::Generate( exo::ast::DeclVarList* decl, bool inMem )
		{
			llvm::Value* last = nullptr;

			for( auto &argument : decl->list ) {
				last = argument->Generate( this, inMem );
			}

			return( last );
		}

		llvm::Value* Codegen::Generate( exo::ast::ExprCallFun* call, bool inMem )
		{
			EXO_LOG( debug, "Call function " << call->id->name );

			llvm::Value* value = nullptr;
			llvm::Function* function = getFunction( call->id->name );

			try {
				value = invokeFunction( function, function->getFunctionType(), {}, call->arguments.get(), inMem );
			} catch( boost::exception &exception ) {
				exception << exo::exceptions::FunctionName( call->id->name );
				throw;
			}

			return( value );
		}

		// TODO: check if the invoker is actually a type / sub type
		llvm::Value* Codegen::Generate( exo::ast::ExprCallMethod* call, bool inMem )
		{
			return( invokeMethod( call->expression->Generate( this, false ), call->id->name, call->arguments.get(), false, inMem ) );
		}

		// FIXME: track if instance property is actually initialized (needs exception handling)
		llvm::Value* Codegen::Generate( exo::ast::ExprProp* expr, bool inMem )
		{
			llvm::Value* expression = expr->expression->Generate( this, false );

			llvm::Type* type = expression->getType();
			if( !type->isPointerTy() ) {
				EXO_DEBUG_LOG( trace, toString( expression ) );
				EXO_THROW_AT( InvalidOp() << exo::exceptions::Message( "Expecting object" ), expr );
			}

			type = type->getPointerElementType();
			if( !type->isStructTy() ) {
				EXO_DEBUG_LOG( trace, toString( expression ) );
				EXO_THROW_AT( InvalidOp() << exo::exceptions::Message( "Invalid object" ), expr );
			}


			int position;
			try {
				position = getPropPos( type->getStructName(), expr->name );
			} catch( boost::exception &exception ) {
				exception << boost::errinfo_at_line( expr->lineNo ) << exo::exceptions::ColumnNo( expr->columnNo );
				throw;
			}

			EXO_DEBUG_LOG( trace, "Property " << std::string( type->getStructName() ) << "->" << expr->name << "@" << position );
			llvm::Value* value = builder.CreateGEP( expression, { llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ), llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), position ) }, expr->name );

			if( !inMem ) {
				return( builder.CreateLoad( value ) );
			}

			return( value );
		}

		llvm::Value* Codegen::Generate( exo::ast::ExprVar* expr, bool inMem )
		{
			llvm::Value* variable = nullptr;

			try {
				variable = stack->Get( expr->name );

				if( stack->isRef( expr->name ) && !inMem ) { //TODO: this is ambiguous, but ok i guess. if variable is a reference and we want register access deref it
					EXO_CODEGEN_LOG( expr, "Dereferencing $" << expr->name );
					variable = builder.CreateLoad( variable );
				}

			} catch( boost::exception &exception ) {
				exception << boost::errinfo_at_line( expr->lineNo ) << exo::exceptions::ColumnNo( expr->columnNo );
				throw;
			}

			if( !inMem ) {
				variable = builder.CreateLoad( variable );
			}

			return( variable );
		}

		llvm::Value* Codegen::Generate( exo::ast::OpBinary* op, bool inMem )
		{
			llvm::Value* lhs = op->lhs->Generate( this, false );
			llvm::Value* rhs = op->rhs->Generate( this, false );
			llvm::Value* result;

			if( typeid(*op) == typeid( exo::ast::OpBinaryAdd ) ) {
				EXO_CODEGEN_LOG( op, "Addition" );
				result = builder.CreateAdd( lhs, rhs, "add" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinarySub ) ) {
				EXO_CODEGEN_LOG( op, "Substraction" );
				result = builder.CreateSub( lhs, rhs, "sub" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryMul ) ) {
				EXO_CODEGEN_LOG( op, "Multiplication" );
				result = builder.CreateMul( lhs, rhs, "mul" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryDiv ) ) {
				EXO_CODEGEN_LOG( op, "Division" );
				result = builder.CreateSDiv( lhs, rhs, "div" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryLt ) ) {
				EXO_CODEGEN_LOG( op, "Lower than comparison" );
				result = builder.CreateICmpSLT( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryLe ) ) {
				EXO_CODEGEN_LOG( op, "Lower equal comparison" );
				result = builder.CreateICmpSLE( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryGe ) ) {
				EXO_CODEGEN_LOG( op, "Greater equal comparison" );
				result = builder.CreateICmpSGE( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryGt ) ) {
				EXO_CODEGEN_LOG( op, "Greater than comparison" );
				result = builder.CreateICmpSGT( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryEq ) ) {
				EXO_CODEGEN_LOG( op, "Is equal comparison" );
				result = builder.CreateICmpEQ( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryEq ) ) {
				EXO_CODEGEN_LOG( op, "Not equal comparison" );
				result = builder.CreateICmpNE( lhs, rhs, "cmp" );
			} else {
				EXO_THROW_AT( InvalidOp(), op );
				return( nullptr );
			}

			if( inMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( result->getType() );
				builder.CreateStore( result, memory );
				return( memory );
			}

			return( result );
		}

		llvm::Value* Codegen::Generate( exo::ast::OpBinaryAssign* assign, bool inMem )
		{
			EXO_CODEGEN_LOG( assign, "Assignment" );

			if( !assign->lhs || !assign->rhs ) {
				EXO_THROW_AT( InvalidOp(), assign );
			}

			llvm::Value* variable = assign->lhs->Generate( this, true );
			llvm::Value* value = assign->rhs->Generate( this, false );

			if( variable->getType()->getPointerElementType()->isPointerTy() ) { // dealing with references
				if( !value->getType()->isPointerTy() ) { //TODO: this is ambiguous, but ok i guess. dereference our reference in assigments.
					variable = builder.CreateLoad( variable );
				}
			}

			builder.CreateStore( value, variable );

			if( !inMem ) {
				return( value );
			}

			return( variable );
		}

		llvm::Value* Codegen::Generate( exo::ast::OpBinaryAssignShort* assign, bool inMem )
		{
			EXO_CODEGEN_LOG( assign, "Assign operation" );

			if( !assign->lhs || !assign->rhs ) {
				EXO_THROW_AT( InvalidOp(), assign );
			}

			llvm::Value* variable = assign->lhs->Generate( this, true );
			llvm::Value* value = assign->rhs->Generate( this, false );
			llvm::Value* result = nullptr;

			if( typeid(*assign) == typeid( exo::ast::OpBinaryAssignAdd ) ) {
				result = builder.CreateAdd( builder.CreateLoad( variable ), value, "add" );
			} else if( typeid(*assign) == typeid( exo::ast::OpBinaryAssignSub ) ) {
				result = builder.CreateSub( builder.CreateLoad( variable ), value, "sub" );
			} else if( typeid(*assign) == typeid( exo::ast::OpBinaryAssignMul ) ) {
				result = builder.CreateMul( builder.CreateLoad( variable ), value, "mul" );
			} else if( typeid(*assign) == typeid( exo::ast::OpBinaryAssignDiv ) ) {
				result = builder.CreateSDiv( builder.CreateLoad( variable ), value, "div" );
			} else {
				EXO_THROW_AT( InvalidOp(), assign );
			}

			builder.CreateStore( result, variable );

			if( !inMem ) {
				return( result );
			}

			return( variable );
		}

		/*
		 * FIXME: we expect a variable, thats bad since expression can return memory on their own so track memory addresses instead of var names
		 * FIXME: deallocation should occur on/thru the stack
		 */
		llvm::Value* Codegen::Generate( exo::ast::OpUnaryDel* op, bool inMem )
		{
			llvm::Value* memory = op->rhs->Generate( this, true );
			llvm::Value* value = builder.CreateLoad( memory );

			try {
				std::unique_ptr<exo::ast::ExprList> arguments = std::make_unique<exo::ast::ExprList>();
				invokeMethod( value, "__destruct", arguments.get(), true, false );

				EXO_CODEGEN_LOG( op, "Deallocating heap memory" );
				builder.CreateCall( getFunction( EXO_DEALLOC ), { builder.CreateBitCast( value, module->getDataLayout().getIntPtrType( module->getContext() )->getPointerTo() ) } );
				builder.CreateStore( llvm::Constant::getNullValue( value->getType() ), memory );

				// FIXME: stack->Del( op->rhs->name );
			} catch( boost::exception &exception ) {
				exception << boost::errinfo_at_line( op->lineNo ) << exo::exceptions::ColumnNo( op->columnNo );
				throw;
			}

			value = llvm::ConstantInt::getTrue( llvm::Type::getInt1Ty( module->getContext() ) );
			if( !inMem ) {
				return( value );
			}

			llvm::AllocaInst* retval = builder.CreateAlloca( value->getType() );
			builder.CreateStore( value, retval );
			return( retval );
		}

		/*
		 * TODO: we should track heap allocations
		 * TODO: get rid of cast
		 */
		llvm::Value* Codegen::Generate( exo::ast::OpUnaryNew* op, bool inMem )
		{
			exo::ast::ExprCallFun* constructor = dynamic_cast<exo::ast::ExprCallFun*>( op->rhs.get() );

			if( constructor == nullptr ) {
				EXO_THROW_AT( InvalidOp() << exo::exceptions::Message( "Constructor not found" ), op );
			}

			EXO_CODEGEN_LOG( op, "Allocating heap memory for " << constructor->id->name );

			std::string className = EXO_CLASS( constructor->id->name );
			llvm::Type* type = module->getTypeByName( className );
			if( type == nullptr ) {
				EXO_THROW_AT( UnknownClass() << exo::exceptions::ClassName( constructor->id->name ), op );
			}

			// allocate heap memory for struct of our class
			llvm::CallInst* memory = builder.CreateCall( getFunction( EXO_ALLOC ), { llvm::ConstantExpr::getSizeOf( type ) } );
			llvm::Value* cast = builder.CreateBitCast( memory, type->getPointerTo() );

			// initialize property defaults
			for( auto &property : properties.at( className ) ) {
				llvm::Value* value = builder.CreateGEP( cast, { llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ), llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), property.second.first ) }, property.first );
				builder.CreateStore( property.second.second, value );
			}

			// populate vtbl
			/*
			llvm::Value* vtbl = builder.CreateGEP( cast, { llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ), llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ) } );
			for( auto &method : methods.at( className ) ) {
				llvm::Value* value = builder.CreateGEP( vtbl, { llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ), llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), method.second.first ) }, method.first );
				builder.CreateStore( method.second.second, value );
			}
			*/

			invokeMethod( cast, "__construct", constructor->arguments.get(), true, false );
			return( cast );
		}

		llvm::Value* Codegen::Generate( exo::ast::OpUnaryRef* op, bool inMem )
		{
			EXO_CODEGEN_LOG( op, "Creating reference" );
			return( op->rhs->Generate( this, true ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtBlock* stmts, bool inMem )
		{
			//EXO_CODEGEN_LOG( stmts, "Generating " << stmts->list.size() << " statement(s)" );

			llvm::Value *last = nullptr;
			for( auto &stmt : stmts->list ) {
				// flow might have been altered (i.e. a break) stop generating in that case
				if( stack->Block()->getTerminator() != nullptr ) {
					break;
				}

				last = stmt->Generate( this, inMem );
			}

			return( last );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtBreak* stmt, bool inMem )
		{
			llvm::BasicBlock* exitBlock = stack->Exit();
			if( exitBlock == nullptr ) {
				EXO_THROW_AT( InvalidBreak(), stmt );
			}

			EXO_CODEGEN_LOG( stmt, "Break" );

			// we dont differentiate return memory
			return( builder.CreateBr( exitBlock ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtCont* stmt, bool inMem )
		{
			llvm::BasicBlock* exitBlock = stack->Exit();
			if( exitBlock == nullptr ) { // check if we are actually in a loop
				EXO_THROW_AT( InvalidCont(), stmt );
			}

			EXO_CODEGEN_LOG( stmt, "Continue" );

			// we need to split the stack
			llvm::Function* scope		= stack->Block()->getParent();
			llvm::BasicBlock* contBlock	= llvm::BasicBlock::Create( module->getContext(), "continue", scope );
			llvm::Value* retval = builder.CreateBr( contBlock );
			builder.SetInsertPoint( contBlock );

			// we dont differentiate return memory
			return( retval );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtExpr* stmt, bool inMem )
		{
			EXO_CODEGEN_LOG( stmt, "Expression statement" );
			return( stmt->expression->Generate( this, inMem ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtFor* stmt, bool inMem )
		{
			EXO_CODEGEN_LOG( stmt, "For loop" );

			// setup basic blocks for loop statments and exit
			llvm::Function* scope		= stack->Block()->getParent();
			llvm::BasicBlock* forBlock	= llvm::BasicBlock::Create( module->getContext(), "for", scope );
			llvm::BasicBlock* contBlock	= llvm::BasicBlock::Create( module->getContext(), "continue" );

			// initialize loop variables
			for( auto &statement : stmt->initialization->list ) {
				statement->Generate( this, false );
			}
			builder.CreateBr( forBlock );

			// keep track of our exit/continue block
			builder.SetInsertPoint( stack->Push( forBlock, contBlock ) );

			// generate our for loop statements
			llvm::Value* condition = stmt->expression->Generate( this, false );
			stmt->block->Generate( this );

			// update loop variables
			for( auto &statement : stmt->update->list ) {
				statement->Generate( this, false );
			}

			// branch into loop again
			builder.CreateCondBr( condition, forBlock, contBlock );

			stack->Pop();

			scope->getBasicBlockList().push_back( contBlock );
			builder.SetInsertPoint( stack->Join( contBlock ) );

			if( inMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( condition->getType() );
				builder.CreateStore( condition, memory );
				return( memory );
			}

			return( condition );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtIf* stmt, bool inMem )
		{
			EXO_CODEGEN_LOG( stmt, "If statement" );

			// setup basic blocks for if, else and exit
			llvm::Function* scope		= stack->Block()->getParent();
			llvm::BasicBlock* ifBlock	= llvm::BasicBlock::Create( module->getContext(), "if", scope );
			llvm::BasicBlock* contBlock	= llvm::BasicBlock::Create( module->getContext(), "continue" );
			llvm::BasicBlock* elseBlock;

			if( stmt->onFalse ) {
				elseBlock = llvm::BasicBlock::Create( module->getContext(), "else" );
			}


			// evaluate our if expression
			llvm::Value* condition = stmt->expression->Generate( this, false );

			// if we have no else block, we can directly branch to exit/continue block
			if( stmt->onFalse == nullptr ) {
				builder.CreateCondBr( condition, ifBlock, contBlock );
			} else {
				builder.CreateCondBr( condition, ifBlock, elseBlock );
			}

			// generate our if statements, inherit parent scope variables
			builder.SetInsertPoint( stack->Push( ifBlock ) );
			stmt->onTrue->Generate( this, false );

			// flow might have been altered by i.e. a break
			if( stack->Block()->getTerminator() == nullptr ) {
				builder.CreateBr( contBlock );
			}

			builder.SetInsertPoint( stack->Pop() );

			// generate our else statements
			if( stmt->onFalse ) {
				EXO_CODEGEN_LOG( stmt->onFalse, "Else statement" );

				scope->getBasicBlockList().push_back( elseBlock );
				builder.SetInsertPoint( stack->Push( elseBlock ) );
				stmt->onFalse->Generate( this, false );

				// flow might have been altered by i.e. a break
				if( stack->Block()->getTerminator() == nullptr ) {
					builder.CreateBr( contBlock );
				}

				builder.SetInsertPoint( stack->Pop() );
			}

			scope->getBasicBlockList().push_back( contBlock );
			builder.SetInsertPoint( stack->Join( contBlock ) );

			if( inMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( condition->getType() );
				builder.CreateStore( condition, memory );
				return( memory );
			}

			return( condition );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtImport* stmt, bool inMem )
		{
			EXO_CODEGEN_LOG( stmt, "Import" );

			boost::system::error_code error;
			boost::filesystem::path fileName = boost::filesystem::path( stmt->library->value );

			if( !fileName.has_extension() ) {
				fileName = fileName.replace_extension( ".so" );
			}

			if( !fileName.is_absolute() ) {
				for( const auto &path : libraryPaths ) {
					boost::filesystem::path testName = boost::filesystem::path( path ) / fileName;

					if( boost::filesystem::exists( testName, error ) ) {
						fileName = testName;
						break;
					}
				}
			}

			boost::filesystem::path libFile;
			try {
				libFile = boost::filesystem::canonical( fileName );
			} catch( boost::exception &exception ) {
				EXO_THROW_AT( NotFound() << exo::exceptions::RessouceName( fileName.string() ), stmt );
			}

			std::string libName = libFile.string();
			imports.insert( libName );

			EXO_LOG( debug, "Importing \"" << libName << "\" in (" << stack->blockName() << ")" );

			// we dont differentiate return memory
			return( llvm::ConstantInt::getTrue( llvm::Type::getInt1Ty( module->getContext() ) ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtReturn* stmt, bool inMem )
		{
			EXO_CODEGEN_LOG( stmt, "Return" );

			if( stack->Block()->getTerminator() != nullptr ) {
				EXO_LOG( debug, "Block already terminated" );
			}

			return( builder.CreateRet( stmt->expression->Generate( this, inMem ) ) );
		}

		// FIXME: doesnt handle recursiveness
		// TODO: load compiled (.ll) modules
		llvm::Value* Codegen::Generate( exo::ast::StmtUse* decl, bool inMem )
		{
			EXO_CODEGEN_LOG( decl, "Use " << decl->id->name );

			std::string fileName;
			boost::system::error_code error;

			if( decl->id->inNamespace.length() ) { // we have a namespace, translate namespaces to folder names
				fileName += decl->id->inNamespace;
				boost::replace_all( fileName, "::", "/" );
			}
			fileName += decl->id->name;
			fileName.append( ".exo" );

			boost::filesystem::path filePath = boost::filesystem::path( fileName );
			if( !boost::filesystem::exists( filePath, error ) ) {
				for( const auto &path : includePaths ) {
					boost::filesystem::path testFile = boost::filesystem::path( path ) / filePath;

					if( boost::filesystem::exists( testFile, error ) ) {
						filePath = testFile;
						break;
					}
				}
			}

			boost::filesystem::path moduleFile;
			try {
				moduleFile = boost::filesystem::canonical( filePath );
			} catch( boost::exception &exception ) {
				EXO_THROW_AT( UnknownModule() << exo::exceptions::ModuleName( decl->id->name ), decl );
			}

			EXO_LOG( debug, "Using " << moduleFile.string() );
			std::unique_ptr<exo::ast::Tree> ast = std::make_unique<exo::ast::Tree>();
			ast->Parse( moduleFile.string(), currentTarget );

			// we dont differentiate return memory
			return( ast->stmts->Generate( this ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtWhile* stmt, bool inMem )
		{
			EXO_CODEGEN_LOG( stmt, "While" );

			// setup basic blocks for condition, loop and exit
			llvm::Function* scope				= stack->Block()->getParent();
			llvm::BasicBlock* whileCondition	= llvm::BasicBlock::Create( module->getContext(), "while", scope );
			llvm::BasicBlock* whileLoop			= llvm::BasicBlock::Create( module->getContext(), "loop" );
			llvm::BasicBlock* whileExit			= llvm::BasicBlock::Create( module->getContext(), "continue" );

			// branch into check condition
			builder.CreateBr( whileCondition );
			builder.SetInsertPoint( stack->Push( whileCondition ) );

			// check if we (still) execute our while loop
			llvm::Value* condition = stmt->expression->Generate( this, false );
			builder.CreateCondBr( condition, whileLoop, whileExit );

			// append to proper block
			scope->getBasicBlockList().push_back( whileLoop );

			// keep track of our exit/continue block
			builder.SetInsertPoint( stack->Push( whileLoop, whileExit ) );
			stmt->block->Generate( this );

			// branch into check condition to see if we enter loop another time
			builder.CreateBr( whileCondition );

			stack->Pop(); // loop
			stack->Pop(); // condition

			scope->getBasicBlockList().push_back( whileExit );
			builder.SetInsertPoint( stack->Join( whileExit ) );

			if( inMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( condition->getType() );
				builder.CreateStore( condition, memory );
				return( memory );
			}

			return( condition );
		}

		llvm::Value* Codegen::Generate( exo::ast::Tree* tree )
		{
			llvm::Type* intType = module->getDataLayout().getIntPtrType( module->getContext() );
			llvm::Type* ptrType = intType->getPointerTo();
			llvm::Type* voidType = llvm::Type::getVoidTy( module->getContext() );

			registerExternFun( EXO_ALLOC, ptrType, { intType } );
			registerExternFun( EXO_DEALLOC, voidType, { ptrType } );

			// module function entry, named as the module
			llvm::Function* entry = llvm::Function::Create( llvm::FunctionType::get( intType, false ), llvm::GlobalValue::ExternalLinkage, module->getName(), module.get() );
			llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), module->getName(), entry );

			builder.SetInsertPoint( stack->Push( block ) );

			boost::filesystem::path currentPath = boost::filesystem::current_path();

			try {
				currentFile = tree->fileName; //TODO: maybe make this a boost::filesystem::path
				currentTarget = tree->targetMachine;

				boost::filesystem::current_path( boost::filesystem::path( currentFile ).parent_path() );

				if( tree->stmts ) {
					tree->stmts->Generate( this );
				}
			} catch( boost::exception &exception ) {
				if( !boost::get_error_info<boost::errinfo_file_name>( exception ) ) {
					exception << boost::errinfo_file_name( tree->fileName );
				}
				throw;
			}

			boost::filesystem::current_path( currentPath );

			block = stack->Pop();
			builder.SetInsertPoint( block );

			llvm::TerminatorInst* retVal = block->getTerminator();
			if( retVal == nullptr || !llvm::isa<llvm::ReturnInst>(retVal) ) {
				EXO_LOG( debug, "Generating implicit null return" );
				retVal = builder.CreateRet( llvm::ConstantInt::get( intType, 0 ) );
			}

			return( retVal );
		}

		std::string Codegen::toString( llvm::Value* value )
		{
			if( value == nullptr ) {
				return( "void" );
			}

			std::string buffer;
			llvm::raw_string_ostream bStream( buffer );

			value->print( bStream, true );

			return( bStream.str() );
		}

		std::string Codegen::toString( llvm::Type* type )
		{
			if( type == nullptr ) {
				return( "void" );
			}

			std::string buffer;
			llvm::raw_string_ostream bStream( buffer );

			type->print( bStream, true );

			return( bStream.str() );
		}

		llvm::Function* Codegen::registerExternFun( std::string name, llvm::Type* retType, std::vector<llvm::Type*> fArgs )
		{
			return( llvm::Function::Create( llvm::FunctionType::get( retType, fArgs, false ), llvm::GlobalValue::ExternalLinkage, name, module.get() ) );
		}

		llvm::Function* Codegen::getFunction( std::string functionName )
		{
			llvm::Function* callee = module->getFunction( functionName );

			if( callee == nullptr ) {
				EXO_THROW( UnknownFunction() << exo::exceptions::FunctionName( functionName ) );
			}

			return( callee );
		}

		llvm::Value* Codegen::invokeFunction( llvm::Value* callee, llvm::FunctionType* function, std::vector<llvm::Value*> arguments, exo::ast::ExprList* expressions, bool inMem )
		{
			if( !function->isVarArg() && function->getNumParams() != ( expressions->list.size() + arguments.size() ) ) {
				EXO_THROW_AT( InvalidCall() << exo::exceptions::Message( "Expected parameters mismatch" ), expressions );
			}

			std::vector<llvm::Value*> call;

			int i = 0;
			for( auto &argument : function->params() ) {
				llvm::Value* value = nullptr;

				if( arguments.size() ) {
					value = arguments.front();
					arguments.erase( arguments.begin() );
				} else if( expressions->list.size() ) {
					value = expressions->list.front()->Generate( this, false );
					expressions->list.erase( expressions->list.begin() );
				}

				if( value == nullptr || value->getType()->getTypeID() != argument->getTypeID() ) {
					EXO_DEBUG_LOG( trace, toString( value ) );
					EXO_DEBUG_LOG( trace, toString( argument ) );
					EXO_THROW_AT( InvalidCall() << exo::exceptions::Message( "Parameter:" + std::to_string( ++i ) + " type mismatch" ), expressions );
				}

				// TODO: check hierarchy
				if( argument->isPointerTy() ) {
					if( argument->getPointerElementType()->isStructTy() ) {
						if( !value->getType()->getPointerElementType()->isStructTy() ) {
							EXO_DEBUG_LOG( trace, toString( value->getType() ) );
							EXO_THROW_AT( InvalidCall() << exo::exceptions::Message( "Expecting object" ), expressions );
						}

						if( argument->getPointerElementType()->getStructName() != value->getType()->getPointerElementType()->getStructName() ) {
							value = builder.CreateBitCast( value, argument );
						}
					}
				}

				call.push_back( value );
			}

			// if we have a vararg function, assume by value
			if( function->isVarArg() && ( arguments.size() || expressions->list.size() ) ) {
				while( arguments.size() ) {
					call.push_back( arguments.front() );
					arguments.erase( arguments.begin() );
				}

				while( expressions->list.size() ) {
					call.push_back( expressions->list.front()->Generate( this, false ) );
					expressions->list.erase( expressions->list.begin() );
				}
			}

			llvm::Value* retval = builder.CreateCall( callee, call );
			if( retval->getType()->isVoidTy() ) {
				return( nullptr );
			}

			if( retval->getType()->isEmptyTy() ) {
				retval = llvm::Constant::getNullValue( llvm::Type::getInt1Ty( module->getContext() ) );
			}

			if( !inMem ) {
				return( retval );
			}

			llvm::Value* memory = builder.CreateAlloca( retval->getType() );
			builder.CreateStore( retval, memory );
			return( memory );
		}

		// TODO: check if the invoker is actually a type / sub type
		llvm::Value* Codegen::invokeMethod( llvm::Value* object, std::string methodName, exo::ast::ExprList* expressions, bool isOptional, bool inMem )
		{
			if( expressions == nullptr ) {
				EXO_THROW( InvalidCall() << exo::exceptions::Message( "Invalid arguments" ) );
			}

			llvm::Type* type = object->getType();
			if( !type->isPointerTy() ) {
				EXO_DEBUG_LOG( trace, toString( type ) );
				EXO_THROW_AT( InvalidCall() << exo::exceptions::Message( "Expecting object" ), expressions );
			}

			type = type->getPointerElementType();
			if( !type->isStructTy() ) {
				EXO_DEBUG_LOG( trace, toString( type ) );
				EXO_THROW_AT( InvalidCall() << exo::exceptions::Message( "Invalid object" ), expressions );
			}

			std::string className = type->getStructName();

			llvm::Function* callee;
			try {
				callee = methods.at( className ).at( methodName ).second;
			} catch( ... ) {
				if( !isOptional ) {
					throw;
				} else {
					return( nullptr );
				}
			}

			return( invokeFunction( callee, callee->getFunctionType(), { object }, expressions, inMem ) );
			/*
			int position;
			try {
				position = methods.at( className ).at( methodName ).first;
			} catch( ... ) {
				if( !isOptional ) {
					throw;
				} else {
					return( nullptr );
				}
			}

			EXO_DEBUG_LOG( trace, "Call method " << className << "->" << methodName << "@" << position );

			//load function pointer from vtbl
			llvm::Value* vtbl = builder.CreateGEP( object, { llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ), llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ) } );
			llvm::LoadInst* callee = builder.CreateLoad( builder.CreateGEP( vtbl, { llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ), llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), position ) }, methodName ) );

			type = callee->getOperand(0)->getType();
			if( type->isPointerTy() ) {
				type = type->getPointerElementType();

				if( type->isPointerTy() ) {
					type = type->getPointerElementType();

					llvm::FunctionType* function = llvm::dyn_cast<llvm::FunctionType>( type );
					if( function != nullptr ) {
						return( invokeFunction( callee, function, { object }, expressions, inMem ) );
					}
				}
			}

			EXO_THROW_AT( InvalidCall() << exo::exceptions::Message( "Internal failure" ), expressions );
			return( nullptr );
			*/
		}

		int Codegen::getPropPos( std::string className, std::string propName )
		{
			int position;

			try {
				position = properties.at( className ).at( propName ).first;
			} catch( ... ) {
#ifdef EXO_DEBUG
				for( auto &classes : properties ) {
					EXO_DEBUG_LOG( trace, classes.first );

					if( classes.first == className ) {
						for( auto &props : classes.second ) {
							EXO_DEBUG_LOG( trace, "- " + props.first );
						}
					}
				}
#endif
				EXO_THROW( UnknownProperty() << exo::exceptions::ClassName( className ) << exo::exceptions::PropertyName( propName ) );
			}

			return( position );
		}

		int Codegen::getMethodPos( std::string className, std::string methodName )
		{
			int position;

			try {
				position = methods.at( className ).at( methodName ).first;
			} catch( ... ) {
#ifdef EXO_DEBUG
				for( auto &classes : methods ) {
					EXO_DEBUG_LOG( trace, classes.first );

					if( classes.first == className ) {
						for( auto &method : classes.second ) {
							EXO_DEBUG_LOG( trace, "- " + method.first );
						}
					}
				}
#endif
				EXO_THROW( InvalidMethod() << exo::exceptions::ClassName( className ) << exo::exceptions::FunctionName( methodName ) );
			}

			return( position );
		}
	}
}
