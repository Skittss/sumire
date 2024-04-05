# Sumire

A lightweight graphics development engine built in vulkan.

---

# Building and Running

## Prerequisites

Use the provided CMake template `env_template_win.cmake` (Windows) / `env_template_unix.cmake` (Linux) to create a CMake environment: `.env.cmake`.

Install Vulkan, GLFW and GLM to directories of your choice, and specify their paths in `.env.cmake` as shown
in the template.

If you are using a standalone C++ compiler (e.g. MinGW) you must also specify its path in `.env.cmake`

## Building
A number of build scripts are provided which build the project via CMake. 

Only windows scripts are tested, of which building to a MSVC solution is recommended via running `build_win_msvc`. Sumire is tested primarily using the MSVC compiler then building the provided CMake targets (Sumire executable, and Shaders custom target).

You may also build with a standalone compiler 
via `build_win [--debug] [--release]` (Windows) or `build_unix` (Linux).

# Running
Run the generated binary in the directory `sumire/bin/Debug` or `sumire/bin/Release`.

---

# Todo

- [X] faster glTF animation updating
- [X] model normal matrices (and normal matrices for skinning)
- [X] Pass joint matrices in to shader via SSBO as uniforms overflow.
    - [ ] Bone space on the GPU can be reduced to half by encoding the matrices as a quaternion rotation and vec4 offset
    - [ ] Skinning can be calculated in a compute shader
- [X] Model Normal Mapping from tangent space textures.
- [X] Right handed and +y default camera.
- [X] Proper handling of double-sided triangles (Currently all back-face tris are culled)
    - Solved via dynamic pipeline binding in model draw calls.
- [X] Bitangent reading from glTF needs a fix (fails NormalMirrorTest.glb)
- [X] Mesh Rendering multi-pipeline support
- [X] glTF texture mip-mapping support (runtime)
    - [ ] Loading in mip maps from files
    - [ ] Mip map generation could be moved to compute shader if anything more complicated than linear downsampling is needed
- [ ] glTF Morph-target support (and their animation)
- [X] Deferred Rendering
    - [X] Enable alpha blending when doing the composite deferred render pass
    - [X] Copy depth buffer from gbuffer to subsequent forward rendering passes 
        - Solved using reusing gbuffer depth during subpass
    - [X] Convert gbuffer-swap chain passes into subpasses
- [ ] Shader Hot-reloading
- [ ] Proper logging system via lib such as spdlog
- [ ] Pipeline caching (if pipeline initialisation becomes slow)
