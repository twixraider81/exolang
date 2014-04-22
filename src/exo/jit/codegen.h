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

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include "exo/exo.h"
#include "exo/jit/llvm.h"

namespace exo
{
	namespace ast
	{
		class Node;
	    class Expr;
	    class Stmt;
	    class StmtList;
	    class StmtExpr;
	    class Type;
	    class VarDecl;
	    class VarAssign;
	    class VarDeclList;
	    class ExprList;
	    class FunDecl;
	    class FunCall;
	    class BinaryOp;
	    class CmpOp;
	    class ConstExpr;
	    class ValueNull;
	    class ValueBool;
	    class ValueInt;
	    class ValueFloat;
	    class ValueString;
	    class VarExpr;

		class Tree;
	}

	namespace jit
	{
		class Block;

		class Codegen
		{
			public:
				std::string			name;
				std::stack<Block*>	blocks;

				llvm::Function*		entry;
				llvm::Module*		module;
			    llvm::IRBuilder<>	builder;

			    Codegen( std::string cname );
			    Codegen( std::string cname, std::string target );
			    ~Codegen();

			    void pushBlock( llvm::BasicBlock* block);
			    void popBlock();
			    llvm::BasicBlock* getCurrentBlock();

			    std::map<std::string, llvm::Value*>& Variables();

			    llvm::Value* Generate( exo::ast::Node* node );
			    llvm::Value* Generate( exo::ast::Tree* tree );
			    llvm::Value* Generate( exo::ast::StmtList* list );
			    llvm::Value* Generate( exo::ast::VarDecl* decl );
			    llvm::Value* Generate( exo::ast::VarAssign* assign );
			    llvm::Value* Generate( exo::ast::ValueInt* val );
			    llvm::Value* Generate( exo::ast::ValueFloat* val );
			    llvm::Value* Generate( exo::ast::ValueBool* val );
			    llvm::Value* Generate( exo::ast::ConstExpr* expr );
			    llvm::Value* Generate( exo::ast::BinaryOp* op );
			    llvm::Value* Generate( exo::ast::VarExpr* expr );
			    llvm::Value* Generate( exo::ast::CmpOp* op );
		};
	}
}

#endif /* CONTEXT_H_ */