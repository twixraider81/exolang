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

namespace exo
{
	namespace ast
	{
		class Tree;

		namespace nodes
		{
			class StmtList;
		}

		class Context : public gc
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
			    std::map<std::string, llvm::Value*>& localVariables();

			    void Generate( exo::ast::nodes::StmtList* stmts );
			    void Execute();
		};
	}
}

#endif /* CONTEXT_H_ */
