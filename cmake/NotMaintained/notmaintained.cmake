include(FetchContent)


if (BUILD_WEBASSEMBLY)

else()
	include(cmake/NotMaintained/glad.cmake)
	include(cmake/NotMaintained/glfw.cmake)
endif()

include(cmake/NotMaintained/sqlite3.cmake)
