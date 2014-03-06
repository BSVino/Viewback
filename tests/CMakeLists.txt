include_directories (
	${PROJECT_SOURCE_DIR}/server
)

set (GAME_CPP_SOURCES
	game.cpp
	../server/viewback.cpp
)

add_executable (game_cpp ${GAME_CPP_SOURCES})

set (GAME_C_SOURCES
	game.c
)

add_executable (game_c ${GAME_C_SOURCES})
