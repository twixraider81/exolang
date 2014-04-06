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

// include external stuff
#include <getoptpp/getoptpp/getopt_pp.h>

/*
 * Main/CLI Invocation, see -h
 */
int main( int argc, char **argv )
{
	exo::ast::Tree* ast;
	exo::ast::Context* context;
	std::string	sourceFile;
	llvm::ExecutionEngine* engine;

	// inititalize garbage collector TODO: create initialization framework
	GC_INIT();
	GC_enable_incremental();

#ifdef EXO_TRACE
	setenv( "GC_PRINT_STATS", "1", 1 );
	setenv( "GC_DUMP_REGULARLY", "1", 1 );
	setenv( "GC_FIND_LEAK", "1", 1 );
#endif

	// register signal handler
	// FIXME: figure out why libgc sends a SIGSEGV on terminate
	exo::signals::registerHandler();

	// build optionlist
	GetOpt::GetOpt_pp ops( argc, argv );


	// show help & exit
	if (ops >> GetOpt::OptionPresent( 'h', "help" )) {
		std::cout << "--help, -h\t\t\t\tshow usage/help" << std::endl;
		std::cout << "--input=<file>, -i <file>\t\tparse file" << std::endl;
		std::cout << "--version, -v\t\t\tshow version" << std::endl;
		return( 0 );
	}

	// show version & exit
	if (ops >> GetOpt::OptionPresent( 'v', "help" )) {
		std::cout << "version " << EXO_VERSION << std::endl;

		unsigned gcVersion = GC_get_version();
		std::cout << "libgc version " << ( gcVersion >> 16 ) << "." << ( ( gcVersion >> 8 ) & 0xFF ) << "." << ( gcVersion & 0xF ) << std::endl;

		return( 0 );
	}

	try {
		// we build the ast from a file given via -i / --input or stdin
		if( (ops >> GetOpt::Option( 'i', "input", sourceFile)) ) {
			ast = new exo::ast::Tree( sourceFile );
		} else {
			ast = new exo::ast::Tree( std::cin );
		}

		llvm::InitializeNativeTarget();

		context = new exo::ast::Context( "main", &llvm::getGlobalContext() );
		context->Generate( ast->stmts );


		std::string errorMsg;
		engine = llvm::EngineBuilder( context->module ).setErrorStr( &errorMsg ).create();

		if( !engine ) {
			ERRORRET( errorMsg, -1 );
		}

		delete engine;
	} catch( std::exception& e ) {
		ERRORRET( e.what(), -1 );
	}

#ifdef EXO_TRACE
	GC_gcollect();
#endif

	return( 0 );
}
