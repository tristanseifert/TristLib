####################################################################################################
# TristLib
#
# Main file for TristLib, which includes all other sub-libraries
####################################################################################################
cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
project(TristLib VERSION 0.1 LANGUAGES C CXX)

####################################################################################################
# Support fetching external dependencies
include(ExternalProject)
include(FetchContent)

####################################################################################################
# Set warning levels and language version
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wmissing-declarations -Wformat=2 -fdiagnostics-color=always
    -ftls-model=initial-exec -Wno-undef -Wcast-qual -Wwrite-strings -Wno-format-nonliteral)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-Wimplicit-fallthrough -Wno-deprecated-copy -Wno-address-of-packed-member
        -Wno-expansion-to-defined -Wno-unused-private-field -Wno-deprecated-volatile)
endif()

####################################################################################################
# Figure out the library configuration
if(PROJECT_IS_TOP_LEVEL)
    set(TRISTLIB_BUILD_STYLE "SHARED")
else()
    set(TRISTLIB_BUILD_STYLE "STATIC")
endif()

message(STATUS "TristLib build style: ${TRISTLIB_BUILD_STYLE}")

option(TRISTLIB_BUILD_EVENT "Build TristLib event loop support" ON)

####################################################################################################
# Create a version file
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
cmake_host_system_information(RESULT BUILD_HOSTNAME QUERY FQDN)
set(BUILD_USERNAME $ENV{USER})

configure_file(${CMAKE_CURRENT_LIST_DIR}/BuildInfo.c.in
    ${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.c)
set(BuildInfoFile "${CMAKE_CURRENT_BINARY_DIR}/BuildInfo.c")

####################################################################################################
# create the full (everything) TristLib target
add_library(tristlib ${TRISTLIB_BUILD_STYLE}
    ${BuildInfoFile}
)

set_target_properties(tristlib PROPERTIES LIBRARY_OUTPUT_NAME trist)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS tristlib)

####################################################################################################
# include core library
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/core)

target_sources(tristlib PUBLIC $<TARGET_OBJECTS:tristlib-core>)
target_link_libraries(tristlib PUBLIC tristlib-core)

####################################################################################################
# include additional components
if(${TRISTLIB_BUILD_EVENT})
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/event)

    target_sources(tristlib PUBLIC $<TARGET_OBJECTS:tristlib-event>)
    target_link_libraries(tristlib PUBLIC tristlib-event)
endif()
