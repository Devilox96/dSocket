cmake_minimum_required(VERSION 3.20)
project(dSocket)

set(CMAKE_CXX_STANDARD 17)

#---Threads---#
if (UNIX AND NOT APPLE)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()
#---Threads---#

add_executable(dSocket
        main.cpp
        dSocket.cpp)
target_link_libraries(dSocket
        Threads::Threads)