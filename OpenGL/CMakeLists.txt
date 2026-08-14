cmake_minimum_required(VERSION 3.20)

project(COMP371_Assignment3 LANGUAGES CXX)

# The OpenGL source uses std::filesystem and modern C++ language features.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Required libraries from the assignment.
# Recommended vcpkg command on Windows:
#   vcpkg install glfw3:x64-windows glew:x64-windows glm:x64-windows
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

# The Blender script exports the required model directly to OpenGL/chair.obj.
set(CHAIR_OBJ "${CMAKE_CURRENT_SOURCE_DIR}/chair.obj")

# Give a clear configure-time error instead of allowing an executable to build
# and then mysteriously fail at runtime because the model is missing.
if(NOT EXISTS "${CHAIR_OBJ}")
    message(FATAL_ERROR
        "chair.obj was not found in ${CMAKE_CURRENT_SOURCE_DIR}. "
        "Run Blender/create_chair.py in Blender first; it exports OpenGL/chair.obj."
    )
endif()

# Copy the OBJ beside the executable after every build.  This makes running from
# PowerShell or directly from the generated Release/Debug folder reliable.
add_custom_command(TARGET COMP371_Assignment3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CHAIR_OBJ}"
        "$<TARGET_FILE_DIR:COMP371_Assignment3>/chair.obj"
    COMMENT "Copying chair.obj beside the executable"
)
