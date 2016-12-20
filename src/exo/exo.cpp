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
#include "exo/jit/target.h"
#include "exo/jit/jit.h"
#include "exo/jit/codegen.h"
#include "exo/init/init.h"

#include <boost/program_options.hpp>

/*
 * TODO: 1. implement type system
 * TODO: 2. register signal handlers in standard library, to i.e. allow signaled program termination
 * TODO: 3. get rid of generateInMem in codegen and use proper lvalue/rvalue semantics for expressions
 * TODO: 4. implement REPL
 */
int main( int argc, char **argv )
{
	// commandline variable store
	int severity, optimizeLvl, retval;
	std::string archName, cpuName, emit, inputFile, emitFile;
	std::vector<std::string> includePaths, libraryPaths, defaultIncludePaths = EXO_INCLUDE_PATHS, defaultLibraryPaths = EXO_LIBRARY_PATHS;

	// llvm native information
	std::string nativeCpu = llvm::sys::getHostCPUName();
	llvm::Triple nativeTriple( llvm::sys::getDefaultTargetTriple() );

	// build optionlist
	boost::program_options::options_description availOptions( "Options" );
	availOptions.add_options()
		( "cpu,c", 			boost::program_options::value<std::string>(&cpuName)->default_value( nativeCpu ),	"Set target cpu" )
		( "emit-llvm,e",	boost::program_options::value<std::string>(&emitFile)->implicit_value(""),	"Emit LLVM IR (to stdout if empty)" )
		( "emit-assembly,S",boost::program_options::value<std::string>(&emitFile)->implicit_value(""),	"Emit target assembly code (as inputfile.s if empty)" )
		( "emit-object,o",	boost::program_options::value<std::string>(&emitFile)->implicit_value(""),	"Emit target object code (as inputfile.obj if empty)" )
		( "emit-bitcode,b",	boost::program_options::value<std::string>(&emitFile)->implicit_value(""),	"Emit LLVM bitcode (as inputfile.bc if empty)" )
		( "help,h",																						"Show this usage/help" )
		( "input-file,i",	boost::program_options::value<std::string>(&inputFile),						"File to parse (and execute if nothing is to be emitted)" )
		( "include-path,I",	boost::program_options::value<std::vector<std::string>>(&includePaths),		"Add include path, can occur multiple" )
		( "library-path,L",	boost::program_options::value<std::vector<std::string>>(&libraryPaths),		"Add library path, can occur multiple" )
		( "log-severity,l", boost::program_options::value<int>(&severity)->default_value(4),			"Set log severity; 1 = trace, 2 = debug, 3 = info, 4 = warning, 5 = error, 6 = fatal" )
		( "optimize,O", 	boost::program_options::value<int>(&optimizeLvl)->default_value(2),			"Set optimization level; 0 = none, 1 = less, 2 = default, 3 = all" )
		( "target,t", 		boost::program_options::value<std::string>(&archName)->default_value( nativeTriple.str() ),	"Set target" )
		( "version,v",																					"Show version and configuration" )
		;

	boost::program_options::positional_options_description positionalOptions;
	positionalOptions.add( "input-file", -1 );

	boost::program_options::variables_map commandLine;
	boost::program_options::store( boost::program_options::command_line_parser( argc, argv ).options( availOptions ).positional( positionalOptions ).run(), commandLine );
	boost::program_options::notify( commandLine );
	includePaths.insert( includePaths.end(), defaultIncludePaths.begin(), defaultIncludePaths.end() );
	libraryPaths.insert( libraryPaths.end(), defaultLibraryPaths.begin(), defaultLibraryPaths.end() );


	// show help & exit
	if( commandLine.count( "help" ) || argc == 1 ) {
		std::cout << availOptions;
		exit( 0 );
	}

	// we are running, command line is parsed
	if( !exo::init::Init::Startup( severity ) ) {
		EXO_LOG( fatal, "Unable to complete initialization." );
		exit( 1 );
	}


	// show version & exit
	if( commandLine.count( "version" ) ) {
		std::cout << "EXO version:\t" << EXO_VERSION << std::endl;
#ifndef EXO_GC_DISABLE
		unsigned gcVersion = GC_get_version();
		std::cout << "LibGC version:\t" << ( gcVersion >> 16 ) << "." << ( ( gcVersion >> 8 ) & 0xFF ) << "." << ( gcVersion & 0xF ) << std::endl;
#else
		std::cout << "LibGC version:\tdisabled" << std::endl;
#endif
		std::cout << "LLVM version:\t" << LLVM_VERSION_STRING << std::endl << std::endl;

		std::cout << "Include path(s):" << std::endl;
		for( auto &p : includePaths ) {
			std::cout << " - " << p << std::endl;
		}

		std::cout << std::endl << "Library path(s):" << std::endl;
		for( auto &p : libraryPaths ) {
			std::cout << " - " << p << std::endl;
		}

		std::cout << std::endl << "LLVM native CPU:\t" << nativeCpu << std::endl;
		std::cout << "LLVM native target:\t" << nativeTriple.str() << std::endl << std::endl << "LLVM supported architectures:" << std::endl;

		for( auto &t : llvm::TargetRegistry::targets() ) {
			std::cout << " - " << t.getName() << "\t";
			if( strlen( t.getName() ) < 5 ) {
				std::cout << "\t";
			}
			std::cout << ": " << t.getShortDescription() << std::endl;
		}

		exit( 0 );
	}

	try {
		if( !commandLine.count( "input-file" ) ) {
			EXO_LOG( fatal, "No input." );
			exit( 1 );
		}

		// extract filename from path/fileinfo
		boost::filesystem::path fileName = boost::filesystem::path( inputFile ).filename();

		// create our target information
		std::shared_ptr<exo::jit::Target> target = std::make_shared<exo::jit::Target>( archName, cpuName, optimizeLvl );

		// create our abstract syntax tree
		std::unique_ptr<exo::ast::Tree> ast = std::make_unique<exo::ast::Tree>( target );

		// generate llvm ir code
		std::unique_ptr<exo::jit::Codegen> generator = std::make_unique<exo::jit::Codegen>( std::move( ast->Parse( inputFile ) ), includePaths, libraryPaths );
		generator->visit( *(ast.get()) );

		// create jit and eventually execute our module
		std::unique_ptr<exo::jit::JIT> jit = std::make_unique<exo::jit::JIT>( std::move( generator->module ), target, generator->imports );

		if( commandLine.count( "emit-llvm" ) ) {
			retval = jit->Emit( 0, emitFile );
		} else if( commandLine.count( "emit-assembly" ) ) {
			if( !emitFile.size() ) {
				emitFile = fileName.string();
			}

			retval = jit->Emit( 1, emitFile );
		} else if( commandLine.count( "emit-object" ) ) {
			if( !emitFile.size() ) {
				emitFile = fileName.string();
			}

			retval = jit->Emit( 2, emitFile );
		} else if( commandLine.count( "emit-bitcode" ) ) {
			if( !emitFile.size() ) {
				emitFile = fileName.string();
			}

			retval = jit->Emit( 3, emitFile );
		} else {
			retval = jit->Execute();
		}
	} catch( exo::exceptions::UnsafeException& e ) {
		try{
			EXO_LOG( fatal, e.what() );
			EXO_DEBUG_LOG( fatal, boost::diagnostic_information( e ) );
		} catch( std::exception& n ) {
			EXO_LOG( fatal, n.what() );
		}
		retval = 1;
	} catch( exo::exceptions::SafeException& e ) {
		EXO_LOG( fatal, e.what() );
		EXO_DEBUG_LOG( fatal, boost::diagnostic_information( e ) );
		retval = 1;
	} catch( boost::exception& e ) {
		EXO_LOG( fatal, boost::diagnostic_information( e ) );
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
