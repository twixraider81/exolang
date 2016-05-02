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
		JIT::JIT( exo::jit::Codegen* g, int optimize ) : generator( g ), optLevel( optimize )
		{
			passManager.add( new llvm::TargetLibraryInfoWrapperPass( llvm::Triple( generator->module->getTargetTriple() )  ));
			passManager.add( llvm::createAlwaysInlinerPass() );
			passManager.add( llvm::createPromoteMemoryToRegisterPass() );
			passManager.add( llvm::createLICMPass() );
			passManager.add( llvm::createLoopVectorizePass() );
			passManager.add( llvm::createEarlyCSEPass() );
			passManager.add( llvm::createInstructionCombiningPass() );
			passManager.add( llvm::createReassociatePass() );
			passManager.add( llvm::createGVNPass() );
			passManager.add( llvm::createCFGSimplificationPass() );
			passManager.run( *generator->module );
		}

		JIT::~JIT()
		{
		}

		int JIT::Execute()
		{
			llvm::ExecutionEngine*	engine;

			// careful, this transfers module ownership
			llvm::EngineBuilder builder( (std::move( generator->module )) );
			builder.setEngineKind( llvm::EngineKind::JIT );

			// a bit ugly
			if( optLevel < 0 || optLevel > 3 ) {
				BOOST_LOG_TRIVIAL(warning) << "Invalid optimization level.";
				builder.setOptLevel( llvm::CodeGenOpt::None );
			} else {
				if( optLevel == 0 ) {
					builder.setOptLevel( llvm::CodeGenOpt::None );
				} else if( optLevel == 1 ) {
					builder.setOptLevel( llvm::CodeGenOpt::Less );
				} else if( optLevel == 2 ) {
					builder.setOptLevel( llvm::CodeGenOpt::Default );
				} else if( optLevel == 3 ) {
					builder.setOptLevel( llvm::CodeGenOpt::Aggressive );
				}
			}

			std::string buffer = "";
			builder.setErrorStr( &buffer );
			engine = builder.create();

			if( !engine ) {
				EXO_THROW_EXCEPTION( LLVM, buffer );
				return( -1 );
			}

			BOOST_LOG_TRIVIAL(trace) << "Running main.";
			int( *jitMain )() = (int(*)())engine->getFunctionAddress( "main" );
			int retval = jitMain();
			BOOST_LOG_TRIVIAL(trace) << "Finished.";
			return( retval );
		}

		int JIT::Emit()
		{
			BOOST_LOG_TRIVIAL(trace) << "Emitting LLVM IR.";

			std::string buffer;
			llvm::raw_string_ostream bStream( buffer );

			generator->module->print( bStream, NULL );

			std::cout << bStream.str();

			return( 1 );
		}
	}
}
