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

		void Codegen::visit( exo::ast::ConstBool& val )
		{
			llvm::Type* type = llvm::Type::getInt1Ty(  module->getContext() );
			currentResult = val.value ? llvm::ConstantInt::getTrue( type ) : llvm::ConstantInt::getFalse( type );

			if( generateInMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( type );
				builder.CreateStore( currentResult, memory );
				currentResult = memory;
			}
		}

		void Codegen::visit( exo::ast::ConstFloat& val )
		{
			llvm::Type* type = llvm::Type::getDoubleTy(  module->getContext() );
			currentResult = llvm::ConstantFP::get( type, val.value );

			if( generateInMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( type );
				builder.CreateStore( currentResult, memory );
				currentResult = memory;
			}
		}

		void Codegen::visit( exo::ast::ConstInt& val )
		{
			llvm::Type* type = llvm::Type::getInt64Ty( module->getContext() );
			currentResult = llvm::ConstantInt::get( type, val.value );

			if( generateInMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( type );
				builder.CreateStore( currentResult, memory );
				currentResult = memory;
			}
		}

		void Codegen::visit( exo::ast::ConstNull& val )
		{
			llvm::Type* type = llvm::Type::getInt1Ty( module->getContext() );
			currentResult = llvm::Constant::getNullValue( type );

			if( generateInMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( type );
				builder.CreateStore( currentResult, memory );
				currentResult = memory;
			}
		}

		void Codegen::visit( exo::ast::ConstStr& val )
		{
			currentResult = builder.CreateGlobalStringPtr( val.value );
		}

		/*
		 * TODO: how do we overwrite methods & properties in a proper way?
		 */
		void Codegen::visit( exo::ast::DeclClass& decl )
		{
			EXO_CODEGEN_LOG( decl, "Declaring class " << decl.id->name );

			std::string name = EXO_CLASS( decl.id->name );

			llvm::StructType* structr = llvm::StructType::create( module->getContext(), name );
			llvm::StructType* vstructr = llvm::StructType::create( module->getContext(), EXO_VTABLE( decl.id->name ) );
			std::vector<llvm::Type*> elems;
			/*
			std::vector<llvm::Type*> vtbl;
			int methodsSize, propSize = 0;
			*/

			// if we have a parent, do inherit properties and methods
			if( decl.parent ) {
				std::string pName = EXO_CLASS( decl.parent->name );
				llvm::StructType* parent = module->getTypeByName( pName );

				if( parent == nullptr ) {
					EXO_THROW_AT( UnknownClass() << exo::exceptions::ClassName( decl.parent->name ), decl );
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
			for( auto &property : decl.properties ) {
				llvm::Type* type = getType( property->property->type.get() );
				llvm::Value* value;

				if( property->property->expression ) {
					generateInMem = false;
					property->property->expression->accept( this );
					value = currentResult;
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
			for( auto &method : decl.methods ) {
				std::string methodName = method->id->name;
				method->id->name = EXO_METHOD( decl.id->name, method->id->name );

				method->arguments->list.insert( method->arguments->list.begin(), std::make_unique<exo::ast::DeclVar>( "this", std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( decl.id->name, decl.id->inNamespace ) ) ) );
				method->accept( this );

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
		}

		void Codegen::visit( exo::ast::DeclFunProto& decl )
		{
			EXO_CODEGEN_LOG( decl, "Declaring function prototype " << decl.id->name );

			// build up the function argument list
			std::vector<llvm::Type*> arguments;
			for( auto &argument : decl.arguments->list ) {
				arguments.push_back( getType( argument->type.get() ) );
			}

			llvm::Function* function = llvm::Function::Create(
				llvm::FunctionType::get( getType( decl.returnType.get() ), arguments, decl.hasVaArg ),
				llvm::GlobalValue::ExternalLinkage,
				decl.id->name,
				module.get()
			);
		}

		void Codegen::visit( exo::ast::DeclFun& decl )
		{
			EXO_CODEGEN_LOG( decl, "Declaring function " << decl.id->name );

			// build up the function argument list
			std::vector<llvm::Type*> arguments;
			for( auto &argument : decl.arguments->list ) {
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
					llvm::FunctionType::get( getType( decl.returnType.get() ),	arguments, decl.hasVaArg ),
					( decl.access->isPublic ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::InternalLinkage ),
					decl.id->name,
					module.get()
			);
			llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), decl.id->name, function );

			// track our scope, exit block
			builder.SetInsertPoint( stack->Push( block, stack->Block() ) );

			// create loads for the arguments passed to our function
			int i = 0;
			for( auto &argument : function->args() ) {
				exo::ast::DeclVar* var = decl.arguments->list.at( i ).get();

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
			generateInMem = false;
			decl.scope->stmts->accept( this );

			// sanitize function exit
			llvm::TerminatorInst* retVal = stack->Block()->getTerminator();
			if( retVal == nullptr || !llvm::isa<llvm::ReturnInst>(retVal) ) {
				if( getType( decl.returnType.get() )->isVoidTy() ) {
					EXO_CODEGEN_LOG( decl, "Generating void return" );
					builder.CreateRetVoid();
				} else {
					EXO_CODEGEN_LOG( decl, "Generating null return" );
					builder.CreateRet( llvm::Constant::getNullValue( getType( decl.returnType.get() ) ) );
				}
			}

			// pop our block, void local variables
			builder.SetInsertPoint( stack->Pop() );
		}

		// this is basically a NOP
		void Codegen::visit( exo::ast::DeclMod& decl )
		{
			std::string moduleId = decl.id->inNamespace + decl.id->name;

			EXO_LOG( trace, "Declaring module " << moduleId );

		}

		// TODO: add readonly variable declarations (for i.e. instance pointers)
		void Codegen::visit( exo::ast::DeclVar& decl )
		{
			EXO_LOG( trace, "Declaring " << decl.type->id->name << ( decl.isRef ? " reference " : " " ) << "$" << decl.name );

			llvm::AllocaInst* memory;
			llvm::Value* value;
			llvm::Type* type = getType( decl.type.get() );

			bool inMem = generateInMem;

			if( decl.isRef ) {
				type = type->getPointerTo(); // only needed for primitives?
			}

			try {
				memory = builder.CreateAlloca( type );

				if( decl.expression ) {
					generateInMem = false;
					decl.expression->accept( this );

					value = currentResult;
					if( decl.isRef && !value->getType()->isPointerTy() ) { // we can only assign variables as reference
						EXO_THROW_AT( InvalidOp() << exo::exceptions::Message( "Can only assign variables by reference" ), decl );
					}
				} else {
					value = llvm::Constant::getNullValue( type );
				}

				builder.CreateStore( value, memory );
				stack->Set( decl.name, memory, decl.isRef );
			} catch( boost::exception &exception ) {
				exception << boost::errinfo_at_line( decl.lineNo ) << exo::exceptions::ColumnNo( decl.columnNo );
				throw;
			}

			if( !inMem ) {
				currentResult = value;
			} else {
				currentResult = memory;
			}
		}

		void Codegen::visit( exo::ast::DeclVarList& decl )
		{
			for( auto &argument : decl.list ) {
				argument->accept( this );
			}
		}

		void Codegen::visit( exo::ast::ExprCallFun& call )
		{
			EXO_LOG( debug, "Call function " << call.id->name );

			llvm::Function* function = getFunction( call.id->name );

			try {
				currentResult = invokeFunction( function, function->getFunctionType(), {}, call.arguments.get(), generateInMem );
			} catch( boost::exception &exception ) {
				exception << exo::exceptions::FunctionName( call.id->name );
				throw;
			}
		}

		// TODO: check if the invoker is actually a type / sub type
		void Codegen::visit( exo::ast::ExprCallMethod& call )
		{
			bool inMem = generateInMem;

			generateInMem = false;
			call.expression->accept( this );
			currentResult = invokeMethod( currentResult, call.id->name, call.arguments.get(), false, inMem );
		}

		// FIXME: track if instance property is actually initialized (needs exception handling)
		void Codegen::visit( exo::ast::ExprProp& expr )
		{
			bool inMem = generateInMem;

			generateInMem = false;
			expr.expression->accept( this );

			llvm::Type* type = currentResult->getType();
			if( !type->isPointerTy() ) {
				//EXO_DEBUG_LOG( trace, toString( currentResult ) );
				EXO_THROW_AT( InvalidOp() << exo::exceptions::Message( "Expecting object" ), expr );
			}

			type = type->getPointerElementType();
			if( !type->isStructTy() ) {
				//EXO_DEBUG_LOG( trace, toString( currentResult ) );
				EXO_THROW_AT( InvalidOp() << exo::exceptions::Message( "Invalid object" ), expr );
			}


			int position;
			try {
				position = getPropPos( type->getStructName(), expr.name );
			} catch( boost::exception &exception ) {
				exception << boost::errinfo_at_line( expr.lineNo ) << exo::exceptions::ColumnNo( expr.columnNo );
				throw;
			}

			EXO_DEBUG_LOG( trace, "Property " << std::string( type->getStructName() ) << "->" << expr.name << "@" << position );
			currentResult = builder.CreateGEP( currentResult, { llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ), llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), position ) }, expr.name );

			if( !inMem ) {
				currentResult = builder.CreateLoad( currentResult );
			}
		}

		void Codegen::visit( exo::ast::ExprVar& expr )
		{
			try {
				currentResult = stack->Get( expr.name );

				if( stack->isRef( expr.name ) && !generateInMem ) { //TODO: this is ambiguous, but ok i guess. if variable is a reference and we want register access deref it
					EXO_CODEGEN_LOG( expr, "Dereferencing $" << expr.name );
					currentResult = builder.CreateLoad( currentResult );
				}

			} catch( boost::exception &exception ) {
				exception << boost::errinfo_at_line( expr.lineNo ) << exo::exceptions::ColumnNo( expr.columnNo );
				throw;
			}

			if( !generateInMem ) {
				currentResult = builder.CreateLoad( currentResult );
			}
		}

		void Codegen::visit( exo::ast::Node& node )
		{
			EXO_THROW( UnexpectedNode() );
		}

		void Codegen::visit( exo::ast::OpBinaryAdd& op )
		{
			EXO_CODEGEN_LOG( op, "Addition" );

			bool inMem = generateInMem;
			generateInMem = false;

			op.lhs->accept( this );
			llvm::Value* lhs = currentResult;

			op.rhs->accept( this );
			llvm::Value* rhs = currentResult;

			currentResult = builder.CreateAdd( lhs, rhs, "add" );

			if( inMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( currentResult->getType() );
				builder.CreateStore( currentResult, memory );
				currentResult = memory;
			}
		}

		void Codegen::visit( exo::ast::OpBinaryAssign& assign )
		{
			EXO_CODEGEN_LOG( assign, "Assignment" );

			if( !assign.lhs || !assign.rhs ) {
				EXO_THROW_AT( InvalidOp(), assign );
			}

			bool inMem = generateInMem;

			generateInMem = true;
			assign.lhs->accept( this );
			llvm::Value* variable = currentResult;

			generateInMem = false;
			assign.rhs->accept( this );
			llvm::Value* value = currentResult;

			if( variable->getType()->getPointerElementType()->isPointerTy() ) { // dealing with references
				if( !value->getType()->isPointerTy() ) { //TODO: this is ambiguous, but ok i guess. dereference our reference in assigments.
					variable = builder.CreateLoad( variable );
				}
			}

			builder.CreateStore( value, variable );

			currentResult = inMem ? variable : value;
		}

		void Codegen::visit( exo::ast::OpBinaryAssignAdd& assign )
		{
			EXO_CODEGEN_LOG( assign, "Addition/Assign operation" );

			if( !assign.lhs || !assign.rhs ) {
				EXO_THROW_AT( InvalidOp(), assign );
			}

			bool inMem = generateInMem;

			generateInMem = true;
			assign.lhs->accept( this );
			llvm::Value* variable = currentResult;

			generateInMem = false;
			assign.rhs->accept( this );

			llvm::Value* result = builder.CreateAdd( builder.CreateLoad( variable ), currentResult, "add" );
			builder.CreateStore( result, variable );

			currentResult = inMem ? variable : result;
		}

		void Codegen::visit( exo::ast::OpBinaryAssignMul& assign )
		{
			EXO_CODEGEN_LOG( assign, "Multiplication/Assign operation" );

			if( !assign.lhs || !assign.rhs ) {
				EXO_THROW_AT( InvalidOp(), assign );
			}

			bool inMem = generateInMem;

			generateInMem = true;
			assign.lhs->accept( this );
			llvm::Value* variable = currentResult;

			generateInMem = false;
			assign.rhs->accept( this );

			llvm::Value* result = builder.CreateMul( builder.CreateLoad( variable ), currentResult, "mul" );
			builder.CreateStore( result, variable );

			currentResult = inMem ? variable : result;
		}

		void Codegen::visit( exo::ast::OpBinaryAssignDiv& assign )
		{
			EXO_CODEGEN_LOG( assign, "Division/Assign operation" );

			if( !assign.lhs || !assign.rhs ) {
				EXO_THROW_AT( InvalidOp(), assign );
			}

			bool inMem = generateInMem;

			generateInMem = true;
			assign.lhs->accept( this );
			llvm::Value* variable = currentResult;

			generateInMem = false;
			assign.rhs->accept( this );

			llvm::Value* result = builder.CreateSDiv( builder.CreateLoad( variable ), currentResult, "mul" );
			builder.CreateStore( result, variable );

			currentResult = inMem ? variable : result;
		}

		void Codegen::visit( exo::ast::OpBinaryAssignSub& assign )
		{
			EXO_CODEGEN_LOG( assign, "Subtraction/Assign operation" );

			if( !assign.lhs || !assign.rhs ) {
				EXO_THROW_AT( InvalidOp(), assign );
			}

			bool inMem = generateInMem;

			generateInMem = true;
			assign.lhs->accept( this );
			llvm::Value* variable = currentResult;

			generateInMem = false;
			assign.rhs->accept( this );

			llvm::Value* result = builder.CreateSub( builder.CreateLoad( variable ), currentResult, "sub" );
			builder.CreateStore( result, variable );

			currentResult = inMem ? variable : result;
		}

		void Codegen::visit( exo::ast::OpBinaryDiv& op )
		{
			EXO_CODEGEN_LOG( op, "Division" );

			bool inMem = generateInMem;
			generateInMem = false;

			op.lhs->accept( this );
			llvm::Value* lhs = currentResult;

			op.rhs->accept( this );
			llvm::Value* rhs = currentResult;

			currentResult = builder.CreateSDiv( lhs, rhs, "div" );

			if( inMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( currentResult->getType() );
				builder.CreateStore( currentResult, memory );
				currentResult = memory;
			}
		}

		void Codegen::visit( exo::ast::OpBinaryEq& op )
		{
			EXO_CODEGEN_LOG( op, "Is equal comparison" );

			bool inMem = generateInMem;
			generateInMem = false;

			op.lhs->accept( this );
			llvm::Value* lhs = currentResult;

			op.rhs->accept( this );
			llvm::Value* rhs = currentResult;

			currentResult = builder.CreateICmpEQ( lhs, rhs, "cmp" );;

			if( inMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( currentResult->getType() );
				builder.CreateStore( currentResult, memory );
				currentResult = memory;
			}
		}

		void Codegen::visit( exo::ast::OpBinaryGe& op )
		{
			EXO_CODEGEN_LOG( op, "Greater equal comparison" );

			bool inMem = generateInMem;
			generateInMem = false;

			op.lhs->accept( this );
			llvm::Value* lhs = currentResult;

			op.rhs->accept( this );
			llvm::Value* rhs = currentResult;

			currentResult = builder.CreateICmpSGE( lhs, rhs, "cmp" );

			if( inMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( currentResult->getType() );
				builder.CreateStore( currentResult, memory );
				currentResult = memory;
			}
		}

		void Codegen::visit( exo::ast::OpBinaryGt& op )
		{
			EXO_CODEGEN_LOG( op, "Greater than comparison" );

			bool inMem = generateInMem;
			generateInMem = false;

			op.lhs->accept( this );
			llvm::Value* lhs = currentResult;

			op.rhs->accept( this );
			llvm::Value* rhs = currentResult;

			currentResult = builder.CreateICmpSGT( lhs, rhs, "cmp" );;

			if( inMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( currentResult->getType() );
				builder.CreateStore( currentResult, memory );
				currentResult = memory;
			}
		}

		void Codegen::visit( exo::ast::OpBinaryLe& op )
		{
			EXO_CODEGEN_LOG( op, "Lower equal comparison" );

			bool inMem = generateInMem;
			generateInMem = false;

			op.lhs->accept( this );
			llvm::Value* lhs = currentResult;

			op.rhs->accept( this );
			llvm::Value* rhs = currentResult;

			currentResult = builder.CreateICmpSLE( lhs, rhs, "cmp" );

			if( inMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( currentResult->getType() );
				builder.CreateStore( currentResult, memory );
				currentResult = memory;
			}
		}

		void Codegen::visit( exo::ast::OpBinaryLt& op )
		{
			EXO_CODEGEN_LOG( op, "Lower than comparison" );

			bool inMem = generateInMem;
			generateInMem = false;

			op.lhs->accept( this );
			llvm::Value* lhs = currentResult;

			op.rhs->accept( this );
			llvm::Value* rhs = currentResult;

			currentResult = builder.CreateICmpSLT( lhs, rhs, "cmp" );

			if( inMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( currentResult->getType() );
				builder.CreateStore( currentResult, memory );
				currentResult = memory;
			}
		}

		void Codegen::visit( exo::ast::OpBinaryMul& op )
		{
			EXO_CODEGEN_LOG( op, "Multiplication" );

			bool inMem = generateInMem;
			generateInMem = false;

			op.lhs->accept( this );
			llvm::Value* lhs = currentResult;

			op.rhs->accept( this );
			llvm::Value* rhs = currentResult;

			currentResult = builder.CreateMul( lhs, rhs, "mul" );

			if( inMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( currentResult->getType() );
				builder.CreateStore( currentResult, memory );
				currentResult = memory;
			}
		}

		void Codegen::visit( exo::ast::OpBinaryNeq& op )
		{
			EXO_CODEGEN_LOG( op, "Not equal comparison" );

			bool inMem = generateInMem;
			generateInMem = false;

			op.lhs->accept( this );
			llvm::Value* lhs = currentResult;

			op.rhs->accept( this );
			llvm::Value* rhs = currentResult;

			currentResult = builder.CreateICmpNE( lhs, rhs, "cmp" );

			if( inMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( currentResult->getType() );
				builder.CreateStore( currentResult, memory );
				currentResult = memory;
			}
		}

		void Codegen::visit( exo::ast::OpBinarySub& op )
		{
			EXO_CODEGEN_LOG( op, "Subtraction" );

			bool inMem = generateInMem;
			generateInMem = false;

			op.lhs->accept( this );
			llvm::Value* lhs = currentResult;

			op.rhs->accept( this );
			llvm::Value* rhs = currentResult;

			currentResult = builder.CreateSub( lhs, rhs, "sub" );

			if( inMem ) {
				llvm::AllocaInst* memory = builder.CreateAlloca( currentResult->getType() );
				builder.CreateStore( currentResult, memory );
				currentResult = memory;
			}
		}

		/*
		 * FIXME: we expect a variable, thats bad since expression can return memory on their own so track memory addresses instead of var names
		 * FIXME: deallocation should occur on/thru the stack
		 */
		void Codegen::visit( exo::ast::OpUnaryDel& op )
		{
			bool inMem = generateInMem;

			generateInMem = true;
			op.rhs->accept( this );
			llvm::Value* value = builder.CreateLoad( currentResult );

			try {
				std::unique_ptr<exo::ast::ExprList> arguments = std::make_unique<exo::ast::ExprList>();
				invokeMethod( value, "__destruct", arguments.get(), true, false );

				EXO_CODEGEN_LOG( op, "Deallocating heap memory" );
				builder.CreateCall( getFunction( EXO_DEALLOC ), { builder.CreateBitCast( value, module->getDataLayout().getIntPtrType( module->getContext() )->getPointerTo() ) } );
				builder.CreateStore( llvm::Constant::getNullValue( value->getType() ), currentResult );

				// FIXME: stack->Del( op->rhs->name );
			} catch( boost::exception &exception ) {
				exception << boost::errinfo_at_line( op.lineNo ) << exo::exceptions::ColumnNo( op.columnNo );
				throw;
			}
		}

		/*
		 * TODO: we should track heap allocations
		 * TODO: get rid of cast
		 */
		void Codegen::visit( exo::ast::OpUnaryNew& op )
		{
			exo::ast::ExprCallFun* constructor = dynamic_cast<exo::ast::ExprCallFun*>( op.rhs.get() );

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
			currentResult = builder.CreateBitCast( memory, type->getPointerTo() );

			// initialize property defaults
			for( auto &property : properties.at( className ) ) {
				llvm::Value* value = builder.CreateGEP( currentResult, { llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ), llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), property.second.first ) }, property.first );
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

			invokeMethod( currentResult, "__construct", constructor->arguments.get(), true, false );
		}

		void Codegen::visit( exo::ast::OpUnaryRef& op )
		{
			EXO_CODEGEN_LOG( op, "Creating reference" );

			bool inMem = generateInMem;
			generateInMem = true;
			op.rhs->accept( this );
			generateInMem = inMem;
		}

		void Codegen::visit( exo::ast::StmtBreak& stmt )
		{
			EXO_CODEGEN_LOG( stmt, "Break" );

			llvm::BasicBlock* exitBlock = stack->Break();
			if( exitBlock == nullptr ) {
				EXO_THROW_AT( InvalidBreak(), stmt );
			}

			builder.CreateBr( exitBlock );
		}

		void Codegen::visit( exo::ast::StmtCont& stmt )
		{
			EXO_CODEGEN_LOG( stmt, "Continue" );

			llvm::BasicBlock* exitBlock = stack->Continue();
			if( exitBlock == nullptr ) { // check if we are actually in a loop
				EXO_THROW_AT( InvalidCont(), stmt );
			}

			builder.CreateBr( exitBlock );
		}


		void Codegen::visit( exo::ast::StmtDo& stmt )
		{
			EXO_CODEGEN_LOG( stmt, "Do" );

			llvm::Function* scope			= stack->Block()->getParent();
			llvm::BasicBlock* doLoop		= llvm::BasicBlock::Create( module->getContext(), "do-loop", scope );
			llvm::BasicBlock* doCondition	= llvm::BasicBlock::Create( module->getContext(), "do-condition", scope );
			llvm::BasicBlock* doExit		= llvm::BasicBlock::Create( module->getContext(), "do-exit", scope );

			doLoop->moveAfter( stack->Block() );
			doCondition->moveAfter( doLoop );
			doExit->moveAfter( doCondition );

			// execute our loop
			builder.CreateBr( doLoop );
			builder.SetInsertPoint( stack->Push( doLoop, doExit, doCondition ) );
			stmt.scope->accept( this );

			// flow might have been altered by i.e. a break
			if( stack->Block()->getTerminator() == nullptr ) {
				// branch into check condition
				builder.CreateBr( doCondition );
			}

			builder.SetInsertPoint( stack->Push( doCondition ) );

			// branch into check condition to see if we enter loop another time
			generateInMem = false;
			stmt.expression->accept( this );
			builder.CreateCondBr( currentResult, doLoop, doExit );

			stack->Pop(); // condition
			stack->Pop(); // loop

			builder.SetInsertPoint( stack->Join( doExit ) );
		}

		void Codegen::visit( exo::ast::StmtExpr& stmt )
		{
			EXO_CODEGEN_LOG( stmt, "Expression statement" );
			stmt.expression->accept( this );
		}

		void Codegen::visit( exo::ast::StmtFor& stmt )
		{
			EXO_CODEGEN_LOG( stmt, "For loop" );

			// setup basic blocks for loop statments and exit
			llvm::Function* scope			= stack->Block()->getParent();
			llvm::BasicBlock* forBlock		= llvm::BasicBlock::Create( module->getContext(), "for-init", scope );
			llvm::BasicBlock* forCondition	= llvm::BasicBlock::Create( module->getContext(), "for-condition", scope );
			llvm::BasicBlock* forLoop		= llvm::BasicBlock::Create( module->getContext(), "for-loop", scope );
			llvm::BasicBlock* forUpdate		= llvm::BasicBlock::Create( module->getContext(), "for-update", scope );
			llvm::BasicBlock* forExit		= llvm::BasicBlock::Create( module->getContext(), "for-exit", scope );

			forBlock->moveAfter( stack->Block() );
			forCondition->moveAfter( forBlock );
			forLoop->moveAfter( forCondition );
			forUpdate->moveAfter( forLoop );
			forExit->moveAfter( forUpdate );

			// initialize loop variables
			builder.CreateBr( forBlock );
			builder.SetInsertPoint( stack->Push( forBlock, forExit, forUpdate ) );
			for( auto &statement : stmt.initialization->list ) {
				statement->accept( this );
			}

			// evaluate loop condition
			builder.CreateBr( forCondition );
			builder.SetInsertPoint( stack->Push( forCondition ) );
			generateInMem = false;
			stmt.expression->accept( this );
			builder.CreateCondBr( currentResult, forLoop, forExit );

			// execute loop
			builder.SetInsertPoint( stack->Push( forLoop ) );
			stmt.scope->accept( this );

			// flow might have been altered by i.e. a break
			if( stack->Block()->getTerminator() == nullptr ) {
				// update loop variables
				builder.CreateBr( forUpdate );
				builder.SetInsertPoint( stack->Push( forUpdate ) );

				for( auto &statement : stmt.update->list ) {
					statement->accept( this );
				}

				// evaluate loop again
				builder.CreateBr( forCondition );
				stack->Pop();
			}

			stack->Pop();
			stack->Pop();
			stack->Pop();

			builder.SetInsertPoint( stack->Join( forExit ) );
		}

		void Codegen::visit( exo::ast::StmtIf& stmt )
		{
			EXO_CODEGEN_LOG( stmt, "If statement" );

			// setup basic blocks for if, else and exit
			llvm::Function* scope		= stack->Block()->getParent();
			llvm::BasicBlock* ifBlock	= llvm::BasicBlock::Create( module->getContext(), "if-true", scope );
			llvm::BasicBlock* contBlock	= llvm::BasicBlock::Create( module->getContext(), "if-continue", scope );
			llvm::BasicBlock* elseBlock;

			ifBlock->moveAfter( stack->Block() );

			if( stmt.onFalse ) {
				elseBlock = llvm::BasicBlock::Create( module->getContext(), "if-false", scope );
				elseBlock->moveAfter( ifBlock );
			}

			contBlock->moveAfter( ifBlock );


			// evaluate our if expression
			generateInMem = false;
			stmt.expression->accept( this );

			// if we have no else block, we can directly branch to exit/continue block
			if( stmt.onFalse == nullptr ) {
				builder.CreateCondBr( currentResult, ifBlock, contBlock );
			} else {
				builder.CreateCondBr( currentResult, ifBlock, elseBlock );
			}


			// generate our if statements, inherit parent scope variables
			builder.SetInsertPoint( stack->Push( ifBlock ) );
			stmt.onTrue->accept( this );

			// flow might have been altered by i.e. a break
			if( stack->Block()->getTerminator() == nullptr ) {
				builder.CreateBr( contBlock );
			}
			builder.SetInsertPoint( stack->Pop() );


			// generate our else statements
			if( stmt.onFalse ) {
				builder.SetInsertPoint( stack->Push( elseBlock ) );
				stmt.onFalse->accept( this );

				// flow might have been altered by i.e. a break
				if( stack->Block()->getTerminator() == nullptr ) {
					builder.CreateBr( contBlock );
				}
				builder.SetInsertPoint( stack->Pop() );
			}

			builder.SetInsertPoint( stack->Join( contBlock ) );
		}

		void Codegen::visit( exo::ast::StmtImport& stmt )
		{
			EXO_CODEGEN_LOG( stmt, "Import" );

			boost::system::error_code error;
			boost::filesystem::path fileName = boost::filesystem::path( stmt.library->value );

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
		}

		/*
		 * TODO: handle the types of our constant expressions/labels
		 */
		void Codegen::visit( exo::ast::StmtLabel& stmt )
		{
			generateInMem = false;
			stmt.expression->accept( this );
			llvm::Type* labelType = currentResult->getType();
			std::string constantValue;

			if( labelType->isIntegerTy() ) { // null, bool or int
				llvm::ConstantInt* value = llvm::dyn_cast<llvm::ConstantInt>( currentResult );
				constantValue = std::to_string( value->getSExtValue() );
			} else if( labelType->isDoubleTy() ) { // float
				llvm::ConstantFP* value = llvm::dyn_cast<llvm::ConstantFP>( currentResult );

				llvm::SmallVector<char, 128> buffer;
				value->getValueAPF().toString( buffer );

				constantValue = std::string( buffer.data(), buffer.size() );
			} else {
				constantValue = stmt.expression->getAsString();
				//EXO_THROW_AT( InvalidLabel() << exo::exceptions::Message( "Unexpected non constant label" ), stmt );
			}

			EXO_CODEGEN_LOG( &stmt, "Creating labeled statement \"" << constantValue << "\"" );

			llvm::Function* scope			= stack->Block()->getParent();
			llvm::BasicBlock* labelBegin	= llvm::BasicBlock::Create( module->getContext(), constantValue + "-begin", scope );
			llvm::BasicBlock* labelExit		= llvm::BasicBlock::Create( module->getContext(), constantValue + "-end", scope );

			labelBegin->moveAfter( stack->Block() );
			labelExit->moveAfter( labelBegin );

			builder.CreateBr( labelBegin );
			builder.SetInsertPoint( stack->Push( labelBegin ) );

			stmt.stmt->accept( this );
			if( stack->Block()->getTerminator() == nullptr ) {
				builder.CreateBr( labelExit );
			}

			stack->Pop();

			builder.SetInsertPoint( stack->Join( labelExit ) );
		}

		void Codegen::visit( exo::ast::StmtList& stmts )
		{
			for( auto& stmt : stmts.list ) {
				// flow might have been altered (i.e. a break) stop generating in that case
				if( stack->Block()->getTerminator() != nullptr ) {
					break;
				}

				stmt->accept( this );
			}
		}

		void Codegen::visit( exo::ast::StmtReturn& stmt )
		{
			EXO_CODEGEN_LOG( stmt, "Return" );

			if( stack->Block()->getTerminator() != nullptr ) {
				EXO_LOG( debug, "Block already terminated" );
			}

			stmt.expression->accept( this );
			builder.CreateRet( currentResult );
		}


		void Codegen::visit( exo::ast::StmtScope& block )
		{
			EXO_CODEGEN_LOG( block, "Creating scope with " << block.stmts->list.size() << " statement(s)" );

			llvm::Function* scope			= stack->Block()->getParent();
			llvm::BasicBlock* scopeBegin	= llvm::BasicBlock::Create( module->getContext(), stack->blockName() + "-scope-begin", scope );
			llvm::BasicBlock* scopeExit		= llvm::BasicBlock::Create( module->getContext(), stack->blockName() + "-scope-end", scope );

			scopeBegin->moveAfter( stack->Block() );
			scopeExit->moveAfter( scopeBegin );

			builder.CreateBr( scopeBegin );
			builder.SetInsertPoint( stack->Push( scopeBegin ) );

			block.stmts->accept( this );
			if( stack->Block()->getTerminator() == nullptr ) {
				builder.CreateBr( scopeExit );
			}

			stack->Pop();
			builder.SetInsertPoint( stack->Join( scopeExit ) );
		}

		void Codegen::visit( exo::ast::StmtSwitch& stmt )
		{
			EXO_CODEGEN_LOG( stmt, "Switch" );

			llvm::Function* scope = stack->Block()->getParent();
			llvm::BasicBlock* switchBegin	= llvm::BasicBlock::Create( module->getContext(), stack->blockName() + "-switch-begin", scope );
			llvm::BasicBlock* switchExit 	= llvm::BasicBlock::Create( module->getContext(), stack->blockName() + "-switch-end", scope );

			switchBegin->moveAfter( stack->Block() );

			builder.CreateBr( switchBegin );
			builder.SetInsertPoint( stack->Push( switchBegin, switchExit ) );

			generateInMem = false;
			stmt.expression->accept( this );
			llvm::SwitchInst* swi;

			if( stmt.defaultCase.get() ) {
				llvm::BasicBlock* switchDefault = llvm::BasicBlock::Create( module->getContext(), stack->blockName() + "-switch-default", scope );
				switchDefault->moveAfter( switchBegin );
				switchExit->moveAfter( switchDefault );

				swi = builder.CreateSwitch( currentResult, switchDefault );
				builder.SetInsertPoint( stack->Push( switchDefault ) );
				stmt.defaultCase->accept( this );

				if( stack->Block()->getTerminator() == nullptr ) {
					builder.CreateBr( switchExit );
				}

				stack->Pop();
			} else {
				switchExit->moveAfter( switchBegin );

				swi = builder.CreateSwitch( currentResult, switchExit );
			}

			for( auto &swCase : stmt.cases ) {
			}

			stack->Pop();
			builder.SetInsertPoint( stack->Join( switchExit ) );
		}

		// FIXME: doesnt handle recursiveness
		// TODO: load compiled (.ll) modules
		void Codegen::visit( exo::ast::StmtUse& decl )
		{
			EXO_CODEGEN_LOG( decl, "Use " << decl.id->name );

			std::string fileName;
			boost::system::error_code error;

			if( decl.id->inNamespace.length() ) { // we have a namespace, translate namespaces to folder names
				fileName += decl.id->inNamespace;
				boost::replace_all( fileName, "::", "/" );
			}
			fileName += decl.id->name;
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
				EXO_THROW_AT( UnknownModule() << exo::exceptions::ModuleName( decl.id->name ), decl );
			}

			EXO_LOG( debug, "Using " << moduleFile.string() );
			std::unique_ptr<exo::ast::Tree> ast = std::make_unique<exo::ast::Tree>( target );
			ast->Parse( moduleFile.string() );

			ast->stmts->accept( this );
		}

		void Codegen::visit( exo::ast::StmtWhile& stmt )
		{
			EXO_CODEGEN_LOG( stmt, "While" );

			llvm::Function* scope				= stack->Block()->getParent();
			llvm::BasicBlock* whileCondition	= llvm::BasicBlock::Create( module->getContext(), "while-condition", scope );
			llvm::BasicBlock* whileLoop			= llvm::BasicBlock::Create( module->getContext(), "while-loop", scope );
			llvm::BasicBlock* whileExit			= llvm::BasicBlock::Create( module->getContext(), "while-exit", scope );

			whileCondition->moveAfter( stack->Block() );
			whileLoop->moveAfter( whileCondition );
			whileExit->moveAfter( whileLoop );

			// branch into check condition
			builder.CreateBr( whileCondition );
			builder.SetInsertPoint( stack->Push( whileCondition ) );

			// check if we (still) execute our while loop
			generateInMem = false;
			stmt.expression->accept( this );
			builder.CreateCondBr( currentResult, whileLoop, whileExit );

			// keep track of our exit/continue block and generate statements
			builder.SetInsertPoint( stack->Push( whileLoop, whileExit, whileCondition ) );
			stmt.scope->accept( this );

			// branch into check condition to see if we enter loop another time
			builder.CreateBr( whileCondition );

			stack->Pop(); // loop
			stack->Pop(); // condition

			builder.SetInsertPoint( stack->Join( whileExit ) );
		}

		void Codegen::visit( exo::ast::Tree& tree )
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
				currentFile = tree.fileName; //TODO: maybe make this a boost::filesystem::path
				boost::filesystem::current_path( boost::filesystem::path( currentFile ).parent_path() );

				target = tree.target;
				if( tree.stmts ) {
					tree.stmts->accept( this );
				}
			} catch( boost::exception &exception ) {
				if( !boost::get_error_info<boost::errinfo_file_name>( exception ) ) {
					exception << boost::errinfo_file_name( tree.fileName );
				}
				throw;
			}

			boost::filesystem::current_path( currentPath );

			block = stack->Pop();
			builder.SetInsertPoint( block );

			llvm::TerminatorInst* retVal = block->getTerminator();
			if( retVal == nullptr || !llvm::isa<llvm::ReturnInst>(retVal) ) {
				EXO_LOG( debug, "Generating implicit null return" );
				builder.CreateRet( llvm::ConstantInt::get( intType, 0 ) );
			}
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
				EXO_THROW_AT( InvalidCall() << exo::exceptions::Message( "Expected parameters mismatch" ), (*expressions) );
			}

			std::vector<llvm::Value*> call;

			generateInMem = false;

			int i = 0;
			for( auto &argument : function->params() ) {
				llvm::Value* value = nullptr;

				if( arguments.size() ) {
					value = arguments.front();
					arguments.erase( arguments.begin() );
				} else if( expressions->list.size() ) {
					expressions->list.front()->accept( this );
					value = currentResult;
					expressions->list.erase( expressions->list.begin() );
				}

				if( value == nullptr || value->getType()->getTypeID() != argument->getTypeID() ) {
					//EXO_DEBUG_LOG( trace, toString( value ) );
					//EXO_DEBUG_LOG( trace, toString( argument ) );
					EXO_THROW_AT( InvalidCall() << exo::exceptions::Message( "Parameter:" + std::to_string( ++i ) + " type mismatch" ), (*expressions) );
				}

				// TODO: check hierarchy
				if( argument->isPointerTy() ) {
					if( argument->getPointerElementType()->isStructTy() ) {
						if( !value->getType()->getPointerElementType()->isStructTy() ) {
							EXO_DEBUG_LOG( trace, toString( value->getType() ) );
							EXO_THROW_AT( InvalidCall() << exo::exceptions::Message( "Expecting object" ), (*expressions) );
						}

						if( argument->getPointerElementType()->getStructName() != value->getType()->getPointerElementType()->getStructName() ) {
							value = builder.CreateBitCast( value, argument );
						}
					}
				}

				call.push_back( value );
			}

			// if we have a vararg function, assume by value
			generateInMem = false;

			if( function->isVarArg() && ( arguments.size() || expressions->list.size() ) ) {
				while( arguments.size() ) {
					call.push_back( arguments.front() );
					arguments.erase( arguments.begin() );
				}

				while( expressions->list.size() ) {
					expressions->list.front()->accept( this );
					call.push_back( currentResult );
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
				EXO_THROW_AT( InvalidCall() << exo::exceptions::Message( "Expecting object" ), (*expressions) );
			}

			type = type->getPointerElementType();
			if( !type->isStructTy() ) {
				EXO_DEBUG_LOG( trace, toString( type ) );
				EXO_THROW_AT( InvalidCall() << exo::exceptions::Message( "Invalid object" ), (*expressions) );
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
