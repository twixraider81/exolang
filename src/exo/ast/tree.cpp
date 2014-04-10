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
		Tree::Tree( std::string fName, llvm::LLVMContext* c )
		{
			fileName = fName;
			context = c;
			TRACESECTION( "AST", "opening file: <" << fileName << ">" );

			parser = ParseAlloc( GC_malloc );
			quex::lexer lexer( fileName );
			quex::Token* token = new quex::Token;

#ifdef EXO_TRACE
			char prefix[] = "PARSER: ";
			ParseTrace( stderr, prefix );
#endif

			if( parser != NULL ) {
				lexer.receive( &token );

				while( token->type_id() != QUEX_TKN_TERMINATION ) {
					TRACESECTION( "LEXER", "received <" << token->type_id_name() << "> in " << fileName << " on " << token->line_number() << ":" << token->column_number() );

					Parse( parser, token->type_id(), token, this );
					lexer.receive( &token );
				}

				Parse( parser, 0, token, this );
				ParseFree( parser, GC_free );
			}
		}

		Tree::Tree( std::istream& stream, llvm::LLVMContext* c )
		{
			fileName = "<stdin>";
			context = c;
			TRACESECTION( "AST", "opening <stdin>" );

			parser = ParseAlloc( GC_malloc );
			quex::lexer lexer( (QUEX_TYPE_CHARACTER*)0x0, 0 );
			quex::Token* token = new quex::Token;

#ifdef EXO_TRACE
			char prefix[] = "PARSER: ";
			ParseTrace( stderr, prefix );
#endif

			if( parser != NULL ) {
				while( stream ) {
					lexer.buffer_fill_region_prepare();

					stream.getline( (char*)lexer.buffer_fill_region_begin(), lexer.buffer_fill_region_size() );

					if( stream.gcount() == 0 ) {
						break;
					}

					lexer.buffer_fill_region_finish( stream.gcount() - 1 );

					lexer.receive( &token );

					while( token->type_id() != QUEX_TKN_TERMINATION ) {
						TRACESECTION( "LEXER", "received <" << token->type_id_name() << "> on " << token->line_number() << ":" << token->column_number() );

						Parse( parser, token->type_id(), token, this );
						lexer.receive( &token );
					};
				}

				Parse( parser, 0, token, this );
				ParseFree( parser, GC_free );
			}
		}
	}
}
