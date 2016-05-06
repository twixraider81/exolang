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
#include "exo/ast/nodes.h"
#include "exo/jit/jit.h"
#include "exo/jit/codegen.h"
#include "exo/init/init.h"

#include <boost/program_options.hpp>

/*
 * Main/CLI Invocation, see -h
 * TODO: 1. implement type system
 * TODO: 2. document ast and jit
 * TODO: 3. use boost unit tests!
 * TODO: 4. fix leaks
 * TODO: 5. implement a real symbol table which tracks types, memory locations and pointer/refs
 * TODO: 6. support other targets / "asm" output
 */
int main( int argc, char **argv )
{
	int severity, optimize, retval;
	std::string target, emit, input;

	// build optionlist
	boost::program_options::options_description availOptions( "Options" );
	availOptions.add_options()
		( "help,h",																				"Show this usage/help" )
		( "input,i",		boost::program_options::value<std::string>(&input),					"File to parse and execute if -e is not given" )
		( "emit,e",																				"Emit LLVM IR to stdout" )
		( "log-severity,s", boost::program_options::value<int>(&severity)->default_value(4),	"Set log severity; 1 = trace, 2 = debug, 3 = info, 4 = warning, 5 = error, 6 = fatal" )
		( "optimize,o", 	boost::program_options::value<int>(&optimize)->default_value(2),	"Set optimization level; 0 = none, 1 = less, 2 = default, 3 = all" )
		( "target,t", 		boost::program_options::value<std::string>(&target)->default_value( llvm::Triple( llvm::sys::getDefaultTargetTriple() ).getArchName().str() ),
																								"Set target" )
		( "version,v",																			"Show version, libraries and targets" )
		;

	boost::program_options::positional_options_description positionalOptions;
	positionalOptions.add( "input", -1 );

	boost::program_options::variables_map commandLine;
	boost::program_options::store( boost::program_options::command_line_parser( argc, argv ).options( availOptions ).positional( positionalOptions ).run(), commandLine );
	boost::program_options::notify( commandLine );


	// show help & exit
	if( commandLine.count( "help" ) || argc == 1 ) {
		std::cout << availOptions;
		exit( 0 );
	}

	// show version & exit
	if( commandLine.count( "version" ) ) {
		std::string hostName = llvm::sys::getHostCPUName();

		std::cout << "Version:\t" << EXO_VERSION << std::endl;
		std::cout << "Host CPU:\t" << hostName << std::endl;
#ifndef EXO_GC_DISABLE
		unsigned gcVersion = GC_get_version();
		std::cout << "LibGC version:\t\t" << ( gcVersion >> 16 ) << "." << ( ( gcVersion >> 8 ) & 0xFF ) << "." << ( gcVersion & 0xF ) << std::endl;
#else
		std::cout << "LibGC:\t\tdisabled" << std::endl;
#endif
		std::cout << "LLVM Framework:\t" << LLVM_VERSION_STRING << std::endl;

		llvm::InitializeAllTargets();

		llvm::Triple t( llvm::sys::getDefaultTargetTriple() );
		std::cout << std::endl << "Native target:\t" << t.getArchName().str() << std::endl << "Supported targets:" << std::endl;

		for( auto &t : llvm::TargetRegistry::targets() ) {
			std::cout << " - " << t.getName() << "\t";
			if( strlen( t.getName() ) < 5 ) {
				std::cout << "\t";
			}
			std::cout << ": " << t.getShortDescription() << std::endl;
		}

		exit( 0 );
	}


	// we are running, command line is parsed
	if( !exo::init::Init::Startup( severity ) ) {
		EXO_LOG( fatal, "Unable to complete initialization." );
		exit( 1 );
	}

	try {
		if( !commandLine.count( "input" ) ) {
			EXO_LOG( fatal, "No input file given." );
			exit( 1 );
		}

		std::unique_ptr<exo::ast::Tree> ast( new exo::ast::Tree() );
		ast->Parse( input, target );

		std::unique_ptr<exo::jit::Codegen> generator( new exo::jit::Codegen( "main", target ) );
		generator->Generate( ast.get() );

		std::unique_ptr<exo::jit::JIT> jit( new exo::jit::JIT( generator.get(), optimize ) );

		if( !commandLine.count( "emit" ) ) {
			retval = jit->Execute();
		} else {
			retval = jit->Emit();
		}

	} catch( boost::exception& e ) {
        if( std::string const *message = boost::get_error_info<exo::exceptions::ExceptionString>(e) ) {
        	EXO_LOG( fatal, *message );
        } else {
        	EXO_LOG( fatal, boost::diagnostic_information( e ) );
        }

		retval = 1;
	}  catch( std::exception& e ) {
		EXO_LOG( fatal, e.what() );
		retval = 1;
	} catch( ... ) {
		EXO_LOG( fatal, "Unknown exception caught." );
		retval = 1;
	}

	exo::init::Init::Shutdown();

	exit( retval );
}
