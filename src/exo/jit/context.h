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
	    class Tree;
	    class Stmt;
	    class Expr;
	    class StmtList;
	    class VarDecl;
	    class VarAssign;
	}

	namespace jit
	{
		class Block;

		class Context : public virtual gc
		{
			public:
				std::string name;
				std::stack<Block*> blocks;

				llvm::Function* entry;
			    llvm::Module* module;
			    llvm::LLVMContext* context;

			    Context( std::string cname, llvm::LLVMContext* c );
			    virtual ~Context();

			    void pushBlock( llvm::BasicBlock* block);
			    void popBlock();
			    llvm::BasicBlock* getCurrentBlock();

			    std::map<std::string, llvm::Value*>& Variables();

			    llvm::Value* Generate( exo::ast::Tree* tree );
			    llvm::Value* Generate( exo::ast::Stmt* stmt );
			    llvm::Value* Generate( exo::ast::Expr* expr );
			    llvm::Value* Generate( exo::ast::StmtList* stmts );
			    llvm::Value* Generate( exo::ast::VarDecl* decl );
			    llvm::Value* Generate( exo::ast::VarAssign* assign );
		};
	}
}

#endif /* CONTEXT_H_ */
