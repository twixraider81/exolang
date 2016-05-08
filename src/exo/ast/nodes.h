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
#include "exo/jit/codegen.h"

namespace exo
{
	namespace ast
	{
		class Node
		{
			public:
				long long lineNo;
				long long columnNo;

				Node();
				virtual ~Node();
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx );
		};

		class Expr : public virtual Node { };
		class Stmt : public virtual Node { };

		// forward declare
		class DecProp;
		class DecMethod;
		class DecList;
		class DecVar;
		class ExprList;
		class ModAccess;
		class StmtExpr;
		class StmtList;
		class Type;


		class CallFun : public virtual Expr
		{
			public:
				std::string name;
				std::unique_ptr<ExprList> arguments;

				CallFun( std::string n, std::unique_ptr<ExprList> a );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class CallMethod : public virtual CallFun
		{
			public:
				std::unique_ptr<Expr> expression;

				CallMethod( std::string n, std::unique_ptr<Expr> e, std::unique_ptr<ExprList> a );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};


		class ClassBlock : public virtual Stmt
		{
			public:
				std::vector< std::unique_ptr<DecProp> >		properties;
				std::vector< std::unique_ptr<DecMethod> >	methods;

				ClassBlock();
		};



		class ConstBool : public virtual Expr
		{
			public:
				bool value;

				ConstBool( bool value );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ConstFloat : public virtual Expr
		{
			public:
				double value;

				ConstFloat( double value );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ConstInt : public virtual Expr
		{
			public:
				long long value;

				ConstInt( long long value );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ConstNull : public virtual Expr
		{
			public:
				ConstNull();
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ConstStr : public virtual Expr
		{
			public:
				std::string value;

				ConstStr( std::string v );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};


		class DecClass : public virtual Stmt
		{
			public:
				std::string	name;
				std::string	parent;
				std::unique_ptr<ClassBlock>	block;

				DecClass( std::string n, std::string p, std::unique_ptr<ClassBlock> b );
				DecClass( std::string n, std::unique_ptr<ClassBlock> b );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class DecFunProto : public virtual Stmt
		{
			public:
				std::string	name;
				std::unique_ptr<Type>		returnType;
				std::unique_ptr<DecList>	arguments;
				bool		hasVaArg;

				DecFunProto( std::string n, std::unique_ptr<Type> t, std::unique_ptr<DecList> a, bool va = false );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class DecFun : public virtual DecFunProto
		{
			public:
				std::unique_ptr<StmtList>	stmts;

				DecFun( std::string n, std::unique_ptr<Type> t, std::unique_ptr<DecList> a, std::unique_ptr<StmtList> b, bool va = false );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class DecList : public virtual Stmt
		{
			public:
				std::vector< std::unique_ptr<DecVar> > list;

				DecList();
		};

		class DecMethod : public virtual Stmt
		{
			public:
				std::unique_ptr<ModAccess>	access;
				std::unique_ptr<DecFun>		method;

				DecMethod( std::unique_ptr<DecFun> m, std::unique_ptr<ModAccess> a );
		};

		class DecVar : public virtual Stmt
		{
			public:
				std::string name;
				std::unique_ptr<Type> type;
				std::unique_ptr<Expr> expression;

				DecVar( std::string n, std::unique_ptr<Type> t, std::unique_ptr<Expr> e );
				DecVar( std::string n, std::unique_ptr<Type> t );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class DecProp : public virtual Stmt
		{
			public:
				std::unique_ptr<ModAccess>	access;
				std::unique_ptr<DecVar>		property;

				DecProp( std::unique_ptr<DecVar> d, std::unique_ptr<ModAccess> a );
		};



		class ExprList : public virtual Expr
		{
			public:
				std::vector< std::unique_ptr<Expr> > list;

				ExprList();
		};

		class ExprVar : public virtual Expr
		{
			public:
				std::string name;

				ExprVar( std::string vName );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ExprProp : public virtual ExprVar
		{
			public:
				std::unique_ptr<Expr> expression;

				ExprProp( std::string pName, std::unique_ptr<Expr> e );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ModAccess : public virtual Node
		{
			public:
				ModAccess();
		};



		class OpBinary : public virtual Expr
		{
			public:
				std::unique_ptr<Expr> lhs;
				std::unique_ptr<Expr> rhs;

				OpBinary( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class OpBinaryAdd : public virtual OpBinary
		{
			public:
				OpBinaryAdd( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
		};

		class OpBinaryAssign : public virtual OpBinary
		{
			public:
				OpBinaryAssign( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); }
		};

		class OpBinaryDiv : public virtual OpBinary
		{
			public:
				OpBinaryDiv( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
		};

		class OpBinaryEq : public virtual OpBinary
		{
			public:
				OpBinaryEq( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
		};

		class OpBinaryGe : public virtual OpBinary
		{
			public:
				OpBinaryGe( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
		};

		class OpBinaryGt : public virtual OpBinary
		{
			public:
				OpBinaryGt( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
		};

		class OpBinaryLe : public virtual OpBinary
		{
			public:
				OpBinaryLe( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
		};

		class OpBinaryLt : public virtual OpBinary
		{
			public:
				OpBinaryLt( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
		};

		class OpBinaryMul : public virtual OpBinary
		{
			public:
				OpBinaryMul( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
		};

		class OpBinaryNeq : public virtual OpBinary
		{
			public:
				OpBinaryNeq( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
		};

		class OpBinarySub : public virtual OpBinary
		{
			public:
				OpBinarySub( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
		};

		class OpBinaryAssignShort : public virtual OpBinaryAssign
		{
			public:
				OpBinaryAssignShort( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); }
		};

		class OpBinaryAssignAdd : public virtual OpBinaryAssignShort
		{
			public:
				OpBinaryAssignAdd( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
		};

		class OpBinaryAssignSub : public virtual OpBinaryAssignShort
		{
			public:
				OpBinaryAssignSub( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
		};

		class OpBinaryAssignMul : public virtual OpBinaryAssignShort
		{
			public:
				OpBinaryAssignMul( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
		};

		class OpBinaryAssignDiv : public virtual OpBinaryAssignShort
		{
			public:
				OpBinaryAssignDiv( std::unique_ptr<Expr> a, std::unique_ptr<Expr> b );
		};


		class OpUnary : public virtual Expr
		{
			public:
				std::unique_ptr<Expr> rhs;

				OpUnary( std::unique_ptr<Expr> e );
		};

		class OpUnaryDel : public virtual OpUnary
		{
			public:
				OpUnaryDel( std::unique_ptr<Expr> e );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class OpUnaryNew : public virtual OpUnary
		{
			public:
				OpUnaryNew( std::unique_ptr<Expr> e );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};


		class StmtExpr : public virtual Stmt
		{
			public:
				std::unique_ptr<Expr> expression;

				StmtExpr( std::unique_ptr<Expr> e );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtBreak : public virtual Stmt
		{
			public:
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtFor : public virtual StmtExpr
		{
			public:
				std::unique_ptr<StmtList>	block;
				std::unique_ptr<DecList>	initialization;
				std::unique_ptr<ExprList>	update;

				StmtFor( std::unique_ptr<Expr> e, std::unique_ptr<DecList> i, std::unique_ptr<ExprList> u, std::unique_ptr<StmtList> b );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtIf : public virtual StmtExpr
		{
			public:
				std::unique_ptr<StmtList> onTrue;
				std::unique_ptr<StmtList> onFalse;

				StmtIf( std::unique_ptr<Expr> e, std::unique_ptr<StmtList> t, std::unique_ptr<StmtList> f );
				StmtIf( std::unique_ptr<Expr> e, std::unique_ptr<StmtList> t );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtList : public virtual Expr
		{
			public:
				std::vector< std::unique_ptr<Stmt> > list;

				StmtList();
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtReturn : public virtual StmtExpr
		{
			public:
				StmtReturn( std::unique_ptr<Expr> e );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtWhile : public virtual StmtExpr
		{
			public:
				std::unique_ptr<StmtList> block;

				StmtWhile( std::unique_ptr<Expr> e, std::unique_ptr<StmtList> b );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class Type : public virtual Node
		{
			public:
				std::string name;

				Type( std::string tName );
		};


		class Tree
		{
			public:
				long long currentLineNo;
				long long currentColumnNo;

				void*			parser;
				std::string		fileName;
				std::string		targetMachine;
				std::unique_ptr<exo::ast::StmtList> stmts;

				Tree();
				~Tree();
				void Parse( std::string fName, std::string target );
		};
	}
}

#endif /* NODES_H_ */
