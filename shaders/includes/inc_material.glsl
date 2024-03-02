struct Material {
    vec4 baseColorFactors;
    vec3 emissiveFactors;
    vec2 metallicRoughnessFactors;
    float normalScale;
    float occlusionStrength;
    int baseColorTexCoord;
    int metallicRoughnessTexCoord;
    int normalTexCoord;
    int occlusionTexCoord;
    int emissiveTexCoord;
    bool useAlphaMask;
    float alphaMaskCutoff;
};