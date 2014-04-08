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

#ifndef TYPES_H_
#define TYPES_H_

namespace exo
{
	namespace jit
	{
		namespace types
		{
			class Type : public virtual gc
			{
				public:
					llvm::Value* value;
					llvm::LLVMContext* context;
					llvm::Type* type;

					Type( llvm::LLVMContext* c );
					virtual ~Type() {};

					/**
					 * Performs an addition with the operand and returns the result
					 */
					virtual Type* opAdd( Type* rhs );

					/**
					 * Performs a subtraction with the operand and returns the result
					 */
					virtual Type* opSub( Type* rhs );

					/**
					 * Performs a multiplication with the operand and returns the result
					 */
					virtual Type* opMul( Type* rhs );

					/**
					 * Performs a division with the operand and returns the result
					 */
					virtual Type* opDiv( Type* rhs );

					/**
					 * Performs a concatenation with the operand and returns the result
					 */
					virtual Type* opConCat( Type* rhs );
			};

			class NullType : public virtual Type
			{
			};

			class ScalarType : public virtual Type
			{
			};

			class BooleanType : public virtual ScalarType
			{
				public:
					BooleanType( llvm::LLVMContext* c, bool bVal );
			};

			class IntegerType : public virtual ScalarType
			{
				public:
					IntegerType( llvm::LLVMContext* c, long long lVal );
			};

			class FloatType : public virtual ScalarType
			{
				public:
					FloatType( llvm::LLVMContext* c, double dVal );
			};

			class StringType : public virtual ScalarType
			{
			};

			class CallableType : public virtual Type
			{
			};

			class FunctionType : public virtual CallableType
			{
			};

			class MethodType : public virtual CallableType
			{
			};

			class ClassType : public virtual Type
			{
			};
		}
	}
}

#endif /* TYPES_H_ */
