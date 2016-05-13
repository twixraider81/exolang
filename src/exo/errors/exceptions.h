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

#define EXO_THROW_MSG(m)			BOOST_THROW_EXCEPTION( exo::exceptions::SafeException() << exo::exceptions::Message( m ) )
#define EXO_THROW(e)				BOOST_THROW_EXCEPTION( exo::exceptions::e )

namespace exo
{
	namespace exceptions
	{
		/**
		 * Structure to a generic exception message
		 */
		typedef boost::error_info<struct tag_error_description, std::string> Message;

		/**
		 * Structure to hold a parser token name
		 */
		typedef boost::error_info<struct tag_error_description, std::string> TokenName;

		/**
		 * Structure to hold a variable name
		 */
		typedef boost::error_info<struct tag_error_description, std::string> VariableName;

		/**
		 * Structure to hold a function/method name
		 */
		typedef boost::error_info<struct tag_error_description, std::string> FunctionName;

		/**
		 * Structure to hold a class name
		 */
		typedef boost::error_info<struct tag_error_description, std::string> ClassName;

		/**
		 * Structure to hold a property name
		 */
		typedef boost::error_info<struct tag_error_description, std::string> PropertyName;

		/**
		 * Structure to hold a ressouce name
		 */
		typedef boost::error_info<struct tag_error_description, std::string> RessouceName;

		/**
		 * Generic unspecified safe exception
		 */
		struct SafeException : public virtual boost::exception, public virtual std::exception
		{
			public: virtual const char* what() const noexcept;
		};

		/**
		 * Generic unspecified unsafe (may throw bad_alloc or not have all type_infos) exception
		 */
		struct UnsafeException : public virtual boost::exception, public virtual std::exception
		{
			protected: std::string msg = "";
			public: virtual const char* what();
		};

		/**
		 * Exception that will be thrown via our segmentation fault handler
		 */
		struct Segfault : public virtual SafeException
		{
			public: virtual const char* what() const noexcept;
		};

		/**
		 * Exception that will be thrown from lemon/parse in case the stack was overblown
		 */
		struct StackOverflow : public virtual SafeException
		{
			public: virtual const char* what() const noexcept;
		};

		/**
		 * Exception that will be thrown from quex/lexer
		 */
		struct UnknownToken : public virtual UnsafeException
		{
			public: virtual const char* what();
		};

		/**
		 * Exception that will be thrown from lemon/parse in case it received something unexpected. a.k.a. syntax error
		 */
		struct UnexpectedToken : public virtual UnsafeException
		{
			public: virtual const char* what();
		};

		/**
		 * Exception that will be thrown if we try to build an unknown/unsupported AST node
		 */
		struct UnexpectedNode : public virtual SafeException
		{
			public: virtual const char* what() const noexcept;
		};

		/**
		 * Exception that will be thrown in case the IR generator stumbles across an undefined variable.
		 */
		struct UnknownVar : public virtual UnsafeException
		{
			public: virtual const char* what();
		};

		/**
		 * Exception that will be thrown in case the IR generator stumbles across an undefined function.
		 */
		struct UnknownFunction : public virtual UnsafeException
		{
			public: virtual const char* what();
		};

		/**
		 * Exception that will be thrown in case the IR generator stumbles across an undefined struct.
		 */
		struct UnknownClass : public virtual UnsafeException
		{
			public: virtual const char* what();
		};

		/**
		 * Exception that will be thrown in case the IR generator stumbles across an undefined struct.
		 */
		struct UnknownProperty : public virtual UnsafeException
		{
			public: virtual const char* what();
		};

		/**
		 * Exception that will be thrown in case of an unknown primtive
		 */
		struct UnknownPrimitive : public virtual SafeException
		{
			public: virtual const char* what() const noexcept;
		};

		/**
		 * Exception that will be thrown in case the IR generator stumbles across an invalid function call.
		 */
		struct InvalidCall : public virtual UnsafeException
		{
			public: virtual const char* what();
		};

		/**
		 * Exception that will be thrown in case the IR generator stumbles across an undefined method.
		 */
		struct InvalidMethod : public virtual UnsafeException
		{
			public: virtual const char* what();
		};

		/**
		 * Exception that will be thrown in case the IR generator stumbles across an invalid function call.
		 */
		struct ArgumentsMismatch : public virtual UnsafeException
		{
			public: virtual const char* what();
		};

		/**
		 * Exception that will be thrown in case the IR generator stumbles across an invalid operator/operation.
		 */
		struct InvalidOp : public virtual SafeException
		{
			public: virtual const char* what() const noexcept;
		};

		/**
		 * Exception that will be thrown in case the IR generator stumbles across an invalid expression.
		 */
		struct InvalidExpr : public virtual SafeException
		{
			public: virtual const char* what() const noexcept;
		};

		/**
		 * Exception that will be thrown in case the IR generator stumbles across an invalid break.
		 */
		struct InvalidBreak : public virtual SafeException
		{
			public: virtual const char* what() const noexcept;
		};

		/**
		 * Exception that will be thrown in case we ran out of memory
		 */
		struct OutOfMemory : public virtual SafeException
		{
			public: virtual const char* what() const noexcept;
		};

		/**
		 * Exception that will be thrown if something is expected but not found
		 */
		struct NotFound : public virtual UnsafeException
		{
			public: virtual const char* what();
		};
	}
}

#endif /* EXCEPTIONS_H_ */
