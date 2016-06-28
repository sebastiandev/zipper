###############################################################################
#
# Description       : CMake build script for zipper
# Original author(s): Frank Bergmann <fbergman@caltech.edu>
# Organization      : California Institute of Technology
#
###############################################################################

cmake_minimum_required(VERSION 2.6)
project(zipper)

include (CMakeTestCCompiler)
include (CheckCSourceCompiles)
include (CheckCXXSourceCompiles)
include (CheckStructHasMember)
include (CheckLibraryExists)
include (CheckFunctionExists)
include (CheckCCompilerFlag)
include (CheckCSourceRuns)
include (CheckSymbolExists)
include (CheckTypeSize)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt")

    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" VersionString NEWLINE_CONSUME)
    string(STRIP "${VersionString}" VersionString)
    set(LIBSBML_DOTTED_VERSION ${VersionString})
    string(REPLACE "." ";" VersionString "${VersionString}" )
    string(REPLACE "-" ";" VersionString "${VersionString}" )
    list(LENGTH VersionString versionLength)
    list(GET VersionString 0 ZIPPER_VERSION_MAJOR )
    list(GET VersionString 1 ZIPPER_VERSION_MINOR )
    list(GET VersionString 2 ZIPPER_VERSION_PATCH )

    if(${versionLength} GREATER 3)
        list(GET VersionString 3 ZIPPER_VERSION_RELEASE )
    endif()

endif()

# version number needs to be calculated correctly
MATH(EXPR ZIPPER_VERSION_NUMERIC "${ZIPPER_VERSION_MAJOR} * 10000 + ${ZIPPER_VERSION_MINOR} * 100 + ${ZIPPER_VERSION_PATCH}" )
set(PACKAGE_VERSION "${ZIPPER_VERSION_MAJOR}.${ZIPPER_VERSION_MINOR}.${ZIPPER_VERSION_PATCH}${ZIPPER_VERSION_RELEASE}")
set(PACKAGE_NAME "Zipper")

# add make dist and make check target as they are already familiar for
# everyone using the gnumake build
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
add_custom_target(check COMMAND ${CMAKE_MAKE_PROGRAM} test)

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C++ wrapper around minizip compression library.")
set(CPACK_PACKAGE_NAME "${PACKAGE_NAME}")
set(CPACK_PACKAGE_VENDOR "The SBML Team")
set(CPACK_PACKAGE_CONTACT "SBML Team <sbml-team@caltech.edu>")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
set(CPACK_PACKAGE_VERSION_MAJOR "${ZIPPER_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${ZIPPER_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${ZIPPER_VERSION_PATCH}")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Libraries/Development")
set(CPACK_DEBIAN_PACKAGE_SECTION "Libraries")

set(CPACK_SOURCE_IGNORE_FILES "${CMAKE_CURRENT_BINARY_DIR};/.svn/;/.libs/;/.deps/;/.bzr/;.*.o$;.*.lo$;.*.la$;/.git/;${CPACK_SOURCE_IGNORE_FILES};/.DS_Store;/.svnignore;blib;libsbml-dist")

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "i386")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
endif()

include(CPack)

# Add an option to compile with all warnings shown
option(WITH_WALL     "Compile with -Wall, so that the compiler will display all warnings." OFF)
mark_as_advanced(WITH_WALL)

if(WITH_WALL)
    if(MSVC OR USING_INTEL)
        add_definitions(/W4)
    else()
        add_definitions(-Wall)
    endif()
endif()

if (UNIX)
  set(ZIPPER_LIBRARY Zipper)
else()
  set(ZIPPER_LIBRARY libZipper)
endif()
# Set build type default.
set(CMAKE_BUILD_TYPE "Release" CACHE STRING
  "Choose the type of build to perform. The options are: None (CMAKE_CXX_FLAGS
or CMAKE_C_FLAGS are used), Debug, Release, RelWithDebInfo, MinSizeRel.")
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
    "Choose the type of build to perform. The options are: None (CMAKE_CXX_FLAGS
or CMAKE_C_FLAGS are used), Debug, Release, RelWithDebInfo, MinSizeRel." FORCE)
endif()


find_library(LIBZ_LIBRARY
    NAMES zdll.lib z zlib.lib
    PATHS /usr/lib /usr/local/lib
          ${CMAKE_OSX_SYSROOT}/usr/lib
          ${LIBSBML_DEPENDENCY_DIR}/lib
    DOC "The file name of the zip compression library."
)


if (NOT LIBZ_INCLUDE_DIR)
  find_path(LIBZ_INCLUDE_DIR
      NAMES zlib.h zlib/zlib.h
      PATHS ${CMAKE_OSX_SYSROOT}/usr/include
            /usr/include /usr/local/include
            ${LIBSBML_DEPENDENCY_DIR}/include
            NO_DEFAULT_PATH
      DOC "The directory containing the zlib include files."
            )
endif()
    
if (NOT LIBZ_INCLUDE_DIR)
    find_path(LIBZ_INCLUDE_DIR
    NAMES zlib.h zlib/zlib.h
    PATHS ${CMAKE_OSX_SYSROOT}/usr/include
          /usr/include /usr/local/include
          ${LIBSBML_DEPENDENCY_DIR}/include
    DOC "The directory containing the zlib include files."
          )
endif()
set(USE_ZLIB ON)
add_definitions( -DUSE_ZLIB )
list(APPEND SWIG_EXTRA_ARGS -DUSE_ZLIB)  

# make sure that we have a valid zip library
check_library_exists("${LIBZ_LIBRARY}" "gzopen" "" LIBZ_FOUND_SYMBOL)
if(NOT LIBZ_FOUND_SYMBOL)
    # this is odd, but on windows this check always fails! must be a
    # bug in the current cmake version so for now only issue this
    # warning on linux
    if(UNIX)
        message(WARNING
"The chosen zlib library does not appear to be valid because it is
missing certain required symbols. Please check that ${LIBZ_LIBRARY} is
the correct zlib library. For details about the error, please see
${LIBSBML_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log")
    endif()
endif()

if(NOT EXISTS "${LIBZ_INCLUDE_DIR}/zlib.h")
    message(FATAL_ERROR
"The include directory specified for zlib does not appear to be
valid. It should contain the file zlib.h, but it does not.")
endif()

enable_testing()

if (UNIX)
      # setup valgrind
      set(CMAKE_MEMORYCHECK_COMMAND valgrind)
      set(CMAKE_MEMORYCHECK_COMMAND_OPTIONS
         "--error-exitcode=1 --trace-children=yes --leak-check=full --show-reachable=yes --leak-resolution=high --track-origins=yes --error-limit=no ")

      set(MEMCHECK_COMMAND
         "${CMAKE_MEMORYCHECK_COMMAND} ${CMAKE_MEMORYCHECK_COMMAND_OPTIONS}")
      separate_arguments(MEMCHECK_COMMAND)
endif()

include(CTest)

set(USING_INTEL)
if (WIN32 AND CMAKE_C_COMPILER AND ${CMAKE_C_COMPILER} MATCHES ".*icl.*$")
  message(STATUS "Detected Intel Compiler")
  set(USING_INTEL TRUE)
endif ()

    
if(MSVC OR USING_INTEL)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
    set(BUILD_DEFINITIONS "${BUILD_DEFINITIONS} -D_CRT_SECURE_NO_WARNINGS")
    option(WITH_STATIC_RUNTIME "Compile using the static MSVC Runtime." OFF)
    if(WITH_STATIC_RUNTIME)
        foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)

            if(${flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif(${flag_var} MATCHES "/MD")
            
        endforeach(flag_var)
    add_definitions( -D_MT)
    endif(WITH_STATIC_RUNTIME)


  # CMake no longer creates PDB files for static libraries after 2.8.11
  # so we store debug information in the object files instead
  if (${CMAKE_VERSION} VERSION_GREATER "2.8.11")
          foreach(flag_var
              CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
              CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
              CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
              CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
  
              if(${flag_var} MATCHES "/Zi")
          STRING(REPLACE "/Zi"  "/Z7" "${flag_var}" "${${flag_var}}")
              endif(${flag_var} MATCHES "/Zi")
  
          endforeach(flag_var)      
  endif()
endif()

include_directories(BEFORE ${LIBZ_INCLUDE_DIR})
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/zipper)
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/minizip)


file(GLOB MINIZIP_SOURCES 
  ${CMAKE_CURRENT_SOURCE_DIR}/minizip/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/minizip/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/minizip/aes/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/minizip/aes/*.h
)
list(REMOVE_ITEM MINIZIP_SOURCES
${CMAKE_CURRENT_SOURCE_DIR}/minizip/minizip.c
${CMAKE_CURRENT_SOURCE_DIR}/minizip/miniunz.c
)

file(GLOB ZIPPER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/zipper/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/zipper/*.h
)

file(GLOB TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/test/*.hpp
)

add_library (${ZIPPER_LIBRARY} SHARED ${ZIPPER_SOURCES} ${MINIZIP_SOURCES} )

if (ZIPPER_SHARED_VERSION)
  SET_TARGET_PROPERTIES(${ZIPPER_LIBRARY} PROPERTIES
                        SOVERSION ${ZIPPER_VERSION_MAJOR}
                        VERSION ${ZIPPER_VERSION_MAJOR}.${ZIPPER_VERSION_MINOR}.${ZIPPER_VERSION_PATCH})
endif()

target_link_libraries(${ZIPPER_LIBRARY} ${LIBZ_LIBRARY} ${EXTRA_LIBS})

INSTALL(TARGETS ${ZIPPER_LIBRARY}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

add_library (${ZIPPER_LIBRARY}-static STATIC ${ZIPPER_SOURCES} ${MINIZIP_SOURCES})

target_link_libraries(${ZIPPER_LIBRARY}-static ${LIBZ_LIBRARY} ${EXTRA_LIBS})

INSTALL(TARGETS ${ZIPPER_LIBRARY}-static
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

add_executable(${ZIPPER_LIBRARY}-test ${TEST_SOURCES})
target_link_libraries(${ZIPPER_LIBRARY}-test  ${ZIPPER_LIBRARY}-static ${LIBZ_LIBRARY} ${EXTRA_LIBS})

add_test(
  NAME test_zipper_run
  COMMAND $<TARGET_FILE:${ZIPPER_LIBRARY}-test>
)

set_tests_properties(test_zipper_run 
  PROPERTIES PASS_REGULAR_EXPRESSION "All tests passed"
)
