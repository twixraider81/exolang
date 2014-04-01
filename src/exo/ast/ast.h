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

#include <llvm/Analysis/Verifier.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>

#include "lexer/lexer"
#include "parser/parser.h"

#include "nodes/token.h"
#include "nodes/node.h"
#include "nodes/nodeexpression.h"
#include "nodes/nodestatement.h"
#include "nodes/nodeinteger.h"
#include "nodes/nodefloat.h"
#include "nodes/tree.h"

#endif /* AST_H_ */
