# Sumire

A lightweight graphics development engine built in vulkan.

---

# Building and Running

## Prerequisites

- Use the provided CMake template `env_template_win.cmake` (Windows) / `env_template_unix.cmake` (Linux) to create a CMake environment: `.env.cmake`.

- Install Vulkan, GLFW and GLM to directories of your choice, and specify their paths in `.env.cmake` as shown
in the template.

- If you are using a standalone C++ compiler (e.g. MinGW) you must also specify its path in `.env.cmake`

## Building
A number of build scripts are provided which build the project via CMake. 

Only windows scripts are tested, of which building to a MSVC solution is recommended via running `build_win_msvc`. Sumire is tested primarily using the MSVC compiler then building the provided CMake targets (Sumire executable, and Shaders custom target).

You may also build with a standalone compiler 
via `build_win [--debug] [--release]` (Windows) or `build_unix` (Linux).

# Running
- Run the generated binary in the directory `sumire/bin/Debug` or `sumire/bin/Release`.

---

# Partial Feature List and Todos

## GLTF support
- [X] faster glTF skinned animation updating
    - Cached local node transforms and swapped to top-down update rather than bottom up
    - [ ] This could be even faster if we dispatched compute for it.
- [X] Model normal matrices (and normal matrices for skinning)
- [ ] Morph-target support (and their animation)
- [ ] Bone space on the GPU can be reduced to half by encoding the matrices as a quaternion rotation and vec4 offset rather than mat4
- [X] Model Normal Mapping from tangent space textures.
- [ ] KTX (compressed texture) reading and load support.
- [X] Texture mip-mapping support (runtime)
    - [ ] Loading in mip maps from files
    - [ ] Mip map generation could be moved to compute shader if anything more complicated than linear downsampling is needed
- [X] Bitangent reading
- [X] Mikktspace tangent generation

## Conventions
- [X] Right handed and +y default camera from Vulkan -y canonical viewing volume.

## Frame Pipeline
The current frame pipeline works as follows:
Shadow pass (Compute) -> [Deferred Fill, Deferred Resolve, Forward subpass] (Graphics subpasses) -> Async post process (Compute) -> Composite (Graphics)

- [X] Proper handling of double-sided triangles (Currently all back-face tris are culled)
    - Solved via dynamic pipeline binding in model draw calls.
- [X] Mesh Rendering multi-pipeline support
- [X] Tile-based Deferred Rendering
    - [X] Enable alpha blending when doing the composite deferred render pass
    - [X] Enable direct writing to swapchain for non-deferred geometry (e.g. unlit).
    - [X] VK_BY_REGION_BIT usage for tile based rendering support.
- [X] PBR material workfllow
- [ ] IBL support
    - Do not want the design to be purely IBL based, but having IBL for features such as environment probes would be a plus.
- [X] Async compute utilisation as well as more robust queue family management.
    - [ ] Pipeline bind caching may have to be updated to accommodate more complex queue family ecosystem
    - [ ] Compute -> Fragment interleaving needs profiling to ensure it's working correctly. (Cannot test on my Pascal GPU)
- [X] Compute-based post-processing dispatch.
- [ ] High quality deferred shadows (modification on cascaded shadow maps) via compute.
- [ ] Shader Hot-reloading
- [ ] Proper logging system via lib such as spdlog
- [ ] Pipeline caching (if pipeline initialisation becomes slow - currently non-issue)

## High Quality Shadow Mapping
- [ ] Zbin testing