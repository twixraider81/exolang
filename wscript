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

#class quex(Task):
#	shell = True
#	run_str = 'QUEX_PATH=${QUEX_PATH} ${PYTHON} ${QUEX} -i ${SRC} --odir ' + os.path.abspath( SRCDIR ) + 'exo/ast/lexer -o lexer'
#	color = 'CYAN'
#	after = ['lemon']
#
#class lemon(Task):
#	shell = True
#	run_str = '${LEMON} -l -s ${SRC}; mv ' + os.path.abspath( SRCDIR ) + 'exo/ast/parser/parser.c ' + os.path.abspath( SRCDIR ) + 'exo/ast/parser/parser.cpp'
#	color = 'CYAN'
#
#@extension('qx')
#def process_quex( self, node ):
#	task = self.create_task( 'quex', src = node, tgt = node.change_ext( '.cpp' ) )
#	self.source.extend( task.outputs )
#
#@extension('y')
#def process_lemon( self, node ):
#	task = self.create_task( 'lemon', src = node, tgt = node.change_ext( '.cpp' ) )
#	self.source.extend( task.outputs )

# configure
def configure( conf ):
	conf.env.CXX = 'clang++'
	conf.env.CC = 'clang'

	conf.load( 'compiler_cxx compiler_c' )
	
	if conf.options.mode == 'release':
		conf.env.append_value( 'CXXFLAGS', ['-O2', '-std=c++11', '-DVERSION='+VERSION,'-DQUEX_OPTION_ASSERTS_WARNING_MESSAGE_DISABLED'] )
	elif conf.options.mode == 'debug':
		conf.env.append_value( 'CXXFLAGS', ['-O0', '-g', '-std=c++11', '-DVERSION='+VERSION,'-DQUEX_OPTION_ASSERTS_WARNING_MESSAGE_DISABLED','-DDEBUG'] )

#	conf.load( 'python' )
#	conf.check_python_version((2,7,0))
#	conf.find_program( 'lemon', var = 'LEMON', path_list=, mandatory = False )
#	conf.find_program( 'quex-exe.py', var = 'QUEX', path_list=BINDIR + 'quex', mandatory = False )
#	conf.env['QUEX_PATH'] = os.path.dirname( conf.env.QUEX )

# build
def build( bld ):
	sources = bld.path.ant_glob( SRCDIR + '**/*.cpp' )
#	sources += bld.path.ant_glob( SRCDIR + '**/*.y' )
#	sources += bld.path.ant_glob( SRCDIR + '**/*.qx' )
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
	subprocess.call( BINDIR + 'lemon/lemon -l -s ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.y; mv ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.c ' + os.path.abspath( SRCDIR ) + '/exo/ast/parser/parser.cpp', shell=True )

# (re)create lexer
def buildlexer( ctx ):
	"Recreate Lexer (needs quex binary)"
	subprocess.call( 'QUEX_PATH=' + BINDIR + '/quex python ' + BINDIR + '/quex/quex-exe.py -i ' + os.path.abspath( SRCDIR ) + '/exo/ast/lexer/lexer.qx --odir ' + os.path.abspath( SRCDIR ) + '/exo/ast/lexer -o lexer', shell=True )