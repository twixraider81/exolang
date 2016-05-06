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
		StmtIf::StmtIf( std::unique_ptr<Expr> e, std::unique_ptr<StmtList> t, std::unique_ptr<StmtList> f ) : StmtExpr( std::move( e ) ), onTrue( std::move( t ) ), onFalse( std::move( f ) )
		{
		}

		StmtIf::StmtIf( std::unique_ptr<Expr> e, std::unique_ptr<StmtList> t ) : StmtExpr( std::move( e ) ), onTrue( std::move( t ) )
		{
		}
	}
}
