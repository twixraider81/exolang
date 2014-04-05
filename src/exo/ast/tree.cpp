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
		Tree::Tree( std::string fName )
		{
			fileName = fName;
			TRACESECTION( "AST", "opening file: <" << fileName << ">" );

			token = new quex::Token;
			lexer = new quex::lexer( fileName );
			parser = ParseAlloc( malloc );

#ifdef EXO_TRACE
			char prefix[] = "PARSER: ";
			ParseTrace( stderr, prefix );
#endif

			if( parser != NULL ) {
				lexer->receive( &token );

				while( token->type_id() != QUEX_TKN_TERMINATION ) {
					TRACESECTION( "LEXER", "received <" << token->type_id_name() << "> in " << fileName << " on " << token->line_number() << ":" << token->column_number() );

					Parse( parser, token->type_id(), token, this );
					lexer->receive( &token );
				}

				this->finishParse();
			}
		}

		Tree::Tree( std::istream& stream )
		{
			fileName = "<stdin>";
			TRACESECTION( "AST", "opening <stdin>" );

			token = new quex::Token;
			lexer = new quex::lexer( (QUEX_TYPE_CHARACTER*)0x0, 0 );
			parser = ParseAlloc( malloc );

#ifdef EXO_TRACE
			char prefix[] = "PARSER: ";
			ParseTrace( stderr, prefix );
#endif

			if( parser != NULL ) {
				while( stream ) {
					lexer->buffer_fill_region_prepare();

					stream.getline( (char*)lexer->buffer_fill_region_begin(), lexer->buffer_fill_region_size() );

					if( stream.gcount() == 0 ) {
						break;
					}

					lexer->buffer_fill_region_finish( stream.gcount() - 1 );

					lexer->receive( &token );

					while( token->type_id() != QUEX_TKN_TERMINATION ) {
						TRACESECTION( "LEXER", "received <" << token->type_id_name() << "> on " << token->line_number() << ":" << token->column_number() );

						Parse( parser, token->type_id(), token, this );
						lexer->receive( &token );
					};
				}

				this->finishParse();
			}
		}

		void Tree::finishParse()
		{
			Parse( this->parser, 0, this->token, this );
			ParseFree( this->parser, free );
		}
	}
}
