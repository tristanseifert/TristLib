####################################################################################################
# TristLib Core
#
# Provides logging and various header-only helpers
####################################################################################################

####################################################################################################
# Find plog or fetch it externally
find_package(plog QUIET)

if(NOT ${plog_FOUND})
    message(STATUS "Using plog: Fetch automatically (static)")
    FetchContent_Declare(
        plog
        GIT_REPOSITORY  https://github.com/SergiusTheBest/plog.git
        # 1.1.8
        GIT_TAG         89ac49396ae6978a056034d1e34bb170bfd3de33
    )
    FetchContent_MakeAvailable(plog)
else()
    message(STATUS "Using plog: System provided (shared)")
endif()

####################################################################################################
# Define the library
add_library(tristlib-core OBJECT
    Sources/Logging.cpp
)

target_link_libraries(tristlib-core PUBLIC plog::plog)

target_include_directories(tristlib-core PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Sources)
target_include_directories(tristlib-core PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Includes)

add_library(${PROJECT_NAME}::core ALIAS tristlib-core)
