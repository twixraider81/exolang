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
		boost::algorithm::trim_right( msg );

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
	exo::exceptions::UnexpectedToken e;
	e << boost::errinfo_file_name( ast->fileName );

	if( TOKEN != nullptr ) { /* can trigger at EOF */
		e << boost::errinfo_at_line( TOKEN->line_number() );
		e << exo::exceptions::ColumnNo( TOKEN->column_number() );
		e << exo::exceptions::TokenName( TOKEN->type_id_name() );
	}

	BOOST_THROW_EXCEPTION( e );
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
%left		T_COMMA.
%nonassoc	T_IF.
%nonassoc	T_ELSE.
%right		T_ASSIGN T_ASSIGN_PLUS T_ASSIGN_MINUS T_ASSIGN_MUL T_ASSIGN_DIV.
%left		T_EQ T_NE.
%left		T_LT T_LE T_GT T_GE.
%right		T_AMP.
%left		T_PLUS T_MINUS.
%left		T_MUL T_DIV.
%right		T_NEW T_DELETE.
%left		T_PTR.

/* a program is build out of statements. or is empty */
program ::= stmts(s). {
	ast->stmts = std::move(s);
}
program ::= . {
	ast->stmts = std::make_unique<exo::ast::StmtList>();
}



%type stmts { std::unique_ptr<exo::ast::StmtList> }
stmts(a) ::= stmt(b). {
	a = std::make_unique<exo::ast::StmtList>();
	a->addStmt( std::move(b) );
	EXO_TRACK_NODE(a);
}
stmts(s) ::= stmts(a) stmt(b). {
	a->addStmt( std::move(b) );
	s = std::move(a);
}


/*
 * a statement can be a break, do-while-loop, continue, declaration, expression, for-loop, if-else-elseif, import, label, return, scope, use or while-loop statement
 * statements are terminated by a semicolon
 */
%type stmt { std::unique_ptr<exo::ast::Stmt> }
stmt(s) ::= stmtbreak(b) T_SEMICOLON. {
	s = std::move(b);
}
stmt(s) ::= stmtdo(d). { /* ends with a statement */
	s = std::move(d);
}
stmt(s) ::= stmtcont(c) T_SEMICOLON. {
	s = std::move(c);
}
stmt(s) ::= decl(d) T_SEMICOLON. {
	s = std::move(d);
}
stmt(s) ::= stmtexpr(e) T_SEMICOLON. {
	s = std::move(e);
}
stmt(s) ::= stmtfor(f). { /* ends with a statement */
	s = std::move(f);
}
stmt(s) ::= stmtif(i). { /* ends with a statement */
	s = std::move(i);
}
stmt(s) ::= stmtimport(v) T_SEMICOLON. {
	s = std::move(v);
}
stmt(s) ::= stmtlabel(u). { /* ends with a statement */
	s = std::move(u);
}
stmt(s) ::= stmtreturn(r) T_SEMICOLON. {
	s = std::move(r);
}
stmt(s) ::= stmtscope(b). { /* no implicit termination wanted */
	s = std::move(b);
}
stmt(s) ::= stmtswitch(b) T_SEMICOLON. {
	s = std::move(b);
}
stmt(s) ::= stmtuse(u) T_SEMICOLON. {
	s = std::move(u);
}
stmt(s) ::= stmtwhile(w). { /* ends with a statement */
	s = std::move(w);
}

/* break is a simple keyword */
%type stmtbreak { std::unique_ptr<exo::ast::StmtBreak> }
stmtbreak(b) ::= T_BREAK. {
	b = std::make_unique<exo::ast::StmtBreak>();
	EXO_TRACK_NODE(b);
}

/* continue is a simple keyword */
%type stmtcont { std::unique_ptr<exo::ast::StmtCont> }
stmtcont(c) ::= T_CONTINUE. {
	c = std::make_unique<exo::ast::StmtCont>();
	EXO_TRACK_NODE(c);
}

/* a do-while has statement(s) and and expression */
%type stmtdo { std::unique_ptr<exo::ast::StmtDo> }
stmtdo(d) ::= T_DO stmt(s) T_WHILE T_LANGLE expr(e) T_RANGLE. {
	d = std::make_unique<exo::ast::StmtDo>( std::move(e), std::move(s) );
	EXO_TRACK_NODE(d);
}

/* an expression is a statement */
%type stmtexpr { std::unique_ptr<exo::ast::StmtExpr> }
stmtexpr(s) ::= expr(e). {
	s = std::make_unique<exo::ast::StmtExpr>( std::move(e) );
	EXO_TRACK_NODE(s);
}

/* a for block has a variable declaration list, followed by an expression condition, an expression update list and an associated statement(s) */
%type stmtfor { std::unique_ptr<exo::ast::StmtFor> }
stmtfor(f) ::= T_FOR T_LANGLE declvarlist(l) T_SEMICOLON expr(c) T_SEMICOLON exprlist(u) T_RANGLE stmt(s). {
	f = std::make_unique<exo::ast::StmtFor>( std::move(c), std::move(l), std::move(u), std::move(s ) );
	EXO_TRACK_NODE(f);
}

/* an if or if-else block */
%type stmtif { std::unique_ptr<exo::ast::StmtIf> }
stmtif(i) ::= T_IF T_LANGLE expr(e) T_RANGLE stmt(t). {
	i = std::make_unique<exo::ast::StmtIf>( std::move(e), std::move(t) );
	EXO_TRACK_NODE(i);
}
stmtif(i) ::= T_IF T_LANGLE expr(e) T_RANGLE stmt(t) T_ELSE stmt(f). {
	i = std::make_unique<exo::ast::StmtIf>( std::move(e), std::move(t), std::move(f) );
	EXO_TRACK_NODE(i);
}

/* a import declaration has a string identifier */
%type stmtimport { std::unique_ptr<exo::ast::StmtImport> }
stmtimport(i) ::= T_IMPORT string(s). {
	i = std::make_unique<exo::ast::StmtImport>( std::move(s) );
	EXO_TRACK_NODE(i);
}

/* a label is a constant identifier followed by a colon and is bound to a statement */
%type stmtlabel { std::unique_ptr<exo::ast::StmtLabel> }
stmtlabel(l) ::= constant(c) T_COLON stmt(s). {
	l = std::make_unique<exo::ast::StmtLabel>( std::move(s), std::move(c) );
	EXO_TRACK_NODE(l);
}

/* a return has an expression */
%type stmtreturn { std::unique_ptr<exo::ast::StmtReturn> }
stmtreturn(r) ::= T_RETURN expr(e). {
	r = std::make_unique<exo::ast::StmtReturn>( std::move(e) );
	EXO_TRACK_NODE(r);
}

/* a scope is a collection of statements delimited by brackets or a single statement */
%type stmtscope { std::unique_ptr<exo::ast::StmtScope> }
stmtscope(b) ::= T_LBRACKET stmts(s) T_RBRACKET. {
	b = std::make_unique<exo::ast::StmtScope>( std::move(s) );
	EXO_TRACK_NODE(b);
}
stmtscope(b) ::= T_LBRACKET T_RBRACKET. {
	b = std::make_unique<exo::ast::StmtScope>();
	EXO_TRACK_NODE(b);
}

/* a switch */
%type stmtswitch { std::unique_ptr<exo::ast::StmtSwitch> }
stmtswitch(s) ::= T_SWITCH T_LANGLE expr(e) T_RANGLE T_LBRACKET stmtswitchcases(l) T_RBRACKET. {
	s = std::move(l);
	s->setCondition( std::move(e) );
}
%type stmtswitchcases { std::unique_ptr<exo::ast::StmtSwitch> }
stmtswitchcases(c) ::= T_CASE stmtlabel(l). {
	c = std::make_unique<exo::ast::StmtSwitch>();
	c->addCase( std::move(l) );
	EXO_TRACK_NODE(c);
}
stmtswitchcases(c) ::= stmtswitchcases(a) T_CASE stmtlabel(l). {
	c = std::move(a);
	c->addCase( std::move(l) );
}
stmtswitchcases(c) ::= T_DEFAULT T_COLON stmt(d). {
	c = std::make_unique<exo::ast::StmtSwitch>();
	c->setDefaultCase( std::move(d) );
	EXO_TRACK_NODE(c);
}
stmtswitchcases(c) ::= stmtswitchcases(a) T_DEFAULT T_COLON stmt(d). {
	c = std::move(a);
	c->setDefaultCase( std::move(d) );
}

/* a use declaration has a module identifier */
%type stmtuse { std::unique_ptr<exo::ast::StmtUse> }
stmtuse(u) ::= T_USE id(i). {
	u = std::make_unique<exo::ast::StmtUse>( std::move(i) );
	EXO_TRACK_NODE(u);
}

/* a while has a condition which and an associated statement(s) */
%type stmtwhile { std::unique_ptr<exo::ast::StmtWhile> }
stmtwhile(w) ::= T_WHILE T_LANGLE expr(e) T_RANGLE stmt(s). {
	w = std::make_unique<exo::ast::StmtWhile>( std::move(e), std::move(s) );
	EXO_TRACK_NODE(w);
}


/*
 * a declaration can be a class-, function prototype-, function-, module-, variable-, variable-list- declaration
 * a declaration is a statement.
 */
%type decl { std::unique_ptr<exo::ast::Stmt> }
decl(d) ::= declclass(c). {
	d = std::move(c);
}
decl(d) ::= declfunproto(f). {
	d = std::move(f);
}
decl(d) ::= declfun(f). {
	d = std::move(f);
}
decl(d) ::= declmod(m). {
	d = std::move(m);
}
decl(d) ::= declvarlist(v). { /* also catches variable declarations */
	d = std::move(v);
}

/* a class declaration block contains the declarations properties and methods */
%type declclassblock { std::unique_ptr<exo::ast::DeclClass> }
declclassblock(b) ::= declprop(d) T_SEMICOLON. {
	b = std::make_unique<exo::ast::DeclClass>();
	b->addProperty( std::move(d) );
	EXO_TRACK_NODE(b);
}
declclassblock(b) ::= declfun(d) T_SEMICOLON. {
	b = std::make_unique<exo::ast::DeclClass>();
	b->addMethod( std::move(d) );
	EXO_TRACK_NODE(b);
}
declclassblock(b) ::= declclassblock(l) declprop(d) T_SEMICOLON. {
	b = std::move(l);
	b->addProperty( std::move(d) );
}
declclassblock(b) ::= declclassblock(l) declfun(d) T_SEMICOLON. {
	b = std::move(l);
	b->addMethod( std::move(d) );
}

/* a class declaration is a class keyword, followed by an identifier, optionally an extend with a classname, and and associated class block */
%type declclass { std::unique_ptr<exo::ast::DeclClass> }
declclass(c) ::= T_CLASS id(i) T_EXTENDS id(p) T_LBRACKET declclassblock(b) T_RBRACKET. {
	c = std::move(b);
	EXO_TRACK_NODE(c);
	c->setId( std::move(i), std::move(p) );
}
declclass(c) ::= T_CLASS id(i) T_LBRACKET declclassblock(b) T_RBRACKET. {
	c = std::move(b);
	EXO_TRACK_NODE(c);
	c->setId( std::move(i) );
}
declclass(c) ::= T_CLASS id(i) T_EXTENDS id(p) T_LBRACKET T_RBRACKET. {
	c = std::make_unique<exo::ast::DeclClass>();
	EXO_TRACK_NODE(c);
	c->setId( std::move(i), std::move(p) );
}
declclass(c) ::= T_CLASS id(i) T_LBRACKET T_RBRACKET. {
	c = std::make_unique<exo::ast::DeclClass>();
	EXO_TRACK_NODE(c);
	c->setId( std::move(i) );
}

/*
 * a function prototype declaration can have a type identifier followed by the keyword function a function identifier and a variaböe decleration
 */
%type declfunproto { std::unique_ptr<exo::ast::DeclFunProto> }
declfunproto(f) ::= type(t) T_FUNCTION id(i) T_LANGLE declvarlist(l) T_RANGLE. {
	f = std::make_unique<exo::ast::DeclFunProto>( std::move(i), std::move(t), std::move(l), false );
	EXO_TRACK_NODE(f);
}
declfunproto(f) ::= type(t) T_FUNCTION id(i) T_LANGLE declvarlist(l) T_VARG T_RANGLE. {
	f = std::make_unique<exo::ast::DeclFunProto>( std::move(i), std::move(t), std::move(l), true );
	EXO_TRACK_NODE(f);
}
declfunproto(f) ::= T_FUNCTION id(i) T_LANGLE declvarlist(l) T_RANGLE. {
	f = std::make_unique<exo::ast::DeclFunProto>( std::move(i), std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "null" ), true ), std::move(l), false );
	EXO_TRACK_NODE(f);
}
declfunproto(f) ::= T_FUNCTION id(i) T_LANGLE declvarlist(l) T_VARG T_RANGLE. {
	f = std::make_unique<exo::ast::DeclFunProto>( std::move(i), std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "null" ), true ), std::move(l), true );
	EXO_TRACK_NODE(f);
}

/*
 * a function declaration has an access modifier followed by a function prototype and a statement block
 */
%type declfun { std::unique_ptr<exo::ast::DeclFun> }
declfun(f) ::= T_FUNCTION id(i) T_LANGLE declvarlist(l) T_RANGLE stmtscope(b). {
	f = std::make_unique<exo::ast::DeclFun>( std::move(i), std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "null" ), true ), std::move(l), std::move(b), false );
	EXO_TRACK_NODE(f);
}
declfun(f) ::= T_FUNCTION id(i) T_LANGLE declvarlist(l) T_VARG T_RANGLE stmtscope(b). {
	f = std::make_unique<exo::ast::DeclFun>( std::move(i), std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "null" ), true ), std::move(l), std::move(b), true );
	EXO_TRACK_NODE(f);
}
declfun(f) ::= type(t) T_FUNCTION id(i) T_LANGLE declvarlist(l) T_RANGLE stmtscope(b). {
	f = std::make_unique<exo::ast::DeclFun>( std::move(i), std::move(t), std::move(l), std::move(b), false );
	EXO_TRACK_NODE(f);
}
declfun(f) ::= type(t) T_FUNCTION id(i) T_LANGLE declvarlist(l) T_VARG T_RANGLE stmtscope(b). {
	f = std::make_unique<exo::ast::DeclFun>( std::move(i), std::move(t), std::move(l), std::move(b), true );
	EXO_TRACK_NODE(f);
}
declfun(f) ::= access(a) type(t) T_FUNCTION id(i) T_LANGLE declvarlist(l) T_RANGLE stmtscope(b). {
	f = std::make_unique<exo::ast::DeclFun>( std::move(i), std::move(a), std::move(t), std::move(l), std::move(b), false );
	EXO_TRACK_NODE(f);
}
declfun(f) ::= access(a) type(t) T_FUNCTION id(i) T_LANGLE declvarlist(l) T_VARG T_RANGLE stmtscope(b). {
	f = std::make_unique<exo::ast::DeclFun>( std::move(i), std::move(a), std::move(t), std::move(l), std::move(b), true );
	EXO_TRACK_NODE(f);
}
declfun(f) ::= access(a) T_FUNCTION id(i) T_LANGLE declvarlist(l) T_RANGLE stmtscope(b). {
	f = std::make_unique<exo::ast::DeclFun>( std::move(i), std::move(a), std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "null" ), true ), std::move(l), std::move(b), false );
	EXO_TRACK_NODE(f);
}
declfun(f) ::= access(a) T_FUNCTION id(i) T_LANGLE declvarlist(l) T_VARG T_RANGLE stmtscope(b). {
	f = std::make_unique<exo::ast::DeclFun>( std::move(i), std::move(a), std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "null" ), true ), std::move(l), std::move(b), true );
	EXO_TRACK_NODE(f);
}


/* a module declaration is a module identifier */
%type declmod { std::unique_ptr<exo::ast::DeclMod> }
declmod(m) ::= T_MODULE id(i). {
	ast->moduleName = i->inNamespace + i->name;
	m = std::make_unique<exo::ast::DeclMod>( std::move(i) );
	EXO_TRACK_NODE(m);
}

/* a property declaration is an access modifier followed by a variable declaration */
%type declprop { std::unique_ptr<exo::ast::DeclProp> }
declprop(p) ::= access(a) declvar(v). {
	p = std::make_unique<exo::ast::DeclProp>( std::move(v), std::move(a) );
	EXO_TRACK_NODE(p);
}

/* a variable declaration is a type identifier followed by a variable name optionally followed by an assignment to an expression */
%type declvar { std::unique_ptr<exo::ast::DeclVar> }
declvar(d) ::= type(t) S_VAR(v). {
	d = std::make_unique<exo::ast::DeclVar>( TOKENSTR(v), std::move(t) );
	EXO_TRACK_NODE(d);
}
declvar(d) ::= T_REF type(t) S_VAR(v). {
	d = std::make_unique<exo::ast::DeclVar>( TOKENSTR(v), std::move(t), true );
	EXO_TRACK_NODE(d);
}
declvar(d) ::= type(t) S_VAR(v) T_ASSIGN expr(e). {
	d = std::make_unique<exo::ast::DeclVar>( TOKENSTR(v), std::move(t), std::move(e) );
	EXO_TRACK_NODE(d);
}
declvar(d) ::= T_REF type(t) S_VAR(v) T_ASSIGN expr(e). {
	d = std::make_unique<exo::ast::DeclVar>( TOKENSTR(v), std::move(t), std::move(e), true );
	EXO_TRACK_NODE(d);
}

/* a variable declaration list are variable declarations seperated by a colon optionally or empty */
%type declvarlist { std::unique_ptr<exo::ast::DeclVarList> }
declvarlist(l)::= . {
	l = std::make_unique<exo::ast::DeclVarList>();
	EXO_TRACK_NODE(l);
}
declvarlist(l) ::= declvar(d). {
	l = std::make_unique<exo::ast::DeclVarList>();
	l->addDecl( std::move(d) );
	EXO_TRACK_NODE(l);
}
declvarlist(e) ::= declvarlist(l) T_COMMA declvar(d). {
	e = std::move(l);
	e->addDecl( std::move(d) );
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


/* a type may be a primitive (bool, integer, float, string, auto, callable, null) or an identifier for a complex */
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
	t = std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "auto" ) );
	EXO_TRACK_NODE(t);
}
type(t) ::= T_TCALLABLE. {
	t = std::make_unique<exo::ast::Type>( std::make_unique<exo::ast::Id>( "callable" ) );
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


/* an expression list may be empty or expressions delimited by a colon */
%type exprlist { std::unique_ptr<exo::ast::ExprList> }
exprlist(l) ::= . {
	l = std::make_unique<exo::ast::ExprList>();
	EXO_TRACK_NODE(l);
}
exprlist(l) ::= expr(e). {
	l = std::make_unique<exo::ast::ExprList>();
	l->addExpr( std::move(e) );
	EXO_TRACK_NODE(l);
}
exprlist(f) ::= exprlist(l) T_COMMA expr(e). {
	f = std::move(l);
	f->addExpr( std::move(e) );
}


/* an expression may be delimited/grouped by brackets, binary operation, call, constant, unary operation, variable */
%type expr { std::unique_ptr<exo::ast::Expr> }
expr(e) ::= T_LANGLE expr(a) T_RANGLE. {
	e = std::move(a);
}
expr(e) ::= binop(b). {
	e = std::move(b);
}
expr(e) ::= call(c). {
	e = std::move(c);
}
expr(e) ::= constant(c). {
	e = std::move(c);
}
expr(e) ::= unop(b). {
	e = std::move(b);
}
expr(e) ::= var(v). {
	e = std::move(v);
}

/* binary operations ( + - * / == != < <= > >= = ) */
%type binop { std::unique_ptr<exo::ast::OpBinary> }
binop(e) ::= expr(a) T_PLUS expr(b). {
	e = std::make_unique<exo::ast::OpBinaryAdd>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
binop(e) ::= expr(a) T_MINUS expr(b). {
	e = std::make_unique<exo::ast::OpBinarySub>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
binop(e) ::= expr(a) T_MUL expr(b). {
	e = std::make_unique<exo::ast::OpBinaryMul>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
binop(e) ::= expr(a) T_DIV expr(b). {
	e = std::make_unique<exo::ast::OpBinaryDiv>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
binop(e) ::= expr(a) T_EQ expr(b). {
	e = std::make_unique<exo::ast::OpBinaryEq>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
binop(e) ::= expr(a) T_NE expr(b). {
	e = std::make_unique<exo::ast::OpBinaryNeq>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
binop(e) ::= expr(a) T_LT expr(b). {
	e = std::make_unique<exo::ast::OpBinaryLt>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
binop(e) ::= expr(a) T_LE expr(b). {
	e = std::make_unique<exo::ast::OpBinaryLe>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
binop(e) ::= expr(a) T_GT expr(b). {
	e = std::make_unique<exo::ast::OpBinaryGt>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
binop(e) ::= expr(a) T_GE expr(b). {
	e = std::make_unique<exo::ast::OpBinaryGe>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
binop(e) ::= expr(n) T_ASSIGN expr(v). {
	e = std::make_unique<exo::ast::OpBinaryAssign>( std::move(n), std::move(v) );
	EXO_TRACK_NODE(e);
}
/* binary shorthand ops */
binop(e) ::= expr(a) T_ASSIGN_PLUS expr(b). {
	e = std::make_unique<exo::ast::OpBinaryAssignAdd>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
binop(e) ::= expr(a) T_ASSIGN_MINUS expr(b). {
	e = std::make_unique<exo::ast::OpBinaryAssignSub>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
binop(e) ::= expr(a) T_ASSIGN_MUL expr(b). {
	e = std::make_unique<exo::ast::OpBinaryAssignMul>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}
binop(e) ::= expr(a) T_ASSIGN_DIV expr(b). {
	e = std::make_unique<exo::ast::OpBinaryAssignDiv>( std::move(a), std::move(b) );
	EXO_TRACK_NODE(e);
}

/* a call can be a function or method call */
%type call { std::unique_ptr<exo::ast::ExprCallFun> }
call(c) ::= id(i) T_LANGLE exprlist(a) T_RANGLE. {
	c = std::make_unique<exo::ast::ExprCallFun>( std::move(i), std::move(a) );
	EXO_TRACK_NODE(c);
}
call(c) ::= expr(v) T_PTR S_ID(i) T_LANGLE exprlist(a) T_RANGLE. {
	c = std::make_unique<exo::ast::ExprCallMethod>( std::make_unique<exo::ast::Id>( TOKENSTR(i) ), std::move(v), std::move(a) );
	EXO_TRACK_NODE(c);
}

/* a constant can be a builtin (null, true, false, __*__), number or string */
%type constant { std::unique_ptr<exo::ast::ConstExpr> }
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
	c = std::make_unique<exo::ast::ConstStr>( ast->target->getName() );
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
}
constant(c) ::= string(s). {
	c = std::move(s);
}
/* a number can be an integer or a float */
%type number { std::unique_ptr<exo::ast::ConstExpr> }
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
%type string { std::unique_ptr<exo::ast::ConstStr> }
string(s) ::= T_QUOTE S_STRING(q) T_QUOTE. {
	s = std::make_unique<exo::ast::ConstStr>( TOKENSTR(q) );
	EXO_TRACK_NODE(s);
}

/* unary operation may be a delete, new or reference */
%type unop { std::unique_ptr<exo::ast::OpUnary> }
unop(u) ::= T_DELETE expr(e). {
	u = std::make_unique<exo::ast::OpUnaryDel>( std::move(e) );
	EXO_TRACK_NODE(u);
}
unop(u) ::= T_NEW expr(e). {
	u = std::make_unique<exo::ast::OpUnaryNew>( std::move(e) );
	EXO_TRACK_NODE(u);
}
unop(u) ::= T_AMP expr(e). {
	u = std::make_unique<exo::ast::OpUnaryRef>( std::move(e) );
	EXO_TRACK_NODE(u);
}

/* a variable begins with dollar sign and may be a class property */
%type var { std::unique_ptr<exo::ast::ExprVar> }
var(v) ::= expr(e) T_PTR S_ID(n). {
	v = std::make_unique<exo::ast::ExprProp>( TOKENSTR(n), std::move(e) );
	EXO_TRACK_NODE(v);
}
var(v) ::= S_VAR(n). {
	v = std::make_unique<exo::ast::ExprVar>( TOKENSTR(n) );
	EXO_TRACK_NODE(v);
}


/* an access modifier is either public, private or protected */
%type access { std::unique_ptr<exo::ast::ModAccess> }
access(a) ::= T_PUBLIC. {
	a = std::make_unique<exo::ast::ModAccess>();
	a->isPublic = true;
	a->isPrivate = false;
	a->isProtected = false;
	EXO_TRACK_NODE(a);
}
access(a) ::= T_PRIVATE. {
	a = std::make_unique<exo::ast::ModAccess>();
	a->isPublic = false;
	a->isPrivate = true;
	a->isProtected = false;
	EXO_TRACK_NODE(a);
}
access(a) ::= T_PROTECTED. {
	a = std::make_unique<exo::ast::ModAccess>();
	a->isPublic = false;
	a->isPrivate = false;
	a->isProtected = true;
	EXO_TRACK_NODE(a);
}