# Sumire

A lightweight graphics development engine built in vulkan.

---

# Building and Running
You can build Sumire using the provided build scripts and CMake, through the MSVC compiler.

Currently, Sumire is only supported on windows.

## Prerequisites
- Build and setup requirements:
    - [Python 3](https://www.python.org/downloads/), for running setup scripts. (Latest tested: 3.10.6)
    - [Visual Studio](https://visualstudio.microsoft.com/downloads/) (Latest tested: MSVS 2022)
    - [Vulkan](https://vulkan.lunarg.com/sdk/home) (Latest tested: v1.3.283.0)
    - [GLFW](https://www.glfw.org/download) (Latest tested: v3.4)

## Setup
**1. Download the Vulkan SDK and GLFW to directories of your choice**

If you want to build a *debug* build of Sumire, make sure to install the component `Shader Toolchain Debug Symbols` alongside the Vulkan SDK core.

**2. Clone the repository including submodules:**
```sh
git clone --recurse-submodules https://github.com/Skittss/sumire.git
```

<!---
**2. Install python requirements: (Currently just `requests`):**
```sh
python -m pip install -r requirements.txt
```
-->

**3. Run `setup.py`, specifying paths to your Vulkan and GLFW installations:**
```sh
python setup.py --vulkan <VK_PATH> --glfw <GLFW_PATH>
```
This script generates required build directories, and sets up a local cmake environment `.env.cmake`.

**4. Open the repository root with Visual Studio in CMake view**


## Building
- Use the CMake build system in MSVS with the provided targets (Debug, Release).
- CMakeLists are provided so there is the option of building without Visual Studio if you wish.

## Running
- Run the generated binary in the directory `sumire/bin/Debug` or `sumire/bin/Release`.

---

# Feature List and Todos

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
    - [X] Compute -> Fragment interleaving needs profiling to ensure it's working correctly. (Cannot test on my Pascal GPU)
- [X] Compute-based post-processing dispatch.
- [ ] Pre-draw compute needs to be ring-buffered or better synchroinzed.
- [X] Light buffers should be ring-buffered also as start of frame sort can cause discontinuities for in flight frames
- [ ] High quality deferred shadows (modification on cascaded shadow maps) via compute.
- [X] Shader Hot-reloading
- [ ] Proper logging system via lib such as spdlog
- [ ] Pipeline caching (if pipeline initialisation becomes slow - currently non-issue)

## High Quality Shadow Mapping
- [X] Zbin testing
- [X] Ranged Zbin population
- [X] Fix view space depth calculation for out of bounds / extreme view frustum fovs
- [X] Light culling (frustum culled in 'prepare', per 32x32 tile).
- [ ] Foxel light culling in appox light gather

## Application Config
- [X] Persistent Physical Device (GPU) selection via config system (json)
    - [X] Config data should be constant and instead written to a mirror when it needs changing
