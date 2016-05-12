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

#include "exo/errors/exceptions.h"

namespace exo
{
	namespace exceptions
	{
		const char* InvalidMethod::what()
		{
			if( const std::string *functionName = boost::get_error_info<exo::exceptions::FunctionName>( *this ) ) {
				msg.append( "Unknown method " ).append( *functionName );

				if( const std::string *className = boost::get_error_info<exo::exceptions::ClassName>( *this ) ) {
					msg.append( " for class " ).append( *className );
				}

				if( const std::string *fileName = boost::get_error_info<boost::errinfo_file_name>( *this ) ) {
					msg.append( " in " ).append( *fileName );
				}

				if( const int *lineNo = boost::get_error_info<boost::errinfo_at_line>( *this ) ) {
					msg.append( ":" ).append( std::to_string( *lineNo ) );
				}
			} else {
				msg = "Invalid method";
			}

			return( msg.c_str() );
		};
	}
}
