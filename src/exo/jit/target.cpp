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

#include "exo/jit/target.h"

namespace exo
{
	namespace jit
	{
		Target::Target( std::string arch, std::string cpu, int optimizeLvl )
		{
			llvm::Triple targetTriple( llvm::Triple::normalize( arch ) );
			const llvm::Target* target;
			std::string errorMsg;

			// FIXME: quite hacky
			if( targetTriple.getOSAndEnvironmentName().size() ) {
				target = llvm::TargetRegistry::lookupTarget( targetTriple.str(), errorMsg );
			} else {
				target = llvm::TargetRegistry::lookupTarget( arch, targetTriple, errorMsg );
			}

			if( target == nullptr ) {
				EXO_THROW_EXCEPTION( LLVM, errorMsg );
			}

			llvm::SubtargetFeatures subtargetFeatures;
			subtargetFeatures.getDefaultSubtargetFeatures( llvm::Triple( cpu ) );

			/*
			if( llvm::sys::getHostCPUFeatures( hostFeatures ) ) {
				for( const auto &hf : hostFeatures ) {
					subtargetFeatures.AddFeature( std::string( hf.second ? "+" : "-" ).append( hf.first() ) );
				}
			}
			*/

			llvm::TargetOptions targetOptions;

			if( optimizeLvl < 0 || optimizeLvl > 3 ) {
				codeGenOpt = llvm::CodeGenOpt::None;
			} else {
				if( optimizeLvl == 0 ) {
					codeGenOpt = llvm::CodeGenOpt::None;
				} else if( optimizeLvl == 1 ) {
					codeGenOpt = llvm::CodeGenOpt::Less;
				} else if( optimizeLvl == 2 ) {
					codeGenOpt = llvm::CodeGenOpt::Default;
				} else if( optimizeLvl == 3 ) {
					codeGenOpt = llvm::CodeGenOpt::Aggressive;
				}
			}

			targetMachine = std::unique_ptr<llvm::TargetMachine>( target->createTargetMachine( targetTriple.str(), cpu, subtargetFeatures.getString(), targetOptions, llvm::Reloc::Default, llvm::CodeModel::JITDefault, codeGenOpt ) );
		}

		Target::~Target()
		{
		}

		std::string Target::getName()
		{
			return( targetMachine->getTargetTriple().str() );
		}

		std::unique_ptr<llvm::Module> Target::createModule( std::string moduleName )
		{
			std::unique_ptr<llvm::Module> module = std::make_unique<llvm::Module>( moduleName, llvm::getGlobalContext());
			module->setTargetTriple( targetMachine->getTargetTriple().str() );
			module->setDataLayout( targetMachine->createDataLayout() );
			return( module );
		}
	}
}
