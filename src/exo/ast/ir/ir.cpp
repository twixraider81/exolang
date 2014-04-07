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

#include "exo/types/types.h"
#include "exo/ast/llvm.h"

#include "exo/ast/lexer/lexer"
#include "exo/ast/parser/parser.h"

#include "exo/ast/block.h"
#include "exo/ast/context.h"
#include "exo/ast/nodes/nodes.h"
#include "exo/ast/ir/ir.h"

namespace exo
{
	namespace ast
	{
		namespace ir
		{
			IR::IR( exo::ast::Context* c )
			{
				context = c;
				std::string errorMsg;

				llvm::EngineBuilder builder( context->module );
				builder.setEngineKind( llvm::EngineKind::JIT );
				// TODO: make use of -0x levels - http://llvm.org/docs/doxygen/html/namespacellvm_1_1CodeGenOpt.html
				builder.setOptLevel( llvm::CodeGenOpt::Default );
				builder.setErrorStr( &errorMsg );
				engine = builder.setUseMCJIT( true ).create();

				if( !engine ) {
					BOOST_THROW_EXCEPTION( exo::exceptions::LLVMException( errorMsg ) );
				}

				llvm::TargetMachine* target = builder.selectTarget();
				//llvm::Function* entry = context->module->getFunction( "main" );

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

				//engine->finalizeObject();

#ifdef EXO_TRACE
				TRACE( "LLVM IR" );
				//context->module->dump();
#endif
			}

			IR::~IR()
			{
				//delete engine;
				//delete fpm;
			}

			/*
			 * TODO: check https://llvm.org/svn/llvm-project/llvm/trunk/include/llvm/LinkAllPasses.h
			 * TODO: check http://llvm.org/docs/Passes.html
			 */
			void IR::Optimize()
			{
			}
		}
	}
}
