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
#include "exo/signals/signals.h"

#include "getoptpp/getoptpp/getopt_pp.h"

void printUsage(void)
{
	std::cout << "--help, -h\t\t\t\tshow usage/help" << std::endl;
	std::cout << "--input=<file>, -i <file>\t\tparse file" << std::endl;
}


int main( int argc, char **argv )
{
	exo::ast::Tree* tree;
	std::string	sourceFile;

	// register signal handlers
	exo::signals::registerHandlers();

	// build optionlist
	GetOpt::GetOpt_pp ops( argc, argv );

	// show help & exit
	if (ops >> GetOpt::OptionPresent( 'h', "help" )) {
		printUsage();
		return( 0 );
	}

	try {
		// no file, read stdin
		if( (ops >> GetOpt::Option( 'i', "input", sourceFile)) ) {
			tree = new exo::ast::Tree( sourceFile );
		} else {
			tree = new exo::ast::Tree( std::cin );
		}

		delete tree;
	} catch( std::exception& e ) {
ERRORRET( "Exception: " << e.what(), -1 );
	}

	return( 0 );
}
