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
		class Segfault : public virtual std::exception, public virtual boost::exception {};

		class UnknownToken : public virtual std::exception, public virtual boost::exception
		{
			public:
				std::string token;

				UnknownToken( std::string t )
				{
					token = t;
				}

				virtual const char* what() const throw()
				{
					std::stringstream msg;
					msg << "Unknown token \"" << token << "\"";
					return( msg.str().c_str() );
				}
		};

		class UnexpectedToken : public virtual std::exception, public virtual boost::exception
		{
			public:
				std::string tokenName;
				int lineNr;
				int columnNr;

				UnexpectedToken( std::string tName, int l, int c )
				{
					tokenName = tName;
					lineNr = l;
					columnNr = c;
				}

				virtual const char* what() const throw()
				{
					std::stringstream msg;
					msg << "Unexpected \"" << tokenName << "\" on " << lineNr << ":" << columnNr;
					return( msg.str().c_str() );
				}
		};

		class StackOverflow : public virtual std::exception, public virtual boost::exception
		{
			public:
				virtual const char* what() const throw()
				{
					return( "Stack overflown" );
				}
		};

		class UnknownVar : public virtual std::exception, public virtual boost::exception
		{
			public:
				std::string varName;

				UnknownVar( std::string vName )
				{
					varName = vName;
				}

				virtual const char* what() const throw()
				{
					std::stringstream msg;
					msg << "Unknown variable $" << varName;
					return( msg.str().c_str() );
				}
		};

		class LLVMException : public virtual std::exception, public virtual boost::exception
		{
			public:
				std::string message;

				LLVMException( std::string m )
				{
					message = m;
				}

				virtual const char* what() const throw()
				{
					std::stringstream msg;
					msg << "LLVM Exception \"" << message << "\"";
					return( msg.str().c_str() );
				}
		};
	}
}

#endif /* EXCEPTIONS_H_ */
