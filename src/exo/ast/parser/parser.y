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

%token_prefix QUEX_TKN_

%default_type { quex::Token* }
%token_type { quex::Token* }
%extra_argument { exo::ast::Tree *ast }

%syntax_error {
DEBUGMSG( "Syntax error, unexpected " << exo::ast::Token::getName( yymajor ) << " on " << TOKEN->line_number() << ":" << TOKEN->column_number() );
}

%stack_overflow {
DEBUGMSG( "Parser stack overflown" );
}

%start_symbol program

/* garbage to get needed quex tokes, err somewhat not cool */
garbage ::= UNINITIALIZED TERMINATION.



/* a program is build out of statements. */
program ::= statements TERMINATION. { ; }


/* statements are either a single statement or a statement followed by ; and other statements */
statements ::= statement.
statements ::= statement statements.


/* a statement may be an declaration of a variable type */
statement ::= type(t) variable(v). {
	ast->addNode( new exo::ast::VariableDeclaration( v, t ) );
}

/* a statement may be an assignment of a variable to an expression */
statement ::= variable(v) ASSIGN expression(e) SEMICOLON. {
	ast->addNode( new exo::ast::VariableAssignment( v, e ) );
}


/* a variable is a $ sign followed by an identifier */
variable ::= LABEL(l).{
DEBUGMSG( "Declaring " << l->get_text().c_str() );
}

/* a number may be an integer or a float */
number ::= INT(i). {
	ast->addNode( new exo::ast::NodeInteger( i ) );
}

number ::= FLOAT(f). {
	ast->addNode( new exo::ast::NodeFloat( f ) );
}


/* a type may be an integer */
type ::= TYPE_INT.


/* an expression may be a number */
expression ::= number(n). 

/* an expression may be an addition */
expression(a) ::= expression(b) ADD expression(c). 

/* an expression may be a subtraction */
expression(a) ::= expression(b) SUB expression(c). 

/* an expression may be a multiplication */
expression(a) ::= expression(b) MUL expression(c).

/* an expression may be a division */
expression(a) ::= expression(b) DIV expression(c).

/* an expression can be surrounded by angular brackets a variable */
expression ::= ABRACKET_OPEN expression(e) ABRACKET_CLOSE.