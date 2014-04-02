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
	#include "exo/ast/ast.h"

	#define TOKENSTR(s) std::string( reinterpret_cast<const char *>( s->get_text().c_str() ) )
}

%token_prefix QUEX_TKN_

%default_type { quex::Token* }
%token_type { quex::Token* }
%extra_argument { exo::ast::Tree *ast }

%syntax_error {
ERRORMSG( "Syntax error, unexpected " << exo::ast::Token::getName( yymajor ) << " on " << TOKEN->line_number() << ":" << TOKEN->column_number() );
}

%stack_overflow {
ERRORMSG( "Parser stack overflown" );
}

%start_symbol program



/* a program is build out of statements. */
program ::= statements. { ; }


/* statements are either a single statement or a statement followed by ; and other statements */
statements ::= statement.
statements ::= statement SEMICOLON statements.


/* a statement may be an declaration of a variable type */
statement ::= type(t) VARIABLE(l). {
	ast->addNode( new exo::ast::VariableDeclaration( TOKENSTR(l), TOKENSTR(t) ) );
}

/* a statement may be an assignment of a variable to an expression */
statement ::= VARIABLE(v) ASSIGN expression(e). {
	ast->addNode( new exo::ast::VariableAssignment( TOKENSTR(v) ) );
}


/* a type may be an integer */
type ::= TYPE_INT.

/* a number may be an integer or a float */
number ::= INT(i). 
number ::= FLOAT(f). 


/* an expression may be a number */
expression ::= number. 

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