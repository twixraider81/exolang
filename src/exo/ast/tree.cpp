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
#include "exo/ast/nodes.h"

namespace exo
{
	namespace ast
	{
		Tree::Tree() : stmts( NULL ), fileName( "?" )
		{
#ifndef EXO_GC_DISABLE
			parser = ::ParseAlloc( GC_malloc );
#else
			parser = ::ParseAlloc( malloc );
#endif

			if( parser == NULL ) {
				EXO_THROW_EXCEPTION( OutOfMemory, "Out of memory." );
			}

#ifndef NDEBUG
			// will get rerouted via preprocessor to boost::log
			::ParseTrace( stdout, (char*)banner.c_str() );
#endif
		}

		Tree::~Tree()
		{
#ifndef EXO_GC_DISABLE
			::ParseFree( parser, GC_free );
#else
			::ParseFree( parser, free );
#endif

			if( stmts != NULL ) {
				delete stmts;
			}
		}

		void Tree::Parse( std::string fName, std::string target )
		{
			fileName = fName;
			targetMachine = target;

			if( !boost::filesystem::exists( fileName ) || !boost::filesystem::is_regular_file( fileName ) ) {
				EXO_THROW_EXCEPTION( NotFound, ( boost::format( "File \"%s\" not found." ) % fileName ).str() );
			}

			EXO_LOG( debug, "Opening <" << fileName << ">" );

			// this is a pointer to a region in the buffer, no need to alloc
			quex::Token* currentToken = NULL;

			quex::lexer lexer( fileName );
			lexer.receive( &currentToken );
			while( currentToken->type_id() != QUEX_TKN_TERMINATION ) {
				::Parse( parser, currentToken->type_id(), new quex::Token( *currentToken ), this );
				lexer.receive( &currentToken );
			}

			::Parse( parser, 0, currentToken, this );
		}
	}
}
