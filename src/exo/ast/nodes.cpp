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

#include "exo/ast/nodes.h"

namespace exo
{
	namespace ast
	{
		ConstBool::ConstBool( bool v ) : value( v )
		{
		};

		void ConstBool::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		std::string ConstBool::getAsString()
		{
			return( std::to_string( value ) );
		};

		void ConstBool::setTrue()
		{
			value = true;
		};

		void ConstBool::setFalse()
		{
			value = false;
		};

		std::string ConstExpr::getAsString()
		{
			return( "" );
		};

		ConstFloat::ConstFloat( double v ) : value( v )
		{
		};

		void ConstFloat::setValue( double v )
		{
			value = v;
		};

		void ConstFloat::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		std::string ConstFloat::getAsString()
		{
			return( std::to_string( value ) );
		};

		ConstInt::ConstInt( long long v ) : value( v )
		{
		};

		void ConstInt::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		void ConstInt::setValue( long long v )
		{
			value = v;
		};

		std::string ConstInt::getAsString()
		{
			return( std::to_string( value ) );
		};

		ConstNull::ConstNull()
		{
		};

		void ConstNull::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		ConstStr::ConstStr( std::string v ) : value( v )
		{
		};

		void ConstStr::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		std::string ConstStr::getAsString()
		{
			return( value );
		};

		void ConstStr::setValue( std::string v )
		{
			value = v;
		};

		DeclClass::DeclClass()
		{
		};

		void DeclClass::setId( std::unique_ptr<Id> i, std::unique_ptr<Id> p )
		{
			id = std::move( i );
			parent = std::move( p );
		};

		void DeclClass::setId( std::unique_ptr<Id> i )
		{
			id = std::move( i );
		};

		void DeclClass::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		void DeclClass::addProperty( std::unique_ptr<DeclProp> p )
		{
			properties.push_back( std::move( p ) );
		};

		void DeclClass::addMethod( std::unique_ptr<DeclFun> m )
		{
			methods.push_back( std::move( m ) );
		};

		DeclFunProto::DeclFunProto( std::unique_ptr<Id> i, std::unique_ptr<Type> r, std::unique_ptr<DeclVarList> a, bool va ) :
			id( std::move( i ) ),
			returnType( std::move( r ) ),
			arguments( std::move( a ) ),
			hasVaArg( va )
		{
		};

		void DeclFunProto::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		DeclFun::DeclFun( std::unique_ptr<Id> i, std::unique_ptr<Type> r, std::unique_ptr<DeclVarList> a, std::unique_ptr<StmtScope> b, bool va ) :
			DeclFunProto( std::move( i ), std::move( r ), std::move( a ), va ),
			scope( std::move( b ) ),
			access( std::make_unique<ModAccess>() )
		{
		};

		DeclFun::DeclFun( std::unique_ptr<Id> i, std::unique_ptr<ModAccess> m, std::unique_ptr<Type> r, std::unique_ptr<DeclVarList> a, std::unique_ptr<StmtScope> b, bool va ) :
			DeclFunProto( std::move( i ), std::move( r ), std::move( a ), va ),
			scope( std::move( b ) ),
			access( std::move( m ) )
		{
		};

		void DeclFun::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		DeclMod::DeclMod( std::unique_ptr<Id> i ) : id( std::move( i ) )
		{
		};

		void DeclMod::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		DeclProp::DeclProp( std::unique_ptr<DeclVar> d, std::unique_ptr<ModAccess> a ) :
			access( std::move( a ) ),
			property( std::move( d ) )
		{
		};

		DeclVar::DeclVar( std::string n, std::unique_ptr<Type> t, std::unique_ptr<Expr> e, bool r ) :
			name( n ),
			type( std::move( t ) ),
			expression( std::move( e ) ),
			isRef( r )
		{
		};

		DeclVar::DeclVar( std::string n, std::unique_ptr<Type> t, bool r ) :
			name( n ),
			type( std::move( t ) ),
			isRef( r )
		{
		};

		void DeclVar::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		DeclVarList::DeclVarList()
		{
		};

		void DeclVarList::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		void DeclVarList::addDecl( std::unique_ptr<DeclVar> d )
		{
			list.push_back( std::move( d ) );
		};

		ExprCallFun::ExprCallFun( std::unique_ptr<Id> i, std::unique_ptr<ExprList> a ) :
			id( std::move( i ) ),
			arguments( std::move( a ) )
		{
		};

		void ExprCallFun::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		ExprCallMethod::ExprCallMethod( std::unique_ptr<Id> i, std::unique_ptr<Expr> e, std::unique_ptr<ExprList> a ) :
			ExprCallFun( std::move( i ), std::move( a ) ),
			expression( std::move( e ) )
		{
		};

		void ExprCallMethod::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		ExprList::ExprList()
		{
		};

		void ExprList::addExpr( std::unique_ptr<Expr> e )
		{
			list.push_back( std::move( e ) );
		};

		ExprVar::ExprVar( std::string vName ) : name( vName )
		{
		};

		void ExprVar::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		ExprProp::ExprProp( std::string pName, std::unique_ptr<Expr> e ) :
			ExprVar( pName ),
			expression( std::move( e ) )
		{
		};

		void ExprProp::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		Id::Id( std::string n, std::string ns ) :
			name( n ),
			inNamespace( ns )
		{
		};

		ModAccess::ModAccess() :
			isPublic( false ),
			isPrivate( false ),
			isProtected( true )
		{
		};

		Node::Node() : lineNo( 0 ), columnNo( 0 )
		{
		};

		Node::~Node()
		{
		};

		void Node::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpBinary::OpBinary( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			lhs( std::move( a ) ),
			rhs( std::move( b ) )
		{
		};

		OpBinary::OpBinary()
		{
		};

		OpBinaryAdd::OpBinaryAdd( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ),
			std::move( b ) )
		{
		};

		void OpBinaryAdd::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpBinaryAssign::OpBinaryAssign( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ), std::move( b ) )
		{
		};

		OpBinaryAssign::OpBinaryAssign()
		{
		};

		void OpBinaryAssign::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpBinaryDiv::OpBinaryDiv( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ), std::move( b ) )
		{
		};

		void OpBinaryDiv::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpBinaryEq::OpBinaryEq( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ), std::move( b ) )
		{
		};

		void OpBinaryEq::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpBinaryGe::OpBinaryGe( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ), std::move( b ) )
		{
		};

		void OpBinaryGe::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpBinaryGt::OpBinaryGt( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ), std::move( b ) )
		{
		};

		void OpBinaryGt::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpBinaryLe::OpBinaryLe( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ), std::move( b ) )
		{
		};

		void OpBinaryLe::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpBinaryLt::OpBinaryLt( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ), std::move( b ) )
		{
		};

		void OpBinaryLt::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpBinaryMul::OpBinaryMul( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ), std::move( b ) )
		{
		};

		void OpBinaryMul::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpBinaryNeq::OpBinaryNeq( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ), std::move( b ) )
		{
		};

		OpBinarySub::OpBinarySub( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ), std::move( b ) )
		{
		};

		void OpBinaryNeq::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		void OpBinarySub::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpBinaryAssignAdd::OpBinaryAssignAdd( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ), std::move( b ) )
		{
		};

		void OpBinaryAssignAdd::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpBinaryAssignSub::OpBinaryAssignSub( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ), std::move( b ) )
		{
		};

		void OpBinaryAssignSub::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpBinaryAssignMul::OpBinaryAssignMul( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ), std::move( b ) )
		{
		};

		void OpBinaryAssignMul::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpBinaryAssignDiv::OpBinaryAssignDiv( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b ) :
			OpBinary( std::move( a ), std::move( b ) )
		{
		};

		void OpBinaryAssignDiv::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpUnary::OpUnary( std::unique_ptr<Expr> e ) :
			rhs( std::move( e ) )
		{
		};

		OpUnaryDel::OpUnaryDel( std::unique_ptr<Expr> e ) :
			OpUnary( std::move( e ) )
		{
		};

		void OpUnaryDel::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpUnaryNew::OpUnaryNew( std::unique_ptr<Expr> e ) :
			OpUnary( std::move( e ) )
		{
		};

		void OpUnaryNew::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		OpUnaryRef::OpUnaryRef( std::unique_ptr<Expr> e ) :
			OpUnary( std::move( e ) )
		{
		};

		void OpUnaryRef::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		void StmtBreak::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		void StmtCont::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		StmtDo::StmtDo( std::unique_ptr<Expr> e, std::unique_ptr<Stmt> b ) :
			StmtExpr( std::move( e ) ),
			scope( std::move( b ) )
		{
		};

		void StmtDo::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		StmtExpr::StmtExpr( std::unique_ptr<Expr> e ) :
			expression( std::move( e ) )
		{
		};

		void StmtExpr::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		StmtFor::StmtFor( std::unique_ptr<Expr> e, std::unique_ptr<DeclVarList> i, std::unique_ptr<ExprList> u, std::unique_ptr<Stmt> b ) :
			StmtExpr( std::move( e ) ),
			initialization( std::move( i ) ),
			update( std::move( u ) ),
			scope( std::move( b ) )
		{
		};

		void StmtFor::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		StmtIf::StmtIf( std::unique_ptr<Expr> e, std::unique_ptr<Stmt> t, std::unique_ptr<Stmt> f ) :
			StmtExpr( std::move( e ) ),
			onTrue( std::move( t ) ),
			onFalse( std::move( f ) )
		{
		};

		StmtIf::StmtIf( std::unique_ptr<Expr> e, std::unique_ptr<Stmt> t ) :
			StmtExpr( std::move( e ) ),
			onTrue( std::move( t ) )
		{
		};

		void StmtIf::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		StmtImport::StmtImport( std::unique_ptr<ConstStr> l ) :
			library( std::move( l ) )
		{
		};

		void StmtImport::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		StmtLabel::StmtLabel( std::unique_ptr<Stmt> s, std::unique_ptr<ConstExpr> e ) :
			StmtExpr( nullptr ),
			stmt( std::move( s ) ),
			expression( std::move( e ) )
		{
		};

		void StmtLabel::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		StmtList::StmtList()
		{
		};

		void StmtList::addStmt( std::unique_ptr<Stmt> s )
		{
			list.push_back( std::move( s ) );
		};

		void StmtList::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		StmtReturn::StmtReturn( std::unique_ptr<Expr> e ) :
			StmtExpr( std::move( e ) )
		{
		};

		void StmtReturn::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		StmtScope::StmtScope( std::unique_ptr<StmtList> l ) :
			stmts( std::move( l ) )
		{
		};

		StmtScope::StmtScope() :
			stmts( std::make_unique<StmtList>() )
		{
		};

		void StmtScope::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		StmtSwitch::StmtSwitch() :
			StmtExpr( std::make_unique<ConstBool>( false ) )
		{
		};

		void StmtSwitch::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		void StmtSwitch::setDefaultCase( std::unique_ptr<Stmt> d )
		{
			if( defaultCase.get() ) {
				EXO_THROW_AT( InvalidExpr() << exo::exceptions::Message( "Default case already defined" ), (*this) );
			}

			defaultCase =  std::move( d );
		};

		void StmtSwitch::addCase( std::unique_ptr<StmtLabel> l )
		{
			this->cases.push_back( std::move(l) );
		};

		void StmtSwitch::setCondition( std::unique_ptr<Expr> e )
		{
			expression = std::move( e );
		};

		StmtUse::StmtUse( std::unique_ptr<Id> i ) :
			id( std::move( i ) )
		{
		};

		void StmtUse::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		StmtWhile::StmtWhile( std::unique_ptr<Expr> e, std::unique_ptr<Stmt> b ) :
			StmtExpr( std::move( e ) ),
			scope( std::move( b ) )
		{
		};

		void StmtWhile::accept( Visitor* visitor )
		{
			visitor->visit( *this );
		};

		Type::Type( std::unique_ptr<Id> i, bool p ) :
			id( std::move( i ) ),
			isPrimitive( p )
		{
		};
	}
}
