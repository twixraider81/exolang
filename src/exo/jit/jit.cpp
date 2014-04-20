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
#include "exo/jit/context.h"

namespace exo
{
	namespace jit
	{
		JIT::JIT( exo::jit::Context* c )
		{
			context = c;
			std::string errorMsg;

			llvm::EngineBuilder builder( context->module );
			builder.setEngineKind( llvm::EngineKind::JIT );
			builder.setOptLevel( llvm::CodeGenOpt::Default );
			builder.setErrorStr( &errorMsg );

			engine = builder.setUseMCJIT( true ).create();

			if( !engine ) {
				BOOST_THROW_EXCEPTION( exo::exceptions::LLVM( errorMsg ) );
			}

#ifdef EXO_TRACE
			TRACE( "Immediate LLVM IR" );
			context->module->dump();
#endif

			llvm::TargetMachine* target = builder.selectTarget();
			llvm::PassRegistry &registry = *llvm::PassRegistry::getPassRegistry();
			llvm::initializeScalarOpts( registry );

			fpm = new llvm::FunctionPassManager( context->module );
			fpm->add( llvm::createVerifierPass( llvm::PrintMessageAction ) );
			target->addAnalysisPasses( *fpm );
			fpm->add( new llvm::TargetLibraryInfo( llvm::Triple( context->module->getTargetTriple() ) ) );
			fpm->add( new llvm::DataLayout( context->module ) );
			fpm->add( llvm::createBasicAliasAnalysisPass() );
			fpm->add( llvm::createLICMPass() );
			fpm->add( llvm::createGVNPass() );
			fpm->add( llvm::createPromoteMemoryToRegisterPass() );
			fpm->add( llvm::createLoopVectorizePass() );
			fpm->add( llvm::createEarlyCSEPass() );
			fpm->add( llvm::createInstructionCombiningPass() );
			fpm->add( llvm::createCFGSimplificationPass() );

			fpm->run( *context->entry );

			engine->finalizeObject();

#ifdef EXO_TRACE
				TRACE( "Final LLVM IR" );
				context->module->dump();
#endif
		}

		JIT::~JIT()
		{
			delete engine;
			delete fpm;
		}
	}
}
