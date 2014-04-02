cmake_minimum_required (VERSION 2.8)

project (viewback)

if (MSVC)
	add_definitions(/W4)
endif()

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	add_definitions(-Wall)
endif()

if (NOT WIN32)
	add_definitions("-std=c++0x")
endif()

option(BUILD_CLIENT "Build the client" ON)

if (BUILD_CLIENT)
	add_subdirectory (client)
endif()

add_subdirectory (tests)
