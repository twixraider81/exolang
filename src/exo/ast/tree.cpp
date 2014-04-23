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
		Tree::Tree()
		{
			parser = ParseAlloc( GC_malloc );

			/*
			 * FIXME: figure out whats garbling the prefix on syntax errors.
			 */
#ifdef EXO_TRACE
			char prefix[] = "PARSER: ";
			ParseTrace( stderr, prefix );
#endif

			if( parser == NULL ) {
				BOOST_THROW_EXCEPTION( exo::exceptions::OutOfMemory() );
			}

			stmts = NULL;
		}

		Tree::~Tree()
		{
			::ParseFree( parser, GC_free );
		}

		void Tree::Parse( std::string fName )
		{
			fileName = fName;
			BOOST_LOG_TRIVIAL(trace) <<  "Opening <" << fileName << ">";

			quex::Token* token = new quex::Token;
			quex::lexer lexer( fileName );
			lexer.receive( &token );

			while( token->type_id() != QUEX_TKN_TERMINATION ) {
				BOOST_LOG_TRIVIAL(trace) << "Received <" << token->type_id_name() << "> in " << fileName << " on " << token->line_number() << ":" << token->column_number();

				::Parse( parser, token->type_id(), token, this );
				lexer.receive( &token );
			}

			::Parse( parser, 0, token, this );
		}

		void Tree::Parse( std::istream& stream )
		{
			fileName = "<stdin>";
			BOOST_LOG_TRIVIAL(trace) << "Opening <stdin>";

			quex::Token* token = new quex::Token;
			quex::lexer lexer( (QUEX_TYPE_CHARACTER*)0x0, 0 );

			while( stream ) {
				lexer.buffer_fill_region_prepare();

				stream.getline( (char*)lexer.buffer_fill_region_begin(), lexer.buffer_fill_region_size() );

				if( stream.gcount() == 0 ) {
					break;
				}

				lexer.buffer_fill_region_finish( stream.gcount() - 1 );
				lexer.receive( &token );

				while( token->type_id() != QUEX_TKN_TERMINATION ) {
					BOOST_LOG_TRIVIAL(trace) << "Received <" << token->type_id_name() << "> on " << token->line_number() << ":" << token->column_number();

					::Parse( parser, token->type_id(), token, this );
					lexer.receive( &token );
				}

				::Parse( parser, 0, token, this );
			}
		}
	}
}
