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
	#include "exo/lexer/lexer"

	#define fprintf( file, ... )								__lemonLog( __VA_ARGS__ )
	// TODO: allow to enable/tracking via configure
	#define EXO_TRACK_NODE(n)									n->lineNo = ast->currentLineNo; n->columnNo = ast->currentColumnNo;

	void __lemonLog( std::string msg ) {
#ifdef EXO_DEBUG
		boost::algorithm::trim( msg );

		if( msg.size() ) {
			EXO_DEBUG_LOG( trace, msg );
		};
#endif
	}
	
	template<typename type1, typename type2, typename type3>	void __lemonLog( const char* fmt, type1 msg1, type2 msg2, type3 msg3 ) { boost::format message( fmt ); message % msg1 % msg2 % msg3; __lemonLog( boost::str( message ) ); }
	template<typename type1, typename type2>					void __lemonLog( const char* fmt, type1 msg1, type2 msg2 ) { boost::format message( fmt ); message % msg1 % msg2; __lemonLog( boost::str( message ) ); }
	template<typename type>										void __lemonLog( const char* fmt, type msg1 ) { boost::format message( fmt ); message % msg1; __lemonLog( boost::str( message ) ); }
}

%syntax_error {
	EXO_THROW( UnexpectedToken()<< boost::errinfo_file_name( ast->fileName ) << boost::errinfo_at_line( TOKEN->line_number() ) << exo::exceptions::TokenName( TOKEN->type_id_name() ) );
}
%stack_overflow {
	EXO_THROW( StackOverflow() );
}


/* more like the "end" reduce. */
%start_symbol program

%token_prefix QUEX_TKN_
%token_type { std::unique_ptr<quex::Token> }
%extra_argument { exo::ast::Tree *ast }

/* everything decends from an ast node, tell lemon about it */
%default_type { std::unique_ptr<exo::ast::Node> }


/* token precedences */
%nonassoc	T_RANGLE.
%nonassoc	T_ELSE.
%nonassoc	S_ID.
%right		T_TBOOL T_TINT T_TFLOAT T_TSTRING T_TAUTO T_TCALLABLE.
%right		T_ASSIGN.
%left		T_EQ T_NE.
%left		T_LT T_LE T_GT T_GE.
%left		T_PLUS T_MINUS.
%left		T_MUL T_DIV.
%left		T_PTR.
%right		T_NEW T_DELETE.


/* a program is build out of statements. or is empty */
program ::= stmts(s). {
	ast->stmts = std::move(s);
}
program ::= . {
}


/* statements are a single statement followed by ; and other statements */
%type stmts { std::unique_ptr<exo::ast::StmtList> }
stmts(a) ::= stmt(b) T_SEMICOLON. {
	a = std::make_unique<exo::ast::StmtList>();
	a->list.push_back( std::move(b) );
	EXO_TRACK_NODE(a);
}
stmts(s) ::= stmts(a) stmt(b) T_SEMICOLON. {
	a->list.push_back( std::move(b) );
	s = std::move(a);
	EXO_TRACK_NODE(s);
}


/*
 * statement can be a module-, variable-, function(proto)-, class-declaration, delete-, return-, if-, while-, for-, break-statement or an expression.
 * statements are terminated by a semicolon
 */
%type stmt { std::unique_ptr<exo::ast::Stmt> }
stmt(s) ::= moddec(v). {
	s = std::move(v);
	EXO_TRACK_NODE(s);
}
stmt(s) ::= vardec(v). {
	s = std::move(v);
	EXO_TRACK_NODE(s);
}
stmt(s) ::= funproto(f). {
	s = std::move(f);
	EXO_TRACK_NODE(s);
}
stmt(s) ::= fundec(f). {
	s = std::move(f);
	EXO_TRACK_NODE(s);
}
stmt(s) ::= classdec(c). {
	s = std::move(c);
	EXO_TRACK_NODE(s);
}
stmt(s) ::= T_RETURN expr(e). {
	s = std::make_unique<exo::ast::StmtReturn>( std::move(e) );
	EXO_TRACK_NODE(s);
}
stmt(s) ::= stmtif(i). {
	s = std::move(i);
	EXO_TRACK_NODE(s);
}
stmt(s) ::= stmtwhile(w). {
	s = std::move(w);
	EXO_TRACK_NODE(s);
}
stmt(s) ::= stmtfor(f). {
	s = std::move(f);
	EXO_TRACK_NODE(s);
}
stmt(s) ::= stmtbreak(b). {
	s = std::move(b);
	EXO_TRACK_NODE(s);
}
stmt(s) ::= expr(e). {
	s = std::make_unique<exo::ast::StmtExpr>( std::move(e) );
	EXO_TRACK_NODE(s);
}


/* a module declaration is a module identifier */
%type moddec { std::unique_ptr<exo::ast::DecMod> }
moddec(m) ::= T_MODULE id(i). {
	ast->moduleName = i->inNamespace + i->name;
	m = std::make_unique<exo::ast::DecMod>( std::move(i) );
	EXO_TRACK_NODE(m);
}


/* a block is empty (i.e. protofunctions, class blocks ), a collection of statements delimited by brackets or a single statement */
%type block { std::unique_ptr<exo::ast::StmtList> }
block(b) ::= T_LBRACKET T_RBRACKET. {
	b = std::make_unique<exo::ast::StmtList>();
	EXO_TRACK_NODE(b);
}
block(b) ::= T_LBRACKET stmts(s) T_RBRACKET. {
	b = std::move(s);
	EXO_TRACK_NODE(b);
}
block(b) ::= stmt(s). {
	b = std::make_unique<exo::ast::StmtList>();
	b->list.push_back( std::move(s) );
	EXO_TRACK_NODE(b);
}


/* an if or if else block */
%type stmtif { std::unique_ptr<exo::ast::StmtIf> }
stmtif(i) ::= T_IF T_LANGLE expr(e) T_RANGLE block(t). {
	i = std::make_unique<exo::ast::StmtIf>( std::move(e), std::move(t) );
	EXO_TRACK_NODE(i);
}
stmtif(i) ::= T_IF T_LANGLE expr(e) T_RANGLE block(t) T_ELSE block(f). {
	i = std::make_unique<exo::ast::StmtIf>( std::move(e), std::move(t), std::move(f) );
	EXO_TRACK_NODE(i);
}


/* a while has a condition which is checked block and an asscoiated block */
%type stmtwhile { std::unique_ptr<exo::ast::StmtWhile> }
stmtwhile(i) ::= T_WHILE T_LANGLE expr(e) T_RANGLE block(b). {
	i = std::make_unique<exo::ast::StmtWhile>( std::move(e), std::move(b) );
	EXO_TRACK_NODE(i);
}


/* a for block has a variable declaration list, followed by an expression condition, an expression update list and an asscoiated block */
%type stmtfor { std::unique_ptr<exo::ast::StmtFor> }
stmtfor(f) ::= T_FOR T_LANGLE vardeclist(l) T_SEMICOLON expr(c) T_SEMICOLON exprlist(u) T_RANGLE block(b). {
	f = std::make_unique<exo::ast::StmtFor>( std::move(c), std::move(l), std::move(u), std::move(b) );
	EXO_TRACK_NODE(f);
}


/* a while has a condition which is checked block and an asscoiated block */
%type stmtbreak { std::unique_ptr<exo::ast::StmtBreak> }
stmtbreak(i) ::= T_BREAK. {
	i = std::make_unique<exo::ast::StmtBreak>();
	EXO_TRACK_NODE(i);
}


/* an identifier has an optional namespace */
%type id { std::unique_ptr<exo::ast::Id> }
id(i) ::= S_NS(n) S_ID(s). {
	i = std::make_unique<exo::ast::Id>( TOKENSTR(s), TOKENSTR(n) );
	EXO_TRACK_NODE(i);
}
id(i) ::= S_ID(s). {
	i = std::make_unique<exo::ast::Id>( TOKENSTR(s) );
	EXO_TRACK_NODE(i);
}


/* a type may be a primitive bool, integer, float, string, auto, callable, null or an identifier for a complex */
%type type { std::unique_ptr<exo::ast::Type> }
type(t) ::= T_TBOOL. {
	t = std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "bool" ), true );
	EXO_TRACK_NODE(t);
}
type(t) ::= T_TINT. {
	t = std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "int" ), true );
	EXO_TRACK_NODE(t);
}
type(t) ::= T_TFLOAT. {
	t = std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "float" ), true );
	EXO_TRACK_NODE(t);
}
type(t) ::= T_TSTRING. {
	t = std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "string" ), true );
	EXO_TRACK_NODE(t);
}
type(t) ::= T_TAUTO. {
	t = std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "auto" ), true );
	EXO_TRACK_NODE(t);
}
type(t) ::= T_TCALLABLE. {
	t = std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "callable" ), true );
	EXO_TRACK_NODE(t);
}
type(t) ::= T_VNULL. {
	t = std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "null" ), true );
	EXO_TRACK_NODE(t);
}
type(t) ::= id(i). {
	t = std::make_unique<exo::ast::Type>( std::move(i) );
	EXO_TRACK_NODE(t);
}


/* a variable declaration is a type identifier followed by a variable name optionally followed by an assignment to an expression */
%type vardec { std::unique_ptr<exo::ast::DecVar> }
vardec(d) ::= type(t) S_VAR(v). {
	d = std::make_unique<exo::ast::DecVar>( TOKENSTR(v), std::move(t) );
	EXO_TRACK_NODE(d);
}
vardec(d) ::= type(t) S_VAR(v) T_ASSIGN expr(e). {
	d = std::make_unique<exo::ast::DecVar>( TOKENSTR(v), std::move(t), std::move(e) );
	EXO_TRACK_NODE(d);
}


/* a variable declaration lists are variable declarations seperated by a colon optionally or empty */
%type vardeclist { std::unique_ptr<exo::ast::DecList> }
vardeclist(l)::= . {
	l = std::make_unique<exo::ast::DecList>();
	EXO_TRACK_NODE(l);
}
vardeclist(l) ::= vardec(d). {
	l = std::make_unique<exo::ast::DecList>();
	l->list.push_back( std::move(d) );
	EXO_TRACK_NODE(l);
}
vardeclist(e) ::= vardeclist(l) T_COMMA vardec(d). {
	l->list.push_back( std::move(d) );
	e = std::move(l);
	EXO_TRACK_NODE(e);
}

/*
 * a function declaration is a type identifier followed by the keyword function a functionname
 * optionally function arguments in brackets. if it has an associated block its a proper function and not a prototype
 */
%type funproto { std::unique_ptr<exo::ast::DecFunProto> }
funproto(f) ::= type(t) T_FUNCTION id(i) T_LANGLE vardeclist(l) T_RANGLE. {
	f = std::make_unique<exo::ast::DecFunProto>( std::move(i), std::move(t), std::move(l), false );
	EXO_TRACK_NODE(f);
}
funproto(f) ::= type(t) T_FUNCTION id(i) T_LANGLE vardeclist(l) T_VARG T_RANGLE. {
	f = std::make_unique<exo::ast::DecFunProto>( std::move(i), std::move(t), std::move(l), true );
	EXO_TRACK_NODE(f);
}
%type fundec { std::unique_ptr<exo::ast::DecFun> }
fundec(f) ::= type(t) T_FUNCTION id(i) T_LANGLE vardeclist(l) T_RANGLE block(b). {
	f = std::make_unique<exo::ast::DecFun>( std::move(i), std::move(t), std::move(l), std::move(b), false );
	EXO_TRACK_NODE(f);
}
fundec(f) ::= type(t) T_FUNCTION id(i) T_LANGLE vardeclist(l) T_VARG T_RANGLE block(b). {
	f = std::make_unique<exo::ast::DecFun>( std::move(i), std::move(t), std::move(l), std::move(b), true );
	EXO_TRACK_NODE(f);
}


/* a method declaration is an access modifier followed by a function declaration */
%type methoddec { std::unique_ptr<exo::ast::DecMethod> }
methoddec(m) ::= access(a) fundec(f) T_SEMICOLON. {
	m = std::make_unique<exo::ast::DecMethod>( std::move(f), std::move(a) );
	EXO_TRACK_NODE(m);
}


/* a property declaration is an access modifier followed by a variable declaration */
%type propertydec { std::unique_ptr<exo::ast::DecProp> }
propertydec(p) ::= access(a) vardec(v) T_SEMICOLON. {
	p = std::make_unique<exo::ast::DecProp>( std::move(v), std::move(a) );
	EXO_TRACK_NODE(p);
}


/* a class declaration is a class keyword, followed by an identifier, optionally an extend with a classname, and and associated class block */
%type classdec { std::unique_ptr<exo::ast::DecClass> }
classdec(c) ::= T_CLASS id(i) T_EXTENDS id(p) T_LBRACKET classblock(b) T_RBRACKET. {
	c = std::make_unique<exo::ast::DecClass>( std::move(i), std::move(p), std::move(b) );
	EXO_TRACK_NODE(c);
}
classdec(c) ::= T_CLASS id(i) T_EXTENDS id(p) T_LBRACKET T_RBRACKET. {
	c = std::make_unique<exo::ast::DecClass>( std::move(i), std::move(p), std::make_unique<exo::ast::ClassBlock>() );
	EXO_TRACK_NODE(c);
}
classdec(c) ::= T_CLASS id(i) T_LBRACKET classblock(b) T_RBRACKET. {
	c = std::make_unique<exo::ast::DecClass>( std::move(i), std::move(b) );
	EXO_TRACK_NODE(c);
}
classdec(c) ::= T_CLASS id(i) T_LBRACKET T_RBRACKET. {
	c = std::make_unique<exo::ast::DecClass>( std::move(i), std::make_unique<exo::ast::ClassBlock>() );
	EXO_TRACK_NODE(c);
}


/* a class block contains the declarations of a class. that is properties and methods. */
%type classblock { std::unique_ptr<exo::ast::ClassBlock> }
classblock(b) ::= propertydec(d). {
	b = std::make_unique<exo::ast::ClassBlock>();
	b->properties.push_back( std::move(d) );
	EXO_TRACK_NODE(b);
}
classblock(b) ::= methoddec(d). {
	b = std::make_unique<exo::ast::ClassBlock>();
	b->methods.push_back( std::move(d) );
	EXO_TRACK_NODE(b);
}
classblock(b) ::= classblock(l) propertydec(d). {
	l->properties.push_back( std::move(d) );
	b = std::move(l);
	EXO_TRACK_NODE(b);
}
classblock(b) ::= classblock(l) methoddec(d). {
	l->methods.push_back( std::move(d) );
	b = std::move(l);
	EXO_TRACK_NODE(b);
}


/* an expression list are expression delimited by a colon */
%type exprlist { std::unique_ptr<exo::ast::ExprList> }
exprlist(l) ::= . {
	l = std::make_unique<exo::ast::ExprList>();
	EXO_TRACK_NODE(l);
}
exprlist(l) ::= expr(e). {
	l = std::make_unique<exo::ast::ExprList>();
	l->list.push_back( std::move(e) );
	EXO_TRACK_NODE(l);
}
exprlist(f) ::= exprlist(l) T_COMMA expr(e). {
	l->list.push_back( std::move(e) );
	f = std::move(l);
	EXO_TRACK_NODE(f);
}


/* an expression may be an function call, method call, property, variable (expression), constant, binary (add, ... assignment) operation, delimited/grouped by brackets */
%type expr { std::unique_ptr<exo::ast::Expr> }
expr(e) ::= id(i) T_LANGLE exprlist(a) T_RANGLE. {
	e = std::make_unique<exo::ast::CallFun>( std::move(i), std::move(a) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(v) T_PTR S_ID(i) T_LANGLE exprlist(a) T_RANGLE. {
	e = std::make_unique<exo::ast::CallMethod>( std::make_unique<exo::ast::Id>( TOKENSTR(i) ), std::move(v), std::move(a) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(v) T_PTR S_ID(i). {
	e = std::make_unique<exo::ast::ExprProp>( TOKENSTR(i), std::move(v) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= S_VAR(v). {
	e = std::make_unique<exo::ast::ExprVar>( TOKENSTR(v) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= constant(c). {
	e = std::move(c);
	EXO_TRACK_NODE(e);
}
/* binary ops */
expr(e) ::= expr(a) T_PLUS expr(b). {
	e = std::make_unique<exo::ast::OpBinaryAdd>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(a) T_MINUS expr(b). {
	e = std::make_unique<exo::ast::OpBinarySub>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(a) T_MUL expr(b). {
	e = std::make_unique<exo::ast::OpBinaryMul>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(a) T_DIV expr(b). {
	e = std::make_unique<exo::ast::OpBinaryDiv>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(a) T_EQ expr(b). {
	e = std::make_unique<exo::ast::OpBinaryEq>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(a) T_NE expr(b). {
	e = std::make_unique<exo::ast::OpBinaryNeq>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(a) T_LT expr(b). {
	e = std::make_unique<exo::ast::OpBinaryLt>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(a) T_LE expr(b). {
	e = std::make_unique<exo::ast::OpBinaryLe>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(a) T_GT expr(b). {
	e = std::make_unique<exo::ast::OpBinaryGt>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(a) T_GE expr(b). {
	e = std::make_unique<exo::ast::OpBinaryGe>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(n) T_ASSIGN expr(v). {
	e = std::make_unique<exo::ast::OpBinaryAssign>( std::move(n), std::move(v) );
	EXO_TRACK_NODE(e);
}
/* unary ops */
expr(e) ::= T_NEW expr(a). {
	e = std::make_unique<exo::ast::OpUnaryNew>( std::move(a) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= T_DELETE expr(a). {
	e = std::make_unique<exo::ast::OpUnaryDel>( std::move(a) );
	EXO_TRACK_NODE(e);
}
/* binary shorthand ops */
expr(e) ::= expr(a) T_PLUS T_ASSIGN expr(b). {
	e = std::make_unique<exo::ast::OpBinaryAssignAdd>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(a) T_MINUS T_ASSIGN expr(b). {
	e = std::make_unique<exo::ast::OpBinaryAssignSub>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(a) T_MUL T_ASSIGN expr(b). {
	e = std::make_unique<exo::ast::OpBinaryAssignMul>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= expr(a) T_DIV T_ASSIGN expr(b). {
	e = std::make_unique<exo::ast::OpBinaryAssignDiv>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
expr(e) ::= T_LANGLE expr(a) T_RANGLE. {
	e = std::move(a);
	EXO_TRACK_NODE(e);
}


/* a constant can be a builtin (null, true, false, __*__), number or string */
%type constant { std::unique_ptr<exo::ast::Expr> }
constant(c) ::= T_CONST_FILE. {
	c = std::make_unique<exo::ast::ConstStr>( ast->fileName );
	EXO_TRACK_NODE(c);
}
constant(c) ::= T_CONST_LINE(l). {
	c = std::make_unique<exo::ast::ConstInt>( l->line_number() );
	EXO_TRACK_NODE(c);
}
constant(c) ::= T_CONST_COLUMN(l). {
	c = std::make_unique<exo::ast::ConstInt>( l->column_number() );
	EXO_TRACK_NODE(c);
}
constant(c) ::= T_CONST_TARGET. {
	c = std::make_unique<exo::ast::ConstStr>( ast->targetMachine );
	EXO_TRACK_NODE(c);
}
constant(c) ::= T_CONST_VERSION. {
	c = std::make_unique<exo::ast::ConstStr>( EXO_VERSION );
	EXO_TRACK_NODE(c);
}
constant(c) ::= T_VNULL. {
	c = std::make_unique<exo::ast::ConstNull>();
	EXO_TRACK_NODE(c);
}
constant(c) ::= T_VTRUE. {
	c = std::make_unique<exo::ast::ConstBool>( true );
	EXO_TRACK_NODE(c);
}
constant(c) ::= T_VFALSE. {
	c = std::make_unique<exo::ast::ConstBool>( false );
	EXO_TRACK_NODE(c);
}
constant(c) ::= T_CONST_MODULE. {
	c = std::make_unique<exo::ast::ConstStr>( ast->moduleName );
	EXO_TRACK_NODE(c);
}
constant(c) ::= number(n). {
	c = std::move(n);
	EXO_TRACK_NODE(c);
}
constant(c) ::= string(s). {
	c = std::move(s);
	EXO_TRACK_NODE(c);
}


/* a number can be an integer or a float */
%type number { std::unique_ptr<exo::ast::Expr> }
number(n) ::= S_INT(i). {
	n = std::make_unique<exo::ast::ConstInt>( boost::lexical_cast<long>( TOKENSTR(i) ) );
	EXO_TRACK_NODE(n);
}
number(n) ::= T_MINUS S_INT(i). {
	n = std::make_unique<exo::ast::ConstInt>( -( boost::lexical_cast<long>( TOKENSTR(i) ) ) );
	EXO_TRACK_NODE(n);
}
number(n) ::= S_FLOAT(f). {
	n = std::make_unique<exo::ast::ConstFloat>( boost::lexical_cast<double>( TOKENSTR(f) ) );
	EXO_TRACK_NODE(n);
}
number(n) ::= T_MINUS S_FLOAT(f). {
	n = std::make_unique<exo::ast::ConstFloat>( -( boost::lexical_cast<double>( TOKENSTR(f) ) ) );
	EXO_TRACK_NODE(n);
}


/* a string is delimited by quotes */
%type string { std::unique_ptr<exo::ast::Expr> }
string(s) ::= T_QUOTE S_STRING(q) T_QUOTE. {
	s = std::make_unique<exo::ast::ConstStr>( TOKENSTR(q) );
	EXO_TRACK_NODE(s);
}


/* an access modifier is either public, private or protected */
%type access { std::unique_ptr<exo::ast::ModAccess> }
access(a) ::= T_PUBLIC. {
	a = std::make_unique<exo::ast::ModAccess>();
	EXO_TRACK_NODE(a);
}
access(a) ::= T_PRIVATE. {
	a = std::make_unique<exo::ast::ModAccess>();
	EXO_TRACK_NODE(a);
}
access(a) ::= T_PROTECTED. {
	a = std::make_unique<exo::ast::ModAccess>();
	EXO_TRACK_NODE(a);
}