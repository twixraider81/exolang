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
#include "../ast.h"

namespace exo
{
	namespace ast
	{
		NodeInteger::NodeInteger( quex::Token* token )
		{
DEBUGMSG( "Creating integer (" << token->get_text().c_str() << ")" );
			const char *str = reinterpret_cast<const char *>( token->get_text().c_str() );
			value = atol( str );
		}
	}
}
