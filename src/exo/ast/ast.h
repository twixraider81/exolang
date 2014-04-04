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
#ifndef AST_H_
#define AST_H_

#include <llvm/IR/Value.h>

#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>

#include "exo/ast/lexer/lexer"
#include "exo/ast/parser/parser.h"

#include "exo/ast/context.h"
#include "exo/ast/tree.h"
#include "exo/ast/nodes/nodes.h"

/* we need a safe string from our tokens */
#define TOKENSTR(s) std::string( reinterpret_cast<const char*>( s->get_text().c_str() ) )

/* lemon doesn't define its protos */
void* ParseAlloc( void *(*mallocProc)(size_t) );
void  ParseFree( void *p, void (*freeProc)(void*) );
void  Parse( void *yyp, int yymajor, quex::Token* yyminor, exo::ast::Tree* ast );

#endif /* AST_H_ */
