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
		JIT::JIT( std::unique_ptr<llvm::Module> m, std::unique_ptr<Target> t ) : module( std::move(m) ), target( std::move(t) )
		{
			passManager.add( new llvm::TargetLibraryInfoWrapperPass(  target->targetMachine->getTargetTriple() ) );
			passManager.add( llvm::createTargetTransformInfoWrapperPass( target->targetMachine->getTargetIRAnalysis() ) );
			//passManager.add( llvm::createAlwaysInlinerPass() );
			//passManager.add( llvm::createPromoteMemoryToRegisterPass() );
			//passManager.add( llvm::createLICMPass() );
			//passManager.add( llvm::createLoopVectorizePass() );
			//passManager.add( llvm::createEarlyCSEPass() );
			//passManager.add( llvm::createInstructionCombiningPass() );
			//passManager.add( llvm::createReassociatePass() );
			//passManager.add( llvm::createGVNPass() );
			//passManager.add( llvm::createCFGSimplificationPass() );

			llvm::legacy::FunctionPassManager fpassManager( module.get() );
			//fpassManager.add( llvm::createVerifierPass() );

			llvm::PassManagerBuilder builder;
			builder.OptLevel = target->codeGenOpt; // FIXME: this should be uint
			builder.SizeLevel = 0;
			builder.Inliner = llvm::createAlwaysInlinerPass();
			builder.LoopVectorize = true;
			//builder.populateFunctionPassManager( fpassManager );
			//builder.populateModulePassManager( passManager );

			fpassManager.add( llvm::createTargetTransformInfoWrapperPass( target->targetMachine->getTargetIRAnalysis() ) );
			fpassManager.doInitialization();
			for( auto &f : *module ) {
				fpassManager.run( f );
			}
			fpassManager.doFinalization();
		}

		JIT::~JIT()
		{
		}

		int JIT::Execute( std::string fName )
		{
			std::string buffer;
			llvm::raw_string_ostream bStream( buffer );

			if( !target->targetMachine->getTarget().hasJIT() ) {
				EXO_THROW_EXCEPTION( LLVM, "Unable to create JIT." );
			}

			passManager.run( *module );
			if( llvm::verifyModule( *module, &bStream ) ) {
#ifdef EXO_DEBUG
				std::string moduleContents;
				llvm::raw_string_ostream mStream( moduleContents );
				module->print( mStream, nullptr, false, true );
				EXO_DEBUG_LOG( trace, mStream.str() );
#endif
				EXO_THROW_EXCEPTION( LLVM, bStream.str() );
			}

			// careful, this transfers module ownership
			llvm::EngineBuilder builder( std::move(module) );
			builder.setEngineKind( llvm::EngineKind::JIT );
			builder.setErrorStr( &buffer );

			llvm::ExecutionEngine* jit = builder.create( target->targetMachine.get() );

			if( jit == nullptr ) {
				EXO_THROW_EXCEPTION( LLVM, buffer );
			}

			jit->finalizeObject();

			EXO_LOG( trace, "Executing \"" + fName + "\"." );
			int( *entry )() = (int(*)())jit->getFunctionAddress( fName );
			int retval = entry();
			EXO_LOG( trace, "Finished." );

			return( retval );
		}

		// FIXME: need rtti here
		int JIT::Emit( int type, std::string fileName )
		{
			llvm::SmallString<128> buffer;
			std::unique_ptr<llvm::raw_svector_ostream> bStream = std::make_unique<llvm::raw_svector_ostream>( buffer );
			llvm::raw_pwrite_stream* oStream = bStream.get();

			/* TODO: if we have an actual file, use raw_fd_ostream
			fStream = std::make_unique<llvm::raw_fd_ostream>( absoluteFile.c_str(), err, llvm::sys::fs::F_None );
			oStream = fStream.get();
			*/

			if( type < 0 || type > 2 ) {
				EXO_LOG( error, "Unsupported file type." );
				return( 1 );
			}

			if( type ) {
				target->targetMachine->Options.MCOptions.AsmVerbose = true;

				llvm::TargetMachine::CodeGenFileType fType;
				if( type == 1 ) {
					fType = llvm::TargetMachine::CodeGenFileType::CGFT_AssemblyFile;
				} else if( type == 2  ) {
					fType = llvm::TargetMachine::CodeGenFileType::CGFT_ObjectFile;
				}

				if( target->targetMachine->addPassesToEmitFile( passManager, *oStream, fType, true ) ) {
					EXO_THROW_EXCEPTION( LLVM, "Unable to assemble source." );
				}

				passManager.run( *module );
			} else {
				module->print( *oStream, nullptr );
			}


			if( !fileName.size() ) {
				EXO_LOG( trace, "Emitting." );
				std::cout << bStream->str().str();
			} else {
				boost::filesystem::path absoluteFile( fileName );
				std::error_code err;

				if( type == 0 ) { // emit llvm ir
					absoluteFile.replace_extension( ".ll" );
				} else if( type == 1 ) { // emit assembly
					absoluteFile.replace_extension( ".s" );
				} else if( type == 2  ) { // emit object
					absoluteFile.replace_extension( ".obj" );
				}

				EXO_LOG( trace, "Emitting \"" + absoluteFile.string() + "\"." );

				boost::filesystem::ofstream outFile( absoluteFile );
				outFile << bStream->str().str();
			}

			return( 1 );
		}
	}
}
