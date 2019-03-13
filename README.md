# SDL OpenGL Project Skeleton

This repository is a "base code" for making video games projects "from scratch".
It's configured with CMake to use a number of helpful libraries.

This code will create a window, initialize an OpenGL core profile,
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

## Licencing situation

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
 - OpenAL-Soft : GNU LGPL
 - libsndfile : GNU LGPL
 - ChaiScript : BSD 3-Clause

You can use these packages in a closed source application with the following consideration:
The MIT, zlib, BSD (3-clause), FIPL (and Happy Bunny) permit you to staically link their code
to your executable, if you so desire. The LGPL license require you to *dynamically* link these
libraries with your application. This means that if you distribute an application made with this
framework and wish to keep it source closed, you'll only have the choice to distribute
`OpenAL-soft` and `libsndfile` as a dynamically lodable binary library. This generally means
a .dll/.so/.dylib file on MS Windows/GNU Linux/macOs.

The Happy Bunny License stipulate that millitary uses of the GLM source code will make a bunny
unhappy. This framework uses GLM to implement the linear algebra needed for 3D rendering
(geometry transformation).

I am not a lawyer, the above doesn't constitute legal advice. 

## TODO 
 - [ ] Audio integration
 - [ ] Propper Input system
 - [ ] Defered rendering setup (g-buffers, multiple passes)
 - [ ] Script attachement system to "objects" in scene
 - [ ] Separate simulation and rendering threads