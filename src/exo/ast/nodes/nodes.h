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

/*
 * TODO: user boost templates instead of std::vector
 */
namespace exo
{
	namespace ast
	{
		namespace nodes
		{
			class Node : public virtual gc
			{
				public:
					Node() { };
					virtual ~Node() { };

					virtual llvm::Value* Generate( exo::ast::Context* context );
			};

			class Expr : public Node
			{
			};

			class Stmt : public Node
			{
			};

			class StmtList : public Expr
			{
				public:
					std::vector<Stmt*> list;

					StmtList();

					virtual llvm::Value* Generate( exo::ast::Context* context );
			};

			class StmtExpr : public Stmt
			{
				public:
					Expr* expression;

					StmtExpr( Expr* expr );
			};

			class Type : public Node
			{
				public:
					exo::jit::types::Type* jitType;

					Type( const std::type_info* type );
					Type( const std::type_info* type, std::string tName );
			};

			class VarDecl : public Stmt
			{
				public:
					std::string name;
					Type* type;
					Expr* expression;

					VarDecl( std::string vName, Type* vType, Expr* expr );
					VarDecl( std::string vName, Type* vType );

					virtual llvm::Value* Generate( exo::ast::Context* context );
			};

			class VarAssign : public Expr
			{
				public:
					std::string name;
					Expr* expression;

					VarAssign( std::string vName, Expr* expr );

					virtual llvm::Value* Generate( exo::ast::Context* context );
			};

			class VarDeclList : public Stmt
			{
				public:
					std::vector<VarDecl*> list;

					VarDeclList();
			};


			class ExprList : public Expr
			{
				public:
					std::vector<Expr*> list;

					ExprList();
			};

			class FunDecl : public Stmt
			{
				public:
					Type* type;
					Type* returnType;
					VarDeclList* arguments;
					StmtList* codeBlock;

					FunDecl( Type* fType, Type* rType, VarDeclList* vArgs, StmtList* cBlock );
			};

			class FunCall : public Expr
			{
				public:
					std::string name;
					ExprList* arguments;

					FunCall( std::string n, ExprList* a );
			};

			class CompOp : public Expr
			{
				public:
					Expr* lhs;
					std::string op;
					Expr* rhs;

					CompOp( Expr* a, std::string o, Expr* b );
			};

			class ConstExpr : public Expr
			{
				public:
					std::string				name;
					exo::jit::types::Type*	value;

					ConstExpr( std::string n, exo::jit::types::Type* v );
			};
		}
	}
}

#endif /* NODES_H_ */
