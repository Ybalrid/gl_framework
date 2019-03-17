CMAKE_MINIMUM_REQUIRED (VERSION 3.12)
PROJECT (test_project)
SET (CMAKE_CXX_STANDARD 17)

IF (MSVC)
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
ENDIF ()

FILE (GLOB sources
           CONFIGURE_DEPENDS
           ./src/*.cpp
           ./src/*.hpp
           ./unpacked_res/shaders/*.glsl
           ./third_party/imgui/imgui*.cpp
           ./third_party/imgui/examples/imgui_impl_sdl.cpp
           ./third_party/imgui/examples/imgui_impl_opengl3.cpp
           ./README.md)

# just add this lib as a subdir,
ADD_SUBDIRECTORY (third_party/physfs/)


SET (LIBRARIES_PACKAGE_DIR "path_to_libraries" CACHE PATH "path to the 3rd party libraries directory")
LIST (APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules/" "${LIBRARIES_PACKAGE_DIR}/OpenAL-Soft/lib/cmake")
FIND_PACKAGE (OpenAL REQUIRED)
FIND_PACKAGE (SNDFILE REQUIRED)
FIND_PACKAGE (SDL2 REQUIRED)
FIND_PACKAGE (GLEW REQUIRED)
FIND_PACKAGE (FreeImage REQUIRED)
FIND_PACKAGE (OpenGL REQUIRED)
FIND_PACKAGE (Threads REQUIRED)

ADD_EXECUTABLE (test_project_exe ${sources})
TARGET_INCLUDE_DIRECTORIES (test_project_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/cpp-sdl2/sources/")
TARGET_INCLUDE_DIRECTORIES (test_project_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/ChaiScript/include/")
TARGET_INCLUDE_DIRECTORIES (test_project_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/physfs/src/")
TARGET_INCLUDE_DIRECTORIES (test_project_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/glm/")
TARGET_INCLUDE_DIRECTORIES (test_project_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/tinygltf/")
TARGET_INCLUDE_DIRECTORIES (test_project_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/imgui")
TARGET_INCLUDE_DIRECTORIES (test_project_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/cpptoml/include")
TARGET_INCLUDE_DIRECTORIES (test_project_exe PRIVATE "${OPENAL_INCLUDE_DIR}")
TARGET_COMPILE_DEFINITIONS (test_project_exe PRIVATE NOMINMAX IMGUI_IMPL_OPENGL_LOADER_GLEW)
TARGET_INCLUDE_DIRECTORIES (test_project_exe PRIVATE ${FREEIMAGE_INCLUDE_DIRS})
TARGET_INCLUDE_DIRECTORIES (test_project_exe PRIVATE ${SNDFILE_INCLUDE_DIR})
TARGET_LINK_LIBRARIES (test_project_exe
                       PRIVATE Threads::Threads
                               ${CMAKE_DL_LIBS}
                               physfs-static
                               SDL2::SDL2
                               SDL2::SDL2main
                               GLEW::GLEW
                               OpenGL::GL
							   ${OPENAL_LIBRARY}
                               ${FREEIMAGE_LIBRARIES}
							   ${SNDFILE_LIBRARY})

ADD_CUSTOM_TARGET (copy_resources_to_build_dir ALL
                   COMMAND # add_custom_command(TARGET test_project_exe POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/res.zip ${CMAKE_BINARY_DIR}/res.zip
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/unpacked_res
                           ${CMAKE_BINARY_DIR}/unpacked_res)
