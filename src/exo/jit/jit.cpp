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
		JIT::JIT( exo::jit::Codegen* g )
		{
			generator = g;
			std::string errorMsg;

			// takes ownership of module, thus we can't delete it ourself
			llvm::EngineBuilder builder( generator->module );
			builder.setEngineKind( llvm::EngineKind::JIT );
			builder.setOptLevel( llvm::CodeGenOpt::Default );
			builder.setErrorStr( &errorMsg );

			engine = builder.setUseMCJIT( true ).create();

			if( !engine ) {
				BOOST_THROW_EXCEPTION( exo::exceptions::LLVM( errorMsg ) );
			}

#ifdef EXO_TRACE
			TRACE( "Intermediate LLVM IR" );
			generator->module->dump();
#endif

			llvm::TargetMachine* target = builder.selectTarget();
			llvm::PassRegistry &registry = *llvm::PassRegistry::getPassRegistry();
			llvm::initializeScalarOpts( registry );

			fpm = new llvm::FunctionPassManager( generator->module );
#ifdef EXO_DEBUG
			fpm->add( llvm::createVerifierPass( llvm::PrintMessageAction ) );
#endif
			target->addAnalysisPasses( *fpm );
			fpm->add( new llvm::TargetLibraryInfo( llvm::Triple( generator->module->getTargetTriple() ) ) );
			fpm->add( new llvm::DataLayout( generator->module ) );

			// Stateless Alias Analysis
			fpm->add( llvm::createBasicAliasAnalysisPass() );
			// Eliminate redundant values and load
			fpm->add( llvm::createGVNPass() );
			// CSE pass - eleminates redudant instructions
			fpm->add( llvm::createEarlyCSEPass() );
			// Promote Memory to Register - eleminate alloca's
			fpm->add( llvm::createPromoteMemoryToRegisterPass() );
			// Optimized out global vars
			fpm->add( llvm::createGlobalOptimizerPass() );
			// Eliminate dead stores
			fpm->add( llvm::createDeadStoreEliminationPass() );
			// Loop Invariant Code Motion Pass
			fpm->add( llvm::createLICMPass() );
			// Vectorization passes
			fpm->add( llvm::createLoopVectorizePass() );
			// fpm->add( llvm::createSLPVectorizerPass() );
			// Combine instructions...
			fpm->add( llvm::createInstructionCombiningPass() );
			// Inline functions
			fpm->add( llvm::createFunctionInliningPass() );
			// Combine blocks, simplyfy control flow
			fpm->add( llvm::createCFGSimplificationPass() );

			fpm->run( *generator->entry );
			engine->finalizeObject();

#ifdef EXO_TRACE
				TRACE( "Final LLVM IR" );
				generator->module->dump();
#endif
		}

		JIT::~JIT()
		{
			delete engine;
			delete fpm;
		}

		void JIT::Execute()
		{
			uint64_t main = engine->getFunctionAddress( "main" );
			void( *pMain )() = (void(*)())main;
		}
	}
}
