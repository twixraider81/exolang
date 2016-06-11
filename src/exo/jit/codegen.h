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
		class ConstBool;
		class ConstFloat;
		class ConstInt;
		class ConstNull;
		class ConstStr;
		class DeclClass;
		class DeclFunProto;
		class DeclFun;
		class DeclMod;
		class DeclVar;
		class DeclVarList;
		class Expr;
		class ExprCallFun;
		class ExprCallMethod;
		class ExprList;
		class ExprVar;
		class ExprProp;
		class Id;
		class OpBinary;
		class OpBinaryAssign;
		class OpBinaryAssignShort;
		class OpUnaryDel;
		class OpUnaryNew;
		class OpUnaryRef;
		class StmtBlock;
		class StmtBreak;
		class StmtCont;
		class StmtExpr;
		class StmtFor;
		class StmtIf;
		class StmtImport;
		class StmtReturn;
		class StmtUse;
		class StmtWhile;
		class Type;
		class Tree;
	}

	namespace jit
	{
		class Block;

		class Codegen
		{
			private:
				std::unique_ptr<Stack> stack;

				/*
				 * map layout
				 * {
				 *  "class1":
				 *  {
				 *   "prop1" : { 1, expr },
				 *   "prop2" : { 2, expr }
				 *  },
				 *  ...
				 * }
				 */
				std::unordered_map< std::string, std::unordered_map< std::string, std::pair<int, llvm::Value*> > >		properties;
				std::unordered_map< std::string, std::unordered_map< std::string, std::pair<int, llvm::Function*> > >	methods;

				std::string												currentFile;
				std::string												currentTarget;

				std::vector<std::string>								includePaths;
				std::vector<std::string>								libraryPaths;

			public:
				std::unique_ptr<llvm::Module>	module;
				llvm::IRBuilder<>				builder; // this needs to be defined after module due to how initializer list is used
				std::set<std::string>			imports;

				Codegen( std::unique_ptr<llvm::Module> m, std::vector<std::string> i, std::vector<std::string> l );
				~Codegen();

				llvm::Type*		getType( exo::ast::Type* type );
				llvm::Value*	Generate( exo::ast::ConstBool* val, bool inMem );
				llvm::Value*	Generate( exo::ast::ConstInt* val, bool inMem );
				llvm::Value*	Generate( exo::ast::ConstFloat* val, bool inMem );
				llvm::Value*	Generate( exo::ast::ConstNull* val, bool inMem );
				llvm::Value*	Generate( exo::ast::ConstStr* val, bool inMem );
				llvm::Value*	Generate( exo::ast::DeclClass* dec, bool inMem );
				llvm::Value*	Generate( exo::ast::DeclFunProto* dec, bool inMem );
				llvm::Value*	Generate( exo::ast::DeclFun* dec, bool inMem );
				llvm::Value*	Generate( exo::ast::DeclMod* dec, bool inMem );
				llvm::Value*	Generate( exo::ast::DeclVar* dec, bool inMem );
				llvm::Value*	Generate( exo::ast::DeclVarList* dec, bool inMem );
				llvm::Value*	Generate( exo::ast::ExprCallFun* call, bool inMem );
				llvm::Value*	Generate( exo::ast::ExprCallMethod* call, bool inMem );
				llvm::Value*	Generate( exo::ast::ExprProp* expr, bool inMem );
				llvm::Value*	Generate( exo::ast::ExprVar* expr, bool inMem );
				llvm::Value*	Generate( exo::ast::OpBinary* op, bool inMem );
				llvm::Value*	Generate( exo::ast::OpBinaryAssign* op, bool inMem );
				llvm::Value*	Generate( exo::ast::OpBinaryAssignShort* op, bool inMem );
				llvm::Value*	Generate( exo::ast::OpUnaryDel* op, bool inMem );
				llvm::Value*	Generate( exo::ast::OpUnaryNew* op, bool inMem );
				llvm::Value*	Generate( exo::ast::OpUnaryRef* op, bool inMem );
				llvm::Value*	Generate( exo::ast::StmtBlock* stmts, bool inMem );
				llvm::Value*	Generate( exo::ast::StmtBreak* stmt, bool inMem );
				llvm::Value*	Generate( exo::ast::StmtCont* stmt, bool inMem );
				llvm::Value*	Generate( exo::ast::StmtExpr* stmt, bool inMem );
				llvm::Value*	Generate( exo::ast::StmtFor* stmt, bool inMem );
				llvm::Value*	Generate( exo::ast::StmtIf* stmt, bool inMem );
				llvm::Value*	Generate( exo::ast::StmtImport* stmt, bool inMem );
				llvm::Value*	Generate( exo::ast::StmtReturn* stmt, bool inMem );
				llvm::Value*	Generate( exo::ast::StmtUse* dec, bool inMem );
				llvm::Value*	Generate( exo::ast::StmtWhile* stmt, bool inMem );
				llvm::Value*	Generate( exo::ast::Tree* tree );

				std::string		toString( llvm::Value* value );
				std::string		toString( llvm::Type* type );

				llvm::Function* registerExternFun( std::string name, llvm::Type* retType, std::vector<llvm::Type*> fArgs );

				llvm::Function*	getFunction( std::string functionName );
				llvm::Value* 	invokeFunction( llvm::Value* callee, llvm::FunctionType* function, std::vector<llvm::Value*> arguments, exo::ast::ExprList* expressions, bool inMem );
				llvm::Value* 	invokeMethod( llvm::Value* callee, std::string methodName, exo::ast::ExprList* expressions, bool isOptional, bool inMem );
				int				getPropPos( std::string className, std::string propName );
				int				getMethodPos( std::string className, std::string methodName );
		};
	}
}

#define EXO_CLASS(n)				( "__class_" + n )
#define EXO_METHOD(c,m)				EXO_CLASS(c) + "_method_" + m
#define EXO_VTABLE(n)				EXO_CLASS(n) + "_vtbl"
#define EXO_CODEGEN_LOG(node,msg)	EXO_DEBUG_LOG(trace, msg << " in " << currentFile << "#" << node->lineNo << ":" << node->columnNo/* << " (" << stack->blockName() << ")"*/ )

#ifndef EXO_GC_DISABLE
# define EXO_ALLOC "GC_malloc"
# define EXO_DEALLOC "GC_free"
#else
# define EXO_ALLOC "malloc"
# define EXO_DEALLOC "free"
#endif

#endif /* CODEGEN_H_ */
