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
		Codegen::Codegen( std::string cname, std::string target ) : builder( llvm::getGlobalContext() ), name( cname ), entry( NULL )
		{
			module = llvm::make_unique<llvm::Module>( cname, llvm::getGlobalContext());
			module->setTargetTriple( target );

			intType = module->getDataLayout().getIntPtrType( module->getContext() );
			ptrType = intType->getPointerTo();
			voidType = llvm::Type::getVoidTy( module->getContext() );
		}

		Codegen::~Codegen()
		{
		}

		void Codegen::popBlock()
		{
			// no empty blocks?
			/*
			if( !blocks.top()->block->size() ) {
				blocks.top()->block->eraseFromParent();
			}
			*/

			if( blocks.top()->exitBlock == NULL ) {
				blocks.pop();

				// reset our insert point
				if( blocks.size() > 0 ) {
					EXO_DEBUG_LOG( trace, "Continuing at block \"" << blocks.top()->block->getName().str() << "\"" );
					builder.SetInsertPoint( blocks.top()->block );
				}
			} else {
				EXO_DEBUG_LOG( trace, "Continuing at exit block \"" << blocks.top()->exitBlock->getName().str() << "\"" );
				builder.SetInsertPoint( blocks.top()->exitBlock );
				blocks.pop();
			}
		}

		void Codegen::pushBlock( llvm::BasicBlock* block, llvm::BasicBlock* eblock )
		{
			// keep track of exit/continue block in loops
			if( eblock == NULL && this->blocks.size() > 0 ) {
				eblock = this->blocks.top()->exitBlock;
			}

			blocks.push( new Block() );
			blocks.top()->block = block;
			blocks.top()->exitBlock = eblock;

			// reset our new insert point
			builder.SetInsertPoint( blocks.top()->block );

			EXO_DEBUG_LOG( trace, "Switching to block \"" << block->getName().str() << "\", continue at exit block \"" << ( eblock == NULL ? "(entry)" : eblock->getName().str() ) << "\"" );
		}

		llvm::BasicBlock* Codegen::getBlock()
		{
			return( blocks.top()->block );
		}

		llvm::BasicBlock* Codegen::getBlockExit()
		{
			if( blocks.size() > 0 ) {
				if( blocks.top()->exitBlock == NULL ) {
					Block* currentBlock = blocks.top();
					llvm::BasicBlock* exitBlock = NULL;
					blocks.pop();

					if( blocks.size() > 0 ) {
						exitBlock = blocks.top()->block;
					}

					blocks.push( currentBlock );
					return( exitBlock );
				} else {
					return( blocks.top()->exitBlock );
				}
			}

			return NULL;
		}

		std::string Codegen::getBlockName()
		{
			 return( blocks.top()->block->getName().str() );
		}

		llvm::Value* Codegen::getBlockSymbol( std::string name )
		{
			std::map<std::string,llvm::Value*>::iterator it = blocks.top()->symbols.find( name );
			if( it == blocks.top()->symbols.end() ) {
				EXO_THROW_EXCEPTION( UnknownVar, "Unknown variable $" + name );
			}

			return( it->second );
		}

		void Codegen::setBlockSymbol( std::string name, llvm::Value* value )
		{
			std::map<std::string,llvm::Value*>::iterator it = blocks.top()->symbols.find( name );
			if( it == blocks.top()->symbols.end() ) {
				value->setName( name );
				blocks.top()->symbols.insert( std::pair<std::string,llvm::Value*>( name, value ) );
			} else {
				blocks.top()->symbols[name] = value;
			}
		}

		void Codegen::delBlockSymbol( std::string name )
		{
			std::map<std::string,llvm::Value*>::iterator it = blocks.top()->symbols.find( name );
			if( it == blocks.top()->symbols.end() ) {
				EXO_THROW_EXCEPTION( UnknownVar, "Unknown variable $" + name );
			}

			blocks.top()->symbols.erase( name );
		}

		std::map<std::string,llvm::Value*>& Codegen::getBlockSymbols()
		{
			return( blocks.top()->symbols );
		}


		/*
		 * TODO: allow raw ctypes with proper names, we want int, bool, etc for ourself
		 */
		llvm::Type* Codegen::getType( exo::ast::Type* type )
		{
			if( type->name == "int" ) {
				return( llvm::Type::getInt64Ty( module->getContext() ) );
			} else if( type->name == "float" ) {
				return( llvm::Type::getDoubleTy( module->getContext() ) );
			} else if( type->name == "bool" ) {
				return( llvm::Type::getInt1Ty( module->getContext() ) );
			} else if( type->name == "string" ) {
				return( llvm::Type::getInt8PtrTy( module->getContext() ) );
			} else if( type->name == "null" ) {
				return( llvm::Type::getVoidTy( module->getContext() ) );
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
			std::vector<std::string>::iterator it = std::find( propertyIndex[ EXO_CLASS( className ) ].begin(), propertyIndex[ EXO_CLASS( className ) ].end(), propName );

			if( it == propertyIndex[ EXO_CLASS( className ) ].end() ) {
				EXO_THROW_EXCEPTION( UnknownVar, "Unknown property \"" + propName  + "\"" );
			}

			return( std::distance( propertyIndex[ EXO_CLASS( className ) ].begin(), it ) );
		}

		int Codegen::getMethodPosition( std::string className, std::string methodName )
		{
			std::vector<std::string>::iterator it = std::find( methodIndex[ EXO_CLASS( className ) ].begin(), methodIndex[ EXO_CLASS( className ) ].end(), methodName );

			if( it == methodIndex[ EXO_CLASS( className ) ].end() ) {
				EXO_THROW_EXCEPTION( InvalidCall, "Unknown method \"" + methodName  + "\"" );
			}

			return( std::distance( methodIndex[ EXO_CLASS( className ) ].begin(), it ) );
		}

		llvm::Function* Codegen::getCallee( std::string cName )
		{
			llvm::Function* callee = module->getFunction( cName );

			if( callee == 0 ) {
				EXO_THROW_EXCEPTION( UnknownFunction, "Unknown function \"" + cName + "\"!" );
			}

			return( callee );
		}



		// TODO: check if the invoker is actually a type / sub type
		llvm::Value* Codegen::invokeMethod( llvm::Value* object, std::string method, std::vector<exo::ast::Expr*> arguments )
		{
			std::string className = EXO_OBJECT_CLASSNAME( object );
			int position = getMethodPosition( className, method );
			EXO_LOG( debug, "Call to " << className << ":" << method << "@" << position << " in (" << getBlockName() << ")" );
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
			for( std::vector<exo::ast::Expr*>::iterator it = arguments.begin(); it != arguments.end(); it++ ) {
				args.push_back( builder.CreateLoad( (*it)->Generate( this ) ) );
			}

			llvm::Value* retval = builder.CreateCall( callee, args );
			llvm::Value* memory = builder.CreateAlloca( retval->getType() );
			return( builder.CreateStore( retval, memory ) );
		}


		llvm::Value* Codegen::Generate( exo::ast::CallFun* call )
		{
			EXO_LOG( debug, "Call to \"" << call->name << "\" in (" << getBlockName() << ")" );
			llvm::Function* callee = getCallee( call->name );

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

			//return( invokeMethod( builder.CreateLoad( variable ), call->name, call->arguments->list ) );

			variable = builder.CreateLoad( variable );
			std::string cName = variable->getType()->getPointerElementType()->getStructName();

			int position = getMethodPosition( cName, call->name );
			llvm::Value* idx[] = {
				llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ),
				llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), position )
			};

			EXO_LOG( debug, "Call to \"" << cName << "->" << call->name << "\"@" << position << " in (" << getBlockName() << ")" );
			llvm::Value* vtbl = module->getNamedGlobal( EXO_VTABLE( cName ) );
			llvm::Value* callee = builder.CreateLoad( builder.CreateInBoundsGEP( vtbl, idx, call->name ) );
			//llvm::Function* method = llvm::dyn_cast<llvm::Function>( callee );
			llvm::Function* method = methods[cName][position];

			if( method->arg_size() != ( call->arguments->list.size() + 1 ) && !method->isVarArg() ) {
				EXO_THROW_EXCEPTION( InvalidCall, "Expected arguments mismatch for \"" + cName + "->" + call->name + "\"!" );
			}

			std::vector<llvm::Value*> arguments;

			llvm::Function::arg_iterator it = method->arg_begin();
			if( EXO_OBJECT_CLASSNAME( it ) != EXO_OBJECT_CLASSNAME( variable ) ) {
				EXO_LOG( trace, "Cast " << cName << " to " << std::string( EXO_OBJECT_CLASSNAME( it ) ) << " in (" << getBlockName() << ")" );
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
			EXO_DEBUG_LOG( trace, "Generating boolean \"" << val->value << "\" in (" << getBlockName() << ")" );

			llvm::Type* type = llvm::Type::getInt1Ty( module->getContext() );
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
			EXO_DEBUG_LOG( trace, "Generating float \"" << val->value << "\" in (" << getBlockName() << ")" );
			llvm::Type* type = llvm::Type::getDoubleTy( module->getContext() );
			llvm::Value* memory = builder.CreateAlloca( type );
			builder.CreateStore( llvm::ConstantFP::get( type, val->value ), memory );
			return( memory );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::ConstInt* val )
		{
			EXO_DEBUG_LOG( trace, "Generating integer \"" << val->value << "\" in (" << getBlockName() << ")" );
			llvm::Type* type = llvm::Type::getInt64Ty( module->getContext() );
			llvm::Value* memory = builder.CreateAlloca( type );
			builder.CreateStore( llvm::ConstantInt::get( type, val->value ), memory );
			return( memory );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::ConstNull* val )
		{
			EXO_DEBUG_LOG( trace, "Generating null in (" << getBlockName() << ")" );
			llvm::Type* type = llvm::Type::getInt1Ty( module->getContext() );
			llvm::Value* memory = builder.CreateAlloca( type );
			builder.CreateStore( llvm::Constant::getNullValue( type ), memory );
			return( memory );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::ConstStr* val )
		{
			EXO_DEBUG_LOG( trace, "Generating string \"" << val->value << "\" in (" << getBlockName() << ")" );
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
		 */
		llvm::Value* Codegen::Generate( exo::ast::DecClass* decl )
		{
			EXO_LOG( debug, "Declaring class \"" << decl->name << "\" in (" << getBlockName() << ")" );

			// if we have a parent, do inherit properties and methods
			if( decl->parent != "" ) {
				std::string pName = EXO_CLASS( decl->parent );
				llvm::StructType* parent;

				if( !( parent = module->getTypeByName( pName ) ) ) {
					EXO_THROW_EXCEPTION( UnknownClass, "Unknown parent class \"" + pName + "\"" );
				}

				properties[ EXO_CLASS( decl->name ) ]			= properties[ EXO_CLASS( pName ) ];
				propertyIndex[ EXO_CLASS( decl->name ) ]		= propertyIndex[ EXO_CLASS( pName ) ];
				methodIndex[ EXO_CLASS( decl->name ) ]		= methodIndex[ EXO_CLASS( pName ) ];
				methods[ EXO_CLASS( decl->name ) ]			= methods[ EXO_CLASS( pName ) ];
				vtblSignatures[ EXO_CLASS( decl->name ) ]		= vtblSignatures[ EXO_CLASS( pName ) ];
				vtblInitializers[ EXO_CLASS( decl->name ) ]	= vtblInitializers[ EXO_CLASS( pName ) ];
			}


			// generate our properties
			for( std::vector<exo::ast::DecProp*>::iterator it = decl->block->properties.begin(); it != decl->block->properties.end(); it++ ) {
				EXO_LOG( trace, "Property " << (*it)->property->type->name << " $" << (*it)->property->name );
				propertyIndex[ EXO_CLASS( decl->name ) ].push_back( (*it)->property->name );
				properties[ EXO_CLASS( decl->name ) ].push_back( getType( (*it)->property->type ) );
			}
			llvm::StructType* classStruct = llvm::StructType::create( module->getContext(), properties[ EXO_CLASS( decl->name ) ], EXO_CLASS( decl->name ) );


			// generate our methods & vtbl
			std::vector<exo::ast::DecMethod*>::iterator mit;
			for( mit = decl->block->methods.begin(); mit != decl->block->methods.end(); mit++ ) {
				std::string oName = (*mit)->method->name;
				(*mit)->method->name = EXO_METHOD( decl->name, (*mit)->method->name );

				// check if we are redeclaring
				// TODO: this MUST check method signatures
				bool reDeclare = false;
				std::vector<std::string>::iterator it = std::find( methodIndex[ EXO_CLASS( decl->name ) ].begin(), methodIndex[ EXO_CLASS( decl->name ) ].end(), oName );
				if( it != methodIndex[ EXO_CLASS( decl->name ) ].end() ) {
					reDeclare = true;
				}

				// a pointer to a class structure as first parameter for methods
				(*mit)->method->arguments->list.insert( (*mit)->method->arguments->list.begin(), new exo::ast::DecVar( "this", new exo::ast::Type( decl->name ) ) );
				(*mit)->method->Generate( this );

				// mark position & signature in our vtbl
				llvm::Function* cMethod = module->getFunction( (*mit)->method->name );
				if( reDeclare ) {
					int mIndex = getMethodPosition( decl->name, oName );
					methods[ EXO_CLASS( decl->name ) ][mIndex] = cMethod;
					vtblSignatures[ EXO_CLASS( decl->name ) ][mIndex] =  cMethod->getFunctionType()->getPointerTo();
					vtblInitializers[ EXO_CLASS( decl->name ) ][mIndex] = cMethod;
				} else {
					methodIndex[ EXO_CLASS( decl->name ) ].push_back( oName );
					methods[ EXO_CLASS( decl->name ) ].push_back( cMethod );
					vtblSignatures[ EXO_CLASS( decl->name ) ].push_back( cMethod->getFunctionType()->getPointerTo() );
					vtblInitializers[ EXO_CLASS( decl->name ) ].push_back( cMethod );
				}
			}

			llvm::StructType* vtbl = llvm::StructType::create( module->getContext(), vtblSignatures[ EXO_CLASS( decl->name ) ], EXO_VTABLE_STRUCT( decl->name ) );
			llvm::GlobalVariable* value = new llvm::GlobalVariable( *module, vtbl, false, llvm::GlobalValue::ExternalLinkage, 0, EXO_VTABLE( decl->name ) );
			value->setInitializer( llvm::ConstantStruct::get( vtbl, vtblInitializers[ EXO_CLASS( decl->name ) ] ) );
			return( value );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::DecFun* decl )
		{
			EXO_DEBUG_LOG( trace, "Generating function \"" << decl->name << "\" in (" << getBlockName() << ")" );

			// build up our argument list
			std::vector<llvm::Type*> arguments;
			for( auto &argument : decl->arguments->list ) {
				arguments.push_back( getType( argument->type ) );
				EXO_DEBUG_LOG( trace, "Argument "<< " $" << argument->type->name << " $" << argument->name );
			}

			// create a function block for our local variables
			llvm::Function* function = llvm::Function::Create(
					llvm::FunctionType::get( getType( decl->returnType ),	arguments, decl->hasVaArg ),
					llvm::GlobalValue::InternalLinkage,
					decl->name,
					module.get()
			);
			llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), decl->name, function, 0 );
			pushBlock( block );

			// create loads for the arguments passed to our function
			int i = 0;
			for( auto &argument : function->args() ) {
				llvm::Value* memory = builder.CreateAlloca( argument.getType() );
				builder.CreateStore( &argument, memory );
				setBlockSymbol( decl->arguments->list.at( i )->name, memory );
				i++;
			}

			// generate our actual function statements
			decl->stmts->Generate( this );

			// sanitize function exit
			llvm::TerminatorInst* retVal = block->getTerminator();
			if( retVal == NULL || !llvm::isa<llvm::ReturnInst>(retVal) ) {
				if( getType( decl->returnType )->isVoidTy() ) {
					EXO_DEBUG_LOG( trace, "Generating void return in (" << getBlockName() << ")" );
					builder.CreateRetVoid();
				} else {
					EXO_DEBUG_LOG( trace, "Generating null return in (" << getBlockName() << ")" );
					builder.CreateRet( llvm::Constant::getNullValue( getType( decl->returnType ) ) );
				}
			}

			// pop our block, void local variables
			popBlock();

			return( function );
		}


		llvm::Value* Codegen::Generate( exo::ast::DecFunProto* decl )
		{
			EXO_DEBUG_LOG( trace, "Declaring function prototype \"" << decl->name << "\" in (" << getBlockName() << ")" );

			std::vector<llvm::Type*> fArgs;
			for( std::vector<exo::ast::DecVar*>::iterator it = decl->arguments->list.begin(); it != decl->arguments->list.end(); it++ ) {
				llvm::Type* arg = getType( (*it)->type );
				EXO_DEBUG_LOG( trace, "Argument $" << (*it)->name );
				fArgs.push_back( arg );
			}

			llvm::Function* fun = llvm::Function::Create(
				llvm::FunctionType::get( getType( decl->returnType ), fArgs, decl->hasVaArg ),
				llvm::GlobalValue::ExternalLinkage,
				decl->name,
				module.get()
			);

			return( fun );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::DecVar* decl )
		{
			EXO_LOG( trace, "Allocating " << decl->type->name << " $" << decl->name << " on stack in (" << getBlockName() << ")" );
			setBlockSymbol( decl->name, builder.CreateAlloca( getType( decl->type ) ) );

			if( decl->expression ) {
				std::unique_ptr<exo::ast::OpBinaryAssign> a( new exo::ast::OpBinaryAssign( new exo::ast::ExprVar( decl->name ), decl->expression ) );
				a->Generate( this );
			}

			return( getBlockSymbol( decl->name ) );
		}



		// ok
		llvm::Value* Codegen::Generate( exo::ast::ExprVar* expr )
		{
			return( getBlockSymbol( expr->name ) );
		}

		// FIXME: track if instance property is actually initialized
		llvm::Value* Codegen::Generate( exo::ast::ExprProp* expr )
		{
			llvm::Value* variable = expr->expression->Generate( this );

			if( !EXO_IS_OBJECT( variable ) ) {
				EXO_THROW_EXCEPTION( InvalidOp, "Can only fetch property of an object!" );
			}

			variable = builder.CreateLoad( variable );
			int position = getPropertyPosition( EXO_OBJECT_CLASSNAME( variable ), expr->name );
			llvm::Value* idx[] = {
					llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ),
					llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), position )
			};

			EXO_LOG( trace, "Load property \"" << std::string( EXO_OBJECT_CLASSNAME( variable ) ) << "->" << expr->name << "\"@" << position << " in (" << getBlockName() << ")" );
			return( builder.CreateInBoundsGEP( variable, idx, expr->name ) );
		}



		llvm::Value* Codegen::Generate( exo::ast::OpBinary* op )
		{
			llvm::Value* lhs = builder.CreateLoad( op->lhs->Generate( this ) );
			llvm::Value* rhs = builder.CreateLoad( op->rhs->Generate( this ) );
			llvm::Value* result;

			if( typeid(*op) == typeid( exo::ast::OpBinaryAdd ) ) {
				EXO_DEBUG_LOG( trace, "Generating addition in (" << getBlockName() << ")" );
				result = builder.CreateAdd( lhs, rhs, "add" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinarySub ) ) {
				EXO_DEBUG_LOG( trace, "Generating substraction in (" << getBlockName() << ")" );
				result = builder.CreateSub( lhs, rhs, "sub" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryMul ) ) {
				EXO_DEBUG_LOG( trace, "Generating multiplication in (" << getBlockName() << ")" );
				result = builder.CreateMul( lhs, rhs, "mul" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryDiv ) ) {
				EXO_DEBUG_LOG( trace, "Generating division in (" << getBlockName() << ")" );
				result = builder.CreateSDiv( lhs, rhs, "div" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryLt ) ) {
				EXO_DEBUG_LOG( trace, "Generating lower than comparison in (" << getBlockName() << ")" );
				result = builder.CreateICmpSLT( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryLe ) ) {
				EXO_DEBUG_LOG( trace, "Generating lower equal comparison in (" << getBlockName() << ")" );
				result = builder.CreateICmpSLE( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryGe ) ) {
				EXO_DEBUG_LOG( trace, "Generating greater equal comparison in (" << getBlockName() << ")" );
				result = builder.CreateICmpSGE( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryGt ) ) {
				EXO_DEBUG_LOG( trace, "Generating greater than comparison in (" << getBlockName() << ")" );
				result = builder.CreateICmpSGT( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryEq ) ) {
				EXO_DEBUG_LOG( trace, "Generating is equal comparison in (" << getBlockName() << ")" );
				result = builder.CreateICmpEQ( lhs, rhs, "cmp" );
			} else if( typeid(*op) == typeid( exo::ast::OpBinaryEq ) ) {
				EXO_DEBUG_LOG( trace, "Generating not equal comparison in (" << getBlockName() << ")" );
				result = builder.CreateICmpNE( lhs, rhs, "cmp" );
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

			EXO_DEBUG_LOG( trace, "Assign value type: " << value->getType()->getTypeID() << ", variable type: " << variable->getType()->getPointerElementType ()->getTypeID() );
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
				EXO_LOG( trace, "Deleting $" << var->name << " from heap in (" << getBlockName() << ")" );
				llvm::Function* gcfree = getCallee( EXO_DEALLOC );
				std::vector<llvm::Value*> arguments;
				arguments.push_back( builder.CreateBitCast( builder.CreateLoad( variable ), ptrType ) );
				builder.CreateCall( gcfree, arguments );
			}

			delBlockSymbol( var->name );

			llvm::Value* retval = llvm::ConstantInt::getTrue( llvm::Type::getInt1Ty( module->getContext() ) );
			llvm::Value* memory = builder.CreateAlloca( retval->getType() );
			builder.CreateStore( retval, memory );
			return( memory );
		}

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

			EXO_LOG( trace, "Allocating memory for \"" << init->name << "\" on heap in (" << getBlockName() << ")" );
			llvm::Function* gcmalloc = getCallee( EXO_ALLOC );

			std::vector<llvm::Value*> arguments;
			arguments.push_back( llvm::ConstantExpr::getSizeOf( type ) );
			llvm::Value* value = builder.CreateBitCast( builder.CreateCall( gcmalloc, arguments ), type->getPointerTo() );
			llvm::Value* memory = builder.CreateAlloca( type->getPointerTo() );
			builder.CreateStore( value, memory );

			// call constructor TODO: merge with CallMethod
			std::vector<std::string>::iterator it = std::find( methodIndex[ EXO_CLASS( init->name ) ].begin(), methodIndex[ EXO_CLASS( init->name ) ].end(), "__construct" );
			if( it != methodIndex[ EXO_CLASS( init->name ) ].end() ) {
				int position = getMethodPosition( init->name, "__construct" );
				llvm::Value* idx[] = {
					llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), 0 ),
					llvm::ConstantInt::get( llvm::Type::getInt32Ty( module->getContext() ), position )
				};

				EXO_LOG( debug, "Call to \"" << init->name << "->__construct()\"@" << position << " in (" << getBlockName() << ")" );
				llvm::Value* vtbl = module->getNamedGlobal( EXO_VTABLE( init->name ) );
				llvm::Value* callee = builder.CreateLoad( builder.CreateInBoundsGEP( vtbl, idx, init->name ) );
				llvm::Function* method = methods[init->name][position];

				if( method->arg_size() != ( init->arguments->list.size() + 1 ) && !method->isVarArg() ) {
					EXO_THROW_EXCEPTION( InvalidCall, "Expected arguments mismatch for \"" + init->name + "->__construct()\"!" );
				}

				std::vector<llvm::Value*> arguments;
				llvm::Function::arg_iterator ait = method->arg_begin();
				if( EXO_OBJECT_CLASSNAME( ait ) != init->name ) {
					EXO_LOG( debug, "Cast " << init->name << " to " << std::string( EXO_OBJECT_CLASSNAME( ait ) ) << " in (" << getBlockName() << ")" );
					arguments.push_back( builder.CreateBitCast( value, ait->getType() ) );
				} else {
					arguments.push_back( value );
				}

				for( std::vector<exo::ast::Expr*>::iterator cit = init->arguments->list.begin(); cit != init->arguments->list.end(); cit++ ) {
					arguments.push_back( builder.CreateLoad( (*cit)->Generate( this ) ) );
				}

				builder.CreateCall( callee, arguments );
			}

			return( memory );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::StmtBreak* stmt )
		{
			llvm::BasicBlock* exitBlock = getBlockExit();
			if( exitBlock == NULL ) {
				EXO_THROW_EXCEPTION( InvalidBreak, "Can not break in (" + getBlockName() + ")" );
			}

			EXO_DEBUG_LOG( trace, "Generating break statement in (" << getBlockName() << ")" );
			return( builder.CreateBr( exitBlock ) );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::StmtFor* stmt )
		{
			EXO_DEBUG_LOG( trace, "Generating for statement in (" << getBlockName() << ")" );

			// setup basic blocks for loop statments and exit
			llvm::Function* scope = getBlock()->getParent();
			llvm::BasicBlock* forBlock = llvm::BasicBlock::Create( module->getContext(), "for", scope );
			llvm::BasicBlock* contBlock	= llvm::BasicBlock::Create( module->getContext(), "continue" );

			// store parent scope variables
			std::map<std::string,llvm::Value*>& blockSymbols = getBlockSymbols();

			// initialize loop variables
			for( auto &statement : stmt->initialization->list ) {
				statement->Generate( this );
			}
			builder.CreateBr( forBlock );

			// keep track of our exit/continue block
			pushBlock( forBlock, contBlock );
			getBlockSymbols() = blockSymbols;

			// generate our for loop statements
			llvm::Value* condition = stmt->expression->Generate( this );
			stmt->block->Generate( this );

			// update loop variables
			for( auto &statement : stmt->update->list ) {
				// flow might have been altered by i.e. a break
				if( getBlock()->getTerminator() == NULL ) {
					statement->Generate( this );
				}
			}

			// flow might have been altered by i.e. a break
			if( getBlock()->getTerminator() == NULL ) {
				// branch into loop again
				builder.CreateCondBr( builder.CreateLoad( condition ), forBlock, contBlock );
			}

			popBlock();

			scope->getBasicBlockList().push_back( contBlock );
			pushBlock( contBlock );
			getBlockSymbols() = blockSymbols;

			return( condition );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::StmtExpr* stmt )
		{
			EXO_DEBUG_LOG( trace, "Generating expression statement in (" << getBlockName() << ")" );

			return( stmt->expression->Generate( this ) );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::StmtIf* stmt )
		{
			EXO_DEBUG_LOG( trace, "Generating if statement in (" << getBlockName() << ")" );

			// setup basic blocks for if, else and exit
			llvm::Function* scope = getBlock()->getParent();
			llvm::BasicBlock* ifBlock = llvm::BasicBlock::Create( module->getContext(), "if", scope );
			llvm::BasicBlock* elseBlock;
			if( stmt->onFalse != NULL ) {
				elseBlock = llvm::BasicBlock::Create( module->getContext(), "else" );
			}
			llvm::BasicBlock* contBlock	= llvm::BasicBlock::Create( module->getContext(), "continue" );

			// evaluate our if expression
			llvm::Value* condition = stmt->expression->Generate( this );

			// store parent scope variables
			std::map<std::string,llvm::Value*>& blockSymbols = getBlockSymbols();

			// if we have no else block, we can directly branch to exit/continue block
			if( stmt->onFalse == NULL ) {
				builder.CreateCondBr( builder.CreateLoad( condition ), ifBlock, contBlock );
			} else {
				builder.CreateCondBr( builder.CreateLoad( condition ), ifBlock, elseBlock );
			}

			// generate our if statements, inherit parent scope variables
			pushBlock( ifBlock );
			getBlockSymbols() = blockSymbols;
			stmt->onTrue->Generate( this );

			// flow might have been altered by i.e. a break
			if( getBlock()->getTerminator() == NULL ) {
				builder.CreateBr( contBlock );
			}

			popBlock();

			// generate our else statements
			if( stmt->onFalse != NULL ) {
				EXO_DEBUG_LOG( trace, "Generating else statement in (" << getBlockName() << ")" );

				scope->getBasicBlockList().push_back( elseBlock );
				pushBlock( elseBlock );
				getBlockSymbols() = blockSymbols;
				stmt->onFalse->Generate( this );

				// flow might have been altered by i.e. a break
				if( getBlock()->getTerminator() == NULL ) {
					builder.CreateBr( contBlock );
				}

				popBlock();
			}

			scope->getBasicBlockList().push_back( contBlock );
			pushBlock( contBlock );
			getBlockSymbols() = blockSymbols;

			return( condition );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::StmtWhile* stmt )
		{
			EXO_DEBUG_LOG( trace, "Generating while statement in (" << getBlockName() << ")" );

			// setup basic blocks for condition check, while loop and exit
			llvm::Function* scope = getBlock()->getParent();
			llvm::BasicBlock* conditionBlock = llvm::BasicBlock::Create( module->getContext(), "while", scope );
			llvm::BasicBlock* whileBlock = llvm::BasicBlock::Create( module->getContext(), "loop" );
			llvm::BasicBlock* contBlock	= llvm::BasicBlock::Create( module->getContext(), "continue" );

			// branch into check condition
			builder.CreateBr( conditionBlock );

			// store & inherit parent scope variables
			std::map<std::string,llvm::Value*>& blockSymbols = getBlockSymbols();
			pushBlock( conditionBlock );
			getBlockSymbols() = blockSymbols;

			// check if we (still) execute our while loop
			llvm::Value* condition = stmt->expression->Generate( this );
			builder.CreateCondBr( builder.CreateLoad( stmt->expression->Generate( this ) ), whileBlock, contBlock );

			// execute main loop
			scope->getBasicBlockList().push_back( whileBlock );
			// keep track of our exit/continue block
			pushBlock( whileBlock, contBlock );
			getBlockSymbols() = blockSymbols;
			stmt->block->Generate( this );

			// flow might have been altered (i.e. break)
			if( getBlock()->getTerminator() == NULL ) {
				// branch into check condition to see if we enter loop another time
				builder.CreateBr( conditionBlock );
			}

			popBlock();

			scope->getBasicBlockList().push_back( contBlock );
			pushBlock( contBlock );
			getBlockSymbols() = blockSymbols;

			return( condition );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::StmtReturn* stmt )
		{
			EXO_DEBUG_LOG( trace, "Generating return statement in (" << getBlockName() << ")" );

			return( builder.CreateRet( builder.CreateLoad( stmt->expression->Generate( this ) ) ) );
		}

		// ok
		llvm::Value* Codegen::Generate( exo::ast::StmtList* stmts )
		{
			EXO_DEBUG_LOG( trace, "Generating " << stmts->list.size() << " statement(s) in (" << getBlockName() << ")" );

			llvm::Value *last = NULL;
			for( auto &stmt : stmts->list ) {
				last = stmt->Generate( this );
			}

			return( last );
		}



		llvm::Value* Codegen::Generate( exo::ast::Tree* tree )
		{
			std::vector<llvm::Type*> fArgs;

			/*
			 * register essential externals
			 */
			fArgs.push_back( intType );
			llvm::Function::Create( llvm::FunctionType::get( ptrType, fArgs, false ), llvm::GlobalValue::ExternalLinkage, EXO_ALLOC, module.get() );

			fArgs.clear();
			fArgs.push_back( ptrType );
			llvm::Function::Create( llvm::FunctionType::get( voidType, fArgs, false ), llvm::GlobalValue::ExternalLinkage, EXO_DEALLOC, module.get() );

			/*
			 * this is main()
			 */
			entry = llvm::Function::Create( llvm::FunctionType::get( intType, false ), llvm::GlobalValue::InternalLinkage, name, module.get() );
			llvm::BasicBlock* block = llvm::BasicBlock::Create( module->getContext(), name, entry, 0 );

			pushBlock( block );

			Generate( tree->stmts );

			llvm::TerminatorInst* retVal = block->getTerminator();
			if( retVal == NULL || !llvm::isa<llvm::ReturnInst>(retVal) ) {
				EXO_DEBUG_LOG( trace, "Generating null return in (" << name << ")" );
				retVal = builder.CreateRet( llvm::ConstantInt::get( intType, 0 ) );
			}

			popBlock();

			return( retVal );
		}
	}
}
