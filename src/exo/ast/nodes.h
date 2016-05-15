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
		class Decl : public virtual Stmt { };

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
		class OpBinary;
		class OpBinaryAssign;
		class OpBinaryAssignShort;
		class OpUnaryDel;
		class OpUnaryNew;
		class StmtBlock;
		class StmtBreak;
		class StmtCont;
		class StmtExpr;
		class StmtFor;
		class StmtImport;
		class StmtIf;
		class StmtReturn;
		class StmtUse;
		class StmtWhile;
		class Type;
		class Tree;


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


		class DeclBlock : public virtual Decl
		{
			public:
				std::vector< std::unique_ptr<DeclProp> >	properties;
				std::vector< std::unique_ptr<DeclFun> >		methods;

				DeclBlock();
		};

		class DeclClass : public virtual DeclBlock
		{
			public:
				std::unique_ptr<Id>			id;
				std::unique_ptr<Id>			parent;

				DeclClass( std::unique_ptr<Id> i, std::unique_ptr<Id> p );
				DeclClass( std::unique_ptr<Id> i );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class DeclFunProto : public virtual Decl
		{
			public:
				std::unique_ptr<Id>				id;
				std::unique_ptr<Type>			returnType;
				std::unique_ptr<DeclVarList>	arguments;
				bool							hasVaArg;

				DeclFunProto( std::unique_ptr<Id> i, std::unique_ptr<Type> r, std::unique_ptr<DeclVarList> a, bool va = false );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class DeclFun : public virtual DeclFunProto
		{
			public:
				std::unique_ptr<Stmt>		stmts;
				std::unique_ptr<ModAccess>	access;

				DeclFun( std::unique_ptr<Id> i, std::unique_ptr<Type> r, std::unique_ptr<DeclVarList> a, std::unique_ptr<Stmt> b, bool va = false );
				DeclFun( std::unique_ptr<Id> i, std::unique_ptr<ModAccess> m, std::unique_ptr<Type> r, std::unique_ptr<DeclVarList> a, std::unique_ptr<Stmt> b, bool va = false );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class DeclMod : public virtual Decl
		{
			public:
				std::unique_ptr<Id>			id;

				DeclMod( std::unique_ptr<Id> i );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class DeclProp : public virtual Decl
		{
			public:
				std::unique_ptr<ModAccess>	access;
				std::unique_ptr<DeclVar>	property;

				DeclProp( std::unique_ptr<DeclVar> d, std::unique_ptr<ModAccess> a );
		};

		class DeclVar : public virtual Decl
		{
			public:
				std::string				name;
				std::unique_ptr<Type>	type;
				std::unique_ptr<Expr>	expression;

				DeclVar( std::string n, std::unique_ptr<Type> t, std::unique_ptr<Expr> e );
				DeclVar( std::string n, std::unique_ptr<Type> t );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class DeclVarList : public virtual Decl
		{
			public:
				std::vector< std::unique_ptr<DeclVar> >	list;

				DeclVarList();
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ExprCallFun : public virtual Expr
		{
			public:
				std::unique_ptr<Id>			id;
				std::unique_ptr<ExprList>	arguments;

				ExprCallFun( std::unique_ptr<Id> i, std::unique_ptr<ExprList> a );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ExprCallMethod : public virtual ExprCallFun
		{
			public:
				std::unique_ptr<Expr> expression;

				ExprCallMethod( std::unique_ptr<Id> i, std::unique_ptr<Expr> e, std::unique_ptr<ExprList> a );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
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
				// FIXME: use a bitmask, this here is unsafe
				bool isPublic;
				bool isPrivate;
				bool isProtected;
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

		class StmtBlock : public virtual Stmt
		{
			public:
				std::vector< std::unique_ptr<Stmt> > list;

				StmtBlock();
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtBreak : public virtual Stmt
		{
			public:
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtCont : public virtual Stmt
		{
			public:
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtExpr : public virtual Stmt
		{
			public:
				std::unique_ptr<Expr> expression;

				StmtExpr( std::unique_ptr<Expr> e );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtFor : public virtual StmtExpr
		{
			public:
				std::unique_ptr<Stmt>			block;
				std::unique_ptr<DeclVarList>	initialization;
				std::unique_ptr<ExprList>		update;

				StmtFor( std::unique_ptr<Expr> e, std::unique_ptr<DeclVarList> i, std::unique_ptr<ExprList> u, std::unique_ptr<Stmt> b );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtIf : public virtual StmtExpr
		{
			public:
				std::unique_ptr<Stmt> onTrue;
				std::unique_ptr<Stmt> onFalse;

				StmtIf( std::unique_ptr<Expr> e, std::unique_ptr<Stmt> t, std::unique_ptr<Stmt> f );
				StmtIf( std::unique_ptr<Expr> e, std::unique_ptr<Stmt> t );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtImport : public virtual Stmt
		{
			public:
				std::unique_ptr<ConstStr>	library;

				StmtImport( std::unique_ptr<ConstStr> l );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtReturn : public virtual StmtExpr
		{
			public:
				StmtReturn( std::unique_ptr<Expr> e );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtUse : public virtual Stmt
		{
			public:
				std::unique_ptr<Id>			id;

				StmtUse( std::unique_ptr<Id> i );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtWhile : public virtual StmtExpr
		{
			public:
				std::unique_ptr<Stmt> block;

				StmtWhile( std::unique_ptr<Expr> e, std::unique_ptr<Stmt> b );
				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};


		class Id : public virtual Node
		{
			public:
				std::string name;
				std::string inNamespace;

				Id( std::string n, std::string ns = "::" );
		};

		class Type : public virtual Node
		{
			public:
				bool isPrimitive;
				std::unique_ptr<Id> id;

				Type( std::unique_ptr<Id> i, bool p = false );
		};


		class Tree
		{
			private:
				void* parser;

			public:
				long long currentLineNo;
				long long currentColumnNo;

				std::string		moduleName;
				std::string		fileName;
				std::string		targetMachine;

				std::unique_ptr<exo::ast::StmtBlock> stmts;

				Tree();
				~Tree();
				void Parse( std::string fName, std::string target );
		};
	}
}

#endif /* NODES_H_ */
