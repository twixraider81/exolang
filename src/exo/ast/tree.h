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

#ifndef TREE_H_
#define TREE_H_

#include "exo/lexer/lexer"

namespace exo
{
	namespace ast
	{
		namespace nodes
		{
			class StmtList;
		}

		class Tree : public gc
		{
			public:
				void*		 parser;
				std::string	fileName;

				nodes::StmtList* stmts;

				Tree( std::string fName );
				Tree( std::istream& stream );
		};
	}
}

#endif /* TREE_H_ */
