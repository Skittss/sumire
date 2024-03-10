#pragma once

#include <mikktspace.h>
#include <glm/glm.hpp>

#include <sumire/core/models/vertex.hpp>

#include <vector>

namespace sumire::util {

    struct MikktspaceData {
        std::vector<Vertex> &verts;
        std::vector<uint32_t> &indices;
        // std::vector<glm::vec3> positions;
        // std::vector<glm::vec3> normals;
        // std::vector<glm::vec2> uvs;
        // std::vector<int> indices;
        std::vector<glm::vec4> outTangents;
        uint32_t vertexStart;
        uint32_t vertexCount;
        uint32_t indexStart;
        uint32_t indexCount;
    };

    void generateMikktspaceTangents(MikktspaceData *data);
    
    int  getVertexIndex(
        MikktspaceData *data, 
        int iFace, int iVert
    );
    int  getNumFaces(
        const SMikkTSpaceContext *context
    );
    int  getNumVerticesOfFace(
        const SMikkTSpaceContext *context, 
        int iFace
    );
    void getPosition(
        const SMikkTSpaceContext *context, 
        float outpos[], 
        int iFace, int iVert
    );
    void getNormal(
        const SMikkTSpaceContext *context, 
        float outnormal[],
        int iFace, int iVert
    );
    void getTexCoords(
        const SMikkTSpaceContext *context, 
        float outuv[],
        int iFace, int iVert
    );
    void setTspaceBasic(
        const SMikkTSpaceContext *context,
        const float tangentu[],
        float fSign, 
        int iFace, int iVert
    );

}