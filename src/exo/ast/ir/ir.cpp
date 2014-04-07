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

				engine = llvm::EngineBuilder( context->module ).setErrorStr( &errorMsg ).create();
				fpm = new llvm::FunctionPassManager( context->module );

				if( !engine ) {
					ERRORMSG( errorMsg );
				}

#ifdef EXO_TRACE
				TRACE( "LLVM IR:" );
				llvm::PassManager pm;
				pm.add( llvm::createPrintModulePass( &llvm::outs() ) );
				pm.run( *context->module );
#endif
			}

			IR::~IR()
			{
				delete engine;
				delete fpm;
			}

			/*
			 * TODO: check https://llvm.org/svn/llvm-project/llvm/trunk/include/llvm/LinkAllPasses.h
			 * TODO: check http://llvm.org/docs/Passes.html
			 */
			void IR::Optimize()
			{
				fpm->add( new llvm::DataLayout( *engine->getDataLayout() ) );
				fpm->add( llvm::createBasicAliasAnalysisPass() );
				fpm->add( llvm::createInstructionCombiningPass() );
				fpm->add( llvm::createReassociatePass() );
				fpm->add( llvm::createGVNPass() );
				fpm->add( llvm::createCFGSimplificationPass() );

				fpm->doInitialization();

#ifdef EXO_TRACE
				TRACE( "Optimized LLVM IR:" );
				llvm::PassManager pm;
				pm.add( llvm::createPrintModulePass( &llvm::outs() ) );
				pm.run( *context->module );
#endif
			}
		}
	}
}
