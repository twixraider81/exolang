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
%left		T_PTR.
%right		T_NEW T_DELETE.
%left		T_SEMICOLON.


/* a program is build out of statements. */
program ::= stmts(s). {
	BOOST_LOG_TRIVIAL(debug) << "program ::= stmts(S).";
	ast->stmts = s;
}


/* statements are a single statement followed by ; and other statements */
%type stmts { exo::ast::StmtList* }
%destructor stmts { delete $$; }
stmts(a) ::= stmt(b). {
	BOOST_LOG_TRIVIAL(debug) << "stmts(A) ::= stmt(B).";
	POINTERCHECK(b);
	a = new exo::ast::StmtList;
	a->list.push_back( b );
	BOOST_LOG_TRIVIAL(trace) << "Pushing statement; size:" << a->list.size();
}
stmts(s) ::= stmts(a) stmt(b). {
	BOOST_LOG_TRIVIAL(debug) << "stmts(S) ::= stmts(A) stmt(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	a->list.push_back( b );
	s = a;
	BOOST_LOG_TRIVIAL(trace) << "Pushing statement; size:" << a->list.size();
}


/*
 * statement can be a variable declaration, function (proto) declaration, class declaration, delete statement, a return statement, if flow or an expression.
 * statements are terminated by a semicolon
 */
%type stmt { exo::ast::Stmt* }
%destructor stmt { delete $$; }
stmt(s) ::= vardec(v) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(debug) << "stmt(S) ::= vardec(V) T_SEMICOLON.";
	POINTERCHECK(v);
	s = v;
}
stmt(s) ::= funproto(f) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(debug) << "stmt(S) ::= funproto(F) T_SEMICOLON.";
	POINTERCHECK(f);
	s = f;
}
stmt(s) ::= fundec(f) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(debug) << "stmt(S) ::= fundec(F) T_SEMICOLON.";
	POINTERCHECK(f);
	s = f;
}
stmt(s) ::= classdec(c) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(debug) << "stmt(S) ::= classdec(C) T_SEMICOLON.";
	POINTERCHECK(c);
	s = c;
}
stmt(s) ::= T_RETURN expr(e) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(debug) << "stmt(S) ::= T_RETURN expr(E) T_SEMICOLON.";
	POINTERCHECK(e);
	s = new exo::ast::StmtReturn( e );
}
stmt(s) ::= stmtif(i) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(debug) << "stmt(S) ::= stmtif(I) T_SEMICOLON.";
	POINTERCHECK(i);
	s = i;
}
stmt(s) ::= expr(e) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(debug) << "stmt(S) ::= expr(E) T_SEMICOLON.";
	POINTERCHECK(e);
	s = new exo::ast::StmtExpr( e );
}


/* a block is empty (i.e. protofunctions, class blocks ) or a collection of statements delimited by brackets */
%type block { exo::ast::StmtList* }
%destructor block { delete $$; }
block(b) ::= T_LBRACKET T_RBRACKET. {
	BOOST_LOG_TRIVIAL(debug) << "block(B) ::= T_LBRACKET T_RBRACKET.";
	b = new exo::ast::StmtList;
}
block(b) ::= T_LBRACKET stmts(s) T_RBRACKET. {
	BOOST_LOG_TRIVIAL(debug) << "block(B) ::= T_LBRACKET stmts(S) T_RBRACKET.";
	POINTERCHECK(s);
	b = s;
}


/* an if block */
%type stmtif { exo::ast::StmtIf* }
%destructor stmtif { delete $$; }
stmtif(i) ::= T_IF T_LANGLE expr(e) T_RANGLE block(t). {
	BOOST_LOG_TRIVIAL(debug) << "stmtif(I) ::= T_IF T_LANGLE expr(E) T_RANGLE block(T).";
	POINTERCHECK(e);
	POINTERCHECK(t);
	i = new exo::ast::StmtIf( e, t, new exo::ast::StmtList() );
}
stmtif(i) ::= T_IF T_LANGLE expr(e) T_RANGLE block(t) T_ELSE block(f). {
	BOOST_LOG_TRIVIAL(debug) << "stmtif(I) ::= T_IF T_LANGLE expr(E) T_RANGLE block(T) T_ELSE block(F).";
	POINTERCHECK(e);
	POINTERCHECK(t);
	POINTERCHECK(f);
	i = new exo::ast::StmtIf( e, t, f );
}

	
/* a type may be a null, bool, integer, float, string or auto or a classname */
%type type { exo::ast::Type* }
%destructor type { delete $$; }
type(t) ::= T_TBOOL. {
	BOOST_LOG_TRIVIAL(debug) << "type(T) ::= T_TBOOL.";
	t = new exo::ast::Type( "bool" );
}
type(t) ::= T_TINT. {
	BOOST_LOG_TRIVIAL(debug) << "type(T) ::= T_TINT.";
	t = new exo::ast::Type( "int" );
}
type(t) ::= T_TFLOAT. {
	BOOST_LOG_TRIVIAL(debug) << "type(T) ::= T_TFLOAT.";
	t = new exo::ast::Type( "float" );
}
type(t) ::= T_TSTRING. {
	BOOST_LOG_TRIVIAL(debug) << "type(T) ::= T_TSTRING.";
	t = new exo::ast::Type( "string" );
}
type(t) ::= T_TAUTO. {
	BOOST_LOG_TRIVIAL(debug) << "type(T) ::= T_TAUTO.";
	t = new exo::ast::Type( "auto" );
}
type(t) ::= T_TCALLABLE. {
	BOOST_LOG_TRIVIAL(debug) << "type(T) ::= T_TCALLABLE.";
	t = new exo::ast::Type( "callable" );
}
type(t) ::= S_ID(i). {
	BOOST_LOG_TRIVIAL(debug) << "type(T) ::= S_ID(I).";
	POINTERCHECK(i);
	t = new exo::ast::Type( TOKENSTR(i) );
	delete i;
}


/* a variable declaration is a type identifier followed by a variable name optionally followed by an assignment to an expression */
%type vardec { exo::ast::DecVar* }
%destructor vardec { delete $$; }
vardec(d) ::= type(t) S_VAR(v). {
	BOOST_LOG_TRIVIAL(debug) << "vardec(D) ::= type(T) S_VAR(V).";
	POINTERCHECK(t);
	POINTERCHECK(v);
	d = new exo::ast::DecVar( TOKENSTR(v), t );
	delete v;
}
vardec(d) ::= type(t) S_VAR(v) T_ASSIGN expr(e). {
	BOOST_LOG_TRIVIAL(debug) << "vardec(D) ::= type(T) S_VAR(V) T_ASSIGN expr(E).";
	POINTERCHECK(t);
	POINTERCHECK(v);
	POINTERCHECK(e);
	d = new exo::ast::DecVar( TOKENSTR(v), t, e );
	delete v;
}


/* a variable declaration lists are variable declarations seperated by a colon optionally or empty */
%type vardeclist { exo::ast::DecList* }
%destructor vardeclist { delete $$; }
vardeclist(l)::= . {
	BOOST_LOG_TRIVIAL(debug) << "vardeclist(L)::= .";
	l = new exo::ast::DecList;
}
vardeclist(l) ::= vardec(d). {
	BOOST_LOG_TRIVIAL(debug) << "vardeclist(L) ::= vardec(D).";
	POINTERCHECK(d);
	l = new exo::ast::DecList;
	l->list.push_back( d );
}
vardeclist(e) ::= vardeclist(l) T_COMMA vardec(d). {
	BOOST_LOG_TRIVIAL(debug) << "vardeclist(E) ::= vardeclist(L) T_COMMA vardec(D).";
	POINTERCHECK(l);
	POINTERCHECK(d);
	l->list.push_back( d );
	e = l;
}

/*
 * a function declaration is a type identifier followed by the keyword function a functionname
 * optionally function arguments in brackets. if it has an associated block its a proper function and not a prototype
 */
%type funproto { exo::ast::DecFunProto* }
%destructor funproto { delete $$; }
funproto(f) ::= type(t) T_FUNCTION S_ID(i) T_LANGLE vardeclist(l) T_RANGLE. {
	BOOST_LOG_TRIVIAL(debug) << "funproto(F) ::= type(T) T_FUNCTION S_ID(I) T_LANGLE vardeclist(L) T_RANGLE.";
	POINTERCHECK(t);
	POINTERCHECK(i);
	POINTERCHECK(l);
	f = new exo::ast::DecFunProto( TOKENSTR(i), t, l, false );
	delete i;
}
funproto(f) ::= type(t) T_FUNCTION S_ID(i) T_LANGLE vardeclist(l) T_VARG T_RANGLE. {
	BOOST_LOG_TRIVIAL(debug) << "funproto(F) ::= type(T) T_FUNCTION S_ID(I) T_LANGLE vardeclist(L) T_VARG T_RANGLE.";
	POINTERCHECK(t);
	POINTERCHECK(i);
	POINTERCHECK(l);
	f = new exo::ast::DecFunProto( TOKENSTR(i), t, l, true );
	delete i;
}
%type fundec { exo::ast::DecFun* }
%destructor fundec { delete $$; }
fundec(f) ::= type(t) T_FUNCTION S_ID(i) T_LANGLE vardeclist(l) T_RANGLE block(b). {
	BOOST_LOG_TRIVIAL(debug) << "fundec(F) ::= type(T) T_FUNCTION S_ID(I) T_LANGLE vardeclist(L) T_RANGLE block(B).";
	POINTERCHECK(t);
	POINTERCHECK(i);
	POINTERCHECK(l);
	POINTERCHECK(b);
	f = new exo::ast::DecFun( TOKENSTR(i), t, l, b, false );
	delete i;
}
fundec(f) ::= type(t) T_FUNCTION S_ID(i) T_LANGLE vardeclist(l) T_VARG T_RANGLE block(b). {
	BOOST_LOG_TRIVIAL(debug) << "fundec(F) ::= type(T) T_FUNCTION S_ID(I) T_LANGLE vardeclist(L) T_VARG T_RANGLE block(B).";
	POINTERCHECK(t);
	POINTERCHECK(i);
	POINTERCHECK(l);
	POINTERCHECK(b);
	f = new exo::ast::DecFun( TOKENSTR(i), t, l, b, true );
	delete i;
}


/* a method declaration is an access modifier followed by a function declaration */
%type methoddec { exo::ast::DecMethod* }
%destructor methoddec { delete $$; }
methoddec(m) ::= access(a) fundec(f) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(debug) << "methoddec(M) ::= access(A) fundec(F) T_SEMICOLON.";
	POINTERCHECK(a);
	POINTERCHECK(f);
	m = new exo::ast::DecMethod( f, a );
}


/* a property declaration is an access modifier followed by a variable declaration */
%type propertydec { exo::ast::DecProp* }
%destructor propertydec { delete $$; }
propertydec(p) ::= access(a) vardec(v) T_SEMICOLON. {
	BOOST_LOG_TRIVIAL(debug) << "propertydec(P) ::= access(A) vardec(V) T_SEMICOLON.";
	POINTERCHECK(a);
	POINTERCHECK(v);
	p = new exo::ast::DecProp( v, a );
}


/* a class declaration is a class keyword, followed by an identifier, optionally an extend with a classname, and and associated class block */
%type classdec { exo::ast::DecClass* }
%destructor classdec { delete $$; }
classdec(c) ::= T_CLASS S_ID(i) T_EXTENDS S_ID(p) T_LBRACKET classblock(b) T_RBRACKET. {
	BOOST_LOG_TRIVIAL(debug) << "classdec(C) ::= T_CLASS S_ID(I) T_EXTENDS S_ID(P) T_LBRACKET classblock(B) T_RBRACKET.";
	POINTERCHECK(i);
	POINTERCHECK(p);
	POINTERCHECK(b);
	c = new exo::ast::DecClass( TOKENSTR(i), TOKENSTR(p), b );
	delete i;
	delete p;
}
classdec(c) ::= T_CLASS S_ID(i) T_EXTENDS S_ID(p) T_LBRACKET T_RBRACKET. {
	BOOST_LOG_TRIVIAL(debug) << "classdec(C) ::= T_CLASS S_ID(I) T_EXTENDS S_ID(P) T_LBRACKET T_RBRACKET.";
	POINTERCHECK(i);
	POINTERCHECK(p);
	c = new exo::ast::DecClass( TOKENSTR(i), TOKENSTR(p), new exo::ast::ClassBlock );
	delete i;
	delete p;
}
classdec(c) ::= T_CLASS S_ID(i) T_LBRACKET classblock(b) T_RBRACKET. {
	BOOST_LOG_TRIVIAL(debug) << "classdec(C) ::= T_CLASS S_ID(I) T_LBRACKET classblock(B) T_RBRACKET.";
	POINTERCHECK(i);
	POINTERCHECK(b);
	c = new exo::ast::DecClass( TOKENSTR(i), b );
	delete i;
}
classdec(c) ::= T_CLASS S_ID(i) T_LBRACKET T_RBRACKET. {
	BOOST_LOG_TRIVIAL(debug) << "classdec(C) ::= T_CLASS S_ID(I) T_LBRACKET T_RBRACKET.";
	POINTERCHECK(i);
	c = new exo::ast::DecClass( TOKENSTR(i), new exo::ast::ClassBlock );
	delete i;
}


/* a class block contains the declarations of a class. that is properties and methods. */
%type classblock { exo::ast::ClassBlock* }
%destructor classblock { delete $$; }
classblock(b) ::= propertydec(d). {
	BOOST_LOG_TRIVIAL(debug) << "classblock(B) ::= propertydec(D).";
	POINTERCHECK(d);
	b = new exo::ast::ClassBlock;
	b->properties.push_back( d );
}
classblock(b) ::= methoddec(d). {
	BOOST_LOG_TRIVIAL(debug) << "classblock(B) ::= methoddec(D).";
	POINTERCHECK(d);
	b = new exo::ast::ClassBlock;
	b->methods.push_back( d );
}
classblock(b) ::= classblock(l) propertydec(d). {
	BOOST_LOG_TRIVIAL(debug) << "classblock(B) ::= classblock(L) propertydec(D).";
	POINTERCHECK(l);
	POINTERCHECK(d);
	l->properties.push_back( d );
	b = l;
}
classblock(b) ::= classblock(l) methoddec(d). {
	BOOST_LOG_TRIVIAL(debug) << "classblock(B) ::= classblock(L) methoddec(D).";
	POINTERCHECK(l);
	POINTERCHECK(d);
	l->methods.push_back( d );
	b = l;
}


/* an expression list are expression delimited by a colon */
%type exprlist { exo::ast::ExprList* }
%destructor exprlist { delete $$; }
exprlist(l) ::= . {
	BOOST_LOG_TRIVIAL(debug) << "exprlist(L) ::= .";
	l = new exo::ast::ExprList;
}
exprlist(l) ::= expr(e). {
	BOOST_LOG_TRIVIAL(debug) << "exprlist(L) ::= expr(E).";
	POINTERCHECK(e);
	l = new exo::ast::ExprList;
	l->list.push_back( e );
}
exprlist(f) ::= exprlist(l) T_COMMA expr(e). {
	BOOST_LOG_TRIVIAL(debug) << "exprlist(F) ::= exprlist(L) T_COMMA expr(E).";
	POINTERCHECK(l);
	POINTERCHECK(e);
	l->list.push_back( e );
	f = l;
}


/* an expression may be an function call, method call, variable (expression), constant, binary (add, ... assignment) operation */
%type expr { exo::ast::Expr* }
%destructor expr { delete $$; }
expr(e) ::= S_ID(i) T_LANGLE exprlist(a) T_RANGLE. {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= S_ID(I) T_LANGLE exprlist(A) T_RANGLE.";
	POINTERCHECK(i);
	POINTERCHECK(a);
	e = new exo::ast::CallFun( TOKENSTR(i), a );
	delete i;
}
expr(e) ::= expr(v) T_PTR S_ID(i) T_LANGLE exprlist(a) T_RANGLE. {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= expr(V) T_PTR S_ID(I) T_LANGLE exprlist(A) T_RANGLE.";
	POINTERCHECK(v);
	POINTERCHECK(i);
	POINTERCHECK(a);
	e = new exo::ast::CallMethod( TOKENSTR(i), v, a );
	delete i;
}
expr(e) ::= expr(v) T_PTR S_ID(i). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= expr(V) T_PTR S_ID(I).";
	POINTERCHECK(v);
	POINTERCHECK(i);
	e = new exo::ast::ExprProp( TOKENSTR(i), v );
	delete i;
}
expr(e) ::= S_VAR(v). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= S_VAR(V).";
	POINTERCHECK(v);
	e = new exo::ast::ExprVar( TOKENSTR(v) );
	delete v;
}
expr(e) ::= constant(c). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= constant(C).";
	POINTERCHECK(c);
	e = c;
}
/* binary ops */
expr(e) ::= expr(a) T_PLUS expr(b). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= expr(A) T_PLUS expr(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryAdd( a, b );
}
expr(e) ::= expr(a) T_MINUS expr(b). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= expr(A) T_MINUS expr(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinarySub( a, b );
}
expr(e) ::= expr(a) T_MUL expr(b). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= expr(A) T_MUL expr(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryMul( a, b );
}
expr(e) ::= expr(a) T_DIV expr(b). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= expr(A) T_DIV expr(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryDiv( a, b );
}
expr(e) ::= expr(a) T_EQ expr(b). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= expr(A) T_EQ expr(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryEq( a, b );
}
expr(e) ::= expr(a) T_NE expr(b). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= expr(A) T_NE expr(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryNeq( a, b );
}
expr(e) ::= expr(a) T_LT expr(b). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= expr(A) T_LT expr(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryLt( a, b );
}
expr(e) ::= expr(a) T_LE expr(b). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= expr(A) T_LE expr(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryLe( a, b );
}
expr(e) ::= expr(a) T_GT expr(b). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= expr(A) T_GT expr(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryGt( a, b );
}
expr(e) ::= expr(a) T_GE expr(b). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= expr(A) T_GE expr(B).";
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryGe( a, b );
}
expr(e) ::= expr(n) T_ASSIGN expr(v). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= expr(N) T_ASSIGN expr(V).";
	POINTERCHECK(n);
	POINTERCHECK(v);
	e = new exo::ast::OpBinaryAssign( n, v );
}
/* unary ops */
expr(e) ::= T_NEW expr(a). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= T_NEW expr(A).";
	POINTERCHECK(a);
	e = new exo::ast::OpUnaryNew( a );
}
expr(e) ::= T_DELETE expr(a). {
	BOOST_LOG_TRIVIAL(debug) << "expr(E) ::= T_DELETE expr(A).";
	POINTERCHECK(a);
	e = new exo::ast::OpUnaryDel( a );
}


/* a constant can be a builtin (null, true, false, __*__), number or string */
%type constant { exo::ast::Expr* }
%destructor constant { delete $$; }
constant(c) ::= T_FILE. {
	BOOST_LOG_TRIVIAL(debug) << "constant(C) ::= T_FILE.";
	c = new exo::ast::ConstStr( ast->fileName );
}
constant(c) ::= T_LINE(l). {
	BOOST_LOG_TRIVIAL(debug) << "constant(C) ::= T_LINE(L).";
	POINTERCHECK(l);
	c = new exo::ast::ConstInt( l->line_number() );
}
constant(c) ::= T_TARGET. {
	BOOST_LOG_TRIVIAL(debug) << "constant(C) ::= T_TARGET.";
	c = new exo::ast::ConstStr( ast->targetMachine );
}
constant(c) ::= T_VERSION. {
	BOOST_LOG_TRIVIAL(debug) << "constant(C) ::= T_VERSION.";
	c = new exo::ast::ConstStr( EXO_VERSION );
}
constant(c) ::= T_VNULL. {
	BOOST_LOG_TRIVIAL(debug) << "constant(C) ::= T_VNULL.";
	c = new exo::ast::ConstNull();
}
constant(c) ::= T_VTRUE. {
	BOOST_LOG_TRIVIAL(debug) << "constant(C) ::= T_VTRUE.";
	c = new exo::ast::ConstBool( true );
}
constant(c) ::= T_VFALSE. {
	BOOST_LOG_TRIVIAL(debug) << "constant(C) ::= T_VFALSE.";
	c = new exo::ast::ConstBool( false );
}
constant(c) ::= number(n). {
	BOOST_LOG_TRIVIAL(debug) << "constant(C) ::= number(N).";
	POINTERCHECK(n);
	c = n;
}
constant(c) ::= string(s). {
	BOOST_LOG_TRIVIAL(debug) << "constant(C) ::= string(S).";
	POINTERCHECK(s);
	c = s;
}


/* a number can be an integer or a float */
%type number { exo::ast::Expr* }
%destructor number { delete $$; }
number(n) ::= S_INT(i). {
	BOOST_LOG_TRIVIAL(debug) << "number(N) ::= S_INT(I).";
	POINTERCHECK(i);
	n = new exo::ast::ConstInt( boost::lexical_cast<long>( TOKENSTR(i) ) );
	delete i;
}
number(n) ::= S_FLOAT(f). {
	BOOST_LOG_TRIVIAL(debug) << "number(N) ::= S_FLOAT(F).";
	POINTERCHECK(f);
	n = new exo::ast::ConstFloat( boost::lexical_cast<double>( TOKENSTR(f) ) );
	delete f;
}


/* a string is delimited by quotes */
%type string { exo::ast::Expr* }
%destructor string { delete $$; }
string(s) ::= T_QUOTE S_STRING(q) T_QUOTE. {
	BOOST_LOG_TRIVIAL(debug) << "string(S) ::= T_QUOTE S_STRING(Q) T_QUOTE.";
	POINTERCHECK(q);
	s = new exo::ast::ConstStr( TOKENSTR(q) );
	delete q;
}


/* an access modifier is either public, private or protected */
%type access { exo::ast::ModAccess* }
%destructor access { delete $$; }
access(a) ::= T_PUBLIC. {
	BOOST_LOG_TRIVIAL(debug) << "access(A) ::= T_PUBLIC.";
	a = new exo::ast::ModAccess();
}
access(a) ::= T_PRIVATE. {
	BOOST_LOG_TRIVIAL(debug) << "access(A) ::= T_PRIVATE.";
	a = new exo::ast::ModAccess();
}
access(a) ::= T_PROTECTED. {
	BOOST_LOG_TRIVIAL(debug) << "access(A) ::= T_PROTECTED.";
	a = new exo::ast::ModAccess();
}