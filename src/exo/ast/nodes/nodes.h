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

#ifndef NODE_H_
#define NODE_H_

#include <llvm/Analysis/Verifier.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include "exo/ast/context.h"
#include "exo/types/types.h"

namespace exo
{
	namespace ast
	{
		namespace nodes
		{
			class Node : public gc
			{
				public:
					Node() { };
					virtual ~Node() { };

					llvm::Value* Emit( exo::ast::Context& context );
			};

			class Expr : public Node, gc
			{
			};

			class Stmt : public Node, gc
			{
			};

			class StmtList : public Expr, gc
			{
				public:
					std::vector<Stmt*> list;

					StmtList();
			};

			class StmtExpr : public Stmt, gc
			{
				public:
					Expr* expression;

					StmtExpr( Expr* expr );
			};

			class ValAny : public Expr, gc
			{
				public:
					void* value;

				protected:
					ValAny();
			};

			/*
			 * TODO: use GMP
			 */
			class ValInt : public ValAny, gc
			{
				public:
					long long value;

					ValInt( long long lVal );
					ValInt( std::string lVal );
			};

			class ValFloat : public ValAny, gc
			{
				public:
					double value;

					ValFloat( double dVal );
					ValFloat( std::string lVal );
			};

			class Type : public Node, gc
			{
				public:
					exo::types::typeId id;
					std::string name;

					Type( exo::types::typeId tId );
					Type( std::string tName );
					Type( exo::types::typeId tId, std::string tName );
			};

			class VarDecl : public Stmt, gc
			{
				public:
					std::string name;
					Type* type;
					Expr* expression;

					VarDecl( std::string vName, Type* vType, Expr* expr );
					VarDecl( std::string vName, Type* vType );
			};

			class VarAssign : public Expr, gc
			{
				public:
					std::string name;
					Expr* expression;

					VarAssign( std::string vName, Expr* expr );
			};

			class VarDeclList : public Stmt, gc
			{
				public:
					std::vector<VarDecl*> list;

					VarDeclList();
			};

			class FunDecl : public Stmt, gc
			{
				public:
					Type* type;
					Type* returnType;
					VarDeclList* arguments;
					StmtList* codeBlock;

					FunDecl( Type* fType, Type* rType, VarDeclList* vArgs, StmtList* cBlock );
			};

			class CompOp : public Expr, gc
			{
				public:
					Expr* lhs;
					std::string op;
					Expr* rhs;

					CompOp( Expr* a, std::string o, Expr* b );
			};
		}
	}
}

#endif /* NODE_H_ */
