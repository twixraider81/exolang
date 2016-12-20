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

#ifndef NODES_H_
#define NODES_H_

#include "exo/exo.h"
#include "exo/jit/llvm.h"
#include "exo/jit/target.h"

namespace exo
{
	namespace ast
	{
		// forward declare
		class ConstBool;
		class ConstFloat;
		class ConstInt;
		class ConstNull;
		class ConstStr;
		class DeclClass;
		class DeclFunProto;
		class DeclFun;
		class DeclMod;
		class DeclProp;
		class DeclVar;
		class DeclVarList;
		class ExprCallFun;
		class ExprCallMethod;
		class ExprList;
		class ExprProp;
		class ExprVar;
		class Id;
		class ModAccess;
		class Node;
		class OpBinary;
		class OpBinaryAdd;
		class OpBinaryAssign;
		class OpBinaryAssignAdd;
		class OpBinaryAssignDiv;
		class OpBinaryAssignMul;
		class OpBinaryAssignSub;
		class OpBinaryDiv;
		class OpBinaryEq;
		class OpBinaryGe;
		class OpBinaryGt;
		class OpBinaryLe;
		class OpBinaryLt;
		class OpBinaryMul;
		class OpBinaryNeq;
		class OpBinarySub;
		class OpUnaryDel;
		class OpUnaryNew;
		class OpUnaryRef;
		class StmtBreak;
		class StmtCont;
		class StmtDo;
		class StmtExpr;
		class StmtFor;
		class StmtImport;
		class StmtIf;
		class StmtLabel;
		class StmtList;
		class StmtReturn;
		class StmtScope;
		class StmtSwitch;
		class StmtUse;
		class StmtWhile;
		class Type;
		class Tree;

		/**
		 * abstract class to hold our visitor double dispatched methods
		 */
		class Visitor
		{
			public:
				virtual void visit( ConstBool& ) = 0;
				virtual void visit( ConstFloat& ) = 0;
				virtual void visit( ConstInt& ) = 0;
				virtual void visit( ConstNull& ) = 0;
				virtual void visit( ConstStr& ) = 0;
				virtual void visit( DeclClass& ) = 0;
				virtual void visit( DeclFunProto& ) = 0;
				virtual void visit( DeclFun& ) = 0;
				virtual void visit( DeclMod& ) = 0;
				virtual void visit( DeclVar& ) = 0;
				virtual void visit( DeclVarList& ) = 0;
				virtual void visit( ExprCallFun& ) = 0;
				virtual void visit( ExprCallMethod& ) = 0;
				virtual void visit( ExprVar& ) = 0;
				virtual void visit( ExprProp& ) = 0;
				virtual void visit( Node& ) = 0;
				virtual void visit( OpBinaryAdd& ) = 0;
				virtual void visit( OpBinaryAssign& ) = 0;
				virtual void visit( OpBinaryAssignAdd& ) = 0;
				virtual void visit( OpBinaryAssignDiv& ) = 0;
				virtual void visit( OpBinaryAssignMul& ) = 0;
				virtual void visit( OpBinaryAssignSub& ) = 0;
				virtual void visit( OpBinaryDiv& ) = 0;
				virtual void visit( OpBinaryEq& ) = 0;
				virtual void visit( OpBinaryGe& ) = 0;
				virtual void visit( OpBinaryGt& ) = 0;
				virtual void visit( OpBinaryLe& ) = 0;
				virtual void visit( OpBinaryLt& ) = 0;
				virtual void visit( OpBinaryMul& ) = 0;
				virtual void visit( OpBinaryNeq& ) = 0;
				virtual void visit( OpBinarySub& ) = 0;
				virtual void visit( OpUnaryDel& ) = 0;
				virtual void visit( OpUnaryNew& ) = 0;
				virtual void visit( OpUnaryRef& ) = 0;
				virtual void visit( StmtBreak& ) = 0;
				virtual void visit( StmtCont& ) = 0;
				virtual void visit( StmtDo& ) = 0;
				virtual void visit( StmtExpr& ) = 0;
				virtual void visit( StmtFor& ) = 0;
				virtual void visit( StmtIf& ) = 0;
				virtual void visit( StmtImport& ) = 0;
				virtual void visit( StmtLabel& ) = 0;
				virtual void visit( StmtList& ) = 0;
				virtual void visit( StmtReturn& ) = 0;
				virtual void visit( StmtScope& ) = 0;
				virtual void visit( StmtSwitch& ) = 0;
				virtual void visit( StmtUse& ) = 0;
				virtual void visit( StmtWhile& ) = 0;
				virtual void visit( Tree& ) = 0;

				virtual ~Visitor() {};
		};

		/**
		 * root node for every node in our syntax tree
		 */
		class Node
		{
			public:
				/**
				 * line in wich the node occured
				 */
				long long lineNo;

				/**
				 * column in wich the node occured
				 */
				long long columnNo;

				Node();
				virtual ~Node();
				virtual void accept( Visitor* v );
		};

		/**
		 * root node for every expression
		 */
		class Expr : public virtual Node { };

		/**
		 * root node for every statement
		 */
		class Stmt : public virtual Node { };

		/**
		 * a constant expression
		 */
		class ConstExpr : public virtual Expr
		{
			public:
				virtual std::string getAsString();
		};

		/**
		 * a constant boolean (true/false)
		 */
		class ConstBool : public virtual ConstExpr
		{
			public:
				/**
				 * actual value
				 */
				bool value;

				ConstBool( bool v );
				void setTrue();
				void setFalse();
				virtual void accept( Visitor* v );
				virtual std::string getAsString();
		};

		/**
		 * a constant float
		 */
		class ConstFloat : public virtual ConstExpr
		{
			public:
				/**
				 * actual value
				 */
				double value;

				ConstFloat( double v );
				void setValue( double v );
				virtual void accept( Visitor* v );
				virtual std::string getAsString();
		};

		/**
		 * a constant integer
		 */
		class ConstInt : public virtual ConstExpr
		{
			public:
				/**
				 * actual value
				 */
				long long value;

				ConstInt( long long v );
				void setValue( long long v );
				virtual void accept( Visitor* v );
				virtual std::string getAsString();
		};

		/**
		 * null constant value
		 */
		class ConstNull : public virtual ConstExpr
		{
			public:
				ConstNull();
				virtual void accept( Visitor* v );
		};

		/**
		 * constant string
		 */
		class ConstStr : public virtual ConstExpr
		{
			public:
				/**
				 * actual value
				 */
				std::string value;

				ConstStr( std::string v );
				void setValue( std::string v );
				virtual void accept( Visitor* v );
				virtual std::string getAsString();
		};

		/**
		 * a class delcaration
		 */
		class DeclClass : public virtual Stmt
		{
			public:

				/**
				 * the class identifier/name
				 */
				std::unique_ptr<Id>							id;

				/**
				 * the parent class identifier/name (if any)
				 */
				std::unique_ptr<Id>							parent;

				/**
				 * the own class properties, sans inherited (if any)
				 */
				std::vector< std::unique_ptr<DeclProp> >	properties;

				/**
				 * the own class methods, sans inherited (if any)
				 */
				std::vector< std::unique_ptr<DeclFun> >		methods;

				DeclClass();
				virtual void setId( std::unique_ptr<Id> i, std::unique_ptr<Id> p );
				virtual void setId( std::unique_ptr<Id> i );
				virtual void addProperty( std::unique_ptr<DeclProp> p );
				virtual void addMethod( std::unique_ptr<DeclFun> m );
				virtual void accept( Visitor* v );
		};

		/**
		 * a function prototype definition
		 */
		class DeclFunProto : public virtual Stmt
		{
			public:
				/**
				 * the function name
				 */
				std::unique_ptr<Id>				id;
				std::unique_ptr<Type>			returnType;
				std::unique_ptr<DeclVarList>	arguments;
				bool							hasVaArg;

				DeclFunProto( std::unique_ptr<Id> i, std::unique_ptr<Type> r, std::unique_ptr<DeclVarList> a, bool va = false );
				virtual void accept( Visitor* v );
		};

		/**
		 * a function definition, has a function body
		 */
		class DeclFun : public virtual DeclFunProto
		{
			public:
				std::unique_ptr<StmtScope>	scope;
				std::unique_ptr<ModAccess>	access;

				DeclFun( std::unique_ptr<Id> i, std::unique_ptr<Type> r, std::unique_ptr<DeclVarList> a, std::unique_ptr<StmtScope> b, bool va = false );
				DeclFun( std::unique_ptr<Id> i, std::unique_ptr<ModAccess> m, std::unique_ptr<Type> r, std::unique_ptr<DeclVarList> a, std::unique_ptr<StmtScope> b, bool va = false );
				virtual void accept( Visitor* v );
		};

		/**
		 * a module declaration, naming the module
		 */
		class DeclMod : public virtual Stmt
		{
			public:
				std::unique_ptr<Id>			id;

				DeclMod( std::unique_ptr<Id> i );
				virtual void accept( Visitor* v );
		};

		/**
		 * a class property declaration
		 */
		class DeclProp : public virtual Stmt
		{
			public:
				std::unique_ptr<ModAccess>	access;
				std::unique_ptr<DeclVar>	property;

				DeclProp( std::unique_ptr<DeclVar> d, std::unique_ptr<ModAccess> a );
		};

		/**
		 * a variable declaration
		 */
		class DeclVar : public virtual Stmt
		{
			public:
				std::string					name;
				std::unique_ptr<Type>		type;
				std::unique_ptr<Expr>		expression;
				bool						isRef;

				DeclVar( std::string n, std::unique_ptr<Type> t, std::unique_ptr<Expr> e, bool r = false );
				DeclVar( std::string n, std::unique_ptr<Type> t, bool r = false );
				virtual void accept( Visitor* v );
		};

		/**
		 * a list of variable declarations
		 */
		class DeclVarList : public virtual Stmt
		{
			public:
				std::vector< std::unique_ptr<DeclVar> >	list;

				DeclVarList();
				void addDecl( std::unique_ptr<DeclVar> d );
				virtual void accept( Visitor* v );
		};

		/**
		 * a function call
		 */
		class ExprCallFun : public virtual Expr
		{
			public:
				std::unique_ptr<Id>			id;
				std::unique_ptr<ExprList>	arguments;

				ExprCallFun( std::unique_ptr<Id> i, std::unique_ptr<ExprList> a );
				virtual void accept( Visitor* v );
		};

		/**
		 * a method invocation
		 */
		class ExprCallMethod : public virtual ExprCallFun
		{
			public:
				std::unique_ptr<Expr> expression;

				ExprCallMethod( std::unique_ptr<Id> i, std::unique_ptr<Expr> e, std::unique_ptr<ExprList> a );
				virtual void accept( Visitor* v );
		};

		/**
		 * a list of expression
		 */
		class ExprList : public virtual Expr
		{
			public:
				std::vector< std::unique_ptr<Expr> > list;

				ExprList();
				void addExpr( std::unique_ptr<Expr> e );
		};

		/**
		 * a variable access
		 */
		class ExprVar : public virtual Expr
		{
			public:
				std::string name;

				ExprVar( std::string vName );
				virtual void accept( Visitor* v );
		};

		/**
		 * a class property access
		 */
		class ExprProp : public virtual ExprVar
		{
			public:
				std::unique_ptr<Expr> expression;

				ExprProp( std::string pName, std::unique_ptr<Expr> e );
				virtual void accept( Visitor* v );
		};

		/**
		 * an access modifier
		 */
		class ModAccess : public virtual Node
		{
			public:
				// FIXME: use a bitmask, this here is unsafe
				bool isPublic;
				bool isPrivate;
				bool isProtected;
				ModAccess();
		};

		/**
		 * a binary operation, operating on 2 expressions/operands
		 */
		class OpBinary : public virtual Expr
		{
			public:
				std::unique_ptr<Expr> lhs;
				std::unique_ptr<Expr> rhs;

				OpBinary( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				OpBinary();
		};

		/**
		 * a binary add operation
		 */
		class OpBinaryAdd : public virtual OpBinary
		{
			public:
				OpBinaryAdd( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a binary assign operation
		 */
		class OpBinaryAssign : public virtual OpBinary
		{
			public:
				OpBinaryAssign( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				OpBinaryAssign();
				virtual void accept( Visitor* v );
		};

		/**
		 * a binary division operation
		 */
		class OpBinaryDiv : public virtual OpBinary
		{
			public:
				OpBinaryDiv( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a binary equal comparisson
		 */
		class OpBinaryEq : public virtual OpBinary
		{
			public:
				OpBinaryEq( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a binary greater equal comparisson
		 */
		class OpBinaryGe : public virtual OpBinary
		{
			public:
				OpBinaryGe( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a binary greater than comparisson
		 */
		class OpBinaryGt : public virtual OpBinary
		{
			public:
				OpBinaryGt( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a binary lower equal comparisson
		 */
		class OpBinaryLe : public virtual OpBinary
		{
			public:
				OpBinaryLe( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a binary lower than comparisson
		 */
		class OpBinaryLt : public virtual OpBinary
		{
			public:
				OpBinaryLt( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a binary multiplication operation
		 */
		class OpBinaryMul : public virtual OpBinary
		{
			public:
				OpBinaryMul( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a binary non equal operation
		 */
		class OpBinaryNeq : public virtual OpBinary
		{
			public:
				OpBinaryNeq( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a binary subtraction operation
		 */
		class OpBinarySub : public virtual OpBinary
		{
			public:
				OpBinarySub( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a binary shorthand addition/assign
		 */
		class OpBinaryAssignAdd : public virtual OpBinary
		{
			public:
				OpBinaryAssignAdd( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a binary shorthand subtraction/assign
		 */
		class OpBinaryAssignSub : public virtual OpBinary
		{
			public:
				OpBinaryAssignSub( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a binary shorthand multiplication/assign
		 */
		class OpBinaryAssignMul : public virtual OpBinary
		{
			public:
				OpBinaryAssignMul( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a binary shorthand division/assign
		 */
		class OpBinaryAssignDiv : public virtual OpBinary
		{
			public:
				OpBinaryAssignDiv( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * an unary operation, has one operand
		 */
		class OpUnary : public virtual Expr
		{
			public:
				std::unique_ptr<Expr> rhs;

				OpUnary( std::unique_ptr<Expr> e );
		};

		/**
		 * an unary delete operation, has one operand of type object/expression
		 */
		class OpUnaryDel : public virtual OpUnary
		{
			public:
				OpUnaryDel( std::unique_ptr<Expr> e );
				virtual void accept( Visitor* v );
		};

		/**
		 * an unary new operation, has one operand of type object/expression
		 */
		class OpUnaryNew : public virtual OpUnary
		{
			public:
				OpUnaryNew( std::unique_ptr<Expr> e );
				virtual void accept( Visitor* v );
		};

		/**
		 * an unary reference operation
		 */
		class OpUnaryRef : public virtual OpUnary
		{
			public:
				OpUnaryRef( std::unique_ptr<Expr> e );
				virtual void accept( Visitor* v );
		};

		/**
		 * a break statement
		 */
		class StmtBreak : public virtual Stmt
		{
			public:
				virtual void accept( Visitor* v );
		};

		/**
		 * a continue statement
		 */
		class StmtCont : public virtual Stmt
		{
			public:
				virtual void accept( Visitor* v );
		};

		/**
		 * a statement with a generic expression
		 */
		class StmtExpr : public virtual Stmt
		{
			public:
				std::unique_ptr<Expr> expression;

				StmtExpr( std::unique_ptr<Expr> e );
				virtual void accept( Visitor* v );
		};

		/**
		 * a do/while loop
		 */
		class StmtDo : public virtual StmtExpr
		{
			public:
				std::unique_ptr<Stmt> scope;

				StmtDo( std::unique_ptr<Expr> e, std::unique_ptr<Stmt> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a for loop
		 */
		class StmtFor : public virtual StmtExpr
		{
			public:
				std::unique_ptr<Stmt>			scope;
				std::unique_ptr<DeclVarList>	initialization;
				std::unique_ptr<ExprList>		update;

				StmtFor( std::unique_ptr<Expr> e, std::unique_ptr<DeclVarList> i, std::unique_ptr<ExprList> u, std::unique_ptr<Stmt> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * a if/else statement
		 */
		class StmtIf : public virtual StmtExpr
		{
			public:
				std::unique_ptr<Stmt> onTrue;
				std::unique_ptr<Stmt> onFalse;

				StmtIf( std::unique_ptr<Expr> e, std::unique_ptr<Stmt> t, std::unique_ptr<Stmt> f );
				StmtIf( std::unique_ptr<Expr> e, std::unique_ptr<Stmt> t );
				virtual void accept( Visitor* v );
		};

		/**
		 * a import statement
		 */
		class StmtImport : public virtual Stmt
		{
			public:
				std::unique_ptr<ConstStr>	library;

				StmtImport( std::unique_ptr<ConstStr> l );
				virtual void accept( Visitor* v );
		};

		/**
		 * a labeled statement
		 */
		class StmtLabel : public virtual StmtExpr
		{
			public:
				std::unique_ptr<Stmt>		stmt;
				std::unique_ptr<ConstExpr>	expression;

				StmtLabel( std::unique_ptr<Stmt> s, std::unique_ptr<ConstExpr> e );
				virtual void accept( Visitor* v );
		};

		/**
		 * a list of statements
		 */
		class StmtList : public virtual Stmt
		{
			public:
				std::vector< std::unique_ptr<Stmt> > list;

				StmtList();
				void addStmt( std::unique_ptr<Stmt> s );
				virtual void accept( Visitor* v );
		};

		/**
		 * a return statement
		 */
		class StmtReturn : public virtual StmtExpr
		{
			public:
				StmtReturn( std::unique_ptr<Expr> e );
				virtual void accept( Visitor* v );
		};

		/**
		 * a scoped/compound statement
		 */
		class StmtScope : public virtual Stmt
		{
			public:
				std::unique_ptr<StmtList>	stmts;

				StmtScope( std::unique_ptr<StmtList> l );
				StmtScope();
				virtual void accept( Visitor* v );
		};

		/**
		 * a switch statement
		 */
		class StmtSwitch : public virtual StmtExpr
		{
			public:
				std::unique_ptr<Stmt>						defaultCase;
				std::vector< std::unique_ptr<StmtLabel> >	cases;

				StmtSwitch();
				virtual void accept( Visitor* v );
				void setDefaultCase( std::unique_ptr<Stmt> d );
				void addCase( std::unique_ptr<StmtLabel> l );
				void setCondition( std::unique_ptr<Expr> e );
		};

		/**
		 * a use statement
		 */
		class StmtUse : public virtual Stmt
		{
			public:
				std::unique_ptr<Id>			id;

				StmtUse( std::unique_ptr<Id> i );
				virtual void accept( Visitor* v );
		};

		/**
		 * a while statement
		 */
		class StmtWhile : public virtual StmtExpr
		{
			public:
				std::unique_ptr<Stmt> scope;

				StmtWhile( std::unique_ptr<Expr> e, std::unique_ptr<Stmt> b );
				virtual void accept( Visitor* v );
		};

		/**
		 * an identifier to identify/name things (functions,classes,variables)
		 */
		class Id : public virtual Node
		{
			public:
				std::string name;
				std::string inNamespace;

				Id( std::string n, std::string ns = "" );
		};

		/**
		 * a type specifier for expressions/variables
		 */
		class Type : public virtual Node
		{
			public:
				bool isPrimitive;
				std::unique_ptr<Id> id;

				Type( std::unique_ptr<Id> i, bool p = false );
		};

		/**
		 * the abstract syntax tree
		 */
		class Tree
		{
			private:
				void*								parser;

			public:
				long long							currentLineNo;
				long long							currentColumnNo;

				std::string							fileName;
				std::string							moduleName;

				std::shared_ptr<exo::jit::Target>	target;
				std::unique_ptr<StmtList>			stmts;

				Tree( std::shared_ptr<exo::jit::Target> t );
				virtual ~Tree();
				virtual void accept( Visitor* v );
				std::unique_ptr<llvm::Module> Parse( std::string fName );
		};
	}
}

#endif /* NODES_H_ */
