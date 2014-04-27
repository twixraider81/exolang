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
		DecVar::DecVar( std::string vName, Type* vType )
		{
			BOOST_LOG_TRIVIAL(debug) << "Declaring $" << vName;
			name = vName;
			type = vType;
			expression = NULL;
		}

		DecVar::DecVar( std::string vName, Type* vType, Expr* expr )
		{
			BOOST_LOG_TRIVIAL(debug) << "Declaring/assigning $" << vName;
			name = vName;
			type = vType;
			expression = expr;
		}

		DecVar::~DecVar()
		{
			// there is no codegen method for the type, thus free it ourself
			delete type;
		}
	}
}
