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
from waflib.Build import BuildContext, CleanContext, InstallContext, UninstallContext
from waflib.ConfigSet import ConfigSet

APPNAME = 'exolang'
VERSION = '0.0.1'

TOP = os.path.abspath( os.curdir )
PLATFORM = re.findall( '^[a-zA-Z]+', platform.uname()[0] )[0]
BINDIR = TOP + '/bin/'
SRCDIR = 'src/'

# load options
def options( opt ):
	opt.load( 'compiler_cxx' )
	opt.add_option( '--mode', action = 'store', default = 'debug', help = 'the mode to compile in (debug or release)' )

# configure
def configure( conf ):
	conf.env.CXX = 'clang++'
	conf.env.CC = 'clang'

	if conf.options.mode == 'release':
		conf.env.append_value( 'DFLAGS', ['-O2'] )
	elif conf.options.mode == 'debug':
		conf.env.append_value( 'DFLAGS', ['-O0', '-g'] )

	conf.find_program( 'lemon', var = 'LEMON', path_list=BINDIR + 'lemon', mandatory = True )
	conf.find_program( 'quex', var = 'QUEX', path_list=BINDIR + 'quex', mandatory = True )

	conf.load( 'compiler_cxx' )

# build
def build( bld ):
	sources = bld.path.ant_glob( SRCDIR + '**/*.cpp' )
	bld.program( features="c cxx", target='exolang', source=sources, includes=[SRCDIR,'bin/quex'] )

# todo target
def todo( ctx ):
	"Show todos"
	subprocess.call( 'grep -Hnr "//FIXME" '  + SRCDIR, shell=True )

# backup target
def backup( ctx ):
	"Create backup at ~/backup/"
	subprocess.call( 'tar --exclude bin -vcj '  + TOP + ' -f ~/backup/exolang-$(date +%Y-%m-%d-%H-%M).tar.bz2', shell=True )