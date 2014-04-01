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

#include "getoptpp/getoptpp/getopt_pp.h"

int main( int argc, char **argv )
{
	std::string	sourceFile;

	// build optionlist
	GetOpt::GetOpt_pp ops( argc, argv );

	// show help & exit
	if (ops >> GetOpt::OptionPresent( 'h', "help" )) {
		return( exo::help::print() );
	}

	// no file, interactive mode miiight come, exit for now
	if( !(ops >> GetOpt::Option( 'i', "input", sourceFile)) ) {
ERRORMSG( "No input file specified, try -h to show help", 0 );
	}


	try {
		exo::ast::buildFromFile( sourceFile );
	} catch( std::exception& e ) {
ERRORMSG( "Exception: " << e.what(), -1 );
	}

	return( 0 );
}
