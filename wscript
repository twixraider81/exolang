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

import os, subprocess, sys, re, platform, pipes, pprint, glob

from waflib import Build, Options, Context, Scripting, Utils, Task, TaskGen
from waflib.ConfigSet import ConfigSet
from waflib.TaskGen import extension
from waflib.Task import Task

APPNAME = 'exolang'
VERSION = '0.0.1'

TOP = os.path.abspath( os.curdir )
BINDIR = TOP + '/bin/'
SRCDIR = 'src/'
BUILDDIR = TOP + '/build'

# load options
def options( opt ):
	opt.load( 'compiler_cxx compiler_c python' )
	opt.add_option( '--mode', action = 'store', default = 'release', help = 'the mode to compile in (release,debug)' )
	opt.add_option( '--llvm', action = 'store', default = 'llvm-config', help = 'path to llvm-config' )
	opt.add_option( '--gc', action = 'store', default = 'enabled', help = 'enable, or disable libgc garbage collector for debug purposes only, it will make us leak!' )
	opt.add_option( '--gdb', type="string", default='', dest='gdb', help = 'load the specified script and run it with gdb' )
	opt.add_option( '--memcheck', type="string", default='', dest='memcheck', help = 'load the specified script and run it with valgrind memcheck' )
	opt.add_option( '--runtests', action='store_true', default='', dest='runtests', help = 'run all test scripts' )

# configure
def configure( conf ):
	conf.load( 'compiler_cxx compiler_c')

	conf.find_program( os.path.basename( conf.options.llvm ), var = 'LLVMCONFIG', mandatory = True, path_list = os.path.dirname( conf.options.llvm ) )
	conf.find_program( 'gdb', var = 'GDB', mandatory = False )
	conf.find_program( 'valgrind', var = 'VALGRIND', mandatory = False )
	conf.find_program( 'ld.gold', var = 'GOLD', mandatory = False )

	# read llvm-config
	process = subprocess.Popen( conf.env.LLVMCONFIG + ['--version'], stdout = subprocess.PIPE )
	llvmversion = process.communicate()[0].strip()
	conf.msg( 'llvm version:', llvmversion, 'GREEN' )
	llvmversionNo = int( llvmversion.replace( ".", '' ) )

	if llvmversionNo < 370:
		conf.fatal( 'atleast llvm 3.7.0 is required' );
	
	process = subprocess.Popen( conf.env.LLVMCONFIG + ['--cppflags'], stdout = subprocess.PIPE )
	cxxflags = process.communicate()[0].strip().replace( "\n", '' )
	conf.msg( 'llvm-config cppflags:', cxxflags, 'CYAN' )	

	process = subprocess.Popen( conf.env.LLVMCONFIG + ['--ldflags'], stdout = subprocess.PIPE )
	ldflags = process.communicate()[0].strip().replace( "\n", '' )
	conf.msg( 'llvm-config ldflags:', ldflags, 'CYAN' )

	process = subprocess.Popen( conf.env.LLVMCONFIG + ['--libs'], stdout = subprocess.PIPE )
	llvmlibs = ''.join(process.communicate()[0].strip().replace( "\n", '' ).replace( "-l", '' ))
	conf.env.append_value( 'LLVMLIBS', llvmlibs )
	exit

	# construct compiler/linker flags
	cxxflags = cxxflags.split( ' ' )
	ldflags = ldflags.split( ' ' )

	if conf.env.GOLD:
		ldflags += [ '-fuse-ld=gold' ]

	exoflags = [
		'-DEXO_VERSION="'+VERSION+'"',
		'-DQUEX_OPTION_LINE_NUMBER_COUNTING',
		'-DQUEX_OPTION_COLUMN_NUMBER_COUNTING',
		'-DQUEX_OPTION_SEND_AFTER_TERMINATION_ADMISSIBLE',
		'-std=c++14'
	]
	cxxflags += exoflags

	if conf.options.mode == 'release':
		cxxflags += [ '-O3', '-DBOOST_ALL_DYN_LINK', '-DQUEX_OPTION_ASSERTS_DISABLED', '-DNDEBUG' ]
	elif conf.options.mode == 'debug':
		cxxflags += [ '-O0', '-g', '-DBOOST_ALL_DYN_LINK', '-DQUEX_OPTION_ASSERTS_WARNING_MESSAGE_DISABLED', '-DEXO_DEBUG' ]


	if conf.options.gc == 'disable':
		cxxflags += [ '-DEXO_GC_DISABLE' ]
		conf.msg( 'garbage collector', 'disabled', 'RED' )
	else:
		conf.msg( 'garbage collector', 'enabled', 'GREEN' )


	conf.env.append_value( 'CXXFLAGS', cxxflags )
	conf.env.append_value( 'LINKFLAGS',ldflags  )



	conf.msg( 'configuring for', conf.options.mode, 'BLUE' )

	# header checks
	conf.check_cxx( header_name = "fstream" )
	conf.check_cxx( header_name = "iostream" )
	conf.check_cxx( header_name = "sstream" )
	conf.check_cxx( header_name = "cstdio" )
	conf.check_cxx( header_name = "cstdint" )
	conf.check_cxx( header_name = "string" )
	conf.check_cxx( header_name = "cstring" )
	conf.check_cxx( header_name = "cctype" )
	conf.check_cxx( header_name = "cassert" )
	conf.check_cxx( header_name = "cstdlib" )
	conf.check_cxx( header_name = "exception" )
	conf.check_cxx( header_name = "stdexcept" )
	conf.check_cxx( header_name = "map" )
	conf.check_cxx( header_name = "vector" )
	conf.check_cxx( header_name = "stack" )
	conf.check_cxx( header_name = "memory" )
	conf.check_cxx( header_name = "iterator" )
	conf.check_cxx( header_name = "algorithm" )

	conf.check_cxx( header_name = "boost/program_options.hpp" )
	conf.check_cxx( header_name = "boost/exception/all.hpp" )
	conf.check_cxx( header_name = "boost/throw_exception.hpp" )
	conf.check_cxx( header_name = "boost/log/core.hpp" )
	conf.check_cxx( header_name = "boost/log/trivial.hpp" )
	conf.check_cxx( header_name = "boost/log/expressions.hpp" )
	conf.check_cxx( header_name = "boost/lexical_cast.hpp" )
	#conf.check_cxx( header_name = "boost/filesystem.hpp" )
	conf.check_cxx( header_name = "boost/algorithm/string.hpp" )
	conf.check_cxx( header_name = "boost/format.hpp" )


	conf.check_cxx( header_name = "csignal" )
	conf.check_cxx( header_name = "execinfo.h" )
	conf.check_cxx( header_name = "unistd.h" )
	conf.check_cxx( header_name = "libunwind.h" )
	conf.check_cxx( header_name = "boost/units/detail/utility.hpp" )


	conf.check_cxx( header_name = "llvm/IR/DerivedTypes.h" )
	conf.check_cxx( header_name = "llvm/ExecutionEngine/ExecutionEngine.h" )
	#conf.check_cxx( header_name = "llvm/ExecutionEngine/MCJIT.h" )
	conf.check_cxx( header_name = "llvm/IR/Module.h" )
	conf.check_cxx( header_name = "llvm/IR/LegacyPassManager.h" )
	conf.check_cxx( header_name = "llvm/IR/Verifier.h" )
	#conf.check_cxx( header_name = "llvm/LinkAllPasses.h" )
	conf.check_cxx( header_name = "llvm/Analysis/Passes.h" )
	conf.check_cxx( header_name = "llvm/IR/IRPrintingPasses.h" )
	conf.check_cxx( header_name = "llvm/Support/raw_ostream.h" )
	conf.check_cxx( header_name = "llvm/Support/Host.h" )
	conf.check_cxx( header_name = "llvm/Support/TargetRegistry.h" )
	conf.check_cxx( header_name = "llvm/Support/TargetSelect.h" )
	conf.check_cxx( header_name = "llvm/Support/FileSystem.h" )
	conf.check_cxx( header_name = "llvm/Transforms/Scalar.h" )
	conf.check_cxx( header_name = "llvm/IR/IRBuilder.h" )

	if conf.options.gc != 'disable':
		conf.check_cxx( header_name = "gc/gc.h" )
		conf.check_cxx( header_name = "gc/gc_cpp.h" )
		conf.check_cxx( lib = "gc" )

	# lib checks
	conf.check_cxx( lib = "boost_program_options" )
	conf.check_cxx( lib = "boost_log" )
	conf.check_cxx( lib = "boost_log_setup" )
	conf.check_cxx( lib = "boost_system" )
	conf.check_cxx( lib = "boost_filesystem" )
	#conf.check_cxx( lib = "boost_locale" )
	conf.check_cxx( lib = "pthread" )
	conf.check_cxx( lib = "ffi" )
	conf.check_cxx( lib = "curses" )
	conf.check_cxx( lib = "dl" )
	conf.check_cxx( lib = "m" )
	conf.check_cxx( lib = "unwind" )
	conf.check_cxx( lib = "unwind-generic" )
	conf.check_cxx( lib = "z" )

# build
def build( bld ):
	exo = bld.path.ant_glob( SRCDIR + 'exo/**/*.cpp' )
	libs = [ 'gc', 'boost_program_options', 'boost_log_setup', 'boost_log', 'boost_system', 'boost_filesystem', 'unwind', 'unwind-generic', 'pthread', 'ffi', 'curses', 'dl', 'm', 'z' ]
	libs += bld.env.LLVMLIBS[0].split( ' ' )
	bld.program( target = 'exolang', features = 'cxx', source = exo, includes = [ TOP, SRCDIR, BINDIR + 'quex' ], lib = libs )

	if Options.options.gdb and bld.env.GDB:
		cmd = bld.env.GDB[0] + ' --eval-command="b main" --eval-command="b exo::jit::JIT::JIT" --args ' + BUILDDIR + '/exolang -s1 -i ' + Options.options.gdb
		subprocess.call( cmd, shell=True )

	if Options.options.memcheck and bld.env.VALGRIND:
		cmd = bld.env.VALGRIND[0] + ' --demangle=yes --error-limit=yes --leak-check=full --show-leak-kinds=definite --track-origins=yes ' + BUILDDIR + '/exolang -s1 -i ' + Options.options.memcheck
		subprocess.call( cmd, shell=True )

	if Options.options.runtests:
		for file in glob.glob( SRCDIR + "tests/*.exo"):
			try:
				subprocess.check_output( [BUILDDIR + '/exolang', '-i', os.path.abspath( file ) ] )
				print "\033[90mPassed: " + file
			except subprocess.CalledProcessError as e:
				print "\033[91mFailed: " + file

# (re)create parser
def buildparser( ctx ):
	"Recreate Parser (needs lemon binary)"
	subprocess.call( BINDIR + 'lemon/lemon -T' + BINDIR + 'lemon/lempar.cpp -l -s ' + os.path.abspath( SRCDIR ) + '/exo/parser/parser.y; echo "#define QUEX_TKN_TERMINATION 0b00000000\n#define QUEX_TKN_UNINITIALIZED 0b10000000" >> ' + os.path.abspath( SRCDIR ) + '/exo/parser/parser.h', shell=True )

# (re)create lexer
def buildlexer( ctx ):
	"Recreate Lexer (needs quex binary)"
	subprocess.call( 'QUEX_PATH=' + BINDIR + 'quex python ' + BINDIR + 'quex/quex-exe.py -i ' + os.path.abspath( SRCDIR ) + '/exo/lexer/lexer.qx --foreign-token-id-file ' + os.path.abspath( SRCDIR ) + '/exo/parser/parser.h --odir ' + os.path.abspath( SRCDIR ) + '/exo/lexer -o lexer', shell=True )
