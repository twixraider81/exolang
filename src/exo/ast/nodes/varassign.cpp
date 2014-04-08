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
			VarAssign::VarAssign( std::string vName, Expr* expr )
			{
				TRACESECTION( "AST", "assigning $" << vName );
				name = vName;
				expression = expr;
			}

			llvm::Value* VarAssign::Generate( exo::ast::Context* context )
			{
				TRACESECTION( "IR", "assigning variable $" << name );

				if( context->Variables().find( name ) == context->Variables().end() ) {
					BOOST_THROW_EXCEPTION( exo::exceptions::UnknownVar( name ) );
				}

				llvm::StoreInst* store = new llvm::StoreInst( expression->Generate( context ), context->Variables()[name], false, context->getCurrentBlock() );
				return( store );
			}
		}
	}
}
