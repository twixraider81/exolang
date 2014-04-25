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

			if( parser == NULL ) {
				BOOST_THROW_EXCEPTION( exo::exceptions::OutOfMemory() );
			}

			stmts = NULL;
		}

		Tree::~Tree()
		{
			::ParseFree( parser, GC_free );
			// no freeing of stmts! codegen has ownership and will take care of that.
		}

		void Tree::Parse( std::string fName )
		{
			fileName = fName;
			BOOST_LOG_TRIVIAL(trace) <<  "Opening <" << fileName << ">";

			// pointer to buffer, no need to alloc
			quex::Token* currentToken = 0x0;
			quex::lexer lexer( fileName );
			lexer.receive( &currentToken );

			while( currentToken->type_id() != QUEX_TKN_TERMINATION ) {
				BOOST_LOG_TRIVIAL(trace) << "Received <" << currentToken->type_id_name() << "> in " << fileName << " on " << currentToken->line_number() << ":" << currentToken->column_number();

				::Parse( parser, currentToken->type_id(), currentToken, this );
				lexer.receive( &currentToken );
			}

			::Parse( parser, 0, currentToken, this );
		}

		void Tree::Parse( std::istream& stream )
		{
			fileName = "<stdin>";
			BOOST_LOG_TRIVIAL(trace) << "Opening <stdin>";

			// pointer to buffer, no need to alloc
			quex::Token* currentToken = 0x0;
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
					BOOST_LOG_TRIVIAL(trace) << "Received <" << currentToken->type_id_name() << "> on " << currentToken->line_number() << ":" << currentToken->column_number();

					::Parse( parser, currentToken->type_id(), currentToken, this );
					lexer.receive( &currentToken );
				}

				::Parse( parser, 0, currentToken, this );
			}
		}
	}
}
