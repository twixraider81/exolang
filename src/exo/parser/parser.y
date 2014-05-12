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

	// could use a va_arg function
	#define fprintf(file, ... )		__lemonLog( __VA_ARGS__ )
	void __lemonLog( std::string msg )												{ boost::algorithm::trim( msg ); if( msg.size() ) { BOOST_LOG_TRIVIAL(trace) << msg; }; }
	void __lemonLog( const char* fmt )												{ __lemonLog( std::string( fmt ) ); }
	void __lemonLog( const char* fmt, const char* msg1 )							{ __lemonLog( ( boost::format( fmt ) % msg1 ).str() ); }
	void __lemonLog( const char* fmt, int msg1 )									{ __lemonLog( ( boost::format( fmt ) % msg1 ).str() ); }
	void __lemonLog( const char* fmt, const char* msg1, int msg2 )					{ __lemonLog( ( boost::format( fmt ) % msg1 % msg2 ).str() ); }
	void __lemonLog( const char* fmt, const char* msg1, const char* msg2 )			{ __lemonLog( ( boost::format( fmt ) % msg1 % msg2 ).str() ); }
	void __lemonLog( const char* fmt, const char* msg1, const char* msg2, int msg3 ){ __lemonLog( ( boost::format( fmt ) % msg1 % msg2 % msg3 ).str() ); }
}

%syntax_error {
	EXO_THROW_EXCEPTION( UnexpectedToken, ( boost::format( "Unexpected \"%s\" on %i:%i" ) % TOKEN->type_id_name() % TOKEN->line_number() % TOKEN->column_number() ).str() );
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
%nonassoc	S_ID.
%right		T_TBOOL T_TINT T_TFLOAT T_TSTRING T_TAUTO T_TCALLABLE.
%right		T_ASSIGN.
%left		T_EQ T_NE.
%left		T_LT T_LE T_GT T_GE.
%left		T_PLUS T_MINUS.
%left		T_MUL T_DIV.
%left		T_PTR.
%right		T_NEW T_DELETE.


/* a program is build out of statements. */
program ::= stmts(s). {
	ast->stmts = s;
}


/* statements are a single statement followed by ; and other statements */
%type stmts { exo::ast::StmtList* }
%destructor stmts { delete $$; }
stmts(a) ::= stmt(b) T_SEMICOLON. {
	POINTERCHECK(b);
	a = new exo::ast::StmtList;
	a->list.push_back( b );
}
stmts(s) ::= stmts(a) stmt(b) T_SEMICOLON. {
	POINTERCHECK(a);
	POINTERCHECK(b);
	a->list.push_back( b );
	s = a;
}


/*
 * statement can be a variable declaration, function (proto) declaration, class declaration, delete statement, a return statement, if/while/for flow or an expression.
 * statements are terminated by a semicolon
 */
%type stmt { exo::ast::Stmt* }
%destructor stmt { delete $$; }
stmt(s) ::= vardec(v). {
	POINTERCHECK(v);
	s = v;
}
stmt(s) ::= funproto(f). {
	POINTERCHECK(f);
	s = f;
}
stmt(s) ::= fundec(f). {
	POINTERCHECK(f);
	s = f;
}
stmt(s) ::= classdec(c). {
	POINTERCHECK(c);
	s = c;
}
stmt(s) ::= T_RETURN expr(e). {
	POINTERCHECK(e);
	s = new exo::ast::StmtReturn( e );
}
stmt(s) ::= stmtif(i). {
	POINTERCHECK(i);
	s = i;
}
stmt(s) ::= stmtwhile(w). {
	POINTERCHECK(w);
	s = w;
}
stmt(s) ::= stmtfor(f). {
	POINTERCHECK(f);
	s = f;
}
stmt(s) ::= expr(e). {
	POINTERCHECK(e);
	s = new exo::ast::StmtExpr( e );
}


/* a block is empty (i.e. protofunctions, class blocks ), a collection of statements delimited by brackets or a single statement */
%type block { exo::ast::StmtList* }
%destructor block { delete $$; }
block(b) ::= T_LBRACKET T_RBRACKET. {
	b = new exo::ast::StmtList;
}
block(b) ::= T_LBRACKET stmts(s) T_RBRACKET. {
	POINTERCHECK(s);
	b = s;
}
block(b) ::= stmt(s). {
	POINTERCHECK(s);
	b = new exo::ast::StmtList;
	b->list.push_back( s );
}


/* an if or if else block */
%type stmtif { exo::ast::StmtIf* }
%destructor stmtif { delete $$; }
stmtif(i) ::= T_IF T_LANGLE expr(e) T_RANGLE block(t). {
	POINTERCHECK(e);
	POINTERCHECK(t);
	i = new exo::ast::StmtIf( e, t, NULL );
}
stmtif(i) ::= T_IF T_LANGLE expr(e) T_RANGLE block(t) T_ELSE block(f). {
	POINTERCHECK(e);
	POINTERCHECK(t);
	POINTERCHECK(f);
	i = new exo::ast::StmtIf( e, t, f );
}


/* a while has a condition which is checked block and an asscoiated block */
%type stmtwhile { exo::ast::StmtWhile* }
%destructor stmtwhile { delete $$; }
stmtwhile(i) ::= T_WHILE T_LANGLE expr(e) T_RANGLE block(b). {
	POINTERCHECK(e);
	POINTERCHECK(b);
	i = new exo::ast::StmtWhile( e, b );
}


/* a for block has a variable declaration list, followed by 2 expression statements and an asscoiated block */
%type stmtfor { exo::ast::StmtFor* }
%destructor stmtfor { delete $$; }
stmtfor(f) ::= T_FOR T_LANGLE vardeclist(l) T_SEMICOLON expr(c) T_SEMICOLON expr(u) T_RANGLE block(b). {
	POINTERCHECK(l);
	POINTERCHECK(c);
	POINTERCHECK(u);
	POINTERCHECK(b);
	f = new exo::ast::StmtFor( c, l, u, b );
}

	
/* a type may be a null, bool, integer, float, string or auto or a classname */
%type type { exo::ast::Type* }
%destructor type { delete $$; }
type(t) ::= T_TBOOL. {
	t = new exo::ast::Type( "bool" );
}
type(t) ::= T_TINT. {
	t = new exo::ast::Type( "int" );
}
type(t) ::= T_TFLOAT. {
	t = new exo::ast::Type( "float" );
}
type(t) ::= T_TSTRING. {
	t = new exo::ast::Type( "string" );
}
type(t) ::= T_TAUTO. {
	t = new exo::ast::Type( "auto" );
}
type(t) ::= T_TCALLABLE. {
	t = new exo::ast::Type( "callable" );
}
type(t) ::= T_VNULL. {
	t = new exo::ast::Type( "null" );
}
type(t) ::= S_ID(i). {
	POINTERCHECK(i);
	t = new exo::ast::Type( TOKENSTR(i) );
	delete i;
}


/* a variable declaration is a type identifier followed by a variable name optionally followed by an assignment to an expression */
%type vardec { exo::ast::DecVar* }
%destructor vardec { delete $$; }
vardec(d) ::= type(t) S_VAR(v). {
	POINTERCHECK(t);
	POINTERCHECK(v);
	d = new exo::ast::DecVar( TOKENSTR(v), t );
	delete v;
}
vardec(d) ::= type(t) S_VAR(v) T_ASSIGN expr(e). {
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
	l = new exo::ast::DecList;
}
vardeclist(l) ::= vardec(d). {
	POINTERCHECK(d);
	l = new exo::ast::DecList;
	l->list.push_back( d );
}
vardeclist(e) ::= vardeclist(l) T_COMMA vardec(d). {
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
	POINTERCHECK(t);
	POINTERCHECK(i);
	POINTERCHECK(l);
	f = new exo::ast::DecFunProto( TOKENSTR(i), t, l, false );
	delete i;
}
funproto(f) ::= type(t) T_FUNCTION S_ID(i) T_LANGLE vardeclist(l) T_VARG T_RANGLE. {
	POINTERCHECK(t);
	POINTERCHECK(i);
	POINTERCHECK(l);
	f = new exo::ast::DecFunProto( TOKENSTR(i), t, l, true );
	delete i;
}
%type fundec { exo::ast::DecFun* }
%destructor fundec { delete $$; }
fundec(f) ::= type(t) T_FUNCTION S_ID(i) T_LANGLE vardeclist(l) T_RANGLE block(b). {
	POINTERCHECK(t);
	POINTERCHECK(i);
	POINTERCHECK(l);
	POINTERCHECK(b);
	f = new exo::ast::DecFun( TOKENSTR(i), t, l, b, false );
	delete i;
}
fundec(f) ::= type(t) T_FUNCTION S_ID(i) T_LANGLE vardeclist(l) T_VARG T_RANGLE block(b). {
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
	POINTERCHECK(a);
	POINTERCHECK(f);
	m = new exo::ast::DecMethod( f, a );
}


/* a property declaration is an access modifier followed by a variable declaration */
%type propertydec { exo::ast::DecProp* }
%destructor propertydec { delete $$; }
propertydec(p) ::= access(a) vardec(v) T_SEMICOLON. {
	POINTERCHECK(a);
	POINTERCHECK(v);
	p = new exo::ast::DecProp( v, a );
}


/* a class declaration is a class keyword, followed by an identifier, optionally an extend with a classname, and and associated class block */
%type classdec { exo::ast::DecClass* }
%destructor classdec { delete $$; }
classdec(c) ::= T_CLASS S_ID(i) T_EXTENDS S_ID(p) T_LBRACKET classblock(b) T_RBRACKET. {
	POINTERCHECK(i);
	POINTERCHECK(p);
	POINTERCHECK(b);
	c = new exo::ast::DecClass( TOKENSTR(i), TOKENSTR(p), b );
	delete i;
	delete p;
}
classdec(c) ::= T_CLASS S_ID(i) T_EXTENDS S_ID(p) T_LBRACKET T_RBRACKET. {
	POINTERCHECK(i);
	POINTERCHECK(p);
	c = new exo::ast::DecClass( TOKENSTR(i), TOKENSTR(p), new exo::ast::ClassBlock );
	delete i;
	delete p;
}
classdec(c) ::= T_CLASS S_ID(i) T_LBRACKET classblock(b) T_RBRACKET. {
	POINTERCHECK(i);
	POINTERCHECK(b);
	c = new exo::ast::DecClass( TOKENSTR(i), b );
	delete i;
}
classdec(c) ::= T_CLASS S_ID(i) T_LBRACKET T_RBRACKET. {
	POINTERCHECK(i);
	c = new exo::ast::DecClass( TOKENSTR(i), new exo::ast::ClassBlock );
	delete i;
}


/* a class block contains the declarations of a class. that is properties and methods. */
%type classblock { exo::ast::ClassBlock* }
%destructor classblock { delete $$; }
classblock(b) ::= propertydec(d). {
	POINTERCHECK(d);
	b = new exo::ast::ClassBlock;
	b->properties.push_back( d );
}
classblock(b) ::= methoddec(d). {
	POINTERCHECK(d);
	b = new exo::ast::ClassBlock;
	b->methods.push_back( d );
}
classblock(b) ::= classblock(l) propertydec(d). {
	POINTERCHECK(l);
	POINTERCHECK(d);
	l->properties.push_back( d );
	b = l;
}
classblock(b) ::= classblock(l) methoddec(d). {
	POINTERCHECK(l);
	POINTERCHECK(d);
	l->methods.push_back( d );
	b = l;
}


/* an expression list are expression delimited by a colon */
%type exprlist { exo::ast::ExprList* }
%destructor exprlist { delete $$; }
exprlist(l) ::= . {
	l = new exo::ast::ExprList;
}
exprlist(l) ::= expr(e). {
	POINTERCHECK(e);
	l = new exo::ast::ExprList;
	l->list.push_back( e );
}
exprlist(f) ::= exprlist(l) T_COMMA expr(e). {
	POINTERCHECK(l);
	POINTERCHECK(e);
	l->list.push_back( e );
	f = l;
}


/* an expression may be an function call, method call, property, variable (expression), constant, binary (add, ... assignment) operation */
%type expr { exo::ast::Expr* }
%destructor expr { delete $$; }
expr(e) ::= S_ID(i) T_LANGLE exprlist(a) T_RANGLE. {
	POINTERCHECK(i);
	POINTERCHECK(a);
	e = new exo::ast::CallFun( TOKENSTR(i), a );
	delete i;
}
expr(e) ::= expr(v) T_PTR S_ID(i) T_LANGLE exprlist(a) T_RANGLE. {
	POINTERCHECK(v);
	POINTERCHECK(i);
	POINTERCHECK(a);
	e = new exo::ast::CallMethod( TOKENSTR(i), v, a );
	delete i;
}
expr(e) ::= expr(v) T_PTR S_ID(i). {
	POINTERCHECK(v);
	POINTERCHECK(i);
	e = new exo::ast::ExprProp( TOKENSTR(i), v );
	delete i;
}
expr(e) ::= S_VAR(v). {
	POINTERCHECK(v);
	e = new exo::ast::ExprVar( TOKENSTR(v) );
	delete v;
}
expr(e) ::= constant(c). {
	POINTERCHECK(c);
	e = c;
}
/* binary ops */
expr(e) ::= expr(a) T_PLUS expr(b). {
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryAdd( a, b );
}
expr(e) ::= expr(a) T_MINUS expr(b). {
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinarySub( a, b );
}
expr(e) ::= expr(a) T_MUL expr(b). {
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryMul( a, b );
}
expr(e) ::= expr(a) T_DIV expr(b). {
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryDiv( a, b );
}
expr(e) ::= expr(a) T_EQ expr(b). {
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryEq( a, b );
}
expr(e) ::= expr(a) T_NE expr(b). {
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryNeq( a, b );
}
expr(e) ::= expr(a) T_LT expr(b). {
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryLt( a, b );
}
expr(e) ::= expr(a) T_LE expr(b). {
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryLe( a, b );
}
expr(e) ::= expr(a) T_GT expr(b). {
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryGt( a, b );
}
expr(e) ::= expr(a) T_GE expr(b). {
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryGe( a, b );
}
expr(e) ::= expr(n) T_ASSIGN expr(v). {
	POINTERCHECK(n);
	POINTERCHECK(v);
	e = new exo::ast::OpBinaryAssign( n, v );
}
/* unary ops */
expr(e) ::= T_NEW expr(a). {
	POINTERCHECK(a);
	e = new exo::ast::OpUnaryNew( a );
}
expr(e) ::= T_DELETE expr(a). {
	POINTERCHECK(a);
	e = new exo::ast::OpUnaryDel( a );
}
/* FIXME: these leak */
expr(e) ::= expr(a) T_PLUS T_ASSIGN expr(b). {
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryAssign( a, new exo::ast::OpBinaryAdd( a, b ) );
}
expr(e) ::= expr(a) T_MINUS T_ASSIGN expr(b). {
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryAssign( a, new exo::ast::OpBinarySub( a, b ) );
}
expr(e) ::= expr(a) T_MUL T_ASSIGN expr(b). {
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryAssign( a, new exo::ast::OpBinaryMul( a, b ) );
}
expr(e) ::= expr(a) T_DIV T_ASSIGN expr(b). {
	POINTERCHECK(a);
	POINTERCHECK(b);
	e = new exo::ast::OpBinaryAssign( a, new exo::ast::OpBinaryDiv( a, b ) );
}


/* a constant can be a builtin (null, true, false, __*__), number or string */
%type constant { exo::ast::Expr* }
%destructor constant { delete $$; }
constant(c) ::= T_FILE. {
	c = new exo::ast::ConstStr( ast->fileName );
}
constant(c) ::= T_LINE(l). {
	POINTERCHECK(l);
	c = new exo::ast::ConstInt( l->line_number() );
}
constant(c) ::= T_TARGET. {
	c = new exo::ast::ConstStr( ast->targetMachine );
}
constant(c) ::= T_VERSION. {
	c = new exo::ast::ConstStr( EXO_VERSION );
}
constant(c) ::= T_VNULL. {
	c = new exo::ast::ConstNull();
}
constant(c) ::= T_VTRUE. {
	c = new exo::ast::ConstBool( true );
}
constant(c) ::= T_VFALSE. {
	c = new exo::ast::ConstBool( false );
}
constant(c) ::= number(n). {
	POINTERCHECK(n);
	c = n;
}
constant(c) ::= string(s). {
	POINTERCHECK(s);
	c = s;
}


/* a number can be an integer or a float */
%type number { exo::ast::Expr* }
%destructor number { delete $$; }
number(n) ::= S_INT(i). {
	POINTERCHECK(i);
	n = new exo::ast::ConstInt( boost::lexical_cast<long>( TOKENSTR(i) ) );
	delete i;
}
number(n) ::= S_FLOAT(f). {
	POINTERCHECK(f);
	n = new exo::ast::ConstFloat( boost::lexical_cast<double>( TOKENSTR(f) ) );
	delete f;
}


/* a string is delimited by quotes */
%type string { exo::ast::Expr* }
%destructor string { delete $$; }
string(s) ::= T_QUOTE S_STRING(q) T_QUOTE. {
	POINTERCHECK(q);
	s = new exo::ast::ConstStr( TOKENSTR(q) );
	delete q;
}


/* an access modifier is either public, private or protected */
%type access { exo::ast::ModAccess* }
%destructor access { delete $$; }
access(a) ::= T_PUBLIC. {
	a = new exo::ast::ModAccess();
}
access(a) ::= T_PRIVATE. {
	a = new exo::ast::ModAccess();
}
access(a) ::= T_PROTECTED. {
	a = new exo::ast::ModAccess();
}