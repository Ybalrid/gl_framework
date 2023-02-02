# This is the main cmake file of the frameowk.
# The main variable you want to touch in this is probably GAME_NAME.
#
# Depending on what you want to do, a few option are activable. These option may require additional libraries.
# - HAS_OPENVR (bool) : Activate the code that uses OpenVR. OpenVR is shipped as a git submodule
# - HAS_OPENXR (bool) : Activate the code that uses OpenXR. The OpenXR SDK is shipped as a git submodule
# - HAS_OCULUS_VR (bool) : Activate the code that uses Oculus. The Oculus OVR Native SDK is not shipped and need to be dowloaded by hand from the website.

CMAKE_MINIMUM_REQUIRED (VERSION 3.21)
SET (GAME_NAME "test_project")

#Some generation options
IF(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

if(CMAKE_GENERATOR STREQUAL Xcode)
  message(STATUS "xcode detected, setting compiler explicity to clang(++)")
  set(CMAKE_C_COMPILER "clang")
  set(CMAKE_CXX_COMPILER "clang++")
endif()

PROJECT (${GAME_NAME})

#Language and basic compiler configuratino
SET (CMAKE_CXX_STANDARD 17)

# TODO is the following really necessary?:
IF (UNIX)
  IF (NOT CMAKE_BUILD_TYPE)
    SET (CMAKE_BUILD_TYPE Release)
  ENDIF ()
  SET (CMAKE_CXX_FLAGS_RELEASE "-O3")
ELSEIF (MSVC)
  MESSAGE(STATUS "TODO activate in cmake maximum optimization settings for MSVC!!!")
ENDIF ()

#User configurable fields:
OPTION (HAS_OPENVR "Activate the OpenVR VR system" OFF)
OPTION (HAS_OCULUS_VR "Activate the OpenVR VR system" OFF)
OPTION (HAS_OPENXR "Activate the OpenXR VR system" OFF)
OPTION (USE_CODE_RELOADER "Activate (if possible) a hot-reaload system" OFF)

IF(HAS_OPENVR OR HAS_OCULUS_VR OR HAS_OPENXR)
  SET(HAS_VR TRUE)
ENDIF()

IF(HAS_VR)
  MESSAGE(STATUS "Detected a VR platform")

	IF(HAS_OPENVR)
	  SET(CPP_HAS_OPENVR "true")
	ELSE()
	  SET(CPP_HAS_OPENVR "false")
	ENDIf()

	IF(HAS_OCULUS_VR)
	  SET(CPP_HAS_OCULUS_VR "true")
	ELSE()
	  SET(CPP_HAS_OCULUS_VR "false")
	ENDIf()

	IF(HAS_OPENXR)
	  SET(CPP_HAS_OPENXR "true")
	ELSE()
	  SET(CPP_HAS_OPENXR "false")
	ENDIf()
ENDIF()

# Generate the configured header file
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/build_config.template.hpp ${CMAKE_SOURCE_DIR}/src/build_config.hpp )

#configure glm
ADD_DEFINITIONS( -DGLM_ENABLE_EXPERIMENTAL)

# Detect platforms and configure build options for specials usages

#activate optimizations and things like that
IF (MSVC)
  if(USE_CODE_RELOADER)
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /arch:AVX2 /fp:fast")
    ADD_DEFINITIONS(/ZI) #Edit and Continue sybols. Usefull to use `blink` too
    ADD_DEFINITIONS(-DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)
  endif()
ELSE()
  SET (CMAKE_XCC_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
  IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    ADD_DEFINITIONS(-D_DEBUG -DDEBUG)
  ENDIF()
ENDIF()

IF (UNIX)
  #use JET LIVE for hot realoading of modified code
  if(USE_CODE_RELOADER)
    SET(JET_LIVE_BUILD_EXAMPLE OFF)
    INCLUDE(third_party/jet-live/cmake/jet_live_setup.cmake)
    ADD_SUBDIRECTORY(third_party/jet-live/)
    MESSAGE("we are using jet-live")
    ADD_DEFINITIONS(-DUSING_JETLIVE) #add jet-live integration in my source code
  endif()
ENDIF()

#list source code
FILE (GLOB sources
  CONFIGURE_DEPENDS
  ./src/*.cpp
  ./src/*.hpp
  ./unpacked_res/shaders/*
  ./unpacked_res/scripts/*.chai
  ./unpacked_res/levels/*.json
  ./third_party/imgui/imgui*.cpp
  ./third_party/imgui/examples/imgui_impl_sdl.cpp
  ./third_party/imgui/examples/imgui_impl_opengl3.cpp
  ./third_party/ImGuizmo/ImGuizmo.cpp
  ./README.md)

#TODO use the source tree option to put shaders and chaiscript files in their own folders in the IDE

# just add this lib as a subdir,
SET (PHYSFS_BUILD_TEST OFF CACHE BOOL "no physfs tests" FORCE)
SET (PHYSFS_BUILD_SHARED OFF CACHE BOOL "no physfs tests" FORCE)
ADD_SUBDIRECTORY (third_party/physfs/)

#Get libraries
SET (LIBRARIES_PACKAGE_DIR "path_to_libraries"
  CACHE PATH "path to the 3rd party libraries directory")
LIST (APPEND CMAKE_MODULE_PATH
  "${CMAKE_SOURCE_DIR}/cmake_modules/"
  "${LIBRARIES_PACKAGE_DIR}/OpenAL-Soft/lib/cmake"
  )

# Locate third party libraries
FIND_PACKAGE (OpenAL REQUIRED)
FIND_PACKAGE (SNDFILE REQUIRED)
FIND_PACKAGE (SDL2 REQUIRED)
FIND_PACKAGE (GLEW REQUIRED)
FIND_PACKAGE (FreeImage REQUIRED)
FIND_PACKAGE (OpenGL REQUIRED)
FIND_PACKAGE (Threads REQUIRED)

# Create our build target
ADD_EXECUTABLE (${GAME_NAME}_exe ${sources}  "${CMAKE_SOURCE_DIR}/third_party/LIV_v1/SharedTextureProtocol.cpp")
TARGET_COMPILE_DEFINITIONS(${GAME_NAME}_exe PRIVATE _LIBCPP_DISABLE_AVAILABILITY)

target_include_directories (${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/steamworks/public/")
if(WIN32)
  target_link_libraries(${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/steamworks/redistributable_bin/win64/steam_api64.lib")
endif()
if(LINUX)
  target_link_libraries(${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/steamworks/redistributable_bin/linux64/libsteam_api.so")
endif()

# Add header only libraries include path
# Bullet is not "convinient" in the way their repo is structured for this:
TARGET_INCLUDE_DIRECTORIES (${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/bullet3/src")
TARGET_INCLUDE_DIRECTORIES (${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/nameof/include")
TARGET_INCLUDE_DIRECTORIES (${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/cpp-sdl2/sources/")
TARGET_INCLUDE_DIRECTORIES (${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/ChaiScript/include/")
TARGET_INCLUDE_DIRECTORIES (${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/physfs/src/")
TARGET_INCLUDE_DIRECTORIES (${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/glm/")
TARGET_INCLUDE_DIRECTORIES (${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/tinygltf/")
TARGET_INCLUDE_DIRECTORIES (${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/imgui")
TARGET_INCLUDE_DIRECTORIES (${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/ImGuizmo")
TARGET_INCLUDE_DIRECTORIES (${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/cpptoml/include")
TARGET_INCLUDE_DIRECTORIES (${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/LIV_v1/")
TARGET_INCLUDE_DIRECTORIES (${GAME_NAME}_exe PRIVATE "${OPENAL_INCLUDE_DIR}")
TARGET_COMPILE_DEFINITIONS (${GAME_NAME}_exe PRIVATE NOMINMAX IMGUI_IMPL_OPENGL_LOADER_GLEW)
TARGET_INCLUDE_DIRECTORIES (${GAME_NAME}_exe PRIVATE ${FREEIMAGE_INCLUDE_DIRS})
TARGET_INCLUDE_DIRECTORIES (${GAME_NAME}_exe PRIVATE ${SNDFILE_INCLUDE_DIR})

if(MSVC)
  target_compile_options(${GAME_NAME}_exe PRIVATE /bigobj)
endif()



IF(HAS_OPENVR)
  MESSAGE(STATUS "Configuring OpenVR includes and libraries")
  TARGET_INCLUDE_DIRECTORIES(${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/openvr/headers/")

  IF(MSVC)
    IF(CMAKE_SIZEOF_VOID_P EQUAL 8)
      MESSAGE(STATUS "64bit")
      TARGET_LINK_LIBRARIES(${GAME_NAME}_exe PRIVATE ${CMAKE_SOURCE_DIR}/third_party/openvr/lib/win64/openvr_api.lib)
    ELSEIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
      TARGET_LINK_LIBRARIES(${GAME_NAME}_exe PRIVATE ${CMAKE_SOURCE_DIR}/third_party/openvr/lib/win32/openvr_api.lib)
      MESSAGE(STATUS "32bit")
    ENDIF()
    ELSE() #assume linux x64
      TARGET_LINK_LIBRARIES(${GAME_NAME}_exe PRIVATE ${CMAKE_SOURCE_DIR}/third_party/openvr/lib/linux64/libopenvr_api.so)
  ENDIF()

ENDIF()

IF(HAS_OPENXR)
  MESSAGE(STATUS "Including OpenXR directories")
  ADD_SUBDIRECTORY("${CMAKE_SOURCE_DIR}/third_party/OpenXR-SDK")
  TARGET_LINK_LIBRARIES(${GAME_NAME}_exe PRIVATE openxr_loader)
  TARGET_INCLUDE_DIRECTORIES(${GAME_NAME}_exe PRIVATE
    "${CMAKE_SOURCE_DIR}/third_party/OpenXR-SDK/include"
    "${CMAKE_SOURCE_DIR}/third_party/xrew/generated"
    )
  IF(UNIX)
  TARGET_LINK_LIBRARIES(${GAME_NAME}_exe PRIVATE X11)
  ENDIF()
ENDIF()

IF(HAS_OCULUS_VR)
  MESSAGE(STATUS "Including Oculus SDK")
  TARGET_LINK_LIBRARIES(${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/oculus_sdk/LibOVR/Lib/Windows/x64/Release/VS2017/LibOVR.lib")
  TARGET_INCLUDE_DIRECTORIES(${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/oculus_sdk/LibOVR/Include/")
ENDIF()

MESSAGE(STATUS "Including TiltFive SDK")
TARGET_LINK_LIBRARIES(${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/T5/lib/win64/TiltFiveNative.dll.if.lib")
TARGET_INCLUDE_DIRECTORIES(${GAME_NAME}_exe PRIVATE "${CMAKE_SOURCE_DIR}/third_party/T5/include")

SET(BUILD_ALL_EXTENSIONS OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_BULLET2_DEMOS OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_BULLET3 ON CACHE BOOL "we don't care" FORCE)
SET(BUILD_BULLET_ROBOTICS_EXTRA OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_BULLET_ROBOTICS_GUI_EXTRA OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_CLSOCKET OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_CONVEX_DECOMPOSITION_EXTRA OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_CPU_DEMOS OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_ENET OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_EXTRAS OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_FORCE_GENERATION OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_GIMPACTUTILS_EXTRA OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_INVERSE_DYNAMIC_EXTRA OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_LOADER_WITH_EXCEPTION_HANDLING "we don't care" FORCE)
SET(BUILD_OBJ2SDF_EXTRA OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_PYBULLET OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_SERIALIZE_EXTRA OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_UNIT_TESTS OFF CACHE BOOL "we don't care" FORCE)
SET(BUILD_WITH_STD_FILESYSTEM ON CACHE BOOL "we don't care" FORCE)
ADD_SUBDIRECTORY(third_party/bullet3)

#link libraries
TARGET_LINK_LIBRARIES (${GAME_NAME}_exe
  PRIVATE Threads::Threads
  ${CMAKE_DL_LIBS}
  physfs-static
  SDL2::SDL2
  SDL2::SDL2main
  GLEW::GLEW
  OpenGL::GL
  ${OPENAL_LIBRARY}
  ${FREEIMAGE_LIBRARIES}
  ${SNDFILE_LIBRARY}
  BulletDynamics
  BulletCollision
  Bullet3Common
  LinearMath
  )

IF(UNIX)
  if(USE_CODE_RELOADER)
    TARGET_LINK_LIBRARIES(${GAME_NAME}_exe PUBLIC jet-live)
  endif()

  IF (APPLE)
    FIND_LIBRARY(IOKit IOKit)
    TARGET_LINK_LIBRARIES(${GAME_NAME}_exe PRIVATE ${IOKit})
  ENDIF(APPLE)
ENDIF()

ADD_CUSTOM_TARGET (copy_resources_to_build_dir ALL
  COMMAND # add_custom_command(TARGET ${GAME_NAME}_exe POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/res.zip ${CMAKE_BINARY_DIR}/res.zip
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/unpacked_res
  ${CMAKE_BINARY_DIR}/unpacked_res)

ADD_DEPENDENCIES(${GAME_NAME}_exe copy_resources_to_build_dir)

FILE(GLOB all_the_dlls CONFIGURE_DEPENDS ${CMAKE_BINARY_DIR}/*.dll)

SET(distrib_folder ${CMAKE_BINARY_DIR}/DISTRIBUTE)
ADD_CUSTOM_TARGET(DISTRIBUTE ALL
	COMMAND ${CMAKE_COMMAND} -E make_directory ${distrib_folder}
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/res.zip ${distrib_folder}/res.zip
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/unpacked_res ${distrib_folder}/unpacked_res
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${GAME_NAME}_exe> ${distrib_folder}
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${GAME_NAME}_exe> ${distrib_folder}
	COMMAND ${CMAKE_COMMAND} -E copy ${all_the_dlls} ${distrib_folder}
	)
ADD_DEPENDENCIES(DISTRIBUTE ${GAME_NAME}_exe)
