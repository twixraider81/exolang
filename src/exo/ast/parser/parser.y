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
	std::stringstream msg;
	msg << "unexpected \"" << TOKEN->get_name() << "\" on " << TOKEN->line_number() << ":" << TOKEN->column_number();
	throw std::runtime_error( msg.str() );
}

%stack_overflow {
	throw std::runtime_error( "stack overflown" );
}

%start_symbol program

%token_prefix QUEX_TKN_
%token_type { quex::Token* }

%extra_argument { exo::ast::Tree *ast }
%default_type { exo::ast::nodes::Node* }


%left S_ABRACKET_OPEN.
%right S_ASSIGN.


/* a program is build out of statements. */
program ::= statements(s). {
	TRACESECTION( "PARSER", "program ::= statements(s).");

	ast->stmts = s;
}


/* statements are a single statement followed by ; and other statements */
%type statements { exo::ast::nodes::StmtList* }
statements(a) ::= statement(b). {
	TRACESECTION( "PARSER", "statements(a) ::= statement(b).");

	POINTERCHECK( b );
	a = new exo::ast::nodes::StmtList;
	a->list.push_back( b );

	TRACESECTION( "PARSER", "pushing statement; size:" << a->list.size());
}

statements ::= statements(a) statement(b). {
	TRACESECTION( "PARSER", "statements ::= statements(a) statement(b).");

	POINTERCHECK( a );
	POINTERCHECK( b );
	a->list.push_back( b );

	TRACESECTION( "PARSER",  "pushing statement; size:" << a->list.size() );
}

/* a statement may be an declaration of a variable type */
%type statement { exo::ast::nodes::Stmt* }
statement(s) ::= type(t) T_VAR(v) S_SEMICOLON. {
	TRACESECTION( "PARSER", "statement(s) ::= type(t) T_VAR(v) S_SEMICOLON.");

	POINTERCHECK( t );
	POINTERCHECK( v );
	s = new exo::ast::nodes::VarDecl( TOKENSTR( v ), t );
}

/* a statement may be an assignment of a variable to an expression */
statement(s) ::= T_VAR(v) S_ASSIGN expression(e) S_SEMICOLON. {
	TRACESECTION( "PARSER", "T_VAR(v) S_ASSIGN expression(e) S_SEMICOLON.");

	POINTERCHECK( v );
	POINTERCHECK( e );
	s = new exo::ast::nodes::VarAssign( TOKENSTR( v ), e );
}



/* a type may be a bool, integer, float, string or auto, or a label */
/* whabbout lists, map, closures? */
%type type { exo::ast::nodes::Type* }
type(t) ::= T_TNULL. {
	TRACESECTION( "PARSER", "type(t) ::= T_TNULL.");

	t = new exo::ast::nodes::Type( exo::types::NIL );
}

type(t) ::= T_TBOOL. {
	TRACESECTION( "PARSER", "type(t) ::= T_TBOOL.");

	t = new exo::ast::nodes::Type( exo::types::BOOLEAN );
}

type(t) ::= T_TINT. {
	TRACESECTION( "PARSER", "type(t) ::= T_TINT.");

	t = new exo::ast::nodes::Type( exo::types::INTEGER );
}

type(t) ::= T_TFLOAT. {
	TRACESECTION( "PARSER", "type(t) ::= T_TFLOAT.");

	t = new exo::ast::nodes::Type( exo::types::FLOAT );
}

type(t) ::= T_TSTRING. {
	TRACESECTION( "PARSER", "type(t) ::= T_TSTRING.");

	t = new exo::ast::nodes::Type( exo::types::STRING );
}

type(t) ::= T_TAUTO. {
	TRACESECTION( "PARSER", "type(t) ::= T_TAUTO.");

	t = new exo::ast::nodes::Type( exo::types::AUTO );
}

type(t) ::= T_ID(l). {
	TRACESECTION( "PARSER", "type(t) ::= T_ID(l).");

	t = new exo::ast::nodes::Type( TOKENSTR(l) );
}



/* a number may be an integer or a float */
%type number { exo::ast::nodes::Expr* }
number(n) ::= T_VINT(i). {
	TRACESECTION( "PARSER", "number(n) ::= T_VINT(i).");

	POINTERCHECK( i );
	n = new exo::ast::nodes::ValInt( i->get_lValue() );
}

number(n) ::= T_VFLOAT(f). {
	TRACESECTION( "PARSER", "number(n) ::= T_VFLOAT(f).");

	POINTERCHECK( f );
	n = new exo::ast::nodes::ValFloat( f->get_dValue() );
}



/* an expression may be a number */
%type expression { exo::ast::nodes::Expr* }
expression(e) ::= number(n). {
	TRACESECTION( "PARSER", "expression(e) ::= number(n).");

	POINTERCHECK( e );
	POINTERCHECK( n );
	e = n;
}

/* an expression can be surrounded by angular brackets */
expression ::= S_ABRACKET_OPEN expression S_ABRACKET_CLOSE. {
	TRACESECTION( "PARSER", "S_ABRACKET_OPEN expression S_ABRACKET_CLOSE.");
}
