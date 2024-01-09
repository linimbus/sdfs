cmake_minimum_required (VERSION 2.6)
project(SampleDFS)
include(CTest)

# The version number.
set (SampleDFS_VERSION_MAJOR 1)
set (SampleDFS_VERSION_MINOR 0)

# should we use our own math functions?
#option (USE_MYMATH 
#        "Use provided math implementation" ON) 

# does this system provide the log and exp functions?
include (CheckFunctionExists)
check_function_exists (fuse HAVE_FUSE)


# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
    )

# add the binary tree to the search path for include files
# so that we will find config.h
include_directories("${PROJECT_BINARY_DIR}")

# add the mathfunc library?
#
#if (USE_MYMATH)
#  include_directories ("${PROJECT_SOURCE_DIR}/mathfunc")
#  add_subdirectory (mathfunc)
#  set (EXTRA_LIBS ${EXTRA_LIBS} mathfunc)
#endif (USE_MYMATH)


# add the executable
add_executable(sdfs_client tutorial.c)
TARGET_LINK_LIBRARIES(sdfs_client fuse pthread)

add_executable(sdfs_server tutorial.c)
TARGET_LINK_LIBRARIES(sdfs_server fuse pthread)


# add the install targets
install (TARGETS sdfs_client DESTINATION bin)
install (TARGETS sdfs_server DESTINATION bin)


# does the application run
add_test (TutorialRuns Tutorial 1)

# does the usage message work?
add_test (TutorialUsage Tutorial)
set_tests_properties (TutorialUsage
  PROPERTIES 
  PASS_REGULAR_EXPRESSION "Usage:.*number")

#define a macro to simplify adding tests, then use it
macro (do_test arg result)
  add_test (TutorialComp${arg} Tutorial ${arg})
  set_tests_properties (TutorialComp${arg}
    PROPERTIES PASS_REGULAR_EXPRESSION ${result})
endmacro (do_test)
 
# do a bunch of result based tests
do_test (4 "4 is 2")
do_test (9 "9 is 3")
do_test (5 "5 is 2.236")
do_test (7 "7 is 2.645")
do_test (25 "25 is 5")
do_test (0.0001 "0.0001 is 0.01")
