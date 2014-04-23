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

#ifndef INIT_H_
#define INIT_H_

namespace exo
{
	namespace init
	{
		class Init
		{
			public:
				/**
				 * Initializes the GarbageCollector, LLVM and register Signals Handlers.
				 */
				static bool Startup( int logLevel );

				/**
				 * Shuts down our Interpreter. Only the garbage Collection for now.
				 * FIXME: should also handle parser state.
				 */
				static void Shutdown();
		};
	}
}


#endif /* INIT_H_ */
