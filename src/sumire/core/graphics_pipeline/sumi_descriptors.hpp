#pragma once
 
#include <sumire/core/graphics_pipeline/sumi_device.hpp>
 
#include <memory>
#include <unordered_map>
#include <vector>
 
namespace sumire {
 
	class SumiDescriptorSetLayout {
		public:

			class Builder {
				public:
					Builder(SumiDevice &sumiDevice) : sumiDevice{sumiDevice} {}
				
					Builder &addBinding(
						uint32_t binding,
						VkDescriptorType descriptorType,
						VkShaderStageFlags stageFlags,
						uint32_t count = 1,
						VkDescriptorBindingFlags bindingFlags = 0x0
					);
						
					std::unique_ptr<SumiDescriptorSetLayout> build() const;
				
				private:
					SumiDevice &sumiDevice;
					std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
					std::unordered_map<uint32_t, VkDescriptorBindingFlags> bindingFlags{};
			};
		
			SumiDescriptorSetLayout(
				SumiDevice &sumiDevice, 
				std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings,
				std::unordered_map<uint32_t, VkDescriptorBindingFlags> bindingFlags
			);
			~SumiDescriptorSetLayout();
			SumiDescriptorSetLayout(const SumiDescriptorSetLayout &) = delete;
			SumiDescriptorSetLayout &operator=(const SumiDescriptorSetLayout &) = delete;
			
			VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
		
		private:
			SumiDevice &sumiDevice;
			VkDescriptorSetLayout descriptorSetLayout;
			std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
			std::unordered_map<uint32_t, VkDescriptorBindingFlags> bindingFlags;
			
			friend class SumiDescriptorWriter;
	};
		
	class SumiDescriptorPool {
		public:

			class Builder {
				public:
					Builder(SumiDevice &sumiDevice) : sumiDevice{sumiDevice} {}
				
					Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
					Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
					Builder &setMaxSets(uint32_t count);

					std::unique_ptr<SumiDescriptorPool> build() const;
				
				private:
					SumiDevice &sumiDevice;
					std::vector<VkDescriptorPoolSize> poolSizes{};
					uint32_t maxSets = 1000;
					VkDescriptorPoolCreateFlags poolFlags = 0;
				};

			SumiDescriptorPool(
				SumiDevice &sumiDevice,
				uint32_t maxSets,
				VkDescriptorPoolCreateFlags poolFlags,
				const std::vector<VkDescriptorPoolSize> &poolSizes);
			~SumiDescriptorPool();
			SumiDescriptorPool(const SumiDescriptorPool &) = delete;
			SumiDescriptorPool &operator=(const SumiDescriptorPool &) = delete;
			
			void allocateDescriptorSet(
				const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;
			
			void freeDescriptorSet(std::vector<VkDescriptorSet> &descriptors) const;
			
			void resetPool();

		private:
			SumiDevice &sumiDevice;
			VkDescriptorPool descriptorPool;
			
			friend class SumiDescriptorWriter;
	};
	
	class SumiDescriptorWriter {

		public:
			SumiDescriptorWriter(SumiDescriptorSetLayout &setLayout, SumiDescriptorPool &pool);

			SumiDescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo);
			SumiDescriptorWriter &writeImage(uint32_t binding, VkDescriptorImageInfo *imageInfo);

			void build(VkDescriptorSet &set);
			void overwrite(VkDescriptorSet &set);

		private:
			SumiDescriptorSetLayout &setLayout;
			SumiDescriptorPool &pool;
			std::vector<VkWriteDescriptorSet> writes;
	};
 
}