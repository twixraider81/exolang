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
	#include "exo/ast/nodes.h"
	#include "exo/ast/tree.h"
	#include "exo/jit/type/types.h"
}

%syntax_error {
	BOOST_THROW_EXCEPTION( exo::exceptions::UnexpectedToken( TOKEN->type_id_name(), TOKEN->line_number(), TOKEN->column_number() ) );
}

%stack_overflow {
	BOOST_THROW_EXCEPTION( exo::exceptions::StackOverflow() );
}

%start_symbol program

%token_prefix QUEX_TKN_
%token_type { quex::Token* }

%extra_argument { exo::ast::Tree *ast }
%default_type { exo::ast::Node* }

%nonassoc S_EQ S_NE S_LT S_LE S_GT S_GE.
%left S_PLUS S_MINUS.
%left S_MUL S_DIV.

/* a program is build out of statements. */
program ::= statements(s). {
	TRACESECTION( "PARSER", "program ::= statements(s).");
	ast->stmts = s;
}


/* statements are a single statement followed by ; and other statements */
%type statements { exo::ast::StmtList* }
statements(a) ::= statement(b). {
	TRACESECTION( "PARSER", "statements(a) ::= statement(b).");
	POINTERCHECK( b );
	a = new exo::ast::StmtList;
	a->list.push_back( b );
	TRACESECTION( "PARSER", "pushing statement; size:" << a->list.size());
}
statements(s) ::= statements(a) statement(b). {
	TRACESECTION( "PARSER", "statements ::= statements(a) statement(b).");
	POINTERCHECK( a );
	POINTERCHECK( b );
	a->list.push_back( b );
	TRACESECTION( "PARSER",  "pushing statement; size:" << a->list.size() );
	s = a;
}


/* statement can be a variable declaration, function declaration or an expression. statements are terminated by a semicolon */
%type statement { exo::ast::Stmt* }
statement(s) ::= vardecl(v) S_SEMICOLON. {
	TRACESECTION( "PARSER", "statement ::= vardecl S_SEMICOLON.");
	POINTERCHECK( v );
	s = v;
}
statement(s) ::= fundecl(f) S_SEMICOLON. {
	TRACESECTION( "PARSER", "statement ::= fundecl S_SEMICOLON.");
	POINTERCHECK( f );
	s = f;
}
statement(s) ::= expression(e) S_SEMICOLON. {
	TRACESECTION( "PARSER", "statement(s) ::= expression(e) S_SEMICOLON.");
	POINTERCHECK( e );
	s = new exo::ast::StmtExpr( e );
}


/* a block is empty (i.e. protofunctions) or a collection of statements delimited by brackets */
%type block { exo::ast::StmtList* }
block(b) ::= S_LBRACKET S_RBRACKET. {
	TRACESECTION( "PARSER", "block(b) ::= S_LBRACKET S_RBRACKET.");
	b = new exo::ast::StmtList;
}
block(b) ::= S_LBRACKET statements(s) S_RBRACKET. {
	TRACESECTION( "PARSER", "block(b) ::= S_LBRACKET statements(s) S_RBRACKET.");
	POINTERCHECK( s );
	b = s;
}


/* a type may be a null, bool, integer, float, string or auto */
%type type { exo::ast::Type* }
type(t) ::= T_TBOOL. {
	TRACESECTION( "PARSER", "type(t) ::= T_TBOOL.");
	t = new exo::ast::Type( &typeid( exo::jit::types::BooleanType ) );
}
type(t) ::= T_TINT. {
	TRACESECTION( "PARSER", "type(t) ::= T_TINT.");
	t = new exo::ast::Type( &typeid( exo::jit::types::IntegerType ) );
}
type(t) ::= T_TFLOAT. {
	TRACESECTION( "PARSER", "type(t) ::= T_TFLOAT.");
	t = new exo::ast::Type( &typeid( exo::jit::types::FloatType ) );
}
type(t) ::= T_TSTRING. {
	TRACESECTION( "PARSER", "type(t) ::= T_TSTRING.");
	t = new exo::ast::Type( &typeid( exo::jit::types::StringType ) );
}
type(t) ::= T_TAUTO. {
	TRACESECTION( "PARSER", "type(t) ::= T_TAUTO.");
	t = new exo::ast::Type( &typeid( exo::jit::types::AutoType ) );
}
type(t) ::= T_TCALLABLE. {
	TRACESECTION( "PARSER", "type(t) ::= T_TCALLABLE.");
	t = new exo::ast::Type( &typeid( exo::jit::types::CallableType ) );
}
type(t) ::= T_ID(i). {
	TRACESECTION( "PARSER", "type(t) ::= T_ID(i).");
	POINTERCHECK( i );
	t = new exo::ast::Type( &typeid( exo::jit::types::ClassType ), TOKENSTR( i ) );
}


/* a variable declaration is a type identifier followed by a variable name optionally followed by an assignment to an expression */
%type vardecl { exo::ast::VarDecl* }
vardecl(d) ::= type(t) T_VAR(v). {
	TRACESECTION( "PARSER", "vardecl(d) ::= type(t) T_VAR(v). ");
	POINTERCHECK( t );
	POINTERCHECK( v );
	d = new exo::ast::VarDecl( TOKENSTR(v), t );
}
vardecl(d) ::= type(t) T_VAR(v) S_ASSIGN expression(e). {
	TRACESECTION( "PARSER", "vardecl(d) ::= type(t) T_VAR(v) S_ASSIGN expression(e).");
	POINTERCHECK( t );
	POINTERCHECK( v );
	POINTERCHECK( e );
	d = new exo::ast::VarDecl( TOKENSTR(v), t, e );
}


/* a variable declaration lists are variable declarations seperated by a colon or empty */
%type vardecllist { exo::ast::VarDeclList* }
vardecllist (l)::= . {
	TRACESECTION( "PARSER", "vardecllist (l)::= .");
	l = new exo::ast::VarDeclList;
	TRACESECTION( "PARSER", "pushing declaration; size:" << l->list.size());
}
vardecllist(l) ::= vardecl(d). {
	TRACESECTION( "PARSER", "vardecllist(a) ::= vardecl(d).");
	POINTERCHECK( d );
	l = new exo::ast::VarDeclList;
	l->list.push_back( d );
	TRACESECTION( "PARSER", "pushing declaration; size:" << l->list.size());
}
vardecllist ::= vardecllist(l) S_COMMA vardecl(d). {
	TRACESECTION( "PARSER", "vardecllist(a) ::= vardecl(d).");
	POINTERCHECK( l );
	POINTERCHECK( d );
	l->list.push_back( d );
	TRACESECTION( "PARSER", "pushing declaration; size:" << l->list.size());
}

/* a function declaration is a type identifier followed by the keyword function a functionname, optionally function arguments in brackets and and associated block */
%type fundecl { exo::ast::FunDecl* }
fundecl(f) ::= S_FUNCTION T_ID(i) block(b). {
	TRACESECTION( "PARSER", "fundecl(f) ::= S_FUNCTION T_ID(i) block(b).");
	POINTERCHECK( i );
	POINTERCHECK( b );
	f = new exo::ast::FunDecl( TOKENSTR(i), new exo::ast::Type( &typeid( exo::jit::types::AutoType ) ), new exo::ast::VarDeclList, b );
}
fundecl(f) ::= type(t) S_FUNCTION T_ID(i) block(b). {
	TRACESECTION( "PARSER", "fundecl(f) ::= type(t) S_FUNCTION T_ID(i) block(b).");
	POINTERCHECK( t );
	POINTERCHECK( i );
	POINTERCHECK( b );
	f = new exo::ast::FunDecl( TOKENSTR(i), t, new exo::ast::VarDeclList, b );
}
fundecl(f) ::= S_FUNCTION T_ID(i) S_LANGLE vardecllist(a) S_RANGLE block(b). {
	TRACESECTION( "PARSER", "fundecl(f) ::= S_FUNCTION T_ID(i) S_LANGLE vardecllist(a) S_RANGLE block(b).");
	POINTERCHECK( i );
	POINTERCHECK( a );
	POINTERCHECK( b );
	f = new exo::ast::FunDecl( TOKENSTR(i), new exo::ast::Type( &typeid( exo::jit::types::AutoType ) ), a, b );
}
fundecl(f) ::= type(t) S_FUNCTION T_ID(i) S_LANGLE vardecllist(a) S_RANGLE block(b). {
	TRACESECTION( "PARSER", "type(t) S_FUNCTION T_ID(i) S_LANGLE vardecllist(a) S_RANGLE block(b).");
	POINTERCHECK( t );
	POINTERCHECK( i );
	POINTERCHECK( a );
	POINTERCHECK( b );
	f = new exo::ast::FunDecl( TOKENSTR(i), t, a, b );
}


/* a variable declaration lists are expressions seperated by a colon or empty */
%type exprlist { exo::ast::ExprList* }
exprlist(l) ::= . {
	TRACESECTION( "PARSER", "exprlist(l) ::= .");
	l = new exo::ast::ExprList;
	TRACESECTION( "PARSER", "pushing expressions; size:" << l->list.size());
}
exprlist(l) ::= expression(e). {
	TRACESECTION( "PARSER", "exprlist(l) ::= .");
	POINTERCHECK( e );
	l = new exo::ast::ExprList;
	l->list.push_back( e );
	TRACESECTION( "PARSER", "pushing expressions; size:" << l->list.size());
}
exprlist ::= exprlist(l) S_COMMA expression(e). {
	TRACESECTION( "PARSER", "exprlist ::= exprlist(l) S_COMMA expression(e).");
	POINTERCHECK( l );
	POINTERCHECK( e );
	l->list.push_back( e );
	TRACESECTION( "PARSER", "pushing declaration; size:" << l->list.size());
}


/* a number may be an integer or a float */
%type number { exo::ast::Expr* }
number(n) ::= T_VINT(i). {
	TRACESECTION( "PARSER", "number(n) ::= T_VINT(i).");
	POINTERCHECK( i );
	n = new exo::ast::ValueInt( TOKENSTR(i) );
}
number(n) ::= T_VFLOAT(f). {
	TRACESECTION( "PARSER", "number(n) ::= T_VFLOAT(f).");
	POINTERCHECK( f );
	n = new exo::ast::ValueFloat( TOKENSTR(f) );
}

/* an expression may be a number, an assignment or a function call or a constant */
%type expression { exo::ast::Expr* }
expression(r) ::= T_VAR(v) S_ASSIGN expression(e). {
	TRACESECTION( "PARSER", "expression(r) ::= T_VAR(v) S_ASSIGN expression(e).");
	POINTERCHECK( v );
	POINTERCHECK( e );
	r = new exo::ast::VarAssign( TOKENSTR(v), e );
}
expression(e) ::= T_ID(i) S_LANGLE exprlist(a) S_RANGLE. {
	TRACESECTION( "PARSER", "T_ID(i) S_LANGLE funcall S_RANGLE.");
	POINTERCHECK( i );
	POINTERCHECK( a );
	e = new exo::ast::FunCall( TOKENSTR(i), a );
}
expression(e) ::= number(n). {
	TRACESECTION( "PARSER", "expression ::= number.");
	POINTERCHECK( n );
	e = n;
}
expression(e) ::= expression(a) comparison(c) expression(b). {
	TRACESECTION( "PARSER", "expression(e) ::= expression(a) comparison(c) expression(b).");
	POINTERCHECK( a );
	POINTERCHECK( c );
	POINTERCHECK( b );
	e = new exo::ast::CompOp( a, TOKENSTR(c), b );
}
expression(e) ::= constant(c). {
	TRACESECTION( "PARSER", "expression(e) ::= constant(c).");
	POINTERCHECK( e );
	POINTERCHECK( c );
	e = c;
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

/* a constant can be a builtin */
%type constant { exo::ast::Expr* }
constant(c) ::= S_FILE(f). {
	TRACESECTION( "PARSER", "constant(c) ::= S_FILE(f).");
	POINTERCHECK( f );
	c = new exo::ast::ConstExpr( TOKENSTR(f), new exo::ast::ValueString( ast->fileName ) );
}
constant(c) ::= S_LINE(l). {
	TRACESECTION( "PARSER", "constant(c) ::= S_LINE(l).");
	POINTERCHECK( l );
	c = new exo::ast::ConstExpr( TOKENSTR(l), new exo::ast::ValueInt( std::to_string( l->line_number() ) ) );
}
constant(c) ::= T_VNULL. {
	TRACESECTION( "PARSER", "constant(c) ::= T_VNULL.");
	c = new exo::ast::ConstExpr( "false", new exo::ast::ValueNull() );
}
constant(c) ::= T_VTRUE. {
	TRACESECTION( "PARSER", "constant(c) ::= T_VTRUE.");
	c = new exo::ast::ConstExpr( "true", new exo::ast::ValueBool( true ) );
}
constant(c) ::= T_VFALSE. {
	TRACESECTION( "PARSER", "constant(c) ::= T_VFALSE.");
	c = new exo::ast::ConstExpr( "false", new exo::ast::ValueBool( false ) );
}