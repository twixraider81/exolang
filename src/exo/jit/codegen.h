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
		class Node;
		class Expr;
		class Stmt;
		class StmtList;
		class StmtExpr;
		class StmtIf;
		class Type;
		class DecVar;
		class AssignVar;
		class DecList;
		class ExprList;
		class DecFunProto;
		class DecFun;
		class CallFun;
		class OpBinary;
		class ConstNull;
		class ConstBool;
		class ConstInt;
		class ConstFloat;
		class ConstStr;
		class ExprVar;
		class StmtReturn;
		class ClassBlock;
		class DecClass;
	}

	namespace jit
	{
		class Block;

		class Codegen
		{
			public:
				std::string			name;
				std::stack<Block*>	blocks;
				std::map< std::string, std::vector<std::string> >	properties;
				std::map< std::string, std::vector<std::string> >	methods;

				llvm::Function*		entry;
				llvm::Module*		module;
			    llvm::IRBuilder<>	builder;

			    Codegen( std::string cname );
			    Codegen( std::string cname, std::string target );
			    ~Codegen();

			    void 									pushBlock( llvm::BasicBlock* block, std::string name );
			    void 									popBlock();
			    llvm::BasicBlock*						getCurrentBasicBlock();
			    std::string								getCurrentBlockName();
			    std::map<std::string, llvm::Value*>&	getCurrentBlockVars();

			    llvm::Type* getType( exo::ast::Type* type, llvm::LLVMContext& context );

			    llvm::Value* Generate( exo::ast::Node* node );
			    llvm::Value* Generate( exo::ast::Tree* tree );
			    llvm::Value* Generate( exo::ast::StmtList* list );
			    llvm::Value* Generate( exo::ast::StmtReturn* stmt );
			    llvm::Value* Generate( exo::ast::StmtExpr* stmt );
			    llvm::Value* Generate( exo::ast::StmtIf* stmt );

			    llvm::Value* Generate( exo::ast::AssignVar* assign );
			    llvm::Value* Generate( exo::ast::CallFun* fName );
			    llvm::Value* Generate( exo::ast::ExprVar* expr );

			    llvm::Value* Generate( exo::ast::DecVar* decl );
			    llvm::Value* Generate( exo::ast::DecFunProto* decl );
			    llvm::Value* Generate( exo::ast::DecFun* decl );
			    llvm::Value* Generate( exo::ast::DecClass* decl );

			    llvm::Value* Generate( exo::ast::ConstNull* val );
			    llvm::Value* Generate( exo::ast::ConstBool* val );
			    llvm::Value* Generate( exo::ast::ConstInt* val );
			    llvm::Value* Generate( exo::ast::ConstFloat* val );
			    llvm::Value* Generate( exo::ast::ConstStr* val );

			    llvm::Value* Generate( exo::ast::OpBinary* op );
		};
	}
}

#define EXO_CLASS(n) (n)
#define EXO_VTABLE(n) EXO_CLASS(n) + "_vtbl"
#define EXO_METHOD(c,m) "__" + EXO_CLASS(c) + "_" + m

/*
 * FIXME: not safe, will return 0 even if not found...
 */
#define EXO_METHOD_AT(c,m)	std::distance( methods[ EXO_CLASS( c ) ].begin(), std::find( methods[ EXO_CLASS( c ) ].begin(), methods[ EXO_CLASS( c ) ].end(), EXO_METHOD( c, m ) ) )

#endif /* CONTEXT_H_ */
