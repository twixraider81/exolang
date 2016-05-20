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

			llvm::Type* ltype = module->getTypeByName( EXO_CLASS( type->id->name ) );
			if( ltype != nullptr ) {
				return( ltype->getPointerTo() );
			}

			EXO_THROW( UnknownClass() << exo::exceptions::ClassName( type->id->name ) );
			return( nullptr );
		}

		int Codegen::getPropertyPosition( std::string className, std::string propName )
		{
			std::vector<std::string>::iterator it = std::find( propertyIndex[ EXO_CLASS( className ) ].begin(), propertyIndex[ EXO_CLASS( className ) ].end(), propName );

			if( it == propertyIndex[ EXO_CLASS( className ) ].end() ) {
				EXO_THROW( UnknownProperty() << exo::exceptions::ClassName( className ) << exo::exceptions::PropertyName( propName ) );
			}

			return( std::distance( propertyIndex[ EXO_CLASS( className ) ].begin(), it ) );
		}

		int Codegen::getMethodPosition( std::string className, std::string methodName )
		{
			std::vector<std::string>::iterator it = std::find( methodIndex[ EXO_CLASS( className ) ].begin(), methodIndex[ EXO_CLASS( className ) ].end(), methodName );

			if( it == methodIndex[ EXO_CLASS( className ) ].end() ) {
				EXO_THROW( InvalidMethod() << exo::exceptions::ClassName( className ) << exo::exceptions::FunctionName( methodName ) );
			}

			return( std::distance( methodIndex[ EXO_CLASS( className ) ].begin(), it ) );
		}

		llvm::Function* Codegen::getCallee( std::string cName )
		{
			llvm::Function* callee = module->getFunction( cName );

			if( callee == nullptr ) {
				EXO_THROW( UnknownFunction() << exo::exceptions::FunctionName( cName ) );
			}

			return( callee );
		}



		// TODO: check if the invoker is actually a type / sub type
		llvm::Value* Codegen::invokeMethod( llvm::Value* object, std::string method, std::vector<exo::ast::Expr*> arguments )
		{
			std::string className = EXO_OBJECT_CLASSNAME( object );
			int position = getMethodPosition( className, method );
			EXO_LOG( debug, "Call to " << className << ":" << method << "@" << position << " in (" << stack->blockName() << ")" );
			//llvm::Function* callee = methods[className][position];
			//callee->dump();

			llvm::Value* idx[] = {
				llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ),
				llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), position )
			};

			llvm::GlobalValue* vtbl = module->getNamedGlobal( EXO_VTABLE( className ) );
			llvm::LoadInst* callee = builder.CreateLoad( builder.CreateInBoundsGEP( vtbl, idx, method ) );

			std::vector<llvm::Value*> args;
			args.push_back( object );
			for( auto &argument : arguments ) {
				args.push_back( builder.CreateLoad( argument->Generate( this ) ) );
			}

			llvm::Value* retval = builder.CreateCall( callee, args );
			llvm::Value* memory = builder.CreateAlloca( retval->getType() );
			return( builder.CreateStore( retval, memory ) );
		}



		llvm::Value* Codegen::Generate( exo::ast::ConstBool* val )
		{
			EXO_DEBUG_LOG( trace, "Boolean \"" << val->value << "\" in (" << stack->blockName() << ")" );

			llvm::Type* type = llvm::Type::getInt1Ty( module->getContext() );
			llvm::Value* memory = builder.CreateAlloca( type );

			if( val->value ) {
				builder.CreateStore( llvm::ConstantInt::getTrue( type ), memory );
			} else {
				builder.CreateStore( llvm::ConstantInt::getFalse( type ), memory );
			}
			return( memory );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstFloat* val )
		{
			EXO_DEBUG_LOG( trace, "Float \"" << val->value << "\" in (" << stack->blockName() << ")" );
			llvm::Type* type = llvm::Type::getDoubleTy( module->getContext() );
			llvm::Value* memory = builder.CreateAlloca( type );
			builder.CreateStore( llvm::ConstantFP::get( type, val->value ), memory );
			return( memory );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstInt* val )
		{
			EXO_DEBUG_LOG( trace, "Integer \"" << val->value << "\" in (" << stack->blockName() << ")" );
			llvm::Type* type = llvm::Type::getInt64Ty( module->getContext() );
			llvm::Value* memory = builder.CreateAlloca( type );
			builder.CreateStore( llvm::ConstantInt::get( type, val->value ), memory );
			return( memory );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstNull* val )
		{
			EXO_DEBUG_LOG( trace, "Null in (" << stack->blockName() << ")" );
			llvm::Type* type = llvm::Type::getInt1Ty( module->getContext() );
			llvm::Value* memory = builder.CreateAlloca( type );
			builder.CreateStore( llvm::Constant::getNullValue( type ), memory );
			return( memory );
		}

		llvm::Value* Codegen::Generate( exo::ast::ConstStr* val )
		{
			EXO_DEBUG_LOG( trace, "String \"" << val->value << "\" in (" << stack->blockName() << ")" );
			llvm::Type* type = llvm::Type::getInt8PtrTy( module->getContext() );
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
		 * FIXME: get rid of globalvariable
		 */
		llvm::Value* Codegen::Generate( exo::ast::DeclClass* decl )
		{
			EXO_LOG( trace, "Declaring class \"" << decl->id->name << "\" in (" << stack->blockName() << ")" );

			// if we have a parent, do inherit properties and methods
			if( decl->parent ) {
				std::string pName = EXO_CLASS( decl->parent->name );
				llvm::StructType* parent;

				if( !( parent = module->getTypeByName( pName ) ) ) {
					EXO_THROW( UnknownClass() << exo::exceptions::ClassName( pName ) );
				}

				properties[ EXO_CLASS( decl->id->name ) ]		= properties[ EXO_CLASS( pName ) ];
				propertyIndex[ EXO_CLASS( decl->id->name ) ]	= propertyIndex[ EXO_CLASS( pName ) ];
				methodIndex[ EXO_CLASS( decl->id->name ) ]		= methodIndex[ EXO_CLASS( pName ) ];
				methods[ EXO_CLASS( decl->id->name ) ]			= methods[ EXO_CLASS( pName ) ];
				vtblSignatures[ EXO_CLASS( decl->id->name ) ]	= vtblSignatures[ EXO_CLASS( pName ) ];
				vtblInitializers[ EXO_CLASS( decl->id->name ) ]	= vtblInitializers[ EXO_CLASS( pName ) ];
			}


			// generate our properties
			for( auto &property : decl->properties ) {
				EXO_LOG( trace, "Property " << property->property->type->id->name << " $" << property->property->name );
				propertyIndex[ EXO_CLASS( decl->id->name ) ].push_back( property->property->name );
				properties[ EXO_CLASS( decl->id->name ) ].push_back( getType( property->property->type.get() ) );
			}
			llvm::StructType* classStruct = llvm::StructType::create( module->getContext(), properties[ EXO_CLASS( decl->id->name ) ], EXO_CLASS( decl->id->name ) );


			// generate our methods & vtbl
			for( auto &method : decl->methods ) {
				std::string oName = method->id->name;
				method->id->name = EXO_METHOD( decl->id->name, method->id->name );

				// check if we are redeclaring
				// TODO: this MUST check method signatures
				bool reDeclare = false;
				std::vector<std::string>::iterator it = std::find( methodIndex[ EXO_CLASS( decl->id->name ) ].begin(), methodIndex[ EXO_CLASS( decl->id->name ) ].end(), oName );
				if( it != methodIndex[ EXO_CLASS( decl->id->name ) ].end() ) {
					reDeclare = true;
				}

				// a pointer to a class structure as first parameter for methods
				std::unique_ptr<exo::ast::Id> id = std::make_unique<exo::ast::Id>( decl->id->name );
				method->arguments->list.insert( method->arguments->list.begin(), std::make_unique<exo::ast::DeclVar>( "this", std::make_unique<exo::ast::Type>( std::move( id ) ) ) );
				method->Generate( this );

				// mark position & signature in our vtbl
				llvm::Function* cMethod = module->getFunction( method->id->name );
				if( reDeclare ) {
					int mIndex = getMethodPosition( decl->id->name, oName );
					methods[ EXO_CLASS( decl->id->name ) ][mIndex]			= cMethod;
					vtblSignatures[ EXO_CLASS( decl->id->name ) ][mIndex]	=  cMethod->getFunctionType()->getPointerTo();
					vtblInitializers[ EXO_CLASS( decl->id->name ) ][mIndex]	= cMethod;
				} else {
					methodIndex[ EXO_CLASS( decl->id->name ) ].push_back( oName );
					methods[ EXO_CLASS( decl->id->name ) ].push_back( cMethod );
					vtblSignatures[ EXO_CLASS( decl->id->name ) ].push_back( cMethod->getFunctionType()->getPointerTo() );
					vtblInitializers[ EXO_CLASS( decl->id->name ) ].push_back( cMethod );
				}
			}

			llvm::StructType* vtbl = llvm::StructType::create( module->getContext(), vtblSignatures[ EXO_CLASS( decl->id->name ) ], EXO_VTABLE_STRUCT( decl->id->name ) );
			llvm::GlobalVariable* value = new llvm::GlobalVariable( *module, vtbl, false, llvm::GlobalValue::ExternalLinkage, 0, EXO_VTABLE( decl->id->name ) );
			value->setInitializer( llvm::ConstantStruct::get( vtbl, vtblInitializers[ EXO_CLASS( decl->id->name ) ] ) );
			return( value );
		}

		llvm::Value* Codegen::Generate( exo::ast::DeclFunProto* decl )
		{
			EXO_DEBUG_LOG( trace, "Declaring function prototype \"" << decl->id->name << "\" in (" << stack->blockName() << ")" );

			std::vector<llvm::Type*> fArgs;
			for( auto &argument : decl->arguments->list ) {
				llvm::Type* arg = getType( argument->type.get() );
				EXO_DEBUG_LOG( trace, "Argument $" << argument->name );
				fArgs.push_back( arg );
			}

			llvm::Function* fun = llvm::Function::Create(
				llvm::FunctionType::get( getType( decl->returnType.get() ), fArgs, decl->hasVaArg ),
				llvm::GlobalValue::ExternalLinkage,
				decl->id->name,
				module.get()
			);

			return( fun );
		}

		llvm::Value* Codegen::Generate( exo::ast::DeclFun* decl )
		{
			std::string argumentList;

			// build up our argument list
			std::vector<llvm::Type*> arguments;
			for( auto &argument : decl->arguments->list ) {
				arguments.push_back( getType( argument->type.get() ) );
#ifdef EXO_DEBUG
				argumentList.append(argument->type->id->name);
				argumentList.append(" $");
				argumentList.append(argument->name);
				argumentList.append(", ");
#endif
			}
			EXO_DEBUG_LOG( trace, "Declaring function " << decl->id->name << "(" << argumentList << ") in (" << stack->blockName() << ")" );

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
				llvm::Value* memory = builder.CreateAlloca( argument.getType() );
				builder.CreateStore( &argument, memory );
				stack->setSymbol( decl->arguments->list.at( i )->name, memory );
				i++;
			}

			// generate our actual function statements
			llvm::Value* retVal = decl->stmts->Generate( this );

			// sanitize function exit
			if( retVal == nullptr || !llvm::isa<llvm::ReturnInst>(retVal) ) {
				if( getType( decl->returnType.get() )->isVoidTy() ) {
					EXO_DEBUG_LOG( trace, "Generating void return in (" << stack->blockName() << ")" );
					builder.CreateRetVoid();
				} else {
					EXO_DEBUG_LOG( trace, "Generating null return in (" << stack->blockName() << ")" );
					builder.CreateRet( llvm::Constant::getNullValue( getType( decl->returnType.get() ) ) );
				}
			}

			// pop our block, void local variables
			builder.SetInsertPoint( stack->Pop() );

			return( function );
		}

		llvm::Value* Codegen::Generate( exo::ast::DeclMod* decl )
		{
			/* dont tinker here
			module->setModuleIdentifier( decl->id->inNamespace +  decl->id->name );
			*/
			return( nullptr );
		}

		// FIXME: check instantiation
		llvm::Value* Codegen::Generate( exo::ast::DeclVar* decl )
		{
			EXO_LOG( trace, "Allocating " << decl->type->id->name << " $" << decl->name << " on stack in (" << stack->blockName() << ")" );
			stack->setSymbol( decl->name, builder.CreateAlloca( getType( decl->type.get() ) ) );

			if( decl->expression ) {
				std::unique_ptr<exo::ast::OpBinaryAssign> a = std::make_unique<exo::ast::OpBinaryAssign>( std::make_unique<exo::ast::ExprVar>( decl->name ), std::move( decl->expression ) );
				a->Generate( this );
			}

			llvm::Value* variable;

			try {
				variable = stack->getSymbol( decl->name );
			} catch( boost::exception &exception ) {
				exception << boost::errinfo_at_line( decl->lineNo ) << exo::exceptions::ColumnNo( decl->columnNo );
				throw;
			}

			return( variable );
		}

		llvm::Value* Codegen::Generate( exo::ast::DeclVarList* decl )
		{
			for( auto &argument : decl->list ) {
				argument->Generate( this );
			}

			return( nullptr );
		}

		llvm::Value* Codegen::Generate( exo::ast::ExprCallFun* call )
		{
			EXO_LOG( debug, "Call to \"" << call->id->name << "\" in (" << stack->blockName() << ")" );
			llvm::Function* callee = getCallee( call->id->name );

			if( callee->arg_size() != call->arguments->list.size() && !callee->isVarArg() ) {
				EXO_DEBUG_LOG( trace, printValue( callee ) );
				EXO_THROW( InvalidCall() << exo::exceptions::FunctionName( call->id->name ) );
			}

			std::vector<llvm::Value*> arguments;
			for( auto &argument : call->arguments->list ) {
				arguments.push_back( builder.CreateLoad( argument->Generate( this ) ) );
			}

			llvm::Value* retval = builder.CreateCall( callee, arguments, call->id->name );
			llvm::Value* memory = builder.CreateAlloca( retval->getType() );
			builder.CreateStore( retval, memory );
			return( memory );
		}

		// TODO: check if the invoker is actually a type / sub type
		llvm::Value* Codegen::Generate( exo::ast::ExprCallMethod* call )
		{
			llvm::Value* variable = call->expression->Generate( this );

			if( !EXO_IS_OBJECT( variable ) ) {
				EXO_THROW( InvalidOp() );
			}

			//return( invokeMethod( builder.CreateLoad( variable ), call->name, call->arguments->list ) );

			variable = builder.CreateLoad( variable );
			std::string cName = variable->getType()->getPointerElementType()->getStructName();

			int position = getMethodPosition( cName, call->id->name );
			llvm::Value* idx[] = {
				llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ),
				llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), position )
			};

			EXO_LOG( debug, "Call to \"" << cName << "->" << call->id->name << "\"@" << position << " in (" << stack->blockName() << ")" );
			llvm::Value* vtbl = module->getNamedGlobal( EXO_VTABLE( cName ) );
			llvm::Value* callee = builder.CreateLoad( builder.CreateInBoundsGEP( vtbl, idx, call->id->name ) );
			llvm::Function* method = methods[cName][position];

			if( method->arg_size() != ( call->arguments->list.size() + 1 ) && !method->isVarArg() ) {
				EXO_THROW( InvalidCall() << exo::exceptions::FunctionName( call->id->name ) );
			}

			std::vector<llvm::Value*> arguments;

			llvm::Function::arg_iterator it = method->arg_begin();
			if( EXO_OBJECT_CLASSNAME( it ) != EXO_OBJECT_CLASSNAME( variable ) ) {
				EXO_LOG( debug, "Cast " << cName << " to " << std::string( EXO_OBJECT_CLASSNAME( it ) ) << " in (" << stack->blockName() << ")" );
				arguments.push_back( builder.CreateBitCast( variable, it->getType() ) );
			} else {
				arguments.push_back( variable );
			}

			for( auto &argument : call->arguments->list ) {
				arguments.push_back( builder.CreateLoad( argument->Generate( this ) ) );
			}

			llvm::Value* retval = builder.CreateCall( callee, arguments );
			llvm::Value* memory = builder.CreateAlloca( retval->getType() );
			builder.CreateStore( retval, memory );
			return( memory );
		}

		// FIXME: track if instance property is actually initialized
		llvm::Value* Codegen::Generate( exo::ast::ExprProp* expr )
		{
			llvm::Value* variable = expr->expression->Generate( this );

			if( !EXO_IS_OBJECT( variable ) ) {
				EXO_THROW( InvalidOp() );
			}

			variable = builder.CreateLoad( variable );
			int position = getPropertyPosition( EXO_OBJECT_CLASSNAME( variable ), expr->name );
			llvm::Value* idx[] = {
					llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ),
					llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), position )
			};

			EXO_LOG( trace, "Load property \"" << std::string( EXO_OBJECT_CLASSNAME( variable ) ) << "->" << expr->name << "\"@" << position << " in (" << stack->blockName() << ")" );
			return( builder.CreateInBoundsGEP( variable, idx, expr->name ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::ExprVar* expr )
		{
			llvm::Value* variable;

			try {
				variable = stack->getSymbol( expr->name );
			} catch( boost::exception &exception ) {
				exception << boost::errinfo_at_line( expr->lineNo ) << exo::exceptions::ColumnNo( expr->columnNo );
				throw;
			}

			return( variable );
		}


		llvm::Value* Codegen::Generate( exo::ast::OpBinary* op )
		{
			llvm::Value* lhs = builder.CreateLoad( op->lhs->Generate( this ) );
			llvm::Value* rhs = builder.CreateLoad( op->rhs->Generate( this ) );
			llvm::Value* result;

			if( typeid(*op) == typeid( exo::ast::OpBinaryAdd ) ) {
				EXO_DEBUG_LOG( trace, "Addition in (" << stack->blockName() << ")" );
				result = builder.CreateAdd( lhs, rhs, "add" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinarySub ) ) {
				EXO_DEBUG_LOG( trace, "Substraction in (" << stack->blockName() << ")" );
				result = builder.CreateSub( lhs, rhs, "sub" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryMul ) ) {
				EXO_DEBUG_LOG( trace, "Multiplication in (" << stack->blockName() << ")" );
				result = builder.CreateMul( lhs, rhs, "mul" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryDiv ) ) {
				EXO_DEBUG_LOG( trace, "Division in (" << stack->blockName() << ")" );
				result = builder.CreateSDiv( lhs, rhs, "div" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryLt ) ) {
				EXO_DEBUG_LOG( trace, "Lower than comparison in (" << stack->blockName() << ")" );
				result = builder.CreateICmpSLT( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryLe ) ) {
				EXO_DEBUG_LOG( trace, "Lower equal comparison in (" << stack->blockName() << ")" );
				result = builder.CreateICmpSLE( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryGe ) ) {
				EXO_DEBUG_LOG( trace, "Greater equal comparison in (" << stack->blockName() << ")" );
				result = builder.CreateICmpSGE( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryGt ) ) {
				EXO_DEBUG_LOG( trace, "Greater than comparison in (" << stack->blockName() << ")" );
				result = builder.CreateICmpSGT( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryEq ) ) {
				EXO_DEBUG_LOG( trace, "Is equal comparison in (" << stack->blockName() << ")" );
				result = builder.CreateICmpEQ( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryEq ) ) {
				EXO_DEBUG_LOG( trace, "Not equal comparison in (" << stack->blockName() << ")" );
				result = builder.CreateICmpNE( lhs, rhs, "cmp" );
			} else {
				EXO_THROW( InvalidOp() );
				return( nullptr );
			}

			llvm::Value* memory = builder.CreateAlloca( result->getType() );
			builder.CreateStore( result, memory );
			return( memory );
		}

		llvm::Value* Codegen::Generate( exo::ast::OpBinaryAssign* assign )
		{
			llvm::Value* variable = assign->lhs->Generate( this );
			llvm::Value* value = builder.CreateLoad( assign->rhs->Generate( this ) );

			EXO_DEBUG_LOG( trace, "Assignment in (" << stack->blockName() << ")" );
			return( builder.CreateStore( value, variable ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::OpBinaryAssignShort* assign )
		{
			llvm::Value* variable = assign->lhs->Generate( this );
			llvm::Value* value = builder.CreateLoad( assign->rhs->Generate( this ) );
			llvm::Value* result = nullptr;

			if( typeid(*assign) == typeid( exo::ast::OpBinaryAssignAdd ) ) {
				EXO_DEBUG_LOG( trace, "Assign addition in (" << stack->blockName() << ")" );
				result = builder.CreateAdd( builder.CreateLoad( variable ), value, "add" );
			} else if( typeid(*assign) == typeid( exo::ast::OpBinaryAssignSub ) ) {
				EXO_DEBUG_LOG( trace, "Assign substraction in (" << stack->blockName() << ")" );
				result = builder.CreateSub( builder.CreateLoad( variable ), value, "sub" );
			} else if( typeid(*assign) == typeid( exo::ast::OpBinaryAssignMul ) ) {
				EXO_DEBUG_LOG( trace, "Assign multiplication in (" << stack->blockName() << ")" );
				result = builder.CreateMul( builder.CreateLoad( variable ), value, "mul" );
			} else if( typeid(*assign) == typeid( exo::ast::OpBinaryAssignDiv ) ) {
				EXO_DEBUG_LOG( trace, "Assign division in (" << stack->blockName() << ")" );
				result = builder.CreateSDiv( builder.CreateLoad( variable ), value, "div" );
			} else {
				EXO_THROW( InvalidOp() );
			}

			return( builder.CreateStore( result, variable ) );
		}


		/*
		 * TODO: call destructor
		 * FIXME: cast
		 */
		llvm::Value* Codegen::Generate( exo::ast::OpUnaryDel* op )
		{
			exo::ast::ExprVar* var = dynamic_cast<exo::ast::ExprVar*>( op->rhs.get() );
			llvm::Value* variable = op->rhs->Generate( this );

			if( EXO_IS_OBJECT( variable ) ) {
				EXO_LOG( trace, "Deleting $" << var->name << " from heap in (" << stack->blockName() << ")" );
				llvm::Function* gcfree = getCallee( EXO_DEALLOC );
				std::vector<llvm::Value*> arguments;
				arguments.push_back( builder.CreateBitCast( builder.CreateLoad( variable ), module->getDataLayout().getIntPtrType( module->getContext() )->getPointerTo() ) );
				builder.CreateCall( gcfree, arguments );
			}

			try {
				stack->delSymbol( var->name );
			} catch( boost::exception &exception ) {
				exception << boost::errinfo_at_line( op->lineNo ) << exo::exceptions::ColumnNo( op->columnNo );
				throw;
			}

			llvm::Value* retval = llvm::ConstantInt::getTrue( llvm::Type::getInt1Ty( module->getContext() ) );
			llvm::Value* memory = builder.CreateAlloca( retval->getType() );
			builder.CreateStore( retval, memory );
			return( memory );
		}

		/*
		 * TODO: call constructor
		 * FIXME: cast
		 */
		llvm::Value* Codegen::Generate( exo::ast::OpUnaryNew* op )
		{
			exo::ast::ExprCallFun* init = dynamic_cast<exo::ast::ExprCallFun*>( op->rhs.get() );

			if( !init ) {
				EXO_THROW( InvalidExpr() );
			}

			llvm::Type* type;
			if( !(type = module->getTypeByName( EXO_CLASS( init->id->name ) ) ) ) {
				EXO_THROW( UnknownClass() << exo::exceptions::ClassName( init->id->name ) );
			}

			EXO_LOG( trace, "Allocating memory for \"" << init->id->name << "\" on heap in (" << stack->blockName() << ")" );
			llvm::Function* gcmalloc = getCallee( EXO_ALLOC );

			std::vector<llvm::Value*> arguments;
			arguments.push_back( llvm::ConstantExpr::getSizeOf( type ) );
			llvm::Value* value = builder.CreateBitCast( builder.CreateCall( gcmalloc, arguments ), type->getPointerTo() );
			llvm::Value* memory = builder.CreateAlloca( type->getPointerTo() );
			builder.CreateStore( value, memory );

			// call constructor TODO: merge with CallMethod
			std::vector<std::string>::iterator it = std::find( methodIndex[ EXO_CLASS( init->id->name ) ].begin(), methodIndex[ EXO_CLASS( init->id->name ) ].end(), "__construct" );
			if( it != methodIndex[ EXO_CLASS( init->id->name ) ].end() ) {
				int position = getMethodPosition( init->id->name, "__construct" );
				llvm::Value* idx[] = {
					llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ),
					llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), position )
				};

				EXO_LOG( debug, "Call to \"" << init->id->name << "->__construct()\"@" << position << " in (" << stack->blockName() << ")" );
				llvm::Value* vtbl = module->getNamedGlobal( EXO_VTABLE( init->id->name ) );
				llvm::Value* callee = builder.CreateLoad( builder.CreateInBoundsGEP( vtbl, idx, init->id->name ) );
				llvm::Function* method = methods[init->id->name][position];

				if( method->arg_size() != ( init->arguments->list.size() + 1 ) && !method->isVarArg() ) {
					EXO_THROW( InvalidCall() << exo::exceptions::FunctionName( init->id->name ) );
				}

				std::vector<llvm::Value*> arguments;
				llvm::Function::arg_iterator ait = method->arg_begin();
				if( EXO_OBJECT_CLASSNAME( ait ) != init->id->name ) {
					EXO_LOG( debug, "Cast " << init->id->name << " to " << std::string( EXO_OBJECT_CLASSNAME( ait ) ) << " in (" << stack->blockName() << ")" );
					arguments.push_back( builder.CreateBitCast( value, ait->getType() ) );
				} else {
					arguments.push_back( value );
				}

				for( auto &argument : init->arguments->list ) {
					arguments.push_back( builder.CreateLoad( argument->Generate( this ) ) );
				}

				builder.CreateCall( callee, arguments );
			}

			return( memory );
		}


		llvm::Value* Codegen::Generate( exo::ast::StmtBlock* stmts )
		{
			EXO_DEBUG_LOG( trace, "Generating " << stmts->list.size() << " statement(s) in (" << stack->blockName() << ")" );

			llvm::Value *last = nullptr;
			for( auto &stmt : stmts->list ) {
				// flow might have been altered (i.e. a break) stop generating in that case
				if( stack->Block()->getTerminator() == nullptr ) {
					last = stmt->Generate( this );
				}
			}

			return( last );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtBreak* stmt )
		{
			llvm::BasicBlock* exitBlock = stack->Exit();
			if( exitBlock == nullptr ) {
				EXO_THROW( InvalidBreak() );
			}

			EXO_DEBUG_LOG( trace, "Break statement in (" << stack->blockName() << ") to (" << exitBlock->getName().str() << ")" );
			return( builder.CreateBr( exitBlock ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtCont* stmt )
		{
			llvm::BasicBlock* exitBlock = stack->Exit();
			if( exitBlock == nullptr ) { // check if we are actually in a loop
				EXO_THROW( InvalidCont() );
			}

			EXO_DEBUG_LOG( trace, "Continue statement in (" << stack->blockName() << ")" );

			// we need to split the stack
			llvm::Function* scope		= stack->Block()->getParent();
			llvm::BasicBlock* contBlock	= llvm::BasicBlock::Create( module->getContext(), "continue", scope );
			llvm::Value* retval = builder.CreateBr( contBlock );
			builder.SetInsertPoint( contBlock );
			return( retval );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtExpr* stmt )
		{
			EXO_DEBUG_LOG( trace, "Expression statement in (" << stack->blockName() << ")" );
			return( stmt->expression->Generate( this ) );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtFor* stmt )
		{
			EXO_DEBUG_LOG( trace, "For statement in (" << stack->blockName() << ")" );

			// setup basic blocks for loop statments and exit
			llvm::Function* scope		= stack->Block()->getParent();
			llvm::BasicBlock* forBlock	= llvm::BasicBlock::Create( module->getContext(), "for", scope );
			llvm::BasicBlock* contBlock	= llvm::BasicBlock::Create( module->getContext(), "continue" );

			// initialize loop variables
			for( auto &statement : stmt->initialization->list ) {
				statement->Generate( this );
			}
			builder.CreateBr( forBlock );

			// keep track of our exit/continue block
			builder.SetInsertPoint( stack->Push( forBlock, contBlock ) );

			// generate our for loop statements
			llvm::Value* condition = stmt->expression->Generate( this );
			stmt->block->Generate( this );

			// update loop variables
			for( auto &statement : stmt->update->list ) {
				statement->Generate( this );
			}

			// branch into loop again
			builder.CreateCondBr( builder.CreateLoad( condition ), forBlock, contBlock );

			stack->Pop();

			scope->getBasicBlockList().push_back( contBlock );
			builder.SetInsertPoint( stack->Join( contBlock ) );

			return( condition );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtIf* stmt )
		{
			EXO_DEBUG_LOG( trace, "If statement in (" << stack->blockName() << ")" );

			// setup basic blocks for if, else and exit
			llvm::Function* scope		= stack->Block()->getParent();
			llvm::BasicBlock* ifBlock	= llvm::BasicBlock::Create( module->getContext(), "if", scope );
			llvm::BasicBlock* contBlock	= llvm::BasicBlock::Create( module->getContext(), "continue" );
			llvm::BasicBlock* elseBlock;

			if( stmt->onFalse ) {
				elseBlock = llvm::BasicBlock::Create( module->getContext(), "else" );
			}


			// evaluate our if expression
			llvm::Value* condition = stmt->expression->Generate( this );

			// if we have no else block, we can directly branch to exit/continue block
			if( stmt->onFalse == nullptr ) {
				builder.CreateCondBr( builder.CreateLoad( condition ), ifBlock, contBlock );
			} else {
				builder.CreateCondBr( builder.CreateLoad( condition ), ifBlock, elseBlock );
			}

			// generate our if statements, inherit parent scope variables
			builder.SetInsertPoint( stack->Push( ifBlock ) );
			stmt->onTrue->Generate( this );

			// flow might have been altered by i.e. a break
			if( stack->Block()->getTerminator() == nullptr ) {
				builder.CreateBr( contBlock );
			}

			builder.SetInsertPoint( stack->Pop() );

			// generate our else statements
			if( stmt->onFalse ) {
				EXO_DEBUG_LOG( trace, "Else statement in (" << stack->blockName() << ")" );

				scope->getBasicBlockList().push_back( elseBlock );
				builder.SetInsertPoint( stack->Push( elseBlock ) );
				stmt->onFalse->Generate( this );

				// flow might have been altered by i.e. a break
				if( stack->Block()->getTerminator() == nullptr ) {
					builder.CreateBr( contBlock );
				}

				builder.SetInsertPoint( stack->Pop() );
			}

			scope->getBasicBlockList().push_back( contBlock );
			builder.SetInsertPoint( stack->Join( contBlock ) );

			return( condition );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtImport* stmt )
		{
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

			boost::filesystem::path libFile = boost::filesystem::canonical( fileName ); // this should throw
			/*
			if( !boost::filesystem::exists( fileName ) ) {
				EXO_THROW( NotFound() << exo::exceptions::RessouceName( fileName.string() ) << boost::errinfo_at_line( stmt->lineNo ) << exo::exceptions::ColumnNo( stmt->columnNo ) );
			}
			*/

			std::string libName = libFile.string();
			imports.insert( libName );

			EXO_DEBUG_LOG( trace, "Import \"" << libName << "\" in (" << stack->blockName() << ")" );
			return( nullptr );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtReturn* stmt )
		{
			EXO_DEBUG_LOG( trace, "Return in (" << stack->blockName() << ")" );
			return( builder.CreateRet( builder.CreateLoad( stmt->expression->Generate( this ) ) ) );
		}

		//FIXME: really unsafe (the pathing) and doesnt handle recursiveness
		//TODO: load compiled (.ll) modules
		llvm::Value* Codegen::Generate( exo::ast::StmtUse* decl )
		{
			EXO_DEBUG_LOG( trace, "Use \"" << decl->id->name << "\" in (" << stack->blockName() << ")" );

			std::string fileName;
			boost::system::error_code error;

			if( decl->id->inNamespace.length() ) { // we have a namespace, translate namespaces to folder names
				fileName += decl->id->inNamespace;
				boost::replace_all( fileName, "::", "/" );
			}
			fileName += decl->id->name;
			fileName.append( ".exo" );
			EXO_DEBUG_LOG( trace, "Use \"" << fileName << "\" in (" << stack->blockName() << ")" );
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
			boost::filesystem::path moduleFile = boost::filesystem::canonical( filePath ); // this should throw

			std::unique_ptr<exo::ast::Tree> ast = std::make_unique<exo::ast::Tree>();
			ast->Parse( moduleFile.string(), currentTarget );

			if( ast->stmts ) {
				return( ast->stmts->Generate( this ) );
			}

			//EXO_THROW( UnknownModule() << exo::exceptions::ModuleName( decl->id->name ) << boost::errinfo_at_line( decl->lineNo ) << exo::exceptions::ColumnNo( decl->columnNo ) );
			return( nullptr );
		}

		llvm::Value* Codegen::Generate( exo::ast::StmtWhile* stmt )
		{
			EXO_DEBUG_LOG( trace, "While in (" << stack->blockName() << ")" );

			// setup basic blocks for condition, loop and exit
			llvm::Function* scope				= stack->Block()->getParent();
			llvm::BasicBlock* whileCondition	= llvm::BasicBlock::Create( module->getContext(), "while", scope );
			llvm::BasicBlock* whileLoop			= llvm::BasicBlock::Create( module->getContext(), "loop" );
			llvm::BasicBlock* whileExit			= llvm::BasicBlock::Create( module->getContext(), "continue" );

			// branch into check condition
			builder.CreateBr( whileCondition );
			builder.SetInsertPoint( stack->Push( whileCondition ) );

			// check if we (still) execute our while loop
			llvm::Value* condition = stmt->expression->Generate( this );
			builder.CreateCondBr( builder.CreateLoad( condition ), whileLoop, whileExit );

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
				EXO_DEBUG_LOG( trace, "Generating implicit null return in (" << module->getName().str() << ")" );
				retVal = builder.CreateRet( llvm::ConstantInt::get( intType, 0 ) );
			}

			return( retVal );
		}

		std::string Codegen::printValue( llvm::Value* value )
		{
			std::string buffer;
			llvm::raw_string_ostream bStream( buffer );

			value->print( bStream, true );

			return( bStream.str() );
		}

		llvm::Function* Codegen::registerExternFun( std::string name, llvm::Type* retType, std::vector<llvm::Type*> fArgs )
		{
			return( llvm::Function::Create( llvm::FunctionType::get( retType, fArgs, false ), llvm::GlobalValue::ExternalLinkage, name, module.get() ) );
		}
	}
}
