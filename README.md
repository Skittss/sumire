# Sumire

A lightweight graphics development engine built in vulkan.

---
## Todo

- [X] faster glTF animation updating
- [X] model normal matrices (and normal matrices for skinning)
- [X] Pass joint matrices in to shader via SSBO as uniforms overflow.
    - [ ] Bone space on the GPU can be reduced to half by encoding the matrices as a quaternion rotation and vec4 offset
    - [ ] Skinning can pe calculated in a compute shader
- [X] Model Normal Mapping from tangent space textures.
- [X] Right handed and +y default camera.
- [X] Proper handling of double-sided triangles (Currently all back-face tris are culled)
    - Solved via dynamic pipeline binding in model draw calls.
- [X] Bitangent reading from glTF needs a fix (fails NormalMirrorTest.glb)
- [X] Mesh Rendering multi-pipeline support
- [ ] glTF Morph-target support (and their animation)
- [ ] glTF texture mip-mapping support
- [ ] Deferred Rendering
- [ ] Shader Hot-reloading
- [ ] Proper logging system via lib such as spdlog
- [ ] Pipeline caching (if pipeline initialisation becomes slow)
