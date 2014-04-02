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

extern void* ParseAlloc( void *(*mallocProc)(size_t) );
extern void  ParseFree( void *p, void (*freeProc)(void*) );
extern void  Parse( void *yyp, int yymajor, quex::Token* yyminor, exo::ast::Tree* ast );

namespace exo
{
	namespace ast
	{
		Tree::Tree( std::string fileName )
		{
			quex::Token*	token = 0x0;
DEBUGMSG( "Parsing file: " << fileName );
			quex::lexer		qlex( fileName );

			void* lemon	= ParseAlloc( malloc );

			if( lemon != NULL ) {
				qlex.receive( &token );
				while( token->type_id() != QUEX_TKN_TERMINATION ) {
DEBUGMSG( "Received token QUEX_TKN_" << token->type_id_name() << " - " << token->line_number() << ":" << token->column_number() );
					Parse( lemon, token->type_id(), token, this );
					qlex.receive( &token );
				}

				Parse( lemon, 0, token, this );
				ParseFree( lemon, free );
			}
		}

		void Tree::addNode( Node* node )
		{
		}
	}
}
