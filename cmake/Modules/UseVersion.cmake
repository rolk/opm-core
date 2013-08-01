# - Write version information into the source code
#
# Add an unconditional target to the Makefile which checks the current
# SHA of the source directory and write to a header file if and *only*
# if this has changed (thus we avoid unnecessary rebuilds). By having
# this in the Makefile, we get updated version information even though
# we haven't done any reconfiguring.
#
# The time it takes to probe the VCS for this information and write it
# to the miniature file in negligable.
#
# If the build type is Debug, then we only write a static version
# information as it gets tiresome to rebuild the project everytime one
# makes changes to any of the unit tests.

string (TOUPPER "${CMAKE_BUILD_TYPE}" cmake_build_type_upper_)
if (cmake_build_type_upper_ MATCHES DEBUG)
  file (WRITE "${PROJECT_BINARY_DIR}/project-version.h"
	"#define PROJECT_VERSION \"${${project}_LABEL} (debug)\"\n"
	)
else ()
  if (NOT GIT_FOUND)
	find_package (Git)
  endif ()

  # if git is *still* not found means it is not present on the
  # system, so there is "no" way we can update the SHA. notice
  # that this is a slightly different version of the label than
  # above.
  if (NOT GIT_FOUND)
	file (WRITE "${PROJECT_BINARY_DIR}/project-version.h"
	  "#define PROJECT_VERSION \"${${project}_LABEL}\"\n"
	  )
  else ()
	add_custom_target (update-version ALL
	  COMMAND ${CMAKE_COMMAND}
	  -DCMAKE_HOME_DIRECTORY=${CMAKE_HOME_DIRECTORY}
	  -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
	  -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
	  -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
	  -DPROJECT_LABEL=${${project}_LABEL}
	  -P ${PROJECT_SOURCE_DIR}/cmake/Scripts/WriteVerSHA.cmake
	  COMMENT "Updating version information"
	  )

	# the target above gets built every time thanks to the "ALL" modifier,
	# but it must also be done before the main library so it can pick up
	# any changes it does.
	add_dependencies (${${project}_TARGET} update-version)
  endif ()
endif ()

# parse the project name (!) into constituencies, e.g. "opm" and "core"
set (_proj_name_regexp "([^-]+)-(.*)")
string (REGEX REPLACE "${_proj_name_regexp}" "\\1" _proj_suite "${project}")
string (REGEX REPLACE "${_proj_name_regexp}" "\\2" _proj_module "${project}")

# if we have a template file, we use that as an indicator that we should
# generate a version information file in the build direcory
set (_ver_templ "${PROJECT_SOURCE_DIR}/${_proj_suite}/${_proj_module}/version.h.in")
set (_version_h "${PROJECT_BINARY_DIR}/${_proj_suite}/${_proj_module}/version.h")
if (EXISTS "${_ver_templ}")
  # use uppercase versions of the name in header
  string (TOUPPER "${project}" _proj_upper)
  string (REPLACE "-" "_" _proj_upper "${_proj_upper}")

  # start with the predefined content...
  configure_file ("${_ver_templ}" "${_version_h}" COPYONLY)

  # ...and then add the version bits defined here
  file (APPEND "${_version_h}"
	"\n/* current API version (for use with DUNE_VERSION_xxx): */
#define ${_proj_upper}_MAJOR ${${project}_VERSION_MAJOR}
#define ${_proj_upper}_MINOR ${${project}_VERSION_MINOR}
#define ${_proj_upper}_REVISION 0\n"
  )
endif ()
