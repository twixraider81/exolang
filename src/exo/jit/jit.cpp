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
				EXO_THROW_MSG( "Unable to create JIT." );
			}

			passManager.run( *module );
			if( llvm::verifyModule( *module, &bStream ) ) {
#ifdef EXO_DEBUG
				std::string moduleContents;
				llvm::raw_string_ostream mStream( moduleContents );
				module->print( mStream, nullptr, false, true );
				EXO_DEBUG_LOG( trace, mStream.str() );
#endif
				EXO_THROW_MSG( bStream.str() );
			}

			// careful, this transfers module ownership
			llvm::EngineBuilder builder( std::move(module) );
			builder.setEngineKind( llvm::EngineKind::JIT );
			builder.setErrorStr( &buffer );

			llvm::ExecutionEngine* jit = builder.create( target->targetMachine.get() );

			if( jit == nullptr ) {
				EXO_THROW_MSG( buffer );
			}

			jit->finalizeObject();

			EXO_LOG( trace, "Executing \"" + fName + "\"." );
			int( *entry )() = (int(*)())jit->getFunctionAddress( fName );
			int retval = entry();
			EXO_LOG( trace, "Finished." );

			return( retval );
		}

		// llvm streams are pure evil
		int JIT::Emit( int type, std::string fileName )
		{
			llvm::SmallString<128> buffer;
			std::unique_ptr<llvm::raw_svector_ostream> bStream = std::make_unique<llvm::raw_svector_ostream>( buffer );
			llvm::raw_pwrite_stream* oStream = bStream.get();
			std::string extension;

			switch( type ) {
				case 3: // emit bc
					extension = ".bc";
					llvm::WriteBitcodeToFile( module.get(), *oStream );
				break;

				case 2: // emit obj
				case 1: // emit asm
					target->targetMachine->Options.MCOptions.AsmVerbose = true;

					llvm::TargetMachine::CodeGenFileType fType;
					if( type == 1 ) {
						fType = llvm::TargetMachine::CodeGenFileType::CGFT_AssemblyFile;
						extension = ".s";
					} else if( type == 2  ) {
						fType = llvm::TargetMachine::CodeGenFileType::CGFT_ObjectFile;
						extension = ".obj";
					}

					if( target->targetMachine->addPassesToEmitFile( passManager, *oStream, fType, true ) ) {
						EXO_THROW_MSG( "Unable to assemble source." );
					}

					passManager.run( *module );
				break;

				case 0:
					extension = ".ll";
					module->print( *oStream, nullptr );
				break;

				default:
					EXO_LOG( error, "Unsupported file type." );
					return( 1 );
			}

			if( !fileName.size() ) {
				EXO_LOG( trace, "Emitting." );
				std::cout << bStream->str().str();
			} else {
				boost::filesystem::path absoluteFile = boost::filesystem::absolute( boost::filesystem::path( fileName ) );
				absoluteFile.replace_extension( extension );

				EXO_LOG( trace, "Emitting \"" + absoluteFile.string() + "\"." );

				boost::filesystem::ofstream outFile( absoluteFile );
				outFile << bStream->str().str();
			}

			return( 1 );
		}
	}
}
