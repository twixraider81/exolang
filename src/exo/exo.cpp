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
#include "exo/ast/tree.h"
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
 */
int main( int argc, char **argv )
{
	int severity, optimize, retval;
	std::string target = llvm::sys::getProcessTriple();

	// build optionlist
	boost::program_options::options_description availOptions( "Options" );
	availOptions.add_options()
		( "help,h",			"Show this usage/help" )
		( "input,i",		boost::program_options::value<std::string>(),						"Parse file" )
		( "emit,e",			boost::program_options::value<std::string>(),						"Emit LLVM IR file & don't run script" )
		( "log-severity,s", boost::program_options::value<int>(&severity)->default_value(4),	"Set log severity; 1 = trace, 2 = debug, 3 = info, 4 = warning, 5 = error, 6 = fatal" )
		( "optimize,o", 	boost::program_options::value<int>(&optimize)->default_value(2),	"Set optimization level; 0 = none, 1 = less, 2 = default, 3 = all" )
		( "version,v",		"Show version" )
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
		std::cout << "version: " << EXO_VERSION << std::endl;
		std::cout << "host cpu: " << llvm::sys::getHostCPUName() << std::endl;
		std::cout << "jit target: " << target << std::endl;

#ifndef EXO_GC_DISABLE
		unsigned gcVersion = GC_get_version();
		std::cout << "libgc version: " << ( gcVersion >> 16 ) << "." << ( ( gcVersion >> 8 ) & 0xFF ) << "." << ( gcVersion & 0xF ) << std::endl;
#else
		std::cout << "libgc: disabled" << std::endl;
#endif
		return( 0 );
	}


	// we are running, command line is parsed
	if( !exo::init::Init::Startup( severity ) ) {
		BOOST_LOG_TRIVIAL(fatal) << "Unable to complete initialization. exiting...";
		return( -1 );
	}

	try {
		boost::shared_ptr<exo::ast::Tree> ast( new exo::ast::Tree() );
		// needed for internal constant __TARGET__
		ast->targetMachine = target;

		if( commandLine.count( "input" ) ) {
			ast->Parse( commandLine["input"].as<std::string>() );
		} else {
			ast->Parse( std::cin );
		}

		// codegen takes ownership of the ast nodes, this will however leak horribly on unproper shutdown
		boost::shared_ptr<exo::jit::Codegen> generator( new exo::jit::Codegen( "main" ) );
		generator->Generate( ast );

		boost::scoped_ptr<exo::jit::JIT> jit( new exo::jit::JIT( generator, optimize ) );

		if( !commandLine.count( "emit" ) ) {
			retval = jit->Execute();
		} else {
			retval = jit->Emit( commandLine["emit"].as<std::string>() );
		}

	} catch( boost::exception& e ) {
		BOOST_LOG_TRIVIAL(fatal) << boost::diagnostic_information( e );
		retval = -1;
	}  catch( std::exception& e ) {
		BOOST_LOG_TRIVIAL(fatal) << e.what();
		retval = -1;
	} catch( ... ) {
		BOOST_LOG_TRIVIAL(fatal) << "Unknown exception caught.";
		retval = -1;
	}

	exo::init::Init::Shutdown();

	return( retval );
}
