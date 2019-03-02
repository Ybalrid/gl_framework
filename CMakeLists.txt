cmake_minimum_required(VERSION 3.12)
project(test_project)
set(CMAKE_CXX_STANDARD 17)
file(GLOB sources CONFIGURE_DEPENDS ./src/*.cpp ./src/*.hpp ./unpacked_res/shaders/*.glsl  ./third_party/imgui/imgui*.cpp ./third_party/imgui/examples/imgui_impl_sdl.cpp ./third_party/imgui/examples/imgui_impl_opengl3.cpp ./README.md)

add_subdirectory(third_party/physfs/)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/")
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(FreeImage REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(test_project_exe ${sources})
target_include_directories(test_project_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/cpp-sdl2/sources/")
target_include_directories(test_project_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/chaiscript/include/chaiscript")
target_include_directories(test_project_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/physfs/src/")
target_include_directories(test_project_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/glm/")
target_include_directories(test_project_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/tinygltf/")
target_include_directories(test_project_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/imgui")
target_compile_definitions(test_project_exe PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLEW)
target_include_directories(test_project_exe PRIVATE ${FREEIMAGE_INCLUDE_DIRS})
target_link_libraries(test_project_exe PRIVATE physfs-static SDL2::SDL2 SDL2::SDL2main GLEW::GLEW OpenGL::GL ${FREEIMAGE_LIBRARIES})

add_custom_target(copy_resources_to_build_dir ALL COMMAND
#add_custom_command(TARGET test_project_exe POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/res.zip ${CMAKE_BINARY_DIR}/res.zip
COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/unpacked_res ${CMAKE_BINARY_DIR}/unpakced_res)
