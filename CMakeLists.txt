cmake_minimum_required (VERSION 2.8)

project (viewback)

if (NOT WIN32)
	add_definitions("-std=c++0x")
endif()

option(BUILD_CLIENT "Build the client" ON)

if (BUILD_CLIENT)
	add_subdirectory (client)
endif()

add_subdirectory (tests)
