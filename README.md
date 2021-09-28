# SDL OpenGL Project Skeleton

This repository is an open-source "base code" (As my university teacher used to call this kind of frameworks) for making video games projects "from scratch".
It's configured with CMake to use a number of helpful libraries.

It is cross platform and supports Windows, Linux, and MacOS(*) (and should be "trivially" adaptable
to other platforms too!)

(*) For MacOS you *may* need an updated version of Clang and LibC++. This project has been tested under 10.13 High Sierra, XCode 9.4.1 and Clang 9 installed via brew (llvm). 
I do not have access to more modern macs, so I am stuck with a Mid 2010 iMac 11,2, that is mostly used for entertaiment, not developement. :wink:

This code will create a window, initialize an OpenGL core profile context,
be able to load GLSL shader code and glTF assets from "resource locations"
from a virtual file system, and represent a scene as a standard directed acyclic graph.

This scene graph is accessible from a dynamic scripting system using ChaiScript.

Think of this as a C++ OpenGL framework with some added sugar:

 - **Operating System layer:**
   - SDL2 *Windowing management, OpenGL context creation, inputs, and any low level operation*
   - cpp-sdl2 wrapper (as submodule) *C++ with RAII wrapper API for SDL2*
   - PhysicsFS (as submodule) *Virtual filesystem for safe and flexible resource access*
   - cpptoml (as submodule) *TOML file reader, used for configuration loading*
 - **Graphics support:**
   - GLEW *OpenGL loader*
   - GLM (as submodule) *OpenGL mathematics support*
   - tinygltf *glTF 2.0 loader*
   - FreeImage *generic image loader*
 - **Audio support:**
   - OpenAL *3D audio suppport*
   - libsndfile *audio file loading*
 - **Scripting Engine:**
   - ChaiScript (as submodule) *simple to integrate scripting engine*
 - **Integrated GUI:**
   - Dear ImGui *a flexible immediate mode GUI system*
   - ImGuizmo *a 3D manipulator gizmo build with ImGui*

The resulting combinaison of these components makes up for a *framework* that can serve as the
basis for a cross platform application. This framework is know to work fine on Windows, Linux
and MacOs as long as you have access to the required libraries, and a C++17 complient compiler
and standard library.

All of the non header-only libraries that you would be expected to dynamically link against,
with the exception of FreeImage are currently part of the Steam Runtime for Linux, making for
an easy distribution-version agnostic way of distributing applications using it.

## TODO: 
 - [x] Audio integration
 - [x] Propper Input system
 - [x] OpenVR
 - [x] OpenXR
 - [x] Oculus VR
 - [x] Proper level/environement loader/unloader system
 - [x] Script attachement system to "objects" in scene
 - [x] Physics
 - [ ] Separate simulation and rendering threads

 The 3D renderer is still "too basic" and needs to be improved:
 - [ ] PBR
 - [x] Shadow map (only for one directional map so far)
 - [ ] Defered rendering setup (g-buffers, multiple passes)

## Features

### VR

VR rendering is an optional component. To be activated in your build, you need to set the apropriate CMake cache variables to "ON"

 - `HAS_OPENVR`
 - `HAS_OPENXR`
 - `HAS_OCULUS_VR`

Preliminary VR support has been added. Engine configuration file (config.toml) must have the following keys inserted : 

```toml
is_vr = true
vr_system = "openvr"
```

The option to have each available `vr_system` has to be set in CMake when configuring the project. OpenVR is distributed
as a Git Submodule in the third_party directory. 

Currently supported vr systems are:
 - `"openvr"`
 - `"openxr"`
 - `"oculus"`

#### OpenXR

The OpenXR loader will be statically linked into the game EXE.

Linux support is untested.

#### DirectX11 Fallback

On Win32 platforms, a situation can occur where the currently installed OpenXR runtime can only be rendered to using DirectX 
(At the time of writing WMR and SteamVR do not provide the OpenGL extension). The `vr_sytem_openxr` will attempt to create an
OpenGL version of the system first, then if it fails, it will fallback to DirectX11. Performance with DirectX11 may be slightly
lower due to an extra texture copy being performed to share the texture between the two runtimes.

#### Oculus

The Oculus SDK is not shipped with inside this repository. To use the Oculus SDK, you will need to downlad it from the website, and extract the content of the zip into `third_party/oculus_sdk`. 
Version `1.43.0` is known to work at this time.

You will need to put the `OpenVR_API.dll` (or platform equivalent on Linux) next to your executable (or in the build directory when testing with Visual Studio on Windows)

### Scripting

//TODO

## Open Source dependancies

This framework is fully bassed on *Free* and *Open-Source software*.  These peices of software
have various licences and distribution terms that vary in rights and permissions.
The way this framework was put together was to be able to create Free and Open Source 
applications, but these components were carefully picked so that it is possible to use this
framework in a closed source application.

You can check the individual licences of all the project listed above, but for convinience,
here's a rundown of them, and a non-legally binding description of the "kind" of licence and
how it is used as part of this project:

 - SDL2 : zlib
 - cpp-sdl2 : MIT
 - PhysicsFS : zlib
 - cpptoml : MIT
 - GLEW : Modified BSD/MIT
 - GLM : The Happy Bunny License *or* MIT
 - tinygltf : MIT
 - FreeImage : FIPL license
 - OpenAL-Soft : GNU LGPL (but dynamically linked)
 - libsndfile : GNU LGPL (but dynamically linked)
 - ChaiScript : BSD 3-Clause
 - ImGui : MIT
 - ImGuizmo : MIT

You can use these packages in a closed source application with the following consideration:
The **MIT**, **zlib**, **BSD** (3-clause), **FIPL** (and Happy Bunny) permit you to staically 
link their code to your executable, if you so desire. The **LGPL** license require you to *dynamically* 
link these libraries with your application. This means that if you distribute an application made 
with this framework and wish to keep it source closed, you'll only have the choice to distribute
`OpenAL-soft` and `libsndfile` as a dynamically lodable binary library. This generally means
a .dll/.so/.dylib file on MS Windows/GNU Linux/macOs.

The Happy Bunny License stipulate that millitary uses of the GLM source code will *make a bunny
unhappy*. This framework uses GLM to implement the linear algebra needed for 3D rendering
(geometry transformation).

I am not a lawyer, the above doesn't constitute legal advice. 
