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
#include "exo/ast/nodes.h"

namespace exo
{
	namespace ast
	{
		Type::Type( const std::type_info* t )
		{
			TRACESECTION( "AST", "referenced type:" << t->name() );
			info = t;
		}

		Type::Type( const std::type_info* t , std::string tName ) : Type( t )
		{
			TRACESECTION( "AST","referenced type:" << t->name() << ", name: " << tName );
			name = tName;
		}
	}
}
