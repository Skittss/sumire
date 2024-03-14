#include <sumire/core/models/node.hpp>

namespace sumire {

    void Node::setMatrix(glm::mat4 matrix) {
		this->matrix = matrix;
		needsUpdate = true;
	}

	void Node::setTranslation(glm::vec3 translation) {
		this->translation = translation;
		needsUpdate = true;
	}

	void Node::setRotation(glm::quat rotation) {
		this->rotation = rotation;
		needsUpdate = true;
	}

	void Node::setScale(glm::vec3 scale) {
		this->scale = scale;
		needsUpdate = true;
	}

	// local transform matrix for a *single* node
	glm::mat4 Node::getLocalTransform() {
		if (needsUpdate) {
			cachedLocalTransform = glm::translate(glm::mat4{1.0f}, translation) * glm::mat4(rotation) * glm::scale(glm::mat4{1.0f}, scale) * matrix;
			needsUpdate = false;
		}
		
		return cachedLocalTransform;
	}

	// global transform matrix for a node (considering its parents transforms)
	glm::mat4 Node::getGlobalTransform() {

		glm::mat4 globalMatrix = getLocalTransform();
		Node *parentNode = parent;

		// Recurse through parents and accumulate transforms
		while (parentNode) {
			globalMatrix = parentNode->getLocalTransform() * globalMatrix;
			parentNode = parentNode->parent;
		}

		return globalMatrix;
	}

	void Node::applyTransformHierarchy() {

		worldTransform = parent ? parent->worldTransform * getLocalTransform() : getLocalTransform();
		invWorldTransform = glm::inverse(worldTransform);
		normalMatrix = glm::transpose(invWorldTransform);

		for (auto& child : children) {
			child->applyTransformHierarchy();
		}
	}

	// Updates a node and its children.
	void Node::updateRecursive() {
		update();
		
		// Update Children
		for (auto& child : children) {
			child->updateRecursive();
		}
	}

	// Updates this node only, and not its children.
	void Node::update() {
		// Update mesh nodes
		if (mesh) {
			mesh->uniforms.matrix = worldTransform;
			mesh->uniforms.normalMatrix = normalMatrix;

			// Update joint matrix
			if (skin) {
				uint32_t nJoints = static_cast<uint32_t>(skin->joints.size());
				
				auto jointData = std::vector<Mesh::JointData>(nJoints);
				for (uint32_t i = 0; i < nJoints; i++) {
					Node *jointNode = skin->joints[i];
					glm::mat4 jointMat = invWorldTransform * jointNode->worldTransform * skin->inverseBindMatrices[i];
					glm::mat4 jointNormalMat = glm::transpose(glm::inverse(jointMat));
					// mesh->uniforms.jointMatrices[i] = jointMat;
					// mesh->uniforms.jointNormalMatrices[i] = jointNormalMat;
					jointData[i] = Mesh::JointData{jointMat, jointNormalMat};
				}
				mesh->uniforms.nJoints = static_cast<int>(nJoints);

				// Write updated uniforms
				mesh->uniformBuffer->writeToBuffer(&mesh->uniforms);

				// Write joints to SSBO
				// TODO: Currently we rewrite the whole joint buffer which may be quite slow if a lot of data
				//		 is unchanged. It may be faster to rewrite on changed instances only (would require benchmark)
				mesh->jointBuffer->writeToBuffer(jointData.data());
			} else {
				// Update only the meshnode matrix & normal matrix.
				mesh->uniformBuffer->writeToBuffer(&mesh->uniforms, 2 * sizeof(glm::mat4), 0);
			}
		}
	}

}