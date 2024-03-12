# Sumire

A lightweight graphics development engine built in vulkan.

---
## Todo

- [X] faster glTF animation updating
- [X] model normal matrices (and normal matrices for skinning)
- [X] Pass joint matrices in to shader via SSBO as uniforms overflow.
- [X] Model Normal Mapping from tangent space textures.
- [X] Right handed and +y default camera.
    - Camera rotation would be better done with quaternions.
    - Arbitrary orthonormal basis camera control support would be nice.
- [ ] Bitangent reading from glTF needs a fix (fails NormalMirrorTest.glb)
- [ ] glTF Morph-target support (and their animation)
- [ ] glTF texture mip-mapping support
- [ ] Deferred Rendering
- [ ] Shader Hot-reloading
- [ ] Proper logging system via lib such as spdlog
