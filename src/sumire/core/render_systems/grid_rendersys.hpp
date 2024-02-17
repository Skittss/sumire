#pragma once

#include <sumire/core/sumi_pipeline.hpp>
#include <sumire/core/sumi_device.hpp>
#include <sumire/core/sumi_model.hpp>
#include <sumire/core/sumi_object.hpp>
#include <sumire/core/sumi_camera.hpp>
#include <sumire/core/sumi_frame_info.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace sumire {

	class GridRendersys {
		public:
		
			struct GridMinimalVertex {
				glm::vec3 pos{};
				glm::vec2 uv{};

				static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
				static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
			};

			GridRendersys(
				SumiDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalDescriptorSetLayout
			);
			~GridRendersys();

			GridRendersys(const GridRendersys&) = delete;
			GridRendersys& operator=(const GridRendersys&) = delete;

			void render(FrameInfo &frameInfo);

		private:
			void createGridQuadBuffers();
			void createPipelineLayout(VkDescriptorSetLayout globalDescriptorSetLayout);
			void createPipeline(VkRenderPass renderPass);

			void bindGridQuadBuffers(VkCommandBuffer &commandBuffer);

			SumiDevice& sumiDevice;

			std::unique_ptr<SumiBuffer> quadVertexBuffer;
			std::unique_ptr<SumiBuffer> quadIndexBuffer;
			
			std::unique_ptr<SumiPipeline> sumiPipeline;
			VkPipelineLayout pipelineLayout;
		};
}