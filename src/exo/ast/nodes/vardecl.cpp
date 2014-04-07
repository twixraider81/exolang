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

#include "exo/exo.h"
#include "exo/ast/ast.h"

namespace exo
{
	namespace ast
	{
		namespace nodes
		{
			VarDecl::VarDecl( std::string vName, Type* vType )
			{
				TRACESECTION( "AST", "declaring variable: $" << vName << ":" << vType->id );
				name = vName;
				type = vType;
			}

			VarDecl::VarDecl( std::string vName, Type* vType, Expr* expr )
			{
				TRACESECTION( "AST", "declaring/assigning variable: $" << vName << ":" << vType->id );
				name = vName;
				type = vType;
				expression = expr;
			}

			llvm::Value* VarDecl::Generate( exo::ast::Context* context )
			{
				TRACESECTION( "IR", "creating variable: $" << name );

				// allocate memory and push variable onto the local stack
				llvm::AllocaInst* memory = new llvm::AllocaInst( type->getLLVMType( context->context ), name.c_str(), context->getCurrentBlock() );
				context->Variables()[ name ] = memory;

				TRACESECTION( "IR", "new variable map size:" << context->Variables().size() );

				if( expression ) {
					VarAssign* a = new VarAssign( name, expression );
					a->Generate( context );
				}

				return( memory );
			}
		}
	}
}
