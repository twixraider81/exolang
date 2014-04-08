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

namespace exo
{
	namespace exceptions
	{
		/**
		 * Base exception that will be thrown, holds what
		 */
		class Exception : public virtual std::exception, public virtual boost::exception
		{
			public:
				virtual ~Exception() {};
		};

		/**
		 * Exception that will be thrown via our sigseg handler
		 */
		class Segfault : public virtual Exception {};

		/**
		 * Exception that will be thrown from quex/lexer
		 */
		class UnknownToken : public virtual Exception
		{
			public:
				std::string token;
				UnknownToken( std::string t );
				virtual const char* what() const throw();
		};

		/**
		 * Exception that will be thrown from lemon/parse incase it received something unexpected. a.k.a. syntax error
		 */
		class UnexpectedToken : public virtual Exception
		{
			public:
				std::string tokenName;
				int lineNr;
				int columnNr;
				UnexpectedToken( std::string tName, int l, int c );
				virtual const char* what() const throw();
		};

		/**
		 * Exception that will be thrown from lemon/parse incase the stack was overflown
		 */
		class StackOverflow : public virtual Exception
		{
			public:
				virtual const char* what() const throw();
		};

		/**
		 * Exception that will be incase the IR generator stumbles across an undefined variable.
		 */
		class UnknownVar : public virtual Exception
		{
			public:
				std::string varName;
				UnknownVar( std::string vName );
				virtual const char* what() const throw();
		};

		/**
		 * Exception reporting a generic LLVM error message
		 */
		class LLVM: public virtual Exception
		{
			public:
				std::string message;
				LLVM( std::string m );
				virtual const char* what() const throw();
		};
	}
}

#endif /* EXCEPTIONS_H_ */
