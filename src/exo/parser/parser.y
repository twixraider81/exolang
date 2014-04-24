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


/* more like the "end" reduce ;) */
%start_symbol program

%token_prefix QUEX_TKN_
%token_type { quex::Token* }

/* the extra argument ist unused at the moment */
%extra_argument { exo::ast::Tree *ast }

/* everything decends from an ast node, tell lemon about it */
%default_type { exo::ast::Node* }


/* token precedences */
%right	S_ASSIGN.
%left	S_EQ S_NE.
%left	S_LT S_LE S_GT S_GE.
%left	S_PLUS S_MINUS.
%left	S_MUL S_DIV.

/* a program is build out of statements. */
program ::= statements(s). {
	ast->stmts = s;
}


/* statements are a single statement followed by ; and other statements */
%type statements { exo::ast::StmtList* }
statements(a) ::= statement(b). {
	POINTERCHECK( b );
	a = new exo::ast::StmtList;
	a->list.push_back( b );
	BOOST_LOG_TRIVIAL(trace) << "Pushing statement; size:" << a->list.size();
}
statements(s) ::= statements(a) statement(b). {
	POINTERCHECK( a );
	POINTERCHECK( b );
	a->list.push_back( b );
	s = a;
	BOOST_LOG_TRIVIAL(trace) << "Pushing statement; size:" << a->list.size();
}


/* statement can be a variable declaration, function (proto) declaration, class declaration, a return statement or an expression. statements are terminated by a semicolon */
%type statement { exo::ast::Stmt* }
statement(s) ::= vardecl(v) S_SEMICOLON. {
	POINTERCHECK( v );
	s = v;
}
statement(s) ::= fundeclproto(f) S_SEMICOLON. {
	POINTERCHECK( f );
	s = f;
}
statement(s) ::= fundecl(f) S_SEMICOLON. {
	POINTERCHECK( f );
	s = f;
}
statement(s) ::= classdecl(c) S_SEMICOLON. {
	POINTERCHECK( c );
	s = c;
}
statement(s) ::= S_RETURN expression(e) S_SEMICOLON. {
	POINTERCHECK( e );
	s = new exo::ast::StmtReturn( e );
}
statement(s) ::= expression(e) S_SEMICOLON. {
	POINTERCHECK( e );
	s = new exo::ast::StmtExpr( e );
}


/* a block is empty (i.e. protofunctions) or a collection of statements delimited by brackets */
%type block { exo::ast::StmtList* }
block(b) ::= S_LBRACKET S_RBRACKET. {
	b = new exo::ast::StmtList;
}
block(b) ::= S_LBRACKET statements(s) S_RBRACKET. {
	POINTERCHECK( s );
	b = s;
}


/* a type may be a null, bool, integer, float, string or auto */
%type type { exo::ast::Type* }
type(t) ::= T_TBOOL. {
	t = new exo::ast::Type( &typeid( exo::jit::types::BooleanType ) );
}
type(t) ::= T_TINT. {
	t = new exo::ast::Type( &typeid( exo::jit::types::IntegerType ) );
}
type(t) ::= T_TFLOAT. {
	t = new exo::ast::Type( &typeid( exo::jit::types::FloatType ) );
}
type(t) ::= T_TSTRING. {
	t = new exo::ast::Type( &typeid( exo::jit::types::StringType ) );
}
type(t) ::= T_TAUTO. {
	t = new exo::ast::Type( &typeid( exo::jit::types::AutoType ) );
}
type(t) ::= T_TCALLABLE. {
	t = new exo::ast::Type( &typeid( exo::jit::types::CallableType ) );
}
type(t) ::= T_ID(i). {
	POINTERCHECK( i );
	t = new exo::ast::Type( &typeid( exo::jit::types::ClassType ), TOKENSTR( i ) );
}


/* a variable declaration is a type identifier followed by a variable name optionally followed by an assignment to an expression */
%type vardecl { exo::ast::VarDecl* }
vardecl(d) ::= type(t) T_VAR(v). {
	POINTERCHECK( t );
	POINTERCHECK( v );
	d = new exo::ast::VarDecl( TOKENSTR(v), t );
}
vardecl(d) ::= type(t) T_VAR(v) S_ASSIGN expression(e). {
	POINTERCHECK( t );
	POINTERCHECK( v );
	POINTERCHECK( e );
	d = new exo::ast::VarDecl( TOKENSTR(v), t, e );
}


/* a variable declaration lists are variable declarations seperated by a colon or empty */
%type vardecllist { exo::ast::VarDeclList* }
vardecllist (l)::= . {
	l = new exo::ast::VarDeclList;
	BOOST_LOG_TRIVIAL(trace) << "Pushing variable declaration; size:" << l->list.size();
}
vardecllist(l) ::= vardecl(d). {
	POINTERCHECK( d );
	l = new exo::ast::VarDeclList;
	l->list.push_back( d );
	BOOST_LOG_TRIVIAL(trace) << "Pushing variable declaration; size:" << l->list.size();
}
vardecllist ::= vardecllist(l) S_COMMA vardecl(d). {
	POINTERCHECK( l );
	POINTERCHECK( d );
	l->list.push_back( d );
	BOOST_LOG_TRIVIAL(trace) << "Pushing variable declaration; size:" << l->list.size();
}


/* a function declaration is a type identifier followed by the keyword function a functionname, optionally function arguments in brackets. if it has an associated block its a proper function and not a prototype */
%type fundeclproto { exo::ast::FunDeclProto* }
fundeclproto(f) ::= type(t) S_FUNCTION T_ID(i) S_LANGLE vardecllist(a) S_RANGLE. {
	POINTERCHECK( t );
	POINTERCHECK( i );
	POINTERCHECK( a );
	f = new exo::ast::FunDeclProto( TOKENSTR(i), t, a );
}
fundeclproto(f) ::= S_FUNCTION T_ID(i) S_LANGLE vardecllist(a) S_RANGLE. {
	POINTERCHECK( i );
	POINTERCHECK( a );
	f = new exo::ast::FunDeclProto( TOKENSTR(i), new exo::ast::Type( &typeid( exo::jit::types::AutoType ) ), a );
}
fundeclproto(f) ::= type(t) S_FUNCTION T_ID(i). {
	POINTERCHECK( t );
	POINTERCHECK( i );
	f = new exo::ast::FunDeclProto( TOKENSTR(i), t, new exo::ast::VarDeclList );
}
fundeclproto(f) ::= S_FUNCTION T_ID(i). {
	POINTERCHECK( i );
	f = new exo::ast::FunDeclProto( TOKENSTR(i), new exo::ast::Type( &typeid( exo::jit::types::AutoType ) ), new exo::ast::VarDeclList );
}
%type fundecl { exo::ast::FunDecl* }
fundecl(f) ::= type(t) S_FUNCTION T_ID(i) S_LANGLE vardecllist(a) S_RANGLE block(b). {
	POINTERCHECK( t );
	POINTERCHECK( i );
	POINTERCHECK( a );
	POINTERCHECK( b );
	f = new exo::ast::FunDecl( TOKENSTR(i), t, a, b );
}
fundecl(f) ::= S_FUNCTION T_ID(i) S_LANGLE vardecllist(a) S_RANGLE block(b). {
	POINTERCHECK( i );
	POINTERCHECK( a );
	POINTERCHECK( b );
	f = new exo::ast::FunDecl( TOKENSTR(i), new exo::ast::Type( &typeid( exo::jit::types::AutoType ) ), a, b );
}
fundecl(f) ::= type(t) S_FUNCTION T_ID(i) block(b). {
	POINTERCHECK( t );
	POINTERCHECK( i );
	POINTERCHECK( b );
	f = new exo::ast::FunDecl( TOKENSTR(i), t, new exo::ast::VarDeclList, b );
}
fundecl(f) ::= S_FUNCTION T_ID(i) block(b). {
	POINTERCHECK( i );
	POINTERCHECK( b );
	f = new exo::ast::FunDecl( TOKENSTR(i), new exo::ast::Type( &typeid( exo::jit::types::AutoType ) ), new exo::ast::VarDeclList, b );
}


/* a class declaration is a class keyword, followed by an identifier and class block */
%type classdecl { exo::ast::ClassDecl* }
classdecl(c) ::= T_CLASS T_ID(i) S_LBRACKET classblock(b) S_RBRACKET. {
	POINTERCHECK( i );
	POINTERCHECK( b );
	c = new exo::ast::ClassDecl( TOKENSTR(i), b );
}
/* a class block contains the declarations of a class. that is properties and methods. */
%type classblock { exo::ast::ClassBlock* }
classblock(b) ::= . {
	b = new exo::ast::ClassBlock;
	BOOST_LOG_TRIVIAL(trace) << "Pushing class declaration; properties: " << b->properties.size() << ", methods: " << b->methods.size();
}
classblock(b) ::= vardecl(d) S_SEMICOLON. {
	POINTERCHECK( d );
	b = new exo::ast::ClassBlock;
	b->properties.push_back( d );
	BOOST_LOG_TRIVIAL(trace) << "Pushing property; properties: " << b->properties.size();
}
classblock(b) ::= fundecl(d) S_SEMICOLON. {
	POINTERCHECK( d );
	b = new exo::ast::ClassBlock;
	b->methods.push_back( d );
	BOOST_LOG_TRIVIAL(trace) << "Pushing method; methods: " << b->methods.size();
}
classblock(b) ::= classblock(l) vardecl(d) S_SEMICOLON. {
	POINTERCHECK( l );
	POINTERCHECK( d );
	l->properties.push_back( d );
	b = l;
	BOOST_LOG_TRIVIAL(trace) << "Pushing property; properties: " << l->properties.size();
}
classblock(b) ::= classblock(l) fundecl(d) S_SEMICOLON. {
	POINTERCHECK( l );
	POINTERCHECK( d );
	l->methods.push_back( d );
	b = l;
	BOOST_LOG_TRIVIAL(trace) << "Pushing method; methods: " << l->methods.size();
}


/* an expression list */
%type exprlist { exo::ast::ExprList* }
exprlist(l) ::= . {
	l = new exo::ast::ExprList;
	BOOST_LOG_TRIVIAL(trace) << "Pushing expression; expressions:" << l->list.size();
}
exprlist(l) ::= expression(e). {
	POINTERCHECK( e );
	l = new exo::ast::ExprList;
	l->list.push_back( e );
	BOOST_LOG_TRIVIAL(trace) << "Pushing expression; expressions:" << l->list.size();
}
exprlist ::= exprlist(l) S_COMMA expression(e). {
	POINTERCHECK( l );
	POINTERCHECK( e );
	l->list.push_back( e );
	BOOST_LOG_TRIVIAL(trace) << "Pushing declaration; expressions:" << l->list.size();
}


/* a number may be an integer or a float */
%type number { exo::ast::Expr* }
number(n) ::= T_VINT(i). {
	POINTERCHECK( i );
	n = new exo::ast::ValueInt( TOKENSTR(i) );
}
number(n) ::= T_VFLOAT(f). {
	POINTERCHECK( f );
	n = new exo::ast::ValueFloat( TOKENSTR(f) );
}


/* an expression may be an assignment, function call, variable, number, binary expression, comparison or a constant */
%type expression { exo::ast::Expr* }
expression(a) ::= T_VAR(v) S_ASSIGN expression(e). {
	POINTERCHECK( v );
	POINTERCHECK( e );
	a = new exo::ast::VarAssign( TOKENSTR(v), e );
}
expression(e) ::= T_ID(i) S_LANGLE exprlist(a) S_RANGLE. {
	POINTERCHECK( i );
	POINTERCHECK( a );
	e = new exo::ast::FunCall( TOKENSTR(i), a );
}
expression(e) ::= T_VAR(v). {
	POINTERCHECK( v );
	e = new exo::ast::VarExpr( TOKENSTR(v) );
}
expression(e) ::= number(n). {
	POINTERCHECK( n );
	e = n;
}
expression(e) ::= expression(a) binop(o) expression(b). {
	POINTERCHECK( a );
	POINTERCHECK( b );
	POINTERCHECK( o );
	e = new exo::ast::BinaryOp( a, o, b );
}
expression(e) ::= expression(a) cmpop(c) expression(b). {
	POINTERCHECK( a );
	POINTERCHECK( b );
	POINTERCHECK( c );
	e = new exo::ast::CmpOp( a, c, b );
}
expression(e) ::= constant(c). {
	POINTERCHECK( c );
	e = c;
}


/* comparison operators */
%type cmpop { std::string* }
cmpop(c) ::= S_EQ(o). {
	POINTERCHECK( o );
	c = new std::string( TOKENSTR( o ) );
}
cmpop(c) ::= S_NE(o). {
	POINTERCHECK( o );
	c = new std::string( TOKENSTR( o ) );
}
cmpop(c) ::= S_LT(o). {
	POINTERCHECK( o );
	c = new std::string( TOKENSTR( o ) );
}
cmpop(c) ::= S_LE(o). {
	POINTERCHECK( o );
	c = new std::string( TOKENSTR( o ) );
}
cmpop(c) ::= S_GT(o). {
	POINTERCHECK( o );
	c = new std::string( TOKENSTR( o ) );
}
cmpop(c) ::= S_GE(o). {
	POINTERCHECK( o );
	c = new std::string( TOKENSTR( o ) );
}


/* binary operators */
%type binop { std::string* }
binop(b) ::= S_PLUS(o). {
	POINTERCHECK( o );
	b = new std::string( TOKENSTR( o ) );
}
binop(b) ::= S_MINUS(o). {
	POINTERCHECK( o );
	b = new std::string( TOKENSTR( o ) );
}
binop(b) ::= S_MUL(o). {
	POINTERCHECK( o );
	b = new std::string( TOKENSTR( o ) );
}
binop(b) ::= S_DIV(o). {
	POINTERCHECK( o );
	b = new std::string( TOKENSTR( o ) );
}


/* a constant can be a builtin */
%type constant { exo::ast::Expr* }
constant(c) ::= S_FILE(f). {
	POINTERCHECK( f );
	c = new exo::ast::ConstExpr( TOKENSTR(f), new exo::ast::ValueString( ast->fileName ) );
}
constant(c) ::= S_LINE(l). {
	POINTERCHECK( l );
	c = new exo::ast::ConstExpr( TOKENSTR(l), new exo::ast::ValueInt( std::to_string( l->line_number() ) ) );
}
constant(c) ::= T_VNULL. {
	c = new exo::ast::ConstExpr( "false", new exo::ast::ValueNull() );
}
constant(c) ::= T_VTRUE. {
	c = new exo::ast::ConstExpr( "true", new exo::ast::ValueBool( true ) );
}
constant(c) ::= T_VFALSE. {
	c = new exo::ast::ConstExpr( "false", new exo::ast::ValueBool( false ) );
}