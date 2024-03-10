# Sumire

A lightweight graphics development engine built in vulkan.

---
## Todo

- [X] faster glTF animation updating
- [X] model normal matrices (and normal matrices for skinning)
    - Need passing to shaders. Uniforms are limited for joints so perhaps use a SSBO.
- [ ] Model Normal Mapping from tangent space textures.
- [X] Right handed and +y default camera.
    - Camera rotation would be better done with quaternions.
    - Arbitrary orthonormal basis camera control support would be nice.
- [ ] glTF Morph-target support (and their animation)
- [ ] glTF texture mip-mapping support
- [ ] Shader Hot-reloading
- [ ] Proper logging system via lib such as spdlog
