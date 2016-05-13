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

#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "exo/exo.h"
#include "exo/jit/llvm.h"
#include "exo/jit/stack.h"

namespace exo
{
	// forward declarations
	namespace ast
	{
		class Expr;

		class CallFun;
		class CallMethod;

		class ConstBool;
		class ConstFloat;
		class ConstInt;
		class ConstNull;
		class ConstStr;

		class DecMod;
		class DecClass;
		class DecFun;
		class DecFunProto;
		class DecVar;

		class ExprVar;
		class ExprProp;

		class OpBinary;
		class OpBinaryAssign;
		class OpBinaryAssignShort;

		class OpUnaryDel;
		class OpUnaryNew;

		class StmtBreak;
		class StmtExpr;
		class StmtFor;
		class StmtIf;
		class StmtReturn;
		class StmtList;
		class StmtWhile;

		class Id;
		class Type;

		class Tree;
	}

	namespace jit
	{
		class Block;

		class Codegen
		{
			public:
				std::unique_ptr<Stack>	stack;

				std::map< std::string, std::vector<std::string> >		propertyIndex;
				std::map< std::string, std::vector<llvm::Type*> >		properties;
				std::map< std::string, std::vector<std::string> >		methodIndex;
				std::map< std::string, std::vector<llvm::Function*> >	methods;
				std::map< std::string, std::vector<llvm::Type*> >		vtblSignatures;
				std::map< std::string, std::vector<llvm::Constant*> >	vtblInitializers;

				std::unique_ptr<llvm::Module>	module;
				llvm::IRBuilder<>				builder;

				Codegen( std::unique_ptr<llvm::Module> m );
				~Codegen();

				llvm::Type*		getType( exo::ast::Type* type );
				int				getPropertyPosition( std::string className, std::string propName );
				int				getMethodPosition( std::string className, std::string methodName );
				llvm::Function*	getCallee( std::string className );
				llvm::Value* 	invokeMethod( llvm::Value* object, std::string method, std::vector<exo::ast::Expr*> arguments );


				llvm::Value* Generate( exo::ast::CallFun* call );
				llvm::Value* Generate( exo::ast::CallMethod* call );

				llvm::Value* Generate( exo::ast::ConstBool* val );
				llvm::Value* Generate( exo::ast::ConstInt* val );
				llvm::Value* Generate( exo::ast::ConstFloat* val );
				llvm::Value* Generate( exo::ast::ConstNull* val );
				llvm::Value* Generate( exo::ast::ConstStr* val );

				llvm::Value* Generate( exo::ast::DecMod* dec );
				llvm::Value* Generate( exo::ast::DecClass* dec );
				llvm::Value* Generate( exo::ast::DecFun* dec );
				llvm::Value* Generate( exo::ast::DecFunProto* dec );
				llvm::Value* Generate( exo::ast::DecVar* dec );

				llvm::Value* Generate( exo::ast::ExprVar* expr );
				llvm::Value* Generate( exo::ast::ExprProp* expr );

				llvm::Value* Generate( exo::ast::OpBinary* op );
				llvm::Value* Generate( exo::ast::OpBinaryAssign* op );
				llvm::Value* Generate( exo::ast::OpBinaryAssignShort* op );

				llvm::Value* Generate( exo::ast::OpUnaryDel* op );
				llvm::Value* Generate( exo::ast::OpUnaryNew* op );

				llvm::Value* Generate( exo::ast::StmtBreak* stmt );
				llvm::Value* Generate( exo::ast::StmtExpr* stmt );
				llvm::Value* Generate( exo::ast::StmtFor* stmt );
				llvm::Value* Generate( exo::ast::StmtIf* stmt );
				llvm::Value* Generate( exo::ast::StmtReturn* stmt );
				llvm::Value* Generate( exo::ast::StmtList* stmts );
				llvm::Value* Generate( exo::ast::StmtWhile* stmt );

				llvm::Value* Generate( exo::ast::Tree* tree );

				std::string printValue( llvm::Value* value );
		};
	}
}

#define EXO_CLASS(n) (n)
#define EXO_VTABLE(n) "__vtbl_" + EXO_CLASS(n)
#define EXO_VTABLE_STRUCT(n) "__vtbl_struct_" + EXO_CLASS(n)
#define EXO_METHOD(c,m) "__method_" + EXO_CLASS(c) + "_" + m
#define EXO_IS_CLASS_PTR(a) ( a->isPointerTy() && a->getPointerElementType()->isPointerTy() && a->getPointerElementType()->getPointerElementType()->isStructTy() )
#define EXO_IS_OBJECT(a) EXO_IS_CLASS_PTR( a->getType() )
#define EXO_OBJECT_CLASSNAME(a) a->getType()->getPointerElementType()->getStructName()

#ifndef EXO_GC_DISABLE
# define EXO_ALLOC "GC_malloc"
# define EXO_DEALLOC "GC_free"
#else
# define EXO_ALLOC "malloc"
# define EXO_DEALLOC "free"
#endif

#endif /* CODEGEN_H_ */
