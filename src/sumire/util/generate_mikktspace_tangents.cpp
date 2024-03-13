#include <sumire/util/generate_mikktspace_tangents.hpp>

#include <cassert>

namespace sumire::util {

    // TODO: Mikkt.c passing the data struct around as copied binary data (void/char *) causes
    //       a sizeable (unneeded) memory overhead for tangent generation.
    //       This could probably be reduced if it becomes a problem for large models.

    void generateMikktspaceTangents(MikktspaceData *data) {
        SMikkTSpaceInterface iface{};
        iface.m_getNumFaces = getNumFaces;
        iface.m_getNumVerticesOfFace = getNumVerticesOfFace;
        iface.m_getNormal = getNormal;
        iface.m_getPosition = getPosition;
        iface.m_getTexCoord = getTexCoords;
        iface.m_setTSpaceBasic = setTspaceBasic;

        SMikkTSpaceContext context{};
        context.m_pInterface = &iface;
        context.m_pUserData = data;

        genTangSpaceDefault(&context);
    }

    int getVertexIndex(MikktspaceData *data, int iFace, int iVert) {
        int vertIdx = data->indexCount > 0 
            ? data->indices[data->indexStart + iFace * 3 + iVert] 
            : data->vertexStart + iFace * 3 + iVert;
        
        return vertIdx;
    }

    int getNumFaces(const SMikkTSpaceContext *context) {
        MikktspaceData *data = static_cast<MikktspaceData*>(context->m_pUserData);

        int nVertInstances = data->indexCount > 0 ? data->indexCount : data->vertexCount;

        assert(nVertInstances % 3 == 0 && "Could not calculate number of faces for tangent generation as model's vertex count was not divisible by 3.");
        
        return nVertInstances / 3;
    }

    int getNumVerticesOfFace(const SMikkTSpaceContext *context, int iFace) {
        return 3;
    }

    void getPosition(const SMikkTSpaceContext *context, float outpos[], int iFace, int iVert) {
        MikktspaceData *data = static_cast<MikktspaceData*>(context->m_pUserData);

        int vertIdx = getVertexIndex(data, iFace, iVert);
        glm::vec3 vertPos = data->verts[vertIdx].position;

        outpos[0] = vertPos.x;
        outpos[1] = vertPos.y;
        outpos[2] = vertPos.z;
    }

    void getNormal(const SMikkTSpaceContext *context, float outnormal[], int iFace, int iVert) {
        MikktspaceData *data = static_cast<MikktspaceData*>(context->m_pUserData);

        int vertIdx = getVertexIndex(data, iFace, iVert);
        glm::vec3 vertNorm = data->verts[vertIdx].normal;

        outnormal[0] = vertNorm.x;
        outnormal[1] = vertNorm.y;
        outnormal[2] = vertNorm.z;
    }

    void getTexCoords(const SMikkTSpaceContext *context, float outuv[], int iFace, int iVert
    ) {
        MikktspaceData *data = static_cast<MikktspaceData*>(context->m_pUserData);

        int vertIdx = getVertexIndex(data, iFace, iVert);
        glm::vec2 vertUv = data->verts[vertIdx].uv0;

        outuv[0] = vertUv.x;
        outuv[1] = vertUv.y;
    }

    void setTspaceBasic(const SMikkTSpaceContext *context, const float tangentu[], float fSign, int iFace, int iVert) {
        MikktspaceData *data = static_cast<MikktspaceData*>(context->m_pUserData);

        int vertIdx = getVertexIndex(data, iFace, iVert) - data->vertexStart;
        data->outTangents[vertIdx] = glm::vec4{
            tangentu[0],
            tangentu[1],
            tangentu[2],
            fSign
        };
    }
}