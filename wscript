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
	conf.env.CXX = 'clang++'
	conf.env.CC = 'clang'

	conf.load( 'compiler_cxx compiler_c asm' )

	conf.find_program( 'llvm-config', var = 'LLVMCONFIG', mandatory = True )#
	process = subprocess.Popen([conf.env.LLVMCONFIG, '--ldflags'], stdout=subprocess.PIPE)
	ldflags = process.communicate()[0]
	conf.msg( 'llvm-config ld flags:', ldflags, 'CYAN' )

	flags = [
		'-DEXO_VERSION="'+VERSION+'"',
		'-DQUEX_OPTION_ASSERTS_WARNING_MESSAGE_DISABLED',
		'-DQUEX_OPTION_LINE_NUMBER_COUNTING',
		'-DQUEX_OPTION_COLUMN_NUMBER_COUNTING',
		'-std=c++11',
		'-D__STDC_CONSTANT_MACROS',
		'-D__STDC_FORMAT_MACROS',
		'-D__STDC_LIMIT_MACROS'
	]

	if conf.options.mode == 'release':
		flags += [ '-O2', '-fvectorize', '-funroll-loops' ]
	elif conf.options.mode == 'debug':
		flags += [ '-O0', '-g', '-fno-limit-debug-info', '-DEXO_DEBUG' ]
	elif conf.options.mode == 'trace':
		flags += [ '-O0', '-g', '-fno-limit-debug-info', '-DEXO_DEBUG', '-DEXO_TRACE', '-DGC_DEBUG' ]

	conf.env.append_value( 'CXXFLAGS', flags )
	conf.env.append_value( 'LINKFLAGS', ldflags.strip().split( ' ' ) )

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

	conf.check_cxx( header_name = "llvm/IR/Value.h" )

	conf.check_cxx( lib = "gc" )
	conf.check_cxx( header_name = "gc/gc.h" )
	conf.check_cxx( header_name = "gc/gc_cpp.h" )

# build
def build( bld ):
	getoptpp = bld.path.ant_glob( SRCDIR + 'getoptpp/**/*.cpp' )
	bld.stlib( target = 'getopt_pp', features = 'cxx', source = getoptpp )

	exo = bld.path.ant_glob( SRCDIR + 'exo/**/*.cpp' )
	bld.program( target = 'exolang', features = 'cxx', source = exo, use = 'getopt_pp', includes = [ TOP, SRCDIR, BINDIR + 'quex' ], lib = [ 'gc' ] )

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