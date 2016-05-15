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

#include "exo/ast/nodes.h"

namespace exo
{
	namespace ast
	{
		DeclFun::DeclFun( std::unique_ptr<Id> i, std::unique_ptr<Type> r, std::unique_ptr<DeclVarList> a, std::unique_ptr<Stmt> b, bool va ) :
			DeclFunProto( std::move(i), std::move(r), std::move(a), va ),
			stmts( std::move(b) ),
			access( std::make_unique<ModAccess>() )
		{
		}

		DeclFun::DeclFun( std::unique_ptr<Id> i, std::unique_ptr<ModAccess> m, std::unique_ptr<Type> r, std::unique_ptr<DeclVarList> a, std::unique_ptr<Stmt> b, bool va ):
			DeclFunProto( std::move(i), std::move(r), std::move(a), va ),
			stmts( std::move(b) ),
			access( std::move(m) )
		{

		}
	}
}
