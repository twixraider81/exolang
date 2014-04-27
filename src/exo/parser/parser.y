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
}

%syntax_error {
	BOOST_THROW_EXCEPTION( exo::exceptions::UnexpectedToken( TOKEN->type_id_name(), TOKEN->line_number(), TOKEN->column_number() ) );
}
%stack_overflow {
	BOOST_THROW_EXCEPTION( exo::exceptions::StackOverflow() );
}


/* more like the "end" reduce ;) */
%start_symbol program

%token_prefix QUEX_TKN_
%token_type { quex::Token* }

/* the extra argument ist unused at the moment */
%extra_argument { exo::ast::Tree *ast }

/* everything decends from an ast node, tell lemon about it */
%default_type { exo::ast::Node* }


/* token precedences */
%right		T_TBOOL T_TINT T_TFLOAT T_TSTRING T_TAUTO T_TCALLABLE T_ID.
%right		S_ASSIGN.
%left		S_EQ S_NE.
%left		S_LT S_LE S_GT S_GE.
%left		S_PLUS S_MINUS.
%left		S_MUL S_DIV.
%left		S_SEMICOLON.

/* a program is build out of statements. */
program ::= statements(s). {
	BOOST_LOG_TRIVIAL(trace) << "program ::= statements(S).";
	ast->stmts = s;
}


/* statements are a single statement followed by ; and other statements */
%type statements { exo::ast::StmtList* }
statements(a) ::= statement(b). {
	BOOST_LOG_TRIVIAL(trace) << "statements(A) ::= statement(B).";
	POINTERCHECK(b);
	a = new exo::ast::StmtList;
	a->list.push_back( b );
	BOOST_LOG_TRIVIAL(trace) << "Pushing statement; size:" << a->list.size();
}
statements(s) ::= statements(a) statement(b). {
	BOOST_LOG_TRIVIAL(trace) << "statements(S) ::= statements(A) statement(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	a->list.push_back( b );
	s = a;
	BOOST_LOG_TRIVIAL(trace) << "Pushing statement; size:" << a->list.size();
}


/* statement can be a variable declaration, function (proto) declaration, class declaration, a return statement or an expression. statements are terminated by a semicolon */
%type statement { exo::ast::Stmt* }
statement(s) ::= vardecl(v) S_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "statement(S) ::= vardecl(V) S_SEMICOLON.";
	POINTERCHECK(v);
	s = v;
}
statement(s) ::= fundeclproto(f) S_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "statement(S) ::= fundeclproto(F) S_SEMICOLON.";
	POINTERCHECK(f);
	s = f;
}
statement(s) ::= fundecl(f) S_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "statement(S) ::= fundecl(F) S_SEMICOLON.";
	POINTERCHECK(f);
	s = f;
}
statement(s) ::= classdecl(c) S_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "statement(S) ::= classdecl(C) S_SEMICOLON.";
	POINTERCHECK(c);
	s = c;
}
statement(s) ::= S_RETURN expression(e) S_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "statement(S) ::= S_RETURN expression(E) S_SEMICOLON.";
	POINTERCHECK(e);
	s = new exo::ast::StmtReturn( e );
}
statement(s) ::= expression(e) S_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "statement(S) ::= expression(E) S_SEMICOLON.";
	POINTERCHECK(e);
	s = new exo::ast::StmtExpr( e );
}


/* a block is empty (i.e. protofunctions) or a collection of statements delimited by brackets */
%type block { exo::ast::StmtList* }
block(b) ::= S_LBRACKET S_RBRACKET. {
	BOOST_LOG_TRIVIAL(trace) << "block(B) ::= S_LBRACKET S_RBRACKET.";
	b = new exo::ast::StmtList;
}
block(b) ::= S_LBRACKET statements(s) S_RBRACKET. {
	BOOST_LOG_TRIVIAL(trace) << "block(B) ::= S_LBRACKET statements(S) S_RBRACKET.";
	POINTERCHECK(s);
	b = s;
}


/* a type may be a null, bool, integer, float, string or auto */
%type type { exo::ast::Type* }
type(t) ::= T_TBOOL. {
	BOOST_LOG_TRIVIAL(trace) << "type(T) ::= T_TBOOL.";
	t = new exo::ast::Type( "bool" );
}
type(t) ::= T_TINT. {
	BOOST_LOG_TRIVIAL(trace) << "type(T) ::= T_TINT.";
	t = new exo::ast::Type( "int" );
}
type(t) ::= T_TFLOAT. {
	BOOST_LOG_TRIVIAL(trace) << "type(T) ::= T_TFLOAT.";
	t = new exo::ast::Type( "float" );
}
type(t) ::= T_TSTRING. {
	BOOST_LOG_TRIVIAL(trace) << "type(T) ::= T_TSTRING.";
	t = new exo::ast::Type( "string" );
}
type(t) ::= T_TAUTO. {
	BOOST_LOG_TRIVIAL(trace) << "type(T) ::= T_TAUTO.";
	t = new exo::ast::Type( "auto" );
}
type(t) ::= T_TCALLABLE. {
	BOOST_LOG_TRIVIAL(trace) << "type(T) ::= T_TCALLABLE.";
	t = new exo::ast::Type( "callable" );
}
type(t) ::= T_ID(i). {
	BOOST_LOG_TRIVIAL(trace) << "type(T) ::= T_ID(I).";
	POINTERCHECK(i);
	t = new exo::ast::Type( TOKENSTR(i) );
	delete i;
}


/* a variable declaration is a type identifier followed by a variable name optionally followed by an assignment to an expression */
%type vardecl { exo::ast::VarDecl* }
vardecl(d) ::= type(t) T_VAR(v). {
	BOOST_LOG_TRIVIAL(trace) << "vardecl(D) ::= type(T) T_VAR(V).";
	POINTERCHECK(t);
	POINTERCHECK(v);
	d = new exo::ast::VarDecl( TOKENSTR(v), t );
	delete v;
}
vardecl(d) ::= type(t) T_VAR(v) S_ASSIGN expression(e). {
	BOOST_LOG_TRIVIAL(trace) << "vardecl(D) ::= type(T) T_VAR(V) S_ASSIGN expression(E).";
	POINTERCHECK(t);
	POINTERCHECK(v);
	POINTERCHECK(e);
	d = new exo::ast::VarDecl( TOKENSTR(v), t, e );
	delete v;
}


/* a variable declaration lists are variable declarations seperated by a colon or empty */
%type vardecllist { exo::ast::VarDeclList* }
vardecllist(l)::= . {
	BOOST_LOG_TRIVIAL(trace) << "vardecllist(L)::= .";
	l = new exo::ast::VarDeclList;
	BOOST_LOG_TRIVIAL(trace) << "Pushing variable declaration; size:" << l->list.size();
}
vardecllist(l) ::= vardecl(d). {
	BOOST_LOG_TRIVIAL(trace) << "vardecllist(L) ::= vardecl(D).";
	POINTERCHECK(d);
	l = new exo::ast::VarDeclList;
	l->list.push_back( d );
	BOOST_LOG_TRIVIAL(trace) << "Pushing variable declaration; size:" << l->list.size();
}
vardecllist(e) ::= vardecllist(l) S_COMMA vardecl(d). {
	BOOST_LOG_TRIVIAL(trace) << "vardecllist(E) ::= vardecllist(L) S_COMMA vardecl(D).";
	POINTERCHECK(l);
	POINTERCHECK(d);
	l->list.push_back( d );
	e = l;
	BOOST_LOG_TRIVIAL(trace) << "Pushing variable declaration; size:" << l->list.size();
}


/* a function declaration is a type identifier followed by the keyword function a functionname, optionally function arguments in brackets. if it has an associated block its a proper function and not a prototype */
%type fundeclproto { exo::ast::DecFunProto* }
fundeclproto(f) ::= type(t) S_FUNCTION T_ID(i) S_LANGLE vardecllist(a) S_RANGLE. {
	BOOST_LOG_TRIVIAL(trace) << "fundeclproto(F) ::= type(T) S_FUNCTION T_ID(I) S_LANGLE vardecllist(A) S_RANGLE.";
	POINTERCHECK(t);
	POINTERCHECK(i);
	POINTERCHECK(a);
	f = new exo::ast::DecFunProto( TOKENSTR(i), t, a );
	delete i;
}
%type fundecl { exo::ast::DecFun* }
fundecl(f) ::= type(t) S_FUNCTION T_ID(i) S_LANGLE vardecllist(a) S_RANGLE block(b). {
	BOOST_LOG_TRIVIAL(trace) << "fundecl(F) ::= type(T) S_FUNCTION T_ID(I) S_LANGLE vardecllist(A) S_RANGLE block(B).";
	POINTERCHECK(t);
	POINTERCHECK(i);
	POINTERCHECK(a);
	POINTERCHECK(b);
	f = new exo::ast::DecFun( TOKENSTR(i), t, a, b );
	delete i;
}


/* a class declaration is a class keyword, followed by an identifier optionally and extend with a classname, and and associated class block */
%type classdecl { exo::ast::DecClass* }
classdecl(c) ::= T_CLASS T_ID(i) T_EXTENDS T_ID(p) S_LBRACKET classblock(b) S_RBRACKET. {
	BOOST_LOG_TRIVIAL(trace) << "classdecl(C) ::= T_CLASS T_ID(I) T_EXTENDS T_ID(P) S_LBRACKET classblock(B) S_RBRACKET.";
	POINTERCHECK(i);
	POINTERCHECK(p);
	POINTERCHECK(b);
	c = new exo::ast::DecClass( TOKENSTR(i), TOKENSTR(p), b );
	delete i;
	delete p;
}
classdecl(c) ::= T_CLASS T_ID(i) T_EXTENDS T_ID(p) S_LBRACKET S_RBRACKET. {
	BOOST_LOG_TRIVIAL(trace) << "classdecl(C) ::= T_CLASS T_ID(I) T_EXTENDS T_ID(P) S_LBRACKET S_RBRACKET.";
	POINTERCHECK(i);
	POINTERCHECK(p);
	c = new exo::ast::DecClass( TOKENSTR(i), TOKENSTR(p), new exo::ast::ClassBlock );
	delete i;
	delete p;
}
classdecl(c) ::= T_CLASS T_ID(i) S_LBRACKET classblock(b) S_RBRACKET. {
	BOOST_LOG_TRIVIAL(trace) << "classdecl(C) ::= T_CLASS T_ID(I) S_LBRACKET classblock(B) S_RBRACKET.";
	POINTERCHECK(i);
	POINTERCHECK(b);
	c = new exo::ast::DecClass( TOKENSTR(i), b );
	delete i;
}
classdecl(c) ::= T_CLASS T_ID(i) S_LBRACKET S_RBRACKET. {
	BOOST_LOG_TRIVIAL(trace) << "classdecl(C) ::= T_CLASS T_ID(I) S_LBRACKET S_RBRACKET.";
	POINTERCHECK(i);
	c = new exo::ast::DecClass( TOKENSTR(i), new exo::ast::ClassBlock );
	delete i;
}
/* a class block contains the declarations of a class. that is properties and methods. */
%type classblock { exo::ast::ClassBlock* }
classblock(b) ::= vardecl(d) S_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "classblock(B) ::= vardecl(D) S_SEMICOLON.";
	POINTERCHECK(d);
	b = new exo::ast::ClassBlock;
	b->properties.push_back( d );
	BOOST_LOG_TRIVIAL(trace) << "Pushing property \"" << d->name << "\"; properties: " << b->properties.size();
}
classblock(b) ::= fundecl(d) S_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "classblock(B) ::= fundecl(D) S_SEMICOLON.";
	POINTERCHECK(d);
	b = new exo::ast::ClassBlock;
	b->methods.push_back( d );
	BOOST_LOG_TRIVIAL(trace) << "Pushing method \"" << d->name << "\"; methods: " << b->methods.size();
}
classblock(b) ::= classblock(l) vardecl(d) S_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "classblock(B) ::= classblock(L) vardecl(D) S_SEMICOLON.";
	POINTERCHECK(l);
	POINTERCHECK(d);
	l->properties.push_back( d );
	b = l;
	BOOST_LOG_TRIVIAL(trace) << "Pushing property \"" << d->name << "\"; properties: " << l->properties.size();
}
classblock(b) ::= classblock(l) fundecl(d) S_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "classblock(B) ::= classblock(L) fundecl(D) S_SEMICOLON.";
	POINTERCHECK(l);
	POINTERCHECK(d);
	l->methods.push_back( d );
	b = l;
	BOOST_LOG_TRIVIAL(trace) << "Pushing method \"" << d->name << "\"; methods: " << l->methods.size();
}


/* an expression list */
%type exprlist { exo::ast::ExprList* }
exprlist(l) ::= . {
	BOOST_LOG_TRIVIAL(trace) << "exprlist(L) ::= .";
	l = new exo::ast::ExprList;
	BOOST_LOG_TRIVIAL(trace) << "Pushing expression; expressions:" << l->list.size();
}
exprlist(l) ::= expression(e). {
	BOOST_LOG_TRIVIAL(trace) << "exprlist(L) ::= expression(E).";
	POINTERCHECK(e);
	l = new exo::ast::ExprList;
	l->list.push_back( e );
	BOOST_LOG_TRIVIAL(trace) << "Pushing expression; expressions:" << l->list.size();
}
exprlist(f) ::= exprlist(l) S_COMMA expression(e). {
	BOOST_LOG_TRIVIAL(trace) << "exprlist(F) ::= exprlist(L) S_COMMA expression(E).";
	POINTERCHECK(l);
	POINTERCHECK(e);
	l->list.push_back( e );
	f = l;
	BOOST_LOG_TRIVIAL(trace) << "Pushing declaration; expressions:" << l->list.size();
}


/* an expression may be an assignment, function call, variable, constant, binary expression, comparison */
%type expression { exo::ast::Expr* }
expression(a) ::= T_VAR(v) S_ASSIGN expression(e). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= T_VAR(V) S_ASSIGN expression(E).";
	POINTERCHECK(v);
	POINTERCHECK(e);
	a = new exo::ast::VarAssign( TOKENSTR(v), e );
	delete v;
}
expression(e) ::= T_ID(i) S_LANGLE exprlist(a) S_RANGLE. {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= T_ID(I) S_LANGLE exprlist(A) S_RANGLE.";
	POINTERCHECK(i);
	POINTERCHECK(a);
	e = new exo::ast::FunCall( TOKENSTR(i), a );
	delete i;
}
expression(e) ::= T_VAR(v). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= T_VAR(V).";
	POINTERCHECK(v);
	e = new exo::ast::VarExpr( TOKENSTR(v) );
	delete v;
}
expression(e) ::= constant(c). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= constant(C).";
	POINTERCHECK(c);
	e = c;
}
expression(e) ::= expression(a) S_PLUS expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) S_PLUS expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::BinaryOp( a, "+", b );
}
expression(e) ::= expression(a) S_MINUS expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) S_MINUS expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::BinaryOp( a, "-", b );
}
expression(e) ::= expression(a) S_MUL expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) S_MUL expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::BinaryOp( a, "*", b );
}
expression(e) ::= expression(a) S_DIV expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) S_DIV expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::BinaryOp( a, "/", b );
}
expression(e) ::= expression(a) S_EQ expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) S_EQ expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::CmpOp( a, "==", b );
}
expression(e) ::= expression(a) S_NE expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) S_NE expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::CmpOp( a, "!=", b );
}
expression(e) ::= expression(a) S_LT expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) S_LT expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::CmpOp( a, "<", b );
}
expression(e) ::= expression(a) S_LE expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) S_LE expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::CmpOp( a, "<=", b );
}
expression(e) ::= expression(a) S_GT expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) S_GT expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::CmpOp( a, ">", b );
}
expression(e) ::= expression(a) S_GE expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) S_GE expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::CmpOp( a, ">=", b );
}


/* a constant can be a builtin, a number or a string */
%type constant { exo::ast::Expr* }
constant(c) ::= S_FILE. {
	BOOST_LOG_TRIVIAL(trace) << "constant(C) ::= S_FILE.";
	c = new exo::ast::ConstStr( ast->fileName );
}
constant(c) ::= S_LINE(l). {
	BOOST_LOG_TRIVIAL(trace) << "constant(C) ::= S_LINE(L).";
	POINTERCHECK(l);
	c = new exo::ast::ConstInt( l->line_number() );
}
constant(c) ::= T_VNULL. {
	BOOST_LOG_TRIVIAL(trace) << "constant(C) ::= T_VNULL.";
	c = new exo::ast::ConstNull();
}
constant(c) ::= T_VTRUE. {
	BOOST_LOG_TRIVIAL(trace) << "constant(C) ::= T_VTRUE.";
	c = new exo::ast::ConstBool( true );
}
constant(c) ::= T_VFALSE. {
	BOOST_LOG_TRIVIAL(trace) << "constant(C) ::= T_VFALSE.";
	c = new exo::ast::ConstBool( false );
}
constant(c) ::= number(n). {
	BOOST_LOG_TRIVIAL(trace) << "constant(C) ::= number(N).";
	POINTERCHECK(n);
	c = n;
}
constant(c) ::= string(s). {
	BOOST_LOG_TRIVIAL(trace) << "constant(C) ::= string(S).";
	POINTERCHECK(s);
	c = s;
}

/* a number may be an integer or a float */
%type number { exo::ast::Expr* }
number(n) ::= T_VINT(i). {
	BOOST_LOG_TRIVIAL(trace) << "number(N) ::= T_VINT(I).";
	POINTERCHECK(i);
	n = new exo::ast::ConstInt( boost::lexical_cast<long>( TOKENSTR(i) ) );
	delete i;
}
number(n) ::= T_VFLOAT(f). {
	BOOST_LOG_TRIVIAL(trace) << "number(N) ::= T_VFLOAT(F).";
	POINTERCHECK(f);
	n = new exo::ast::ConstFloat( boost::lexical_cast<double>( TOKENSTR(f) ) );
	delete f;
}
/* a string is delimited by double quotes */
%type string { exo::ast::Expr* }
string(s) ::= T_QUOTE T_VSTRING(q) T_QUOTE. {
	BOOST_LOG_TRIVIAL(trace) << "string(S) ::= T_QUOTE T_VSTRING(Q) T_QUOTE.";
	POINTERCHECK(q);
	s = new exo::ast::ConstStr( TOKENSTR(q) );
	delete q;
}