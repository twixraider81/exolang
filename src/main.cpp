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
#include <fstream>
#include <cstring>
#include <iostream>
#include <exception>

using namespace std;

#include "error/error.h"
#include "lexer/lexer"
#include "parser/parser.h"

extern void *ParseAlloc( void *(*mallocProc)(size_t) );
extern void ParseFree( void *p, void (*freeProc)(void*) );
extern void Parse( void *yyp, int yymajor, quex::Token* yyminor );

int main( int argc, char **argv )
{
	quex::Token*	token = 0x0;
	std::string		sourceFile;

	try{
		switch( argc ) {
			case 2:
				sourceFile = argv[1];
			break;

			default:
DEBUGMSG( "Invalid Argument count" << argc );
				return( -1 );
		};

		quex::lexer		qlex( sourceFile );

DEBUGMSG( "Parsing file: " << sourceFile );

		void* pParser	= ParseAlloc( malloc );

		do {
			qlex.receive( &token );

			if( token->type_id() != QUEX_TKN_TERMINATION ) {
DEBUGMSG( "Got token " << token->type_id_name() << " on line " << token->line_number() << ", " << token->column_number() );
			} else {
			}
		} while( token->type_id() != QUEX_TKN_TERMINATION );

		ParseFree( pParser, free );
	} catch(exception& e) {
		cout << e.what() << '\n';
	}

	return( 0 );
}
