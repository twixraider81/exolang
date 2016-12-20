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
		JIT::JIT( std::unique_ptr<llvm::Module> m, std::shared_ptr<Target> t, std::set<std::string>	i ) :
			module( std::move( m ) ),
			target( t ),
			imports( i )
		{
			std::string buffer;
			llvm::raw_string_ostream bStream( buffer );

			if( llvm::verifyModule( *module, &bStream ) ) {
#ifdef EXO_DEBUG
				std::string moduleContents;
				llvm::raw_string_ostream mStream( moduleContents );
				module->print( mStream, nullptr, false, true );
				EXO_DEBUG_LOG( trace, mStream.str() );
#endif
				EXO_THROW_MSG( bStream.str() );
			}


			llvm::legacy::FunctionPassManager fpassManager( module.get() );

			passManager.add( llvm::createTargetTransformInfoWrapperPass( target->targetMachine->getTargetIRAnalysis() ) );
			fpassManager.add( llvm::createTargetTransformInfoWrapperPass( target->targetMachine->getTargetIRAnalysis() ) );

			llvm::PassManagerBuilder builder;
			builder.OptLevel = target->codeGenOpt; // FIXME: this should be uint
			builder.SizeLevel = 0;
			builder.Inliner = llvm::createFunctionInliningPass();
			builder.LoopVectorize = true;
			builder.SLPVectorize = true;

			builder.populateFunctionPassManager( fpassManager );
			builder.populateModulePassManager( passManager );

			fpassManager.doInitialization();
			for( auto &f : *module ) {
				fpassManager.run( f );
			}
			fpassManager.doFinalization();
		}

		JIT::~JIT()
		{
		}

		int JIT::Execute()
		{
			return( Execute( module->getModuleIdentifier() ) );
		}

		int JIT::Execute( std::string fName )
		{
			std::string buffer;

			if( !target->targetMachine->getTarget().hasJIT() ) {
				EXO_THROW_MSG( "Unable to create JIT." );
			}

			passManager.run( *module );

			llvm::Function* entry = module->getFunction( fName );

			if( entry == nullptr ) {
				EXO_THROW( InvalidCall() << exo::exceptions::FunctionName( fName ) );
			}


			llvm::RTDyldMemoryManager* memMgr( new llvm::SectionMemoryManager() );

			// careful, ownership is transferred from here on
			llvm::EngineBuilder builder( std::move( module ) );
			builder.setErrorStr( &buffer );
			builder.setEngineKind( llvm::EngineKind::JIT );
			builder.setMCJITMemoryManager( std::unique_ptr<llvm::RTDyldMemoryManager>( memMgr ) );

			std::unique_ptr<llvm::ExecutionEngine> jit( builder.create( target->targetMachine.release() ) );

			if( jit == nullptr ) {
				EXO_THROW_MSG( buffer );
			}

			/*
			jit->DisableSymbolSearching( false );
			EXO_DEBUG_LOG( trace, "Symbol searching: " << ( jit->isSymbolSearchingDisabled() ? "disabled" : "enabled" ) << "" );
			jit->DisableGVCompilation ( false );
			EXO_DEBUG_LOG( trace, "Symbol allocation: " << ( jit->isGVCompilationDisabled() ? "disabled" : "enabled" ) << "" );
			*/

			std::string error;
			for( auto &import : imports ) {
				EXO_DEBUG_LOG( trace, "Importing \"" << import << "\"" );

				if( llvm::sys::DynamicLibrary::LoadLibraryPermanently( import.c_str(), &error ) ) {
					EXO_THROW_MSG( error );
				}
			}

			jit->RegisterJITEventListener( llvm::JITEventListener::createOProfileJITEventListener() );
			jit->RegisterJITEventListener( llvm::JITEventListener::createIntelJITEventListener() );

			jit->DisableLazyCompilation( false );

			jit->finalizeObject();

			jit->runStaticConstructorsDestructors( false );

			EXO_LOG( trace, "Executing \"" + fName + "\"." );

			(void)jit->getFunctionAddress( fName );

			static_cast<llvm::SectionMemoryManager*>(memMgr)->invalidateInstructionCache();

			std::vector<llvm::GenericValue> arguments;
			llvm::GenericValue retval = jit->runFunction( entry, arguments );

			EXO_LOG( trace, "Finished." );

			jit->runStaticConstructorsDestructors( true );

			return( retval.IntVal.getZExtValue() );
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
