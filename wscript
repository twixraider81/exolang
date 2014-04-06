#!/usr/bin/env python
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os, subprocess, sys, re, platform, pipes

from waflib import Build, Context, Scripting, Utils, Task, TaskGen
from waflib.ConfigSet import ConfigSet
from waflib.TaskGen import extension
from waflib.Task import Task

APPNAME = 'exolang'
VERSION = '0.0.1'

TOP = os.path.abspath( os.curdir )
BINDIR = TOP + '/bin/'
SRCDIR = 'src/'

# load options
def options( opt ):
	opt.load( 'compiler_cxx compiler_c python' )
	opt.add_option( '--mode', action = 'store', default = 'release', help = 'the mode to compile in (release,debug,trace)' )

# configure
def configure( conf ):
	conf.load( 'compiler_cxx compiler_c' )
	conf.find_program( 'llvm-config', var = 'LLVMCONFIG', mandatory = True )

	# read llvm-config
	print( "\n" )
	process = subprocess.Popen( [conf.env.LLVMCONFIG, '--cppflags'], stdout = subprocess.PIPE )
	cxxflags = process.communicate()[0].strip().replace( "\n", '' )
	conf.msg( 'llvm-config cxxflags:', cxxflags, 'CYAN' )	

	process = subprocess.Popen( [conf.env.LLVMCONFIG, '--ldflags'], stdout = subprocess.PIPE )
	ldflags = process.communicate()[0].strip().replace( "\n", '' )
	conf.msg( 'llvm-config ldflags:', ldflags, 'CYAN' )

	process = subprocess.Popen( [conf.env.LLVMCONFIG, '--libs'], stdout = subprocess.PIPE )
	llvmlibs = ''.join(process.communicate()[0].strip().replace( "\n", '' ).replace( "-l", '' ))
	conf.env.append_value( 'LLVMLIBS', llvmlibs )
	print( "\n" )


	# construct compiler/linker flags
	cxxflags = cxxflags.split( ' ' )
	ldflags = ldflags.split( ' ' )

	exoflags = [
		'-DEXO_VERSION="'+VERSION+'"',
		'-DQUEX_OPTION_LINE_NUMBER_COUNTING',
		'-DQUEX_OPTION_COLUMN_NUMBER_COUNTING',
		'-std=c++11',
	]
	cxxflags += exoflags

	if conf.options.mode == 'release':
		cxxflags += [ '-O3', '-DQUEX_OPTION_ASSERTS_DISABLED' ]
	elif conf.options.mode == 'debug':
		cxxflags += [ '-O0', '-g', '-DEXO_DEBUG', '-DQUEX_OPTION_ASSERTS_WARNING_MESSAGE_DISABLED' ]
	elif conf.options.mode == 'trace':
		cxxflags += [ '-O0', '-g', '-DEXO_DEBUG', '-DEXO_TRACE', '-DGC_DEBUG', '-DQUEX_OPTION_ASSERTS_WARNING_MESSAGE_DISABLED' ]

	conf.env.append_value( 'CXXFLAGS', cxxflags )
	conf.env.append_value( 'LINKFLAGS',ldflags  )

	conf.msg( 'configuring for', conf.options.mode, 'BLUE' )

	conf.check_cxx( header_name = "fstream" )
	conf.check_cxx( header_name = "iostream" )
	conf.check_cxx( header_name = "cstdio" )
	conf.check_cxx( header_name = "cstdint" )
	conf.check_cxx( header_name = "string" )
	conf.check_cxx( header_name = "cstring" )
	conf.check_cxx( header_name = "cassert" )
	conf.check_cxx( header_name = "cstdlib" )
	conf.check_cxx( header_name = "exception" )
	conf.check_cxx( header_name = "stdexcept" )
	conf.check_cxx( header_name = "csignal" )
	conf.check_cxx( header_name = "sstream" )
	conf.check_cxx( header_name = "execinfo.h" )
	conf.check_cxx( header_name = "unistd.h" )
	conf.check_cxx( header_name = "vector" )
	conf.check_cxx( header_name = "stack" )

	conf.check_cxx( header_name = "llvm/ExecutionEngine/ExecutionEngine.h" )
	conf.check_cxx( header_name = "llvm/IR/DerivedTypes.h" )
	conf.check_cxx( header_name = "llvm/IR/IRBuilder.h" )
	conf.check_cxx( header_name = "llvm/IR/Module.h" )
	conf.check_cxx( header_name = "llvm/PassManager.h" )
	conf.check_cxx( header_name = "llvm/Assembly/PrintModulePass.h" )
	conf.check_cxx( header_name = "llvm/Support/raw_ostream.h" )
	conf.check_cxx( header_name = "llvm/Support/TargetRegistry.h" )
	conf.check_cxx( header_name = "llvm/Support/TargetSelect.h" )

	conf.check_cxx( lib = "gc" )
	conf.check_cxx( header_name = "gc/gc.h" )
	conf.check_cxx( header_name = "gc/gc_cpp.h" )

	print( "\n" )
	conf.msg( 'final cxxflags:', ' '.join( cxxflags ), 'GREEN' )
	conf.msg( 'final ldflags:', ' '.join( ldflags ), 'GREEN' )
	print( "\n" )	

# build
def build( bld ):
	getoptpp = bld.path.ant_glob( SRCDIR + 'getoptpp/**/*.cpp' )
	bld.stlib( target = 'getopt_pp', features = 'cxx', source = getoptpp )

	exo = bld.path.ant_glob( SRCDIR + 'exo/**/*.cpp' )
	libs = [ 'gc' ]
	libs += bld.env.LLVMLIBS[0].split( ' ' )
	bld.program( target = 'exolang', features = 'cxx', source = exo, use = 'getopt_pp', includes = [ TOP, SRCDIR, BINDIR + 'quex' ], lib = libs )

# todo target
def todo( ctx ):
	"Show todos"
	subprocess.call( 'grep -Hnr "//FIXME" '  + SRCDIR, shell=True )

# backup target
def backup( ctx ):
	"Create backup at ~/backup/"
	subprocess.call( 'tar --exclude bin -vcj '  + TOP + ' -f ~/backup/exolang-$(date +%Y-%m-%d-%H-%M).tar.bz2', shell=True )

# (re)create parser
def buildparser( ctx ):
	"Recreate Parser (needs lemon binary)"
	subprocess.call( BINDIR + 'lemon/lemon -l -s ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.y; mv -f ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.c ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.cpp; echo "#define QUEX_TKN_TERMINATION 0b00000000\n#define QUEX_TKN_UNINITIALIZED 0b10000000" >> ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.h', shell=True )

# (re)create lexer
def buildlexer( ctx ):
	"Recreate Lexer (needs quex binary)"
	subprocess.call( 'QUEX_PATH=' + BINDIR + 'quex python ' + BINDIR + 'quex/quex-exe.py -i ' + os.path.abspath( SRCDIR ) + '/exo/ast/lexer/lexer.qx --foreign-token-id-file ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.h --odir ' + os.path.abspath( SRCDIR ) + '/exo/ast/lexer -o lexer', shell=True )

# show lexer/parser tokens
def showtokens( ctx ):
	"Show lexer/parser tokens"
	subprocess.call( 'QUEX_PATH=' + BINDIR + 'quex python ' + BINDIR + 'quex/quex-exe.py -i ' + os.path.abspath( SRCDIR ) + '/exo/ast/lexer/lexer.qx --foreign-token-id-file ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.h --foreign-token-id-file-show', shell=True )