# Sumire

A lightweight graphics development engine built in vulkan.

---

# Building and Running

## Prerequisites

- Use the provided CMake template `env_template_win.cmake` (Windows) / `env_template_unix.cmake` (Linux) to create a CMake environment: `.env.cmake`.

- Install:
    - [Vulkan](https://vulkan.lunarg.com/sdk/home) (Latest tested: v1.3.250.1)
    - [GLFW](https://www.glfw.org/download) (Latest tested: v3.4)
    - [GLM](https://github.com/g-truc/glm) (Latest tested v3.3.1)
  
  to directories of your choice, and specify their paths in `.env.cmake`. E.g:
  ```cmake
  set(GLFW_PATH       "~/glfw-<version>.bin.<dist>")
  set(GLM_PATH        "~/glm")
  set(VULKAN_SDK_PATH "~/VulkanSDK/<version>")
  ```
- If you are using a standalone C++ compiler (e.g. MinGW) you must also specify its path in `.env.cmake`. E.g.:
  ```cmake
  set(MINGW_PATH "<YOUR PATH HERE>")
  ```

## Building
A number of build scripts are provided which build the project via CMake. 

Building to a MSVC solution is recommended via running `build_win_msvc`. Sumire is tested primarily using the MSVC compiler then building the provided CMake targets (Sumire executable, and Shaders custom target).
Other build methods are untested and may have issues.

You may also build with a standalone compiler 
via `build_win [--debug] [--release]` (Windows) or `build_unix` (Linux).

## Running
- Run the generated binary in the directory `sumire/bin/Debug` or `sumire/bin/Release`.

---

# Partial Feature List and Todos

## GLTF support
- [X] faster glTF skinned animation updating
    - Cached local node transforms and swapped to top-down update rather than bottom up
- [ ] Move skinning to the compute dispatches.
    - Currently skinning will have read-after-write hazards due to not ring buffering the joint buffers.
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
- [ ] Config for GPU device selection (in the case of more than 1 gpu)

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
- [ ] Pre-draw compute needs to be ring-buffered or better synchroinzed.
- [ ] High quality deferred shadows (modification on cascaded shadow maps) via compute.
- [ ] Shader Hot-reloading
- [ ] Proper logging system via lib such as spdlog
- [ ] Pipeline caching (if pipeline initialisation becomes slow - currently non-issue)

## High Quality Shadow Mapping
- [X] Zbin testing
- [X] Ranged Zbin population
- [X] Fix view space depth calculation for out of bounds / extreme view frustum fovs
- [ ] Light culling.
    - This could / should be frustum based, or AABB.

## Application Config
- [ ] Persistent Physical Device (GPU) selection via config system (e.g. .ini file)