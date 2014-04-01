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

%include {
	#include "exo/exo.h"
	#include "../ast.h"
}

%token_prefix LEMON_TKN_

%default_type { quex::Token* }
%token_type { quex::Token* }

%syntax_error {
DEBUGMSG( "Syntax error, unexpected " << exo::ast::getTokenName( yymajor ) << " on " << TOKEN->line_number() << ":" << TOKEN->column_number() );
}

%stack_overflow {
DEBUGMSG( "Parser stack overflown" );
}

/* a program is build out of statements. */
program ::= statements. { ; }

/* statements are either a single statement or a statement followed by ; and other statements */
statements ::= statement.
statements ::= statement statements.

/* a statement may be an assignment of a variable to an expression */
statement ::= variable(v) ASSIGN expression(e) SEMICOLON. {
DEBUGMSG( "Parsing assignment " << v << " with value " << e );
}


/* a variable may be a $ sign followed by an identifier */
variable ::= DOLLAR IDENTIFIER(i). {
DEBUGMSG( "Parsing variable ($" << i->get_text().c_str() << ")" );
}

/* a number may be an integer or a float */
number ::= INT(i). {
DEBUGMSG( "Parsing integer number (" << i->get_text().c_str() << ")" );
}

number ::= FLOAT(f). {
DEBUGMSG( "Parsing floating number (" << f->get_text().c_str() << ")" );
}


/* an expression may a variable */
expression ::= variable(v). {
DEBUGMSG( "Parsing variable expression (" << v << ")" );
}

/* an expression may be a number */
expression ::= number(n). {
DEBUGMSG( "Parsing number expression (" << n << ")" );
}

/* an expression may be an addition */
expression(a) ::= expression(b) ADD expression(c). {
DEBUGMSG( "Parsing addition expression " << a << "=" << b << "+" << c );
}

/* an expression may be a subtraction */
expression(a) ::= expression(b) SUB expression(c). {
DEBUGMSG( "Parsing subtraction expression " << a << "=" << b << "-" << c );
}

/* an expression may be a multiplication */
expression(a) ::= expression(b) MUL expression(c). {
DEBUGMSG( "Parsing multiplication expression " << a << "=" << b << "*" << c );
}

/* an expression may be a division */
expression(a) ::= expression(b) DIV expression(c). {
DEBUGMSG( "Parsing division expression " << a << "=" << b << "/" << c );
}

/* an expression can be surrounded by angular brackets a variable */
expression ::= ABRACKET_OPEN expression(e) ABRACKET_CLOSE. {
DEBUGMSG( "Parsing bracketed expression (" << e << ")" );
}