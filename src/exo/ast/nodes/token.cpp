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
#include "token.h"
#include "../parser/parser.h"
#include "../lexer/lexer"

namespace exo
{
	namespace ast
	{
		const char* Token::getName( int lexerId )
		{
			switch( lexerId )
			{
				case QUEX_TKN_UNINITIALIZED:
				default:
					return( "uninitilized token" );

				case QUEX_TKN_TERMINATION:
					return( "end of file" );

				case QUEX_TKN_INT:
					return( "integer number" );

				case QUEX_TKN_FLOAT:
					return( "float number" );

				case QUEX_TKN_ADD:
					return( "+" );

				case QUEX_TKN_SUB:
					return( "-" );

				case QUEX_TKN_MUL:
					return( "*" );

				case QUEX_TKN_DIV:
					return( "/" );

				case QUEX_TKN_ASSIGN:
					return( "=" );

				case QUEX_TKN_SEMICOLON:
					return( ";" );

				case QUEX_TKN_VARIABLE:
					return( "variable" );

				case QUEX_TKN_ABRACKET_OPEN:
					return( "(" );

				case QUEX_TKN_ABRACKET_CLOSE:
					return( ")" );

				case QUEX_TKN_TYPE_INT:
					return( "int" );
			}
		}
	}
}

