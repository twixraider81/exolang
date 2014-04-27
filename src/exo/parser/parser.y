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
	std::stringstream m;
	m << "Unexpected \"" << TOKEN->type_id_name() << "\" on " << TOKEN->line_number() << ":" << TOKEN->column_number();
	EXO_THROW_EXCEPTION( UnexpectedToken, m.str() );
}
%stack_overflow {
	EXO_THROW_EXCEPTION( StackOverflow, "Stack overflow." );
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
%right		T_TBOOL T_TINT T_TFLOAT T_TSTRING T_TAUTO T_TCALLABLE S_ID.
%right		T_ASSIGN.
%left		T_EQ T_NE.
%left		T_LT T_LE T_GT T_GE.
%left		T_PLUS T_MINUS.
%left		T_MUL T_DIV.
%left		T_SEMICOLON.


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


/*
 * statement can be a variable declaration, function (proto) declaration, class declaration, a return statement or an expression.
 * statements are terminated by a semicolon
 */
%type statement { exo::ast::Stmt* }
statement(s) ::= vardecl(v) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "statement(S) ::= vardecl(V) T_SEMICOLON.";
	POINTERCHECK(v);
	s = v;
}
statement(s) ::= fundeclproto(f) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "statement(S) ::= fundeclproto(F) T_SEMICOLON.";
	POINTERCHECK(f);
	s = f;
}
statement(s) ::= fundecl(f) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "statement(S) ::= fundecl(F) T_SEMICOLON.";
	POINTERCHECK(f);
	s = f;
}
statement(s) ::= classdecl(c) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "statement(S) ::= classdecl(C) T_SEMICOLON.";
	POINTERCHECK(c);
	s = c;
}
statement(s) ::= T_RETURN expression(e) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "statement(S) ::= T_RETURN expression(E) T_SEMICOLON.";
	POINTERCHECK(e);
	s = new exo::ast::StmtReturn( e );
}
statement(s) ::= expression(e) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "statement(S) ::= expression(E) T_SEMICOLON.";
	POINTERCHECK(e);
	s = new exo::ast::StmtExpr( e );
}


/* a block is empty (i.e. protofunctions) or a collection of statements delimited by brackets */
%type block { exo::ast::StmtList* }
block(b) ::= T_LBRACKET T_RBRACKET. {
	BOOST_LOG_TRIVIAL(trace) << "block(B) ::= T_LBRACKET T_RBRACKET.";
	b = new exo::ast::StmtList;
}
block(b) ::= T_LBRACKET statements(s) T_RBRACKET. {
	BOOST_LOG_TRIVIAL(trace) << "block(B) ::= T_LBRACKET statements(S) T_RBRACKET.";
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
type(t) ::= S_ID(i). {
	BOOST_LOG_TRIVIAL(trace) << "type(T) ::= S_ID(I).";
	POINTERCHECK(i);
	t = new exo::ast::Type( TOKENSTR(i) );
	delete i;
}


/* a variable declaration is a type identifier followed by a variable name optionally followed by an assignment to an expression */
%type vardecl { exo::ast::DecVar* }
vardecl(d) ::= type(t) S_VAR(v). {
	BOOST_LOG_TRIVIAL(trace) << "vardecl(D) ::= type(T) S_VAR(V).";
	POINTERCHECK(t);
	POINTERCHECK(v);
	d = new exo::ast::DecVar( TOKENSTR(v), t );
	delete v;
}
vardecl(d) ::= type(t) S_VAR(v) T_ASSIGN expression(e). {
	BOOST_LOG_TRIVIAL(trace) << "vardecl(D) ::= type(T) S_VAR(V) T_ASSIGN expression(E).";
	POINTERCHECK(t);
	POINTERCHECK(v);
	POINTERCHECK(e);
	d = new exo::ast::DecVar( TOKENSTR(v), t, e );
	delete v;
}


/* a variable declaration lists are variable declarations seperated by a colon optionally or empty */
%type vardecllist { exo::ast::DecList* }
vardecllist(l)::= . {
	BOOST_LOG_TRIVIAL(trace) << "vardecllist(L)::= .";
	l = new exo::ast::DecList;
	BOOST_LOG_TRIVIAL(trace) << "Empty variable declaration list.";
}
vardecllist(l) ::= vardecl(d). {
	BOOST_LOG_TRIVIAL(trace) << "vardecllist(L) ::= vardecl(D).";
	POINTERCHECK(d);
	l = new exo::ast::DecList;
	l->list.push_back( d );
	BOOST_LOG_TRIVIAL(trace) << "Pushing variable $" << d->name << "; size:" << l->list.size();
}
vardecllist(e) ::= vardecllist(l) T_COMMA vardecl(d). {
	BOOST_LOG_TRIVIAL(trace) << "vardecllist(E) ::= vardecllist(L) T_COMMA vardecl(D).";
	POINTERCHECK(l);
	POINTERCHECK(d);
	l->list.push_back( d );
	e = l;
	BOOST_LOG_TRIVIAL(trace) << "Pushing variable $" << d->name << "; size:" << l->list.size();
}

/*
 * a function declaration is a type identifier followed by the keyword function a functionname
 * optionally function arguments in brackets. if it has an associated block its a proper function and not a prototype
 */
%type fundeclproto { exo::ast::DecFunProto* }
fundeclproto(f) ::= type(t) T_FUNCTION S_ID(i) T_LANGLE vardecllist(l) T_RANGLE. {
	BOOST_LOG_TRIVIAL(trace) << "fundeclproto(F) ::= type(T) T_FUNCTION S_ID(I) T_LANGLE vardecllist(L) T_RANGLE.";
	POINTERCHECK(t);
	POINTERCHECK(i);
	POINTERCHECK(l);
	f = new exo::ast::DecFunProto( TOKENSTR(i), t, l, false );
	delete i;
}
fundeclproto(f) ::= type(t) T_FUNCTION S_ID(i) T_LANGLE vardecllist(l) T_VARG T_RANGLE. {
	BOOST_LOG_TRIVIAL(trace) << "fundeclproto(F) ::= type(T) T_FUNCTION S_ID(I) T_LANGLE vardecllist(L) T_VARG T_RANGLE.";
	POINTERCHECK(t);
	POINTERCHECK(i);
	POINTERCHECK(l);
	f = new exo::ast::DecFunProto( TOKENSTR(i), t, l, true );
	delete i;
}
%type fundecl { exo::ast::DecFun* }
fundecl(f) ::= type(t) T_FUNCTION S_ID(i) T_LANGLE vardecllist(l) T_RANGLE block(b). {
	BOOST_LOG_TRIVIAL(trace) << "fundecl(F) ::= type(T) T_FUNCTION S_ID(I) T_LANGLE vardecllist(L) T_RANGLE block(B).";
	POINTERCHECK(t);
	POINTERCHECK(i);
	POINTERCHECK(l);
	POINTERCHECK(b);
	f = new exo::ast::DecFun( TOKENSTR(i), t, l, b, false );
	delete i;
}
fundecl(f) ::= type(t) T_FUNCTION S_ID(i) T_LANGLE vardecllist(l) T_VARG T_RANGLE block(b). {
	BOOST_LOG_TRIVIAL(trace) << "fundecl(F) ::= type(T) T_FUNCTION S_ID(I) T_LANGLE vardecllist(L) T_VARG T_RANGLE block(B).";
	POINTERCHECK(t);
	POINTERCHECK(i);
	POINTERCHECK(l);
	POINTERCHECK(b);
	f = new exo::ast::DecFun( TOKENSTR(i), t, l, b, true );
	delete i;
}


/* a method declaration is an access modifier followed by a function declaration */
%type methoddecl { exo::ast::DecMethod* }
methoddecl(m) ::= access(a) fundecl(f). {
	BOOST_LOG_TRIVIAL(trace) << "methoddecl(M) ::= access(A) fundecl(F).";
	POINTERCHECK(a);
	POINTERCHECK(f);
	m = new exo::ast::DecMethod( f, a );
}


/* a property declaration is an access modifier followed by a variable declaration */
%type propdecl { exo::ast::DecProp* }
propdecl(p) ::= access(a) vardecl(v). {
	BOOST_LOG_TRIVIAL(trace) << "propdecl(P) ::= access(A) vardecl(V).";
	POINTERCHECK(a);
	POINTERCHECK(v);
	p = new exo::ast::DecProp( v, a );
}


/* a class declaration is a class keyword, followed by an identifier optionally and extend with a classname, and and associated class block */
%type classdecl { exo::ast::DecClass* }
classdecl(c) ::= T_CLASS S_ID(i) T_EXTENDS S_ID(p) T_LBRACKET classblock(b) T_RBRACKET. {
	BOOST_LOG_TRIVIAL(trace) << "classdecl(C) ::= T_CLASS S_ID(I) T_EXTENDS S_ID(P) T_LBRACKET classblock(B) T_RBRACKET.";
	POINTERCHECK(i);
	POINTERCHECK(p);
	POINTERCHECK(b);
	c = new exo::ast::DecClass( TOKENSTR(i), TOKENSTR(p), b );
	delete i;
	delete p;
}
classdecl(c) ::= T_CLASS S_ID(i) T_EXTENDS S_ID(p) T_LBRACKET T_RBRACKET. {
	BOOST_LOG_TRIVIAL(trace) << "classdecl(C) ::= T_CLASS S_ID(I) T_EXTENDS S_ID(P) T_LBRACKET T_RBRACKET.";
	POINTERCHECK(i);
	POINTERCHECK(p);
	c = new exo::ast::DecClass( TOKENSTR(i), TOKENSTR(p), new exo::ast::ClassBlock );
	delete i;
	delete p;
}
classdecl(c) ::= T_CLASS S_ID(i) T_LBRACKET classblock(b) T_RBRACKET. {
	BOOST_LOG_TRIVIAL(trace) << "classdecl(C) ::= T_CLASS S_ID(I) T_LBRACKET classblock(B) T_RBRACKET.";
	POINTERCHECK(i);
	POINTERCHECK(b);
	c = new exo::ast::DecClass( TOKENSTR(i), b );
	delete i;
}
classdecl(c) ::= T_CLASS S_ID(i) T_LBRACKET T_RBRACKET. {
	BOOST_LOG_TRIVIAL(trace) << "classdecl(C) ::= T_CLASS S_ID(I) T_LBRACKET T_RBRACKET.";
	POINTERCHECK(i);
	c = new exo::ast::DecClass( TOKENSTR(i), new exo::ast::ClassBlock );
	delete i;
}

/* a class block contains the declarations of a class. that is properties and methods. */
%type classblock { exo::ast::ClassBlock* }
classblock(b) ::= propdecl(d) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "classblock(B) ::= propdecl(D) T_SEMICOLON.";
	POINTERCHECK(d);
	b = new exo::ast::ClassBlock;
	b->properties.push_back( d );
	BOOST_LOG_TRIVIAL(trace) << "Pushing property \"" << d->property->name << "\"; properties: " << b->properties.size();
}
classblock(b) ::= methoddecl(d) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "classblock(B) ::= methoddecl(D) T_SEMICOLON.";
	POINTERCHECK(d);
	b = new exo::ast::ClassBlock;
	b->methods.push_back( d );
	BOOST_LOG_TRIVIAL(trace) << "Pushing method \"" << d->method->name << "\"; methods: " << b->methods.size();
}
classblock(b) ::= classblock(l) propdecl(d) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "classblock(B) ::= classblock(L) propdecl(D) T_SEMICOLON.";
	POINTERCHECK(l);
	POINTERCHECK(d);
	l->properties.push_back( d );
	b = l;
	BOOST_LOG_TRIVIAL(trace) << "Pushing property \"" << d->property->name << "\"; properties: " << l->properties.size();
}
classblock(b) ::= classblock(l) methoddecl(d) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(trace) << "classblock(B) ::= classblock(L) methoddecl(D) T_SEMICOLON.";
	POINTERCHECK(l);
	POINTERCHECK(d);
	l->methods.push_back( d );
	b = l;
	BOOST_LOG_TRIVIAL(trace) << "Pushing method \"" << d->method->name << "\"; methods: " << l->methods.size();
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
exprlist(f) ::= exprlist(l) T_COMMA expression(e). {
	BOOST_LOG_TRIVIAL(trace) << "exprlist(F) ::= exprlist(L) T_COMMA expression(E).";
	POINTERCHECK(l);
	POINTERCHECK(e);
	l->list.push_back( e );
	f = l;
	BOOST_LOG_TRIVIAL(trace) << "Pushing declaration; expressions:" << l->list.size();
}


/* an expression may be an assignment, function call, variable, constant, binary expression, comparison */
%type expression { exo::ast::Expr* }
expression(a) ::= S_VAR(v) T_ASSIGN expression(e). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= S_VAR(V) T_ASSIGN expression(E).";
	POINTERCHECK(v);
	POINTERCHECK(e);
	a = new exo::ast::AssignVar( TOKENSTR(v), e );
	delete v;
}
expression(e) ::= S_ID(i) T_LANGLE exprlist(a) T_RANGLE. {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= S_ID(I) T_LANGLE exprlist(A) T_RANGLE.";
	POINTERCHECK(i);
	POINTERCHECK(a);
	e = new exo::ast::CallFun( TOKENSTR(i), a );
	delete i;
}
expression(e) ::= S_VAR(v). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= S_VAR(V).";
	POINTERCHECK(v);
	e = new exo::ast::ExprVar( TOKENSTR(v) );
	delete v;
}
expression(e) ::= constant(c). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= constant(C).";
	POINTERCHECK(c);
	e = c;
}
expression(e) ::= expression(a) T_PLUS expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) T_PLUS expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryAdd( a, b );
}
expression(e) ::= expression(a) T_MINUS expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) T_MINUS expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinarySub( a, b );
}
expression(e) ::= expression(a) T_MUL expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) T_MUL expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryMul( a, b );
}
expression(e) ::= expression(a) T_DIV expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) T_DIV expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryDiv( a, b );
}
expression(e) ::= expression(a) T_EQ expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) T_EQ expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryEq( a, b );
}
expression(e) ::= expression(a) T_NE expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) T_NE expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryNeq( a, b );
}
expression(e) ::= expression(a) T_LT expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) T_LT expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryLt( a, b );
}
expression(e) ::= expression(a) T_LE expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) T_LE expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryLe( a, b );
}
expression(e) ::= expression(a) T_GT expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) T_GT expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryGt( a, b );
}
expression(e) ::= expression(a) T_GE expression(b). {
	BOOST_LOG_TRIVIAL(trace) << "expression(E) ::= expression(A) T_GE expression(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryGe( a, b );
}


/* a constant can be a builtin, a number or a string */
%type constant { exo::ast::Expr* }
constant(c) ::= T_FILE. {
	BOOST_LOG_TRIVIAL(trace) << "constant(C) ::= T_FILE.";
	c = new exo::ast::ConstStr( ast->fileName );
}
constant(c) ::= T_LINE(l). {
	BOOST_LOG_TRIVIAL(trace) << "constant(C) ::= T_LINE(L).";
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
number(n) ::= S_INT(i). {
	BOOST_LOG_TRIVIAL(trace) << "number(N) ::= S_INT(I).";
	POINTERCHECK(i);
	n = new exo::ast::ConstInt( boost::lexical_cast<long>( TOKENSTR(i) ) );
	delete i;
}
number(n) ::= S_FLOAT(f). {
	BOOST_LOG_TRIVIAL(trace) << "number(N) ::= S_FLOAT(F).";
	POINTERCHECK(f);
	n = new exo::ast::ConstFloat( boost::lexical_cast<double>( TOKENSTR(f) ) );
	delete f;
}


/* a string is delimited by double quotes */
%type string { exo::ast::Expr* }
string(s) ::= T_QUOTE S_STRING(q) T_QUOTE. {
	BOOST_LOG_TRIVIAL(trace) << "string(S) ::= T_QUOTE S_STRING(Q) T_QUOTE.";
	POINTERCHECK(q);
	s = new exo::ast::ConstStr( TOKENSTR(q) );
	delete q;
}


/* an access modifier is either public, private or protected */
%type access { exo::ast::ModAccess* }
access(a) ::= T_PUBLIC. {
	BOOST_LOG_TRIVIAL(trace) << "access(A) ::= T_PUBLIC.";
	a = new exo::ast::ModAccess();
}
access(a) ::= T_PRIVATE. {
	BOOST_LOG_TRIVIAL(trace) << "access(A) ::= T_PRIVATE.";
	a = new exo::ast::ModAccess();
}
access(a) ::= T_PROTECTED. {
	BOOST_LOG_TRIVIAL(trace) << "access(A) ::= T_PROTECTED.";
	a = new exo::ast::ModAccess();
}