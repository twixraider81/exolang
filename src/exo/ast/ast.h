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
#ifndef AST_H_
#define AST_H_

#include <cstring>

#include "lexer/lexer"
#include "parser/parser.h"

void* ParseAlloc( void *(*mallocProc)(size_t) );
void  ParseFree( void *p, void (*freeProc)(void*) );
void  Parse( void *yyp, int yymajor, quex::Token* yyminor );

namespace exo
{
	namespace ast
	{
		bool buildFromFile( std::string	fileName );
		int getTokenId( int lexerId );
		const char* getTokenName( int lexerId );
	}
}

#endif /* AST_H_ */
