cmake_minimum_required(VERSION 3.12)
project(test_project)
set(CMAKE_CXX_STANDARD 17)
file(GLOB sources CONFIGURE_DEPENDS ./src/*.cpp)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(test_project_exe ${sources})
target_include_directories(test_project_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/cpp-sdl2/sources/")
target_link_libraries(test_project_exe PRIVATE SDL2::SDL2 SDL2::SDL2main GLEW::GLEW OpenGL::GL)
