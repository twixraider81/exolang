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
#include "ast.h"

namespace exo
{
	namespace ast
	{
		bool buildFromFile( std::string	fileName )
		{
			quex::Token*	token = 0x0;
DEBUGMSG( "Parsing file: " << fileName );
			quex::lexer		qlex( fileName );

			void* lemon	= ParseAlloc( malloc );

			if( lemon == NULL ) {
ERRORMSG( "Unable to allocate parser", false );
			}

			qlex.receive( &token );
			while( token->type_id() != QUEX_TKN_TERMINATION ) {
DEBUGMSG( "Received token QUEX_TKN_" << token->type_id_name() << " - " << token->line_number() << ":" << token->column_number() );
				Parse( lemon, getTokenId( token->type_id() ), token );
				qlex.receive( &token );
			}

			Parse( lemon, 0, token );
			ParseFree( lemon, free );
			return( true );
		}

		int getTokenId( int lexerId )
		{
			switch( lexerId )
			{
				default:
				case QUEX_TKN_TERMINATION:
					return( 0 );

				case QUEX_TKN_INT:
					return( LEMON_TKN_INT );

				case QUEX_TKN_FLOAT:
					return( LEMON_TKN_FLOAT );

				case QUEX_TKN_ADD:
					return( LEMON_TKN_ADD );

				case QUEX_TKN_SUB:
					return( LEMON_TKN_SUB );

				case QUEX_TKN_MUL:
					return( LEMON_TKN_MUL );

				case QUEX_TKN_DIV:
					return( LEMON_TKN_DIV );

				case QUEX_TKN_ASSIGN:
					return( LEMON_TKN_ASSIGN );

				case QUEX_TKN_SEMICOLON:
					return( LEMON_TKN_SEMICOLON );

				case QUEX_TKN_IDENTIFIER:
					return( LEMON_TKN_IDENTIFIER );

				case QUEX_TKN_DOLLAR:
					return( LEMON_TKN_DOLLAR );

				case QUEX_TKN_ABRACKET_OPEN:
					return( LEMON_TKN_ABRACKET_OPEN );

				case QUEX_TKN_ABRACKET_CLOSE:
					return( LEMON_TKN_ABRACKET_CLOSE );
			}
		}

		const char* getTokenName( int lexerId )
		{
			switch( lexerId )
			{
				default:
				case 0:
					return( "EOF" );

				case LEMON_TKN_INT:
					return( "integer" );

				case LEMON_TKN_FLOAT:
					return( "float" );

				case LEMON_TKN_ADD:
					return( "+" );

				case LEMON_TKN_SUB:
					return( "-" );

				case LEMON_TKN_MUL:
					return( "*" );

				case LEMON_TKN_DIV:
					return( "/" );

				case LEMON_TKN_ASSIGN:
					return( "=" );

				case LEMON_TKN_SEMICOLON:
					return( ";" );

				case LEMON_TKN_IDENTIFIER:
					return( "identifier" );

				case LEMON_TKN_DOLLAR:
					return( "$" );

				case LEMON_TKN_ABRACKET_OPEN:
					return( "(" );

				case LEMON_TKN_ABRACKET_CLOSE:
					return( ")" );
			}
		}
	}
}
