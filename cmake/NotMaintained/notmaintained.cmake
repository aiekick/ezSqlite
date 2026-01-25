include(FetchContent)


if (BUILD_WEBASSEMBLY)
    message(STATUS "Emscripten: Using built-in GLFW and WebGL ports")
else()
	include(cmake/NotMaintained/glad.cmake)
	include(cmake/NotMaintained/glfw.cmake)
endif()

include(cmake/NotMaintained/sqlite3.cmake)
