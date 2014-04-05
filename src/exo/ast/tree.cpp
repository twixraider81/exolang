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
		Tree::Tree( std::string fileName )
		{
			quex::Token*	token = 0x0;

			TRACESECTION( "AST", "opening file: <" << fileName << ">" );

			lexer = new quex::lexer( fileName );
			parser = ParseAlloc( malloc );

#ifdef EXO_TRACE
			char prefix[] = "Debug: ";
			ParseTrace( stderr, prefix );
#endif

			if( parser != NULL ) {
				lexer->receive( &token );
				while( token->type_id() != QUEX_TKN_TERMINATION ) {
					TRACESECTION( "LEXER", "received QUEX_TKN_" << token->type_id_name() << " in " << fileName << " on " << token->line_number() << ":" << token->column_number() );

					Parse( parser, token->type_id(), token, this );
					lexer->receive( &token );
				}

				Parse( parser, 0, token, this );
				ParseFree( parser, free );
			}
		}

		Tree::Tree( std::istream& stream )
		{
			quex::Token*	token = new quex::Token;

			TRACESECTION( "AST", "opening <stdin>" );

			lexer = new quex::lexer( (QUEX_TYPE_CHARACTER*)0x0, 0 );
			parser = ParseAlloc( malloc );

#ifdef EXO_TRACE
			char prefix[] = "Debug: ";
			ParseTrace( stderr, prefix );
#endif

			if( parser != NULL ) {
				while( stream ) {
					lexer->buffer_fill_region_prepare();

					stream.getline( (char*)lexer->buffer_fill_region_begin(), lexer->buffer_fill_region_size() );

					if( stream.gcount() == 0 ) {
						return;
					}

					lexer->buffer_fill_region_finish( stream.gcount() - 1 );

					lexer->receive( &token );
					while( token->type_id() != QUEX_TKN_TERMINATION ) {
						TRACESECTION( "LEXER", "received QUEX_TKN_" << token->type_id_name() << " on " << token->line_number() << ":" << token->column_number() );

						Parse( parser, token->type_id(), token, this );
						lexer->receive( &token );
					};
				}

				Parse( parser, QUEX_TKN_TERMINATION, token, this );
				ParseFree( parser, free );
			}
		}
	}
}
