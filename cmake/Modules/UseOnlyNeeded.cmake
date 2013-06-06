# - Use only needed imports from libraries
#
# Add the -Wl,--as-needed flag to the default linker flags on Linux
# in order to get only the minimal set of dependencies.

function (prepend var_name)
  if (${var_name})
	set (${var_name} "${ARGN} ${${var_name}}" PARENT_SCOPE)
  else (${var_name})
	set (${var_name} "${ARGN}")
  endif (${var_name})
endfunction (prepend var_name)

# only ELF shared objects can be underlinked, and only GNU will accept
# these parameters; otherwise just leave it to the defaults
if ((CMAKE_CXX_PLATFORM_ID STREQUAL "Linux") AND CMAKE_COMPILER_IS_GNUCC)
  # these are the modules whose probes will turn up incompatible
  # flags on some systems
  set (_maybe_underlinked
	SuiteSparse
	)
  # check if any modules actually reported problems (by setting an
  # appropriate linker flag)
  set (_underlinked FALSE)
  foreach (_module IN LISTS _maybe_underlinked)
	if ("${${_module}_LINKER_FLAGS}" MATCHES "-Wl,--no-as-needed")
	  set (_underlinked TRUE)
	endif ("${${_module}_LINKER_FLAGS}" MATCHES "-Wl,--no-as-needed")
  endforeach (_module)
  # if we didn't have any problems, then go ahead and add
  if (NOT _underlinked)
	# some versions of the linker plugin used in optimization have problems
	# if we don't link all the libraries; this would prohibit the use of
	# --as-needed
	set (_as_needed_flag "-Wl,--as-needed")
	string (TOUPPER "${CMAKE_BUILD_TYPE}" _build_type)
	set (_curr_opts "${CMAKE_C_FLAGS_${_build_type}}")
	# compile this little program which seems to recreate the problem
	include (CMakePushCheckState)
	include (CheckCSourceCompiles)
	cmake_push_check_state ()
	set (CMAKE_REQUIRED_FLAGS "${_curr_opts} ${_as_needed_flag}")
	set (CMAKE_REQUIRED_LIBRARIES "m")
	check_c_source_compiles ("
#include <math.h>
int main (int argc, char** argp) {
  sqrt (argc);
  return 0;
}
	" HAVE_LINKER_PLUGIN_WITH_ASNEEDED)
	cmake_pop_check_state ()
	# if we weren't able to do so, then disable the linker plugin
	if (HAVE_LINKER_PLUGIN_WITH_ASNEEDED)
	  # add the set of final linker options determined
	  prepend (CMAKE_EXE_LINKER_FLAGS ${_as_needed_flag})
	  prepend (CMAKE_MODULE_LINKER_FLAGS ${_as_needed_flag})
	  prepend (CMAKE_SHARED_LINKER_FLAGS ${_as_needed_flag})
	endif (HAVE_LINKER_PLUGIN_WITH_ASNEEDED)
  endif (NOT _underlinked)
endif ((CMAKE_CXX_PLATFORM_ID STREQUAL "Linux")  AND CMAKE_COMPILER_IS_GNUCC)
