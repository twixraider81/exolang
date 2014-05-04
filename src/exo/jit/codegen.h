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
		class CallFun;
		class CallMethod;

		class ConstBool;
		class ConstFloat;
		class ConstInt;
		class ConstNull;
		class ConstStr;

		class DecClass;
		class DecFun;
		class DecFunProto;
		class DecVar;

		class ExprVar;

		class OpBinary;
		class OpBinaryAssign;
		class OpBinaryNew;

		class StmtDelete;
		class StmtExpr;
		class StmtIf;
		class StmtReturn;
		class StmtList;

		class Tree;
		class Type;
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

			    Codegen( std::string cname, std::string target );
			    Codegen( std::string cname ) : Codegen( cname, llvm::sys::getProcessTriple() ) {};
			    ~Codegen();

			    void popBlock();
			    void pushBlock( llvm::BasicBlock* block, std::string name );
			    llvm::BasicBlock*					getCurrentBasicBlock();
			    std::string							getCurrentBlockName();
			    std::map<std::string,llvm::Value*>&	getCurrentBlockVars();

			    llvm::Type* getType( exo::ast::Type* type, llvm::LLVMContext& context );

			    llvm::Value* Generate( exo::ast::CallFun* call );
			    llvm::Value* Generate( exo::ast::CallMethod* call );

			    llvm::Value* Generate( exo::ast::ConstBool* val );
			    llvm::Value* Generate( exo::ast::ConstInt* val );
			    llvm::Value* Generate( exo::ast::ConstFloat* val );
			    llvm::Value* Generate( exo::ast::ConstNull* val );
			    llvm::Value* Generate( exo::ast::ConstStr* val );

			    llvm::Value* Generate( exo::ast::DecClass* dec );
			    llvm::Value* Generate( exo::ast::DecFun* dec );
			    llvm::Value* Generate( exo::ast::DecFunProto* dec );
			    llvm::Value* Generate( exo::ast::DecVar* dec );

			    llvm::Value* Generate( exo::ast::ExprVar* expr );

			    llvm::Value* Generate( exo::ast::OpBinary* op );
			    llvm::Value* Generate( exo::ast::OpBinaryAssign* op );
			    llvm::Value* Generate( exo::ast::OpBinaryNew* op );

			    llvm::Value* Generate( exo::ast::StmtDelete* stmt );
			    llvm::Value* Generate( exo::ast::StmtExpr* stmt );
			    llvm::Value* Generate( exo::ast::StmtIf* stmt );
			    llvm::Value* Generate( exo::ast::StmtReturn* stmt );
			    llvm::Value* Generate( exo::ast::StmtList* list );

			    llvm::Value* Generate( boost::shared_ptr<exo::ast::Tree> tree );
		};
	}
}

#define EXO_CLASS(n) (n)
#define EXO_VTABLE(n) EXO_CLASS(n) + "_vtbl"
#define EXO_METHOD(c,m) "__" + EXO_CLASS(c) + "_" + m

/*
 * FIXME: not safe
 */
#define EXO_METHOD_AT(c,m)	std::distance( methods[ EXO_CLASS( c ) ].begin(), std::find( methods[ EXO_CLASS( c ) ].begin(), methods[ EXO_CLASS( c ) ].end(), EXO_METHOD( c, m ) ) )

#endif /* CONTEXT_H_ */
