set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

include_directories (
	${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/../ext-deps/pthreads-w32-2-8-0-release
	${PROJECT_SOURCE_DIR}/../ext-deps/protobuf-2.5.0/vsprojects/include
)

set (CLIENT_TEST_SOURCES
	viewback_client.cpp
	viewback_data.cpp
	viewback_servers.cpp
	client_test.cpp
	../protobuf/data.pb.cc
)

add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-DPTW32_STATIC_LIB)

add_executable (client_test ${CLIENT_TEST_SOURCES})

if (WIN32)
	target_link_libraries(client_test debug ${PROJECT_SOURCE_DIR}/../ext-deps/pthreads-w32-2-8-0-release-vs2013/Debug/pthread.lib)
	target_link_libraries(client_test debug ${PROJECT_SOURCE_DIR}/../ext-deps/protobuf-2.5.0-vs2013/vsprojects/Debug/libprotobuf.lib)

	target_link_libraries(client_test optimized ${PROJECT_SOURCE_DIR}/../ext-deps/pthreads-w32-2-8-0-release-vs2013/Release/pthread.lib)
	target_link_libraries(client_test optimized ${PROJECT_SOURCE_DIR}/../ext-deps/protobuf-2.5.0-vs2013/vsprojects/Release/libprotobuf.lib)
endif (WIN32)
