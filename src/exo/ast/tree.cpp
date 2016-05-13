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

#include "exo/ast/nodes.h"
#include "exo/lexer/lexer"

/* lemon doesn't define its protos */
void* ParseAlloc( void *(*mallocProc)(size_t) );
void  ParseFree( void *p, void (*freeProc)(void*) );
void  Parse( void *yyp, int yymajor, std::unique_ptr<quex::Token> yyminor, exo::ast::Tree* ast );
void  ParseTrace( FILE *stream, char* zPrefix );

namespace exo
{
	namespace ast
	{
		Tree::Tree() : stmts( std::make_unique<exo::ast::StmtList>() ), fileName( "?" ), currentLineNo( 0 ), currentColumnNo( 0 )
		{
#ifndef EXO_GC_DISABLE
			parser = ::ParseAlloc( GC_malloc );
#else
			parser = ::ParseAlloc( malloc );
#endif

			if( parser == nullptr ) {
				EXO_THROW( OutOfMemory() );
			}

#ifndef NDEBUG
			// will get rerouted via preprocessor to boost::log
			::ParseTrace( stdout, (char*)"" );
#endif
		}

		Tree::~Tree()
		{
#ifndef EXO_GC_DISABLE
			::ParseFree( parser, GC_free );
#else
			::ParseFree( parser, free );
#endif
		}

		void Tree::Parse( std::string fName, std::string target )
		{
			fileName = fName;
			targetMachine = target;

			// as base we take the filename we, the extension( should be .exo ) and replace path delimiters ( / ) with namespace delimiters
			moduleName = boost::replace_all_copy( boost::filesystem::change_extension( boost::filesystem::path( fileName ).string(), "" ).string(), "/", "::" );

			if( !boost::filesystem::exists( fileName ) || !boost::filesystem::is_regular_file( fileName ) ) {
				EXO_THROW( NotFound() << exo::exceptions::RessouceName( fileName ) );
			}

			EXO_LOG( debug, "Opening <" << fileName << ">" );

			// this is a pointer to a region in the buffer, no need to alloc
			quex::Token* currentToken = nullptr;

			try {
				quex::lexer lexer( fileName );
				lexer.receive( &currentToken );
				while( currentToken->type_id() != QUEX_TKN_TERMINATION ) {
					currentLineNo = currentToken->line_number();
					currentColumnNo = currentToken->column_number();

					::Parse( parser, currentToken->type_id(), std::make_unique<quex::Token>( *currentToken ), this );
					lexer.receive( &currentToken );
				}

				::Parse( parser, 0, nullptr, this );
			} catch( boost::exception &exception ) {
				exception << boost::errinfo_file_name( fileName ) << boost::errinfo_file_name( fileName ) << boost::errinfo_at_line( currentLineNo );
				throw;
			}
		}
	}
}
