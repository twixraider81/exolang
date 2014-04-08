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
#include "exo/ast/ast.h"

namespace exo
{
	namespace ast
	{
		namespace nodes
		{
			ValInt::ValInt( long long lVal )
			{
				TRACESECTION( "AST", "creating integer value:" << lVal );
				value = lVal;
			}

			ValInt::ValInt( std::string lVal )
			{
				value = atoi( lVal.c_str() );
				TRACESECTION( "AST", "creating integer value:" << value );
			}

			llvm::Value* ValInt::Generate( exo::ast::Context* context )
			{
				TRACESECTION( "IR", "generating int:" << value );
				return( llvm::ConstantInt::get( *(context->context), llvm::APInt( 64, value ) ) );
			}
		}
	}
}
