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
#include "exo/jit/llvm.h"
#include "exo/jit/type/types.h"

namespace exo
{
	namespace jit
	{
		namespace types
		{
			Type::Type( llvm::LLVMContext* c )
			{
				value = NULL;
				context = c;
				type = llvm::Type::getVoidTy( *context );
			};

			Type* Type::opAdd( Type* rhs )
			{
				return( new Type( context ) );
			};

			Type* Type::opSub( Type* rhs )
			{
				return( new Type( context ) );
			};

			Type* Type::opMul( Type* rhs )
			{
				return( new Type( context ) );
			};

			Type* Type::opDiv( Type* rhs )
			{
				return( new Type( context ) );
			};

			Type* Type::opConCat( Type* rhs )
			{
				return( new Type( context ) );
			};
		}
	}
}
