#include <sumire/core/sumi_descriptors.hpp>
 
#include <cassert>
#include <stdexcept>
 
namespace sumire {
 
// --------------- Descriptor Set Layout Builder ---------------------
 
SumiDescriptorSetLayout::Builder &SumiDescriptorSetLayout::Builder::addBinding(
	uint32_t binding,
	VkDescriptorType descriptorType,
	VkShaderStageFlags stageFlags,
	uint32_t count) {
	assert(bindings.count(binding) == 0 && "Binding already in use");
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = descriptorType;
	layoutBinding.descriptorCount = count;
	layoutBinding.stageFlags = stageFlags;
	bindings[binding] = layoutBinding;
	return *this;
}
 
std::unique_ptr<SumiDescriptorSetLayout> SumiDescriptorSetLayout::Builder::build() const {
	return std::make_unique<SumiDescriptorSetLayout>(sumiDevice, bindings);
}
 
// --------------- Descriptor Set Layout ---------------------
 
SumiDescriptorSetLayout::SumiDescriptorSetLayout(
		SumiDevice &sumiDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
	: sumiDevice{sumiDevice}, bindings{bindings} {

	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
	for (auto kv : bindings) {
	setLayoutBindings.push_back(kv.second);
	}
 
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
	descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();
 
	if (vkCreateDescriptorSetLayout(
			sumiDevice.device(),
			&descriptorSetLayoutInfo,
			nullptr,
			&descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}
 
SumiDescriptorSetLayout::~SumiDescriptorSetLayout() {
	vkDestroyDescriptorSetLayout(sumiDevice.device(), descriptorSetLayout, nullptr);
}
 
// --------------- Descriptor Pool Builder ---------------------
 
SumiDescriptorPool::Builder &SumiDescriptorPool::Builder::addPoolSize(
		VkDescriptorType descriptorType, uint32_t count) {

	poolSizes.push_back({descriptorType, count});
	return *this;
}
 
SumiDescriptorPool::Builder &SumiDescriptorPool::Builder::setPoolFlags(
		VkDescriptorPoolCreateFlags flags) {

	poolFlags = flags;
	return *this;
}
SumiDescriptorPool::Builder &SumiDescriptorPool::Builder::setMaxSets(uint32_t count) {
	maxSets = count;
	return *this;
}
 
std::unique_ptr<SumiDescriptorPool> SumiDescriptorPool::Builder::build() const {
	return std::make_unique<SumiDescriptorPool>(sumiDevice, maxSets, poolFlags, poolSizes);
}
 
// --------------- Descriptor Pool ---------------------
 
SumiDescriptorPool::SumiDescriptorPool(
		SumiDevice &sumiDevice,
		uint32_t maxSets,
		VkDescriptorPoolCreateFlags poolFlags,
		const std::vector<VkDescriptorPoolSize> &poolSizes)
	: sumiDevice{sumiDevice} {
			
	VkDescriptorPoolCreateInfo descriptorPoolInfo{};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolInfo.pPoolSizes = poolSizes.data();
	descriptorPoolInfo.maxSets = maxSets;
	descriptorPoolInfo.flags = poolFlags;
 
	if (vkCreateDescriptorPool(
			sumiDevice.device(), 
			&descriptorPoolInfo, 
			nullptr, 
			&descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool!");
	}
}
 
SumiDescriptorPool::~SumiDescriptorPool() {
	vkDestroyDescriptorPool(sumiDevice.device(), descriptorPool, nullptr);
}
 
bool SumiDescriptorPool::allocateDescriptorSet(
		const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const {
			
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.pSetLayouts = &descriptorSetLayout;
	allocInfo.descriptorSetCount = 1;
 
	// Might want to create a "DescriptorPoolManager" class that handles this case, and builds
	// a new pool whenever an old pool fills up. But this is beyond our current scope

	// Would keep track of a list of pool objects, if a pool becomes full, a new pool can be created 
	// and added to the list.
	if (vkAllocateDescriptorSets(sumiDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS)
		return false;

	return true;
}
 
void SumiDescriptorPool::freeDescriptorSet(std::vector<VkDescriptorSet> &descriptors) const {
	vkFreeDescriptorSets(
		sumiDevice.device(),
		descriptorPool,
		static_cast<uint32_t>(descriptors.size()),
		descriptors.data()
	);
}
 
void SumiDescriptorPool::resetPool() {
	vkResetDescriptorPool(sumiDevice.device(), descriptorPool, 0);
}
 
// --------------- Descriptor Writer ---------------------
 
SumiDescriptorWriter::SumiDescriptorWriter(SumiDescriptorSetLayout &setLayout, SumiDescriptorPool &pool)
	: setLayout{setLayout}, pool{pool} {}
 
SumiDescriptorWriter &SumiDescriptorWriter::writeBuffer(
		uint32_t binding, VkDescriptorBufferInfo *bufferInfo) {

	assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");
 
	auto &bindingDescription = setLayout.bindings[binding];
 
	assert(
		bindingDescription.descriptorCount == 1 &&
		"Binding single descriptor info, but binding expects multiple"
	);
 
	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = binding;
	write.pBufferInfo = bufferInfo;
	write.descriptorCount = 1;
 
	writes.push_back(write);
	return *this;
}
 
SumiDescriptorWriter &SumiDescriptorWriter::writeImage(
		uint32_t binding, VkDescriptorImageInfo *imageInfo) {

	assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");
 
	auto &bindingDescription = setLayout.bindings[binding];
 
	assert(
		bindingDescription.descriptorCount == 1 &&
		"Binding single descriptor info, but binding expects multiple");
 
	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorType = bindingDescription.descriptorType;
	write.dstBinding = binding;
	write.pImageInfo = imageInfo;
	write.descriptorCount = 1;
 
	writes.push_back(write);
	return *this;
}
 
bool SumiDescriptorWriter::build(VkDescriptorSet &set) {
	bool success = pool.allocateDescriptorSet(setLayout.getDescriptorSetLayout(), set);
	if (!success) return false;

	overwrite(set);
	return true;
}
 
void SumiDescriptorWriter::overwrite(VkDescriptorSet &set) {
	for (auto &write : writes) {
		write.dstSet = set;
	}
	vkUpdateDescriptorSets(pool.sumiDevice.device(), writes.size(), writes.data(), 0, nullptr);
}
 
}