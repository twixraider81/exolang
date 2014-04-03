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
	opt.add_option( '--mode', action = 'store', default = 'debug', help = 'the mode to compile in (debug or release)' )

# configure
def configure( conf ):
	conf.env.CXX = 'clang++'
	conf.env.CC = 'clang'

	conf.load( 'compiler_cxx compiler_c' )

#	conf.find_program( 'llvm-config', var = 'LLVM', mandatory = True )#
#	process = subprocess.Popen([conf.env.LLVM, '--cppflags'], stdout=subprocess.PIPE)
#	flags = process.communicate()[0]
#	conf.msg( 'llvm-config flags:', flags, 'CYAN' )
#	
#	flags = flags.split(' ');
	flags = [ '-DVERSION='+VERSION, '-DQUEX_OPTION_ASSERTS_WARNING_MESSAGE_DISABLED', '-DQUEX_OPTION_LINE_NUMBER_COUNTING', '-DQUEX_OPTION_COLUMN_NUMBER_COUNTING', '-D__STDC_CONSTANT_MACROS', '-D__STDC_LIMIT_MACROS' ]

	if conf.options.mode == 'release':
		flags += [ '-O2' ]
	elif conf.options.mode == 'debug':
		flags += [ '-O0', '-g', '-DDEBUG' ]

	conf.env.append_value( 'CXXFLAGS', flags )

# build
def build( bld ):
	sources = bld.path.ant_glob( SRCDIR + '**/*.cpp' )
	bld.program( features='cxx', target='exolang', source=sources, includes=[ TOP, SRCDIR, BINDIR + 'quex' ] )

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
	subprocess.call( BINDIR + 'lemon/lemon -l -s ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.y; mv ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.c ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.cpp; echo "#define QUEX_TKN_TERMINATION 0b00000000\n#define QUEX_TKN_UNINITIALIZED 0b10000000" >> ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.h', shell=True )

# (re)create lexer
def buildlexer( ctx ):
	"Recreate Lexer (needs quex binary)"
	subprocess.call( 'QUEX_PATH=' + BINDIR + 'quex python ' + BINDIR + 'quex/quex-exe.py -i ' + os.path.abspath( SRCDIR ) + '/exo/ast/lexer/lexer.qx --foreign-token-id-file ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.h --odir ' + os.path.abspath( SRCDIR ) + '/exo/ast/lexer -o lexer', shell=True )

# show lexer/parser tokens
def showtokens( ctx ):
	"Show lexer/parser tokens"
	subprocess.call( 'QUEX_PATH=' + BINDIR + 'quex python ' + BINDIR + 'quex/quex-exe.py -i ' + os.path.abspath( SRCDIR ) + '/exo/ast/lexer/lexer.qx --foreign-token-id-file ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.h --foreign-token-id-file-show', shell=True )