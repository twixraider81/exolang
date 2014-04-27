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

#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

/**
 * A macro to enable easy throwing of boost::exceptions with a customized exception message
 */
#define EXO_THROW_EXCEPTION(e,m) BOOST_THROW_EXCEPTION( exo::exceptions::e() << exo::exceptions::ExceptionString( m ) )

namespace exo
{
	namespace exceptions
	{
		/**
		 * Structure to hold the customized exception message
		 */
		typedef boost::error_info<struct tag_error_description, std::string> ExceptionString;

		/**
		 * Base exception that will be thrown, holds what
		 */
		class Exception : public virtual boost::exception, public virtual std::exception {};

		/**
		 * Exception that will be thrown via our segmentation fault handler
		 */
		class Segfault : public virtual Exception {};

		/**
		 * Exception that will be thrown from lemon/parse in case the stack was overblown
		 */
		class StackOverflow : public virtual Exception {};

		/**
		 * Exception that will be thrown from quex/lexer
		 */
		class UnknownToken : public virtual Exception {};

		/**
		 * Exception that will be thrown from lemon/parse in case it received something unexpected. a.k.a. syntax error
		 */
		class UnexpectedToken : public virtual Exception {};

		/**
		 * Exception that will be thrown if we try to build an unknown/unsupported AST node
		 */
		class UnexpectedNode : public virtual Exception {};

		/**
		 * Exception that will be thrown in case the IR generator stumbles across an undefined variable.
		 */
		class UnknownVar : public virtual Exception {};

		/**
		 * Exception that will be thrown in case the IR generator stumbles across an unknown operator.
		 */
		class UnknownBinaryOp : public virtual Exception {};

		/**
		 * Exception that will be thrown in case the IR generator stumbles across an undefined function.
		 */
		class UnknownFunction : public virtual Exception {};

		/**
		 * Exception that will be thrown in case the IR generator stumbles across an invalid function call.
		 */
		class InvalidCall : public virtual Exception {};

		/**
		 * Exception that will be thrown in case we ran out of memory
		 */
		class OutOfMemory : public virtual Exception {};

		/**
		 * Exception reporting a generic LLVM error message
		 */
		class LLVM: public virtual Exception {};
	}
}

#endif /* EXCEPTIONS_H_ */
