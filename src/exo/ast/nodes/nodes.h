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

			class ValInt : public Expr, gc
			{
				public:
					long long value;
					ValInt( long long lVal );
			};

			class ValFloat : public Expr, gc
			{
				public:
					double value;
					ValFloat( double dVal );
			};

			class Type : public Node, gc
			{
				public:
					exo::types::typeId typeId;
					std::string typeName;
					Type( exo::types::typeId tId );
					Type( std::string tName );
			};

			class VarDecl : public Stmt, gc
			{
				public:
					VarDecl( std::string varName, Type* varType );
			};

			class VarAssign : public Stmt, gc
			{
				public:
					VarAssign( std::string varName, Expr* e );
			};
		}
	}
}

#endif /* NODE_H_ */
