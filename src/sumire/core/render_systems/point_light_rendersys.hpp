#pragma once

#include <sumire/core/graphics_pipeline/sumi_pipeline.hpp>
#include <sumire/core/graphics_pipeline/sumi_device.hpp>
#include <sumire/core/rendering/sumi_object.hpp>
#include <sumire/core/rendering/sumi_camera.hpp>
#include <sumire/core/rendering/sumi_frame_info.hpp>

#include <memory>
#include <vector>

namespace sumire {

	class PointLightRenderSys {
		public:
			PointLightRenderSys(
				SumiDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalDescriptorSetLayout
			);
			~PointLightRenderSys();

			PointLightRenderSys(const PointLightRenderSys&) = delete;
			PointLightRenderSys& operator=(const PointLightRenderSys&) = delete;

			void render(FrameInfo &frameInfo);

		private:
			void createPipelineLayout(VkDescriptorSetLayout globalDescriptorSetLayout);
			void createPipeline(VkRenderPass renderPass);

			SumiDevice& sumiDevice;

			std::unique_ptr<SumiPipeline> sumiPipeline;
			VkPipelineLayout pipelineLayout;
		};
}