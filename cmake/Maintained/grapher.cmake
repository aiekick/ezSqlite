set(GRAPHER_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/grapher/include)

include_directories(${GRAPHER_INCLUDE_DIR})

add_subdirectory(${CMAKE_SOURCE_DIR}/3rdparty/grapher)

if(USE_SHARED_LIBS)
	set_target_properties(grapher PROPERTIES FOLDER 3rdparty/Shared)
else()
	set_target_properties(grapher PROPERTIES FOLDER 3rdparty/Static)
endif()

set_target_properties(grapher PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${FINAL_BIN_DIR}")
set_target_properties(grapher PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${FINAL_BIN_DIR}")
set_target_properties(grapher PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${FINAL_BIN_DIR}")
set_target_properties(grapher PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${FINAL_BIN_DIR}")

if (MSVC)
	set_property(TARGET grapher PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
