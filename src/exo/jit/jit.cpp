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

#include "exo/jit/jit.h"
#include "exo/jit/codegen.h"

namespace exo
{
	namespace jit
	{
		JIT::JIT( boost::shared_ptr<exo::jit::Codegen> g, int optimize )
		{
			generator = g;

			std::string buffer;
			llvm::raw_string_ostream ostream( buffer );
			llvm::legacy::FunctionPassManager*	fpm;

			// takes ownership of module, thus we can't delete it ourself
			llvm::EngineBuilder builder( generator->module );
			builder.setEngineKind( llvm::EngineKind::JIT );

			// a bit ugly
			if( optimize < 0 || optimize > 3 ) {
				BOOST_LOG_TRIVIAL(warning) << "Invalid optimization level.";
			} else {
				if( optimize == 0 ) {
					builder.setOptLevel( llvm::CodeGenOpt::None );
				} else if( optimize == 1 ) {
					builder.setOptLevel( llvm::CodeGenOpt::Less );
				} else if( optimize == 2 ) {
					builder.setOptLevel( llvm::CodeGenOpt::Default );
				} else if( optimize == 3 ) {
					builder.setOptLevel( llvm::CodeGenOpt::Aggressive );
				}
			}

			buffer = "";
			builder.setErrorStr( &buffer );
			builder.setUseMCJIT( true );
			engine = builder.create();

			if( !engine ) {
				EXO_THROW_EXCEPTION( LLVM, buffer );
			}

			/*
			 * TODO: The docs for this whole stuff related to passes seem wildly out of date. Should use the "new" ModulePassManager, FunctionPassManager, AnalysisPassManager
			 */
			llvm::TargetMachine* target = builder.selectTarget();
			// what? http://llvm.org/docs/doxygen/html/InitializePasses_8h_source.html
			llvm::PassRegistry &registry = *llvm::PassRegistry::getPassRegistry();
			llvm::initializeScalarOpts( registry );
			llvm::initializeTransformUtils( registry );

			fpm = new llvm::legacy::FunctionPassManager( generator->module );
			fpm->add( new llvm::TargetLibraryInfo( llvm::Triple( generator->module->getTargetTriple() ) ) );
			fpm->add( new llvm::DataLayout( generator->module ) );
			target->addAnalysisPasses( *fpm );

			fpm->add( llvm::createBasicAliasAnalysisPass() );

			fpm->add( llvm::createPromoteMemoryToRegisterPass() );
			fpm->add( llvm::createLICMPass() );
			fpm->add( llvm::createLoopVectorizePass() );
			fpm->add( llvm::createEarlyCSEPass() );

			fpm->add( llvm::createInstructionCombiningPass() );
			fpm->add( llvm::createReassociatePass() );
			fpm->add( llvm::createGVNPass() );

			fpm->add( llvm::createCFGSimplificationPass() );

			buffer = "";
			generator->module->print( ostream, NULL );
			BOOST_LOG_TRIVIAL(trace) << "Intermediate LLVM IR" << buffer;

			fpm->run( *generator->entry );
			engine->finalizeObject();
			delete fpm;

			buffer = "";
			generator->module->print( ostream, NULL );
			BOOST_LOG_TRIVIAL(trace) << "Final LLVM IR" << buffer;

			delete target;
		}

		JIT::~JIT()
		{
			delete engine;
		}

		int JIT::Execute()
		{
			BOOST_LOG_TRIVIAL(trace) << "Running main.";
			int( *jitMain )() = (int(*)())engine->getFunctionAddress( "main" );
			int retval = jitMain();
			BOOST_LOG_TRIVIAL(trace) << "Finished.";
			return( retval );
		}

		int JIT::Emit( std::string fileName )
		{
			boost::filesystem::path path( fileName );
			path.replace_extension( ".ll" );

			BOOST_LOG_TRIVIAL(trace) << "Emmiting LLVM IR as \"" << path.native() << "\"";

			std::string errorMsg;
			llvm::raw_fd_ostream fstream( path.c_str(), errorMsg, llvm::sys::fs::F_None );

			if( fstream.has_error() ) {
				BOOST_LOG_TRIVIAL(error) << errorMsg;
				return( -1 );
			}

			generator->module->print( fstream, NULL );
			return( 1 );
		}
	}
}
