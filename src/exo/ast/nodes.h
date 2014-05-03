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
		class Node : public virtual gc
		{
			public:
				Node() { };
				virtual ~Node() { };

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class Expr : public virtual Node
		{
		};

		class Stmt : public virtual Node
		{
		};

		class ExprList : public virtual Expr
		{
			public:
				std::vector<Expr*> list;

				ExprList();
		};

		class StmtList : public virtual Expr
		{
			public:
				std::vector<Stmt*> list;

				StmtList();

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtExpr : public virtual Stmt
		{
			public:
				Expr* expression;

				StmtExpr( Expr* expr );
				~StmtExpr();

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class Type : public virtual Node
		{
			public:
				std::string name;

				Type( std::string tName );
		};

		class ModAccess : public virtual Node
		{
			public:
				ModAccess();

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class DecVar : public virtual Stmt
		{
			public:
				std::string name;
				Type* type;
				Expr* expression;

				DecVar( std::string vName, Type* vType, Expr* expr );
				DecVar( std::string vName, Type* vType );
				virtual ~DecVar();

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class DecList : public virtual Stmt
		{
			public:
				std::vector<DecVar*> list;

				DecList();
		};

		class DecFunProto : public virtual Stmt
		{
			public:
				std::string	name;
				Type*		returnType;
				DecList*	arguments;
				bool		hasVaArg;

				DecFunProto( std::string n, Type* rType, DecList* vArgs, bool va = false );
				virtual ~DecFunProto();

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class DecFun : public virtual DecFunProto
		{
			public:
				StmtList*	stmts;

				DecFun( std::string n, Type* rType, DecList* vArgs, StmtList* cBlock, bool va = false );
				virtual ~DecFun();

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class CallFun : public virtual Expr
		{
			public:
				std::string name;
				ExprList* arguments;

				CallFun( std::string n, ExprList* a );
				virtual ~CallFun();

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ExprVar : public virtual Expr
		{
			public:
				std::string variable;

				ExprVar( std::string vName );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtReturn : public virtual StmtExpr
		{
			public:
				StmtReturn( Expr* expr );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class DecProp : public virtual Stmt
		{
			public:
				ModAccess*	access;
				DecVar*		property;

				DecProp( DecVar* d, ModAccess* a );
				virtual ~DecProp();

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class DecMethod : public virtual Stmt
		{
			public:
				ModAccess*	access;
				DecFun*		method;

				DecMethod( DecFun* m, ModAccess* a );
				virtual ~DecMethod();

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ClassBlock : public virtual Stmt
		{
			public:
				std::vector<DecProp*>	properties;
				std::vector<DecMethod*>	methods;

				ClassBlock();
		};

		class DecClass : public virtual Stmt
		{
			public:
				std::string	name;
				std::string	parent;
				ClassBlock*	block;

				DecClass( std::string n, ClassBlock* b );
				DecClass( std::string n, std::string p, ClassBlock* b );
				virtual ~DecClass();

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ConstStr : public virtual Expr
		{
			public:
				std::string value;

				ConstStr( std::string v );

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

		class OpBinary : public virtual Expr
		{
			public:
				Expr* lhs;
				Expr* rhs;

				OpBinary( Expr* a, Expr* b );
				virtual ~OpBinary();
		};

		class OpBinaryAdd : public virtual OpBinary
		{
			public:
				OpBinaryAdd( Expr* a, Expr* b );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class OpBinarySub : public virtual OpBinary
		{
			public:
				OpBinarySub( Expr* a, Expr* b );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class OpBinaryMul : public virtual OpBinary
		{
			public:
				OpBinaryMul( Expr* a, Expr* b );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class OpBinaryDiv : public virtual OpBinary
		{
			public:
				OpBinaryDiv( Expr* a, Expr* b );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class OpBinaryEq : public virtual OpBinary
		{
			public:
				OpBinaryEq( Expr* a, Expr* b );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class OpBinaryNeq : public virtual OpBinary
		{
			public:
				OpBinaryNeq( Expr* a, Expr* b );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class OpBinaryLt : public virtual OpBinary
		{
			public:
				OpBinaryLt( Expr* a, Expr* b );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class OpBinaryLe : public virtual OpBinary
		{
			public:
				OpBinaryLe( Expr* a, Expr* b );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class OpBinaryGt : public virtual OpBinary
		{
			public:
				OpBinaryGt( Expr* a, Expr* b );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class OpBinaryGe : public virtual OpBinary
		{
			public:
				OpBinaryGe( Expr* a, Expr* b );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class OpBinaryAssign : public virtual OpBinary
		{
			public:
				OpBinaryAssign( Expr* a, Expr* b );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); }
		};

		class StmtIf : public virtual StmtExpr
		{
			public:
				Expr* expression;
				StmtList* onTrue;
				StmtList* onFalse;

				StmtIf( Expr* expr, StmtList* t, StmtList* f );
				virtual ~StmtIf();

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class CallMethod : public virtual CallFun
		{
			public:
				Expr* expression;

				CallMethod( Expr* e, std::string n, ExprList* a );
				virtual ~CallMethod();

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class OpUnary : public virtual Expr
		{
			public:
				Expr* rhs;

				OpUnary( Expr* a );
				virtual ~OpUnary();
		};

		class OpUnaryNew : public virtual OpUnary
		{
			public:
				OpUnaryNew( Expr* a );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); }
		};

		class StmtDelete : public virtual StmtExpr
		{
			public:
				StmtDelete( Expr* expr );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};
	}
}

#endif /* NODES_H_ */
