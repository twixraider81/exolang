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

/*
 * TODO: user boost templates instead of std::vector
 */
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
				const std::type_info*	info;
				std::string name;

				Type( const std::type_info* t );
				Type( const std::type_info* t , std::string tName );
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

		class ConstExpr : public virtual Expr
		{
			public:
				std::string	name;
				Expr*		expression;

				ConstExpr( std::string n, Expr* e );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ValueNull : public virtual Expr
		{
			public:
				ValueNull();

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); }
		};

		class ValueBool : public virtual Expr
		{
			public:
				bool value;
				ValueBool( bool bVal );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ValueInt : public virtual Expr
		{
			public:
				std::string value;
				ValueInt( std::string iVal );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ValueFloat : public virtual Expr
		{
			public:
				std::string value;
				ValueFloat( std::string fVal );

				virtual llvm::Value* Generate( exo::jit::Codegen* ctx ) { return( ctx->Generate( this ) ); };
		};

		class ValueString : public virtual Expr
		{
			public:
				std::string value;
				ValueString( std::string sVal );
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
	}
}

#endif /* NODES_H_ */
