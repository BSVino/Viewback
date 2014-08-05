include_directories (
	${PROJECT_SOURCE_DIR}/server
)

set (GAME_CPP_SOURCES
	game.cpp
	../server/viewback.c
	../server/viewback_util.cpp
)

add_executable (game_cpp ${GAME_CPP_SOURCES})

set (GAME_C_SOURCES
	game.c
)

add_executable (game_c ${GAME_C_SOURCES})

set (GAME_DOUBLE_SOURCES
	game.cpp
	../server/viewback.c
	../server/viewback_util.cpp
)

add_executable (game_double ${GAME_DOUBLE_SOURCES})

set_target_properties (game_double PROPERTIES COMPILE_DEFINITIONS "VIEWBACK_TIME_DOUBLE")

