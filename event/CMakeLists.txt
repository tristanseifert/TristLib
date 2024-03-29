####################################################################################################
# TristLib Event loop
####################################################################################################

####################################################################################################
# Find the system's libevent
find_package(PkgConfig REQUIRED)

pkg_search_module(PKG_LIBEVENT REQUIRED libevent)
pkg_search_module(PKG_LIBEVENT_OPENSSL REQUIRED libevent_openssl)

find_package(OpenSSL REQUIRED)

####################################################################################################
# Define the library
add_library(tristlib-event OBJECT
    Sources/RunLoop.cpp
    Sources/FileDescriptor.cpp
    Sources/Flag.cpp
    Sources/ListenSocket.cpp
    Sources/Timer.cpp
    Sources/Signal.cpp
    Sources/Socket.cpp
    Sources/SystemWatchdog.cpp
)

target_link_libraries(tristlib-event PUBLIC plog::plog ${PKG_LIBEVENT_LIBRARIES}
    ${PKG_LIBEVENT_OPENSSL_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto)

target_include_directories(tristlib-event PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Sources
    ${PKG_LIBEVENT_INCLUDE_DIRS} ${PKG_LIBEVENT_OPENSSL_INCLUDE_DIRS})
target_link_directories(tristlib-event PUBLIC ${PKG_LIBEVENT_LIBRARY_DIRS}
    ${PKG_LIBEVENT_OPENSSL_LIBRARY_DIRS})
target_include_directories(tristlib-event PUBLIC ${CMAKE_CURRENT_LIST_DIR}/Includes)

####################################################################################################
# Add support for systemd watchdog (if on Linux)
if(UNIX AND NOT APPLE)
    pkg_search_module(PKG_SYSTEMD libsystemd)
    if(PKG_SYSTEMD_FOUND)
        target_link_directories(tristlib-event PUBLIC ${PKG_SYSTEMD_LIBRARY_DIRS})

        message(STATUS "Building with systemd support")

        target_include_directories(tristlib-event PRIVATE ${PKG_SYSTEMD_INCLUDE_DIRS})
        target_link_libraries(tristlib-event PUBLIC ${PKG_SYSTEMD_LIBRARIES})

        target_compile_definitions(tristlib-event PRIVATE -DCONFIG_WITH_SYSTEMD)
    endif()
endif()
