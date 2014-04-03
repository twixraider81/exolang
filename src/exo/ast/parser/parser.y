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
	#include "exo/types/types.h"
}

%syntax_error {
	ERRORMSG( "syntax error, unexpected \"" << TOKEN->get_name() << "\" on " << TOKEN->line_number() << ":" << TOKEN->column_number() );
}

%stack_overflow {
	ERRORMSG( "stack overflown" );
}

%start_symbol program

%token_prefix QUEX_TKN_
%token_type { quex::Token* }

%extra_argument { exo::ast::Tree *ast }
%default_type { exo::ast::nodes::Node* }



/* Now comes the language spec... */

/* a program is build out of statements. */
program ::= statements. { ; }


/* statements are either a single statement or a statement followed by ; and other statements */
statements ::= statement.
statements ::= statement S_SEMICOLON statements.


/* a statement may be an declaration of a variable type */
statement(s) ::= type(t) T_VARIABLE(v). {
	POINTERCHECK(t);
	POINTERCHECK(v);
	s = new exo::ast::nodes::VarDecl( TOKENSTR(v), t );
	ast->addNode( s );
}

/* a statement may be an assignment of a variable to an expression */
statement(s) ::= T_VARIABLE(v) S_ASSIGN expression(e). {
	POINTERCHECK(v);
	POINTERCHECK(e);
	s = new exo::ast::nodes::VarAssign( TOKENSTR(v), e );
	ast->addNode( s );
}


/* a type may be a bool, integer, float, string or auto */
/* whabbout lists, map, closures? */
%type type { exo::ast::nodes::Type* }
type(t) ::= T_TYPE_NULL. { t = new exo::ast::nodes::Type( exo::types::NIL ); ast->addNode( t ); }
type(t) ::= T_TYPE_BOOLEAN. { t = new exo::ast::nodes::Type( exo::types::BOOLEAN ); ast->addNode( t ); }
type(t) ::= T_TYPE_INT. { t = new exo::ast::nodes::Type( exo::types::INTEGER ); ast->addNode( t ); }
type(t) ::= T_TYPE_FLOAT. { t = new exo::ast::nodes::Type( exo::types::FLOAT ); ast->addNode( t ); }
type(t) ::= T_TYPE_STRING. { t = new exo::ast::nodes::Type( exo::types::STRING ); ast->addNode( t ); }
type(t) ::= T_TYPE_AUTO. { t = new exo::ast::nodes::Type( exo::types::AUTO ); ast->addNode( t ); }


/* a number may be an integer or a float */
number(n) ::= I_INT(i). {
	POINTERCHECK(i);
	n = new exo::ast::nodes::ValInt( i->get_lValue() );
	ast->addNode( n );
}

number(n) ::= F_FLOAT(f). {
	POINTERCHECK(f);
	n = new exo::ast::nodes::ValFloat( f->get_dValue() );
	ast->addNode( n );
}


/* an expression may be a number */
%type expression { exo::ast::nodes::Expr* }
expression ::= number.

/* an expression may be an addition */
expression ::= expression S_ADD expression. 

/* an expression may be a subtraction */
expression ::= expression S_SUB expression. 

/* an expression may be a multiplication */
expression ::= expression S_MUL expression.

/* an expression may be a division */
expression ::= expression S_DIV expression.

/* an expression can be surrounded by angular brackets a variable */
expression ::= S_ABRACKET_OPEN expression S_ABRACKET_CLOSE.