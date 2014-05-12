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

#include "exo/ast/tree.h"

namespace exo
{
	namespace ast
	{
		Tree::Tree( std::string target )
		{
#ifndef EXO_GC_DISABLE
			parser = ::ParseAlloc( GC_malloc );
#else
			parser = ::ParseAlloc( malloc );
#endif

			if( parser == NULL ) {
				EXO_THROW_EXCEPTION( OutOfMemory, "Out of memory." );
			}

			// will get rerouted via preprocessor to boost::log
			::ParseTrace( stdout, (char*)banner.c_str() );
			stmts = NULL;
			targetMachine = target;
		}

		Tree::~Tree()
		{
#ifndef EXO_GC_DISABLE
			::ParseFree( parser, GC_free );
#else
			::ParseFree( parser, free );
#endif
		}

		void Tree::Parse( std::string fName )
		{
			fileName = fName;
			BOOST_LOG_TRIVIAL(debug) <<  "Opening <" << fileName << ">";

			// this is a pointer to a region in the buffer, no need to alloc
			quex::Token* currentToken = 0x0;
			// safe token will be needed for tokens with text, as previous pointer will be bent. parser will free them
			quex::Token* safeToken;

			quex::lexer lexer( fileName );
			lexer.receive( &currentToken );
			while( currentToken->type_id() != QUEX_TKN_TERMINATION ) {
				switch( currentToken->type_id() ) {
					case QUEX_TKN_S_INT:
					case QUEX_TKN_S_FLOAT:
					case QUEX_TKN_S_VAR:
					case QUEX_TKN_S_ID:
					case QUEX_TKN_S_STRING:
						safeToken = new quex::Token( *currentToken );
						::Parse( parser, safeToken->type_id(), safeToken, this );
					break;

					default:
						::Parse( parser, currentToken->type_id(), currentToken, this );
				}

				lexer.receive( &currentToken );
			}

			::Parse( parser, 0, currentToken, this );
		}

		void Tree::Parse( std::istream& stream )
		{
			fileName = "<stdin>";
			BOOST_LOG_TRIVIAL(debug) << "Opening <stdin>";

			// this is a pointer to a region in the buffer, no need to alloc
			quex::Token* currentToken = 0x0;
			// safe token will be needed for tokens with text, as previous pointer will be bent. parser will free them
			quex::Token* safeToken;

			quex::lexer lexer( (QUEX_TYPE_CHARACTER*)0x0, 0 );
			while( stream ) {
				lexer.buffer_fill_region_prepare();

				stream.getline( (char*)lexer.buffer_fill_region_begin(), lexer.buffer_fill_region_size() );

				if( stream.gcount() == 0 ) {
					break;
				}

				lexer.buffer_fill_region_finish( stream.gcount() - 1 );
				lexer.receive( &currentToken );

				while( currentToken->type_id() != QUEX_TKN_TERMINATION ) {
					switch( currentToken->type_id() ) {
						case QUEX_TKN_S_INT:
						case QUEX_TKN_S_FLOAT:
						case QUEX_TKN_S_VAR:
						case QUEX_TKN_S_ID:
						case QUEX_TKN_S_STRING:
							safeToken = new quex::Token( *currentToken );
							::Parse( parser, safeToken->type_id(), safeToken, this );
						break;

						default:
							::Parse( parser, currentToken->type_id(), currentToken, this );
					}
				}

				::Parse( parser, 0, currentToken, this );
			}
		}
	}
}
