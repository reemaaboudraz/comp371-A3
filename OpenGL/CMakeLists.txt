cmake_minimum_required(VERSION 3.20)
project(COMP371_Assignment3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The project uses vcpkg-friendly package discovery.
# Recommended install command:
#   vcpkg install glfw3 glew glm
# Then configure CMake using your vcpkg toolchain file.
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(COMP371_Assignment3
    src/main.cpp
)

target_link_libraries(COMP371_Assignment3 PRIVATE
    OpenGL::GL
    glfw
    GLEW::GLEW
    glm::glm
)

# Copy chair.obj next to the executable after every successful build,
# provided that chair.obj exists in the OpenGL project root.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/chair.obj")
    add_custom_command(TARGET COMP371_Assignment3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/chair.obj"
            "$<TARGET_FILE_DIR:COMP371_Assignment3>/chair.obj"
    )
endif()
