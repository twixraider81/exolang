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

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class Type : public virtual Node
		{
			public:
				std::string name;

				Type( std::string tName );
		};

		class VarDecl : public virtual Stmt
		{
			public:
				std::string name;
				Type* type;
				Expr* expression;

				VarDecl( std::string vName, Type* vType, Expr* expr );
				VarDecl( std::string vName, Type* vType );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class VarAssign : public virtual Expr
		{
			public:
				std::string name;
				Expr* expression;

				VarAssign( std::string vName, Expr* expr );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); }
		};

		class VarDeclList : public virtual Stmt
		{
			public:
				std::vector<VarDecl*> list;

				VarDeclList();
		};

		class ExprList : public virtual Expr
		{
			public:
				std::vector<Expr*> list;

				ExprList();
		};

		class FunDeclProto : public virtual Stmt
		{
			public:
				std::string		name;
				Type*			returnType;
				VarDeclList*	arguments;

				FunDeclProto( std::string n, Type* rType, VarDeclList* vArgs );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class FunDecl : public virtual FunDeclProto
		{
			public:
				StmtList*		stmts;

				FunDecl( std::string n, Type* rType, VarDeclList* vArgs, StmtList* cBlock );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class FunCall : public virtual Expr
		{
			public:
				std::string name;
				ExprList* arguments;

				FunCall( std::string n, ExprList* a );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class BinaryOp : public virtual Expr
		{
			public:
				Expr* lhs;
				std::string op;
				Expr* rhs;

				BinaryOp( Expr* a, std::string* o, Expr* b );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class CmpOp : public virtual Expr
		{
			public:
				Expr* lhs;
				std::string op;
				Expr* rhs;

				CmpOp( Expr* a, std::string* o, Expr* b );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class VarExpr : public virtual Expr
		{
			public:
				std::string variable;

				VarExpr( std::string vName );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class StmtReturn : public virtual StmtExpr
		{
			public:
				Expr* expression;

				StmtReturn( Expr* expr );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ClassBlock : public virtual Stmt
		{
			public:
				std::vector<VarDecl*>	properties;
				std::vector<FunDecl*>	methods;

				ClassBlock();
		};

		class ClassDecl : public virtual Stmt
		{
			public:
				std::string	name;
				ClassBlock*	block;

				ClassDecl( std::string n, ClassBlock* b );

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
	}
}

#endif /* NODES_H_ */
