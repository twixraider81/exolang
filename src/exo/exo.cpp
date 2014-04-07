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

// include internal stuff
#include "exo/exo.h"
#include "exo/ast/ast.h"
#include "exo/signals/signals.h"

#include <boost/program_options.hpp>

/*
 * Main/CLI Invocation, see -h
 * TODO: 1. do a bit of documentation now
 * TODO: 2. implement boost based exceptions
 * TODO: 3. use boost unit tests!
 * TODO: 4. implement type system
 */
int main( int argc, char **argv )
{
	exo::ast::Tree* ast;
	exo::ast::Context* context;
	exo::ast::ir::IR* ir;

	// inititalize garbage collector TODO: create initialization framework
	GC_INIT();
	GC_enable_incremental();

	// initialize llvm
	llvm::InitializeNativeTarget();

#ifdef EXO_TRACE
	setenv( "GC_PRINT_STATS", "1", 1 );
	setenv( "GC_DUMP_REGULARLY", "1", 1 );
	setenv( "GC_FIND_LEAK", "1", 1 );
#endif

	// register signal handler
	// FIXME: figure out why libgc sends a SIGSEGV on terminate
	exo::signals::registerHandler();



	// build optionlist
	boost::program_options::options_description availOptions( "Options" );
	availOptions.add_options()
		( "help,h",		"show usage/help" )
		( "version,v",	"show version" )
		( "input,i",	boost::program_options::value< std::string >(),	"parse file" )
    ;

	boost::program_options::positional_options_description positionalOptions;
	positionalOptions.add( "input", -1 );

	boost::program_options::variables_map commandLine;
	boost::program_options::store( boost::program_options::command_line_parser( argc, argv ).options( availOptions ).positional( positionalOptions ).run(), commandLine );
	boost::program_options::notify( commandLine );


	// show help & exit
	if( commandLine.count( "help" ) ) {
		std::cout << availOptions;
		return( 0 );
	}

	// show version & exit
	if( commandLine.count( "version" ) ) {
		std::cout << "version " << EXO_VERSION << std::endl;

		unsigned gcVersion = GC_get_version();
		std::cout << "libgc version " << ( gcVersion >> 16 ) << "." << ( ( gcVersion >> 8 ) & 0xFF ) << "." << ( gcVersion & 0xF ) << std::endl;

		return( 0 );
	}

	try {
		// we build the ast from a file given via -i / --input or stdin
		if( commandLine.count( "input" ) ) {
			ast = new exo::ast::Tree( commandLine["input"].as<std::string>() );
		} else {
			ast = new exo::ast::Tree( std::cin );
		}



		context = new exo::ast::Context( "main", &llvm::getGlobalContext() );
		context->generateIR( ast->stmts );

		// we have the IR now, don't need the AST anymore
		delete ast;

		ir = new exo::ast::ir::IR( context );
		ir->Optimize();

		delete ir;
	} catch( std::exception& e ) {
		ERRORRET( e.what(), -1 );
	}

	GC_gcollect();

	return( 0 );
}
