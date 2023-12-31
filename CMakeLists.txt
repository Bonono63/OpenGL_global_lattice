cmake_minimum_required(VERSION 3.10)

project(GlobalLatticeDemo VERSION 1.0)

set(CMAKE_BUILD_TYPE=Debug)

set(C_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(GLFW_SOURCE_DIR "glfw")

add_subdirectory("${GLFW_SOURCE_DIR}")

set(OpenGL_GL_PREFERENCE "GLVND")
find_package( OpenGL REQUIRED)

set(GLAD_GL "${GLFW_SOURCE_DIR}/deps/glad/gl.h"
	    "${GLFW_SOURCE_DIR}/deps/glad_gl.c" )

link_libraries(glfw)

include_directories(
	"${GLFW_SOURCE_DIR}/deps"
	"${GLFW_SOURCE_DIR}/include"
	"include/"
    "glm/glm"
	)

add_executable(GLD src/main.cpp ${GLAD_GL})

target_link_libraries(GLD ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES} m)
