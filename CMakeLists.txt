cmake_minimum_required (VERSION 2.8)

project (viewback)

option(BUILD_CLIENT "Build the client" ON)

if (BUILD_CLIENT)
	add_subdirectory (client)
endif()

add_subdirectory (tests)
