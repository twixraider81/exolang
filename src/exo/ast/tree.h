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

#define TOKENSTR(s) std::string( reinterpret_cast<const char*>( s->get_text().c_str() ) )

namespace exo
{
	namespace ast
	{
		class StmtList;

		class Tree
		{
			public:
				void*			parser;
				std::string		fileName;
				StmtList*		stmts;

				Tree();
				~Tree();

				void Parse( std::string fName );
				void Parse( std::istream& stream );
		};
	}
}

/* lemon doesn't define its protos */
void* ParseAlloc( void *(*mallocProc)(size_t) );
void  ParseFree( void *p, void (*freeProc)(void*) );
void  Parse( void *yyp, int yymajor, quex::Token* yyminor, exo::ast::Tree* ast );
void  ParseTrace( FILE *stream, char* zPrefix );

#endif /* TREE_H_ */
