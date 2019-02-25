# SDL OpenGL Project Skeleton

This repository is a "base code" for making video games projects "from scratch". It's configured with CMake to use a number of helpful libraries.

Think of this as a C++ OpenGL framework with some added sugar:

 - **Operating System layer:**
   - SDL2 *Windowing management, OpenGL context creation, inputs, and any low level poeration*
   - cpp-sdl2 wrapper (as submodule)
   - PhysicsFS (as submodule) *Virtual filesystem for safe and flexible resource access*
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