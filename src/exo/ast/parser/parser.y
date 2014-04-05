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
	msg << "unexpected \"" << TOKEN->type_id_name() << "\" on " << TOKEN->line_number() << ":" << TOKEN->column_number();
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


%left S_PLUS S_MINUS.
%left S_MUL S_DIV.
%left S_ASSIGN.


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


/* statement can be a variable declaration, function declaration or an expression. statements are terminated by a semicolon */
%type statement { exo::ast::nodes::Stmt* }
statement(s) ::= vardecl(v) S_SEMICOLON. {
	TRACESECTION( "PARSER", "statement ::= vardecl S_SEMICOLON.");
	s = v;
}
statement(s) ::= fundecl(f) S_SEMICOLON. {
	TRACESECTION( "PARSER", "statement ::= fundecl S_SEMICOLON.");
	s = f;
}
statement(s) ::= expression(e) S_SEMICOLON. {
	TRACESECTION( "PARSER", "statement(s) ::= expression(e) S_SEMICOLON.");
	POINTERCHECK( e );
	s = new exo::ast::nodes::StmtExpr( e );
}


/* a block is empty (i.e. protofunctions) or a collection of statements delimited by brackets */
%type block { exo::ast::nodes::StmtList* }
block(b) ::= S_LBRACKET S_RBRACKET. {
	TRACESECTION( "PARSER", "block(b) ::= S_LBRACKET S_RBRACKET.");
	b = new exo::ast::nodes::StmtList;
}
block(b) ::= S_LBRACKET statements(s) S_RBRACKET. {
	TRACESECTION( "PARSER", "block(b) ::= S_LBRACKET statements(s) S_RBRACKET.");
	POINTERCHECK( s );
	b = s;
}


/* a type may be a null, bool, integer, float, string or auto */
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
type(t) ::= T_TCALLABLE. {
	TRACESECTION( "PARSER", "type(t) ::= T_TCALLABLE.");
	t = new exo::ast::nodes::Type( exo::types::CALLABLE );
}
type(t) ::= T_ID(i). {
	TRACESECTION( "PARSER", "type(t) ::= T_ID(i).");
	t = new exo::ast::nodes::Type( TOKENSTR(i) );
}


/* a variable declaration is a type identifier followed by a variable name optionally followed by an assignment to an expression */
%type vardecl { exo::ast::nodes::VarDecl* }
vardecl(d) ::= type(t) T_VAR(v). {
	TRACESECTION( "PARSER", "vardecl(d) ::= type(t) T_VAR(v). ");
	POINTERCHECK( t );
	POINTERCHECK( v );
	d = new exo::ast::nodes::VarDecl( TOKENSTR(v), t );
}
vardecl(d) ::= type(t) T_VAR(v) S_ASSIGN expression(e). {
	TRACESECTION( "PARSER", "vardecl(d) ::= type(t) T_VAR(v) S_ASSIGN expression(e).");
	POINTERCHECK( t );
	POINTERCHECK( v );
	POINTERCHECK( e );
	d = new exo::ast::nodes::VarDecl( TOKENSTR(v), t, e );
}


/* a variable decleration list, are variable declarations seperated by a colon */
%type vardecllist { exo::ast::nodes::VarDeclList* }
vardecllist (l)::= . {
	TRACESECTION( "PARSER", "vardecllist (l)::= .");
	l = new exo::ast::nodes::VarDeclList;
	TRACESECTION( "PARSER", "pushing declaration; size:" << l->list.size());
}
vardecllist(l) ::= vardecl(d). {
	TRACESECTION( "PARSER", "vardecllist(a) ::= vardecl(d).");
	POINTERCHECK( d );
	l = new exo::ast::nodes::VarDeclList;
	l->list.push_back( d );
	TRACESECTION( "PARSER", "pushing declaration; size:" << l->list.size());
}
vardecllist ::= vardecllist(l) S_COMMA vardecl(d). {
	TRACESECTION( "PARSER", "vardecllist(a) ::= vardecl(d).");
	POINTERCHECK( d );
	l->list.push_back( d );
	TRACESECTION( "PARSER", "pushing declaration; size:" << l->list.size());
}

/* a function declaration is a type identifier followed by the keyword function a functionname, optionally function arguments in brackets and and associated block */
%type fundecl { exo::ast::nodes::FunDecl* }
fundecl(f) ::= S_FUNCTION T_ID(i) block(b). {
	TRACESECTION( "PARSER", "fundecl(f) ::= S_FUNCTION T_ID(i) block(b).");
	POINTERCHECK( b );
	f = new exo::ast::nodes::FunDecl( new exo::ast::nodes::Type( exo::types::CALLABLE, TOKENSTR(i) ), new exo::ast::nodes::Type( exo::types::AUTO ), new exo::ast::nodes::VarDeclList, b );
}
fundecl(f) ::= type(t) S_FUNCTION T_ID(i) block(b). {
	TRACESECTION( "PARSER", "fundecl(f) ::= type(t) S_FUNCTION T_ID(i) block(b).");
	POINTERCHECK( b );
	f = new exo::ast::nodes::FunDecl( new exo::ast::nodes::Type( exo::types::CALLABLE, TOKENSTR(i) ), t, new exo::ast::nodes::VarDeclList, b );
}
fundecl(f) ::= S_FUNCTION T_ID(i) S_LANGLE vardecllist(a) S_RANGLE block(b). {
	TRACESECTION( "PARSER", "fundecl(f) ::= S_FUNCTION T_ID(i) S_LANGLE vardecllist(a) S_RANGLE block(b).");
	POINTERCHECK( a );
	POINTERCHECK( b );
	f = new exo::ast::nodes::FunDecl( new exo::ast::nodes::Type( exo::types::CALLABLE, TOKENSTR(i) ), new exo::ast::nodes::Type( exo::types::AUTO ), a, b );
}
fundecl(f) ::= type(t) S_FUNCTION T_ID(i) S_LANGLE vardecllist(a) S_RANGLE block(b). {
	TRACESECTION( "PARSER", "type(t) S_FUNCTION T_ID(i) S_LANGLE vardecllist(a) S_RANGLE block(b).");
	POINTERCHECK( a );
	POINTERCHECK( b );
	f = new exo::ast::nodes::FunDecl( new exo::ast::nodes::Type( exo::types::CALLABLE, TOKENSTR(i) ), t, a, b );
}


/* a number may be an integer or a float */
%type number { exo::ast::nodes::ValAny* }
number(n) ::= T_VINT(i). {
	TRACESECTION( "PARSER", "number(n) ::= T_VINT(i).");
	POINTERCHECK( i );
	n = new exo::ast::nodes::ValInt( TOKENSTR(i) );
}
number(n) ::= T_VFLOAT(f). {
	TRACESECTION( "PARSER", "number(n) ::= T_VFLOAT(f).");
	POINTERCHECK( f );
	n = new exo::ast::nodes::ValFloat( TOKENSTR(f) );
}

/* an expression may be a number, an assignment */
%type expression { exo::ast::nodes::Expr* }
expression(r) ::= T_VAR(v) S_ASSIGN expression(e). {
	TRACESECTION( "PARSER", "expression(r) ::= T_VAR(v) S_ASSIGN expression(e).");
	r = new exo::ast::nodes::VarAssign( TOKENSTR(v), e );
}
expression ::= number. {
	TRACESECTION( "PARSER", "expression ::= number.");
}
expression(e) ::= expression(a) comparison(c) expression(b). {
	TRACESECTION( "PARSER", "expression(e) ::= expression(a) comparison(c) expression(b).");
	e = new exo::ast::nodes::CompOp( a, TOKENSTR(c), b );
}

/* comparison operators */
%type comparison { quex::Token* }
comparison(c) ::= S_EQ(o). {
	TRACESECTION( "PARSER", "comparison(c) ::= S_EQ(o).");
	POINTERCHECK( o );
	c = o;
}
comparison(c) ::= S_NE(o). {
	TRACESECTION( "PARSER", "comparison(c) ::= S_NE(o).");
	POINTERCHECK( o );
	c = o;
}
comparison(c) ::= S_LT(o). {
	TRACESECTION( "PARSER", "comparison(c) ::= S_LT(o).");
	POINTERCHECK( o );
	c = o;
}
comparison(c) ::= S_LE(o). {
	TRACESECTION( "PARSER", "comparison(c) ::= S_LE(o).");
	POINTERCHECK( o );
	c = o;
}
comparison(c) ::= S_GT(o). {
	TRACESECTION( "PARSER", "comparison(c) ::= S_GT(o).");
	POINTERCHECK( o );
	c = o;
}
comparison(c) ::= S_GE(o). {
	TRACESECTION( "PARSER", "comparison(c) ::= S_GE(o).");
	POINTERCHECK( o );
	c = o;
}
comparison(c) ::= S_PLUS(o). {
	TRACESECTION( "PARSER", "comparison(c) ::= S_PLUS(o).");
	POINTERCHECK( o );
	c = o;
}
comparison(c) ::= S_MINUS(o). {
	TRACESECTION( "PARSER", "comparison(c) ::= S_MINUS(o).");
	POINTERCHECK( o );
	c = o;
}
comparison(c) ::= S_MUL(o). {
	TRACESECTION( "PARSER", "comparison(c) ::= S_MUL(o).");
	POINTERCHECK( o );
	c = o;
}
comparison(c) ::= S_DIV(o). {
	TRACESECTION( "PARSER", "comparison(c) ::= S_DIV(o).");
	POINTERCHECK( o );
	c = o;
}