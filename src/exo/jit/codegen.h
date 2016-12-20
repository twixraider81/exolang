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
#include "exo/ast/nodes.h"

namespace exo
{
	namespace jit
	{
		class Codegen : public virtual exo::ast::Visitor
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
				std::shared_ptr<exo::jit::Target>						target;

				std::vector<std::string>								includePaths;
				std::vector<std::string>								libraryPaths;

				/**
				 * result of the current/last codgen visit operation
				 */
				llvm::Value*											currentResult;

				/**
				 * defines wether to store the result of the codgen visit operation in memory or register
				 */
				bool													generateInMem = false;

			public:
				std::unique_ptr<llvm::Module>	module;
				llvm::IRBuilder<>				builder; // this needs to be defined after module due to how initializer list is used
				std::set<std::string>			imports;

				Codegen( std::unique_ptr<llvm::Module> m, std::vector<std::string> i, std::vector<std::string> l );
				virtual ~Codegen();

				llvm::Type*		getType( exo::ast::Type* type );
				std::string		toString( llvm::Value* value );
				std::string		toString( llvm::Type* type );

				llvm::Function* registerExternFun( std::string name, llvm::Type* retType, std::vector<llvm::Type*> fArgs );

				llvm::Function*	getFunction( std::string functionName );
				llvm::Value* 	invokeFunction( llvm::Value* callee, llvm::FunctionType* function, std::vector<llvm::Value*> arguments, exo::ast::ExprList* expressions, bool inMem );
				llvm::Value* 	invokeMethod( llvm::Value* callee, std::string methodName, exo::ast::ExprList* expressions, bool isOptional, bool inMem );
				int				getPropPos( std::string className, std::string propName );
				int				getMethodPos( std::string className, std::string methodName );

				virtual void visit( exo::ast::ConstBool& );
				virtual void visit( exo::ast::ConstFloat& );
				virtual void visit( exo::ast::ConstInt& );
				virtual void visit( exo::ast::ConstNull& );
				virtual void visit( exo::ast::ConstStr& );
				virtual void visit( exo::ast::DeclClass& );
				virtual void visit( exo::ast::DeclFunProto& );
				virtual void visit( exo::ast::DeclFun& );
				virtual void visit( exo::ast::DeclMod& );
				virtual void visit( exo::ast::DeclVar& );
				virtual void visit( exo::ast::DeclVarList& );
				virtual void visit( exo::ast::ExprCallFun& );
				virtual void visit( exo::ast::ExprCallMethod& );
				virtual void visit( exo::ast::ExprVar& );
				virtual void visit( exo::ast::ExprProp& );
				virtual void visit( exo::ast::Node& );
				virtual void visit( exo::ast::OpBinaryAdd& );
				virtual void visit( exo::ast::OpBinaryAssign& );
				virtual void visit( exo::ast::OpBinaryAssignAdd& );
				virtual void visit( exo::ast::OpBinaryAssignDiv& );
				virtual void visit( exo::ast::OpBinaryAssignMul& );
				virtual void visit( exo::ast::OpBinaryAssignSub& );
				virtual void visit( exo::ast::OpBinaryDiv& );
				virtual void visit( exo::ast::OpBinaryEq& );
				virtual void visit( exo::ast::OpBinaryGe& );
				virtual void visit( exo::ast::OpBinaryGt& );
				virtual void visit( exo::ast::OpBinaryLe& );
				virtual void visit( exo::ast::OpBinaryLt& );
				virtual void visit( exo::ast::OpBinaryMul& );
				virtual void visit( exo::ast::OpBinaryNeq& );
				virtual void visit( exo::ast::OpBinarySub& );
				virtual void visit( exo::ast::OpUnaryDel& );
				virtual void visit( exo::ast::OpUnaryNew& );
				virtual void visit( exo::ast::OpUnaryRef& );
				virtual void visit( exo::ast::StmtBreak& );
				virtual void visit( exo::ast::StmtCont& );
				virtual void visit( exo::ast::StmtDo& );
				virtual void visit( exo::ast::StmtExpr& );
				virtual void visit( exo::ast::StmtFor& );
				virtual void visit( exo::ast::StmtIf& );
				virtual void visit( exo::ast::StmtImport& );
				virtual void visit( exo::ast::StmtLabel& );
				virtual void visit( exo::ast::StmtList& );
				virtual void visit( exo::ast::StmtReturn& );
				virtual void visit( exo::ast::StmtScope& );
				virtual void visit( exo::ast::StmtSwitch& );
				virtual void visit( exo::ast::StmtUse& );
				virtual void visit( exo::ast::StmtWhile& );
				virtual void visit( exo::ast::Tree& );
		};
	}
}

#define EXO_CLASS(n)				( "__class_" + n )
#define EXO_METHOD(c,m)				EXO_CLASS(c) + "_method_" + m
#define EXO_VTABLE(n)				EXO_CLASS(n) + "_vtbl"
#define EXO_CODEGEN_LOG(node,msg)	EXO_DEBUG_LOG(trace, msg << " in " << currentFile << "#" << node.lineNo << ":" << node.columnNo )

#ifndef EXO_GC_DISABLE
# define EXO_ALLOC "GC_malloc"
# define EXO_DEALLOC "GC_free"
#else
# define EXO_ALLOC "malloc"
# define EXO_DEALLOC "free"
#endif

#endif /* CODEGEN_H_ */
