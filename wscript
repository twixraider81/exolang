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

import os, subprocess, re, glob

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
	opt.add_option( '--gdbserver', type="string", default='', dest='gdbserver', help = 'load the specified script and run it with gdb server' )
	opt.add_option( '--lldb', type="string", default='', dest='lldb', help = 'load the specified script and run it with lldb' )
	opt.add_option( '--r2', type="string", default='', dest='r2', help = 'load the specified script and run it with radare2' )
	opt.add_option( '--memcheck', type="string", default='', dest='memcheck', help = 'load the specified script and run it with valgrind memcheck' )
	opt.add_option( '--callgrind', type="string", default='', dest='callgrind', help = 'load the specified script and run it with valgrind callgrind' )
	opt.add_option( '--runtests', action='store_true', default='', dest='runtests', help = 'run all test scripts' )

# configure
def configure( conf ):
	conf.msg( 'Configuring for', conf.options.mode, 'BLUE' )

	if conf.options.gc == 'disable':
		conf.define( 'EXO_GC_DISABLE', 1 )
		conf.msg( 'Garbage collector', 'disabled', 'RED' )
	else:
		conf.msg( 'Garbage collector', 'enabled', 'GREEN' )

	conf.load( 'compiler_cxx compiler_c')

	conf.find_program( os.path.basename( conf.options.llvm ), var = 'LLVMCONFIG', mandatory = True, path_list = os.path.dirname( conf.options.llvm ) )
	conf.find_program( 'gdb', var = 'GDB', mandatory = False )
	conf.find_program( 'lldb', var = 'LLDB', mandatory = False, path_list = os.path.dirname( conf.options.llvm ) )
	conf.find_program( 'gdbserver', var = 'GDBSERVER', mandatory = False )
	conf.find_program( 'r2', var = 'R2', mandatory = False )
	conf.find_program( 'valgrind', var = 'VALGRIND', mandatory = False )
	conf.find_program( 'ld.gold', var = 'GOLD', mandatory = False )

	# read llvm-config
	process = subprocess.Popen( conf.env.LLVMCONFIG + ['--version'], stdout = subprocess.PIPE )
	llvmversion = process.communicate()[0].strip()
	conf.msg( 'Checking for LLVM version', llvmversion, 'GREEN' )
	llvmversionNo = int( llvmversion.replace( ".", '' ) )
	if llvmversionNo < 390:
		conf.fatal( 'atleast LLVM 3.9.0 is required' );

	process = subprocess.Popen( conf.env.LLVMCONFIG + ['--has-rtti'], stdout = subprocess.PIPE )
	rtti = process.communicate()[0].strip()
	conf.msg( 'Checking for LLVM RTTI', rtti, 'CYAN' )
	if rtti == "NO":
		conf.fatal( 'LLVM build with RTTI is required' );

	process = subprocess.Popen( conf.env.LLVMCONFIG + ['--cppflags'], stdout = subprocess.PIPE )
	cppflags = process.communicate()[0].strip().replace( "\n", '' )
	for define in re.findall( "-D([\w]+)", cppflags ):
		#conf.define( define, 1 )
		cppflags = cppflags.replace( "-D" + define, "" )
	#conf.msg( 'LLVM cppflags:', cppflags, 'CYAN' )

	process = subprocess.Popen( conf.env.LLVMCONFIG + ['--ldflags'], stdout = subprocess.PIPE )
	ldflags = process.communicate()[0].strip().replace( "\n", '' )
	#conf.msg( 'LLVM ldflags:', ldflags, 'CYAN' )

	process = subprocess.Popen( conf.env.LLVMCONFIG + ['--libs'], stdout = subprocess.PIPE )
	llvmlibs = ''.join(process.communicate()[0].strip().replace( "\n", '' ).replace( "-l", '' ))
	conf.env.append_value( 'LLVMLIBS', llvmlibs )

	# construct compiler/linker flags
	cppflags = cppflags.split( ' ' )
	ldflags = ldflags.split( ' ' )

	if conf.env.GOLD:
		ldflags += [ '-fuse-ld=gold' ]

	cppflags += [ '-std=c++14' ]
	conf.define( 'EXO_VERSION', VERSION )
	#conf.define( 'QUEX_OPTION_LINE_NUMBER_COUNTING', 1 )
	#conf.define( 'QUEX_OPTION_COLUMN_NUMBER_COUNTING', 1 )
	conf.define( 'QUEX_OPTION_SEND_AFTER_TERMINATION_ADMISSIBLE', 1 )
	conf.define( 'BOOST_ALL_DYN_LINK', 1 )

	if conf.options.mode == 'release':
		cppflags += [ '-O3' ]
		conf.define( 'QUEX_OPTION_ASSERTS_DISABLED', 1 )
		conf.define( 'NDEBUG', 1 )
	elif conf.options.mode == 'debug':
		cppflags += [ '-O0', '-g' ]
		conf.define( 'QUEX_OPTION_ASSERTS_WARNING_MESSAGE_DISABLED', 1 )
		conf.define( 'EXO_DEBUG', 1 )

	conf.env.append_value( 'CXXFLAGS', cppflags )
	conf.env.append_value( 'LINKFLAGS', ldflags )

	# detect system link paths (we could also use llvm for this)
	process = subprocess.Popen( ['ld', '--verbose'], stdout = subprocess.PIPE )
	result = process.communicate()[0].strip()
	result = re.findall( "SEARCH_DIR\(\"=([\w/-]+)", result )
	if result:
		result = '{\"' + '\",\"'.join( result ) + '\"}'
		conf.define( 'EXO_LIBRARY_PATHS', result, False )
	else:
		conf.define( 'EXO_LIBRARY_PATHS', '{}', False )
	# only stdlib for now
	conf.define( 'EXO_INCLUDE_PATHS', '{\"' + os.path.abspath( SRCDIR ) + '/stdlib\"}', False )

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
	conf.check_cxx( header_name = "unordered_map" )
	conf.check_cxx( header_name = "memory" )
	conf.check_cxx( header_name = "iterator" )
	conf.check_cxx( header_name = "algorithm" )

	conf.check_cxx( header_name = "boost/program_options.hpp" )
	conf.check_cxx( header_name = "boost/exception/all.hpp" )
	conf.check_cxx( header_name = "boost/throw_exception.hpp" )
	conf.check_cxx( header_name = "boost/log/core.hpp" )
	conf.check_cxx( header_name = "boost/log/trivial.hpp" )
	#conf.check_cxx( header_name = "boost/log/expressions.hpp" )
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
	conf.check_cxx( header_name = "llvm/ExecutionEngine/SectionMemoryManager.h" )
	conf.check_cxx( header_name = "llvm/ExecutionEngine/JITEventListener.h" )
	conf.check_cxx( header_name = "llvm/ExecutionEngine/GenericValue.h" )
	conf.check_cxx( header_name = "llvm/IR/Module.h" )
	conf.check_cxx( header_name = "llvm/IR/LegacyPassManager.h" )
	conf.check_cxx( header_name = "llvm/IR/Verifier.h" )
	conf.check_cxx( header_name = "llvm/Analysis/TargetLibraryInfo.h" )
	conf.check_cxx( header_name = "llvm/Analysis/TargetTransformInfo.h" )
	conf.check_cxx( header_name = "llvm/MC/SubtargetFeature.h" )
	conf.check_cxx( header_name = "llvm/Support/raw_ostream.h" )
	conf.check_cxx( header_name = "llvm/Support/Host.h" )
	conf.check_cxx( header_name = "llvm/Support/TargetRegistry.h" )
	conf.check_cxx( header_name = "llvm/Support/TargetSelect.h" )
	conf.check_cxx( header_name = "llvm/Support/FileSystem.h" )
	conf.check_cxx( header_name = "llvm/Transforms/Scalar.h" )
	conf.check_cxx( header_name = "llvm/Transforms/IPO/PassManagerBuilder.h" )
	conf.check_cxx( header_name = "llvm/IR/IRBuilder.h" )
	conf.check_cxx( header_name = "llvm/Bitcode/ReaderWriter.h" )
	conf.check_cxx( header_name = "llvm/Support/DynamicLibrary.h" )


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

	conf.write_config_header('config.h')

# build
def build( bld ):
	exo = bld.path.ant_glob( SRCDIR + 'exo/**/*.cpp' )
	libs = [ 'gc', 'boost_program_options', 'boost_log_setup', 'boost_log', 'boost_system', 'boost_filesystem', 'unwind', 'unwind-generic', 'pthread', 'ffi', 'curses', 'dl', 'm', 'z' ]
	libs += bld.env.LLVMLIBS[0].split( ' ' )
	bld.program( target = 'exolang', features = 'cxx', source = exo, includes = [ TOP, SRCDIR, BINDIR + 'quex' ], lib = libs )

	if Options.options.gdb and bld.env.GDB:
		cmd = bld.env.GDB[0] + ' --eval-command="b main" --eval-command="b exo::jit::JIT::JIT" --eval-command="r" --args ' + BUILDDIR + '/exolang -l1 -i ' + Options.options.gdb
		subprocess.call( cmd, shell=True )

	if Options.options.gdbserver and bld.env.GDBSERVER:
		cmd = bld.env.GDBSERVER[0] + ' --debug --remote-debug :2159 ' + BUILDDIR + '/exolang -l1 -i ' + Options.options.gdbserver
		subprocess.call( cmd, shell=True )

	if Options.options.lldb and bld.env.LLDB:
		cmd = bld.env.LLDB[0] + ' -- ' + BUILDDIR + '/exolang -l1 -i ' + Options.options.lldb
		subprocess.call( cmd, shell=True )

	if Options.options.r2 and bld.env.R2:
		cmd = bld.env.R2[0] + ' -e http.bind=0.0.0.0 -e http.dirlist=1 -c="h&" ' + BUILDDIR + '/exolang -l1 -i ' + Options.options.r2
		subprocess.call( cmd, shell=True )

	if Options.options.memcheck and bld.env.VALGRIND:
		cmd = bld.env.VALGRIND[0] + ' --tool=memcheck --demangle=yes --error-limit=yes --leak-check=full --show-leak-kinds=definite,possible --track-origins=yes ' + BUILDDIR + '/exolang -i ' + Options.options.memcheck
		subprocess.call( cmd, shell=True )

	if Options.options.callgrind and bld.env.VALGRIND:
		cmd = bld.env.VALGRIND[0] + ' --tool=callgrind --demangle=yes --error-limit=yes --callgrind-out-file=' + Options.options.callgrind + '.out ' + BUILDDIR + '/exolang -i ' + Options.options.callgrind
		subprocess.call( cmd, shell=True )

	if Options.options.runtests:
		for file in sorted( glob.glob( SRCDIR + "tests/*.exo") ):
			try:
				subprocess.check_output( [BUILDDIR + '/exolang', '-i', os.path.abspath( file ) ] )
				print "\033[90mPassed: " + file
			except subprocess.CalledProcessError as e:
				print "\033[91mFailed: " + file

# (re)create parser
def buildparser( ctx ):
	"Recreate parser (needs lemon binary)"
	subprocess.call( BINDIR + 'lemon/lemon -T' + BINDIR + 'lemon/lempar.cpp -l -s ' + os.path.abspath( SRCDIR ) + '/exo/parser/parser.y; echo "#define QUEX_TKN_TERMINATION 0b00000000\n#define QUEX_TKN_UNINITIALIZED 0b10000000" >> ' + os.path.abspath( SRCDIR ) + '/exo/parser/parser.h', shell=True )

# (re)create lexer
def buildlexer( ctx ):
	"Recreate lexer (needs quex binary)"
	subprocess.call( 'QUEX_PATH=' + BINDIR + 'quex python ' + BINDIR + 'quex/quex-exe.py -i ' + os.path.abspath( SRCDIR ) + '/exo/lexer/lexer.qx --foreign-token-id-file ' + os.path.abspath( SRCDIR ) + '/exo/parser/parser.h --odir ' + os.path.abspath( SRCDIR ) + '/exo/lexer -o lexer', shell=True )